import os
import logging
import base64
import smtplib
from email.message import EmailMessage
from functools import wraps
from flask import Flask, request, jsonify, render_template_string
from dotenv import load_dotenv
from datetime import datetime, timedelta

load_dotenv()

# --- CONFIG ---
PORT = int(os.environ.get("SERVER_PORT", 3000))
AUTH_TOKEN = os.environ.get("AUTH_TOKEN")
PUBLIC_URL = "https://unarithmetically-peppiest-libbie.ngrok-free.dev"

# --- DEVICE MAPPING ---
DEVICE_NAMES = {
    51: "PostBox SAN",
    1: "Test Unit Alpha"
}

# Email Config
EMAIL_SENDER = os.environ.get("EMAIL_SENDER")
EMAIL_PASSWORD = os.environ.get("EMAIL_PASSWORD")
# We fetch the raw string here, splitting happens in the send function
EMAIL_RECIPIENT_STRING = os.environ.get("EMAIL_RECIPIENT") 

# --- LOGGING ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(message)s')
logger = logging.getLogger(__name__)

app = Flask(__name__)

# --- GLOBAL STORAGE ---
dashboard_data = {
    "device_name": "Unknown Device",
    "status_text": "Waiting for data...",
    "status_color": "gray", 
    "timestamp": "Never",
    "history": [] 
}

# Store the last time we sent an email for specific events
alert_cooldowns = {} 

# --- HELPER: FORMAT TIME ---
def get_clean_time(iso_string):
    if not iso_string or "T" not in iso_string:
        return iso_string 
    try:
        return iso_string.split('T')[1].split('.')[0].replace('Z', '')
    except:
        return iso_string

# --- HELPER: SEND EMAIL (UPDATED FOR MULTIPLE RECIPIENTS) ---
def send_email_alert(device_name, status, timestamp):
    if not EMAIL_SENDER or not EMAIL_PASSWORD or not EMAIL_RECIPIENT_STRING:
        logger.warning("Email configuration missing in .env, skipping alert.")
        return

    try:
        # 1. Process recipients: Split by comma and remove spaces
        recipients_list = [email.strip() for email in EMAIL_RECIPIENT_STRING.split(',') if email.strip()]
        
        # 2. Join them back into a clean comma-separated string for the email header
        recipients_header = ", ".join(recipients_list)

        msg = EmailMessage()
        content = (f"Your Smart Mailbox ({device_name}) detected a new event:\n\n"
                   f"Status: {status}\n"
                   f"Time: {timestamp}\n\n"
                   f"View Dashboard: {PUBLIC_URL}")
                   
        msg.set_content(content)
        msg['Subject'] = f"📬 Alert: {status} - {device_name}"
        msg['From'] = EMAIL_SENDER
        msg['To'] = recipients_header # This sends to all emails in the list

        server = smtplib.SMTP_SSL('smtp.gmail.com', 465)
        server.login(EMAIL_SENDER, EMAIL_PASSWORD)
        server.send_message(msg)
        server.quit()
        logger.info(f"Email sent to: {recipients_header}")
    except Exception as e:
        logger.error(f"Failed to send email: {e}")

# --- HELPER: DECODE MAILBOX BYTES ---
def decode_mailbox_data(base64_string):
    try:
        raw_bytes = base64.b64decode(base64_string)
        if len(raw_bytes) < 2:
            return None, "Error: Data too short", "red"

        # Extract ID and State
        device_id = raw_bytes[0]
        state_byte = raw_bytes[1]

        if state_byte == 0x04:
            return device_id, "⚠️ TAMPERING DETECTED", "red"
        elif state_byte == 0x05:
            return device_id, "📬 NEW MAIL!", "green"
        elif state_byte == 0x06:
            return device_id, "📭 No Mail", "blue"
        elif state_byte == 0x07:
            return device_id, "✅ System Ready", "purple"
        else:
            return device_id, f"Unknown Code: {hex(state_byte)}", "orange"
            
    except Exception as e:
        logger.error(f"Decoding error: {e}")
        return None, "Decoding Failed", "black"

# --- AUTH DECORATOR ---
def require_bearer_token(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        auth_header = request.headers.get('Authorization')
        if not auth_header:
            logger.warning("Rejected request: Missing Authorization Header")
            return jsonify({"error": "Unauthorized"}), 401
        
        token = auth_header.split(' ')[1] if ' ' in auth_header else ""
        if token != AUTH_TOKEN:
            logger.warning(f"Rejected request: Invalid Token.")
            return jsonify({"error": "Forbidden"}), 403
            
        return f(*args, **kwargs)
    return decorated

# --- ROUTE: RECEIVE DATA (POST) ---
@app.route('/', methods=['POST'])
@app.route('/uplink', methods=['POST']) 
@require_bearer_token
def handle_uplink():
    if not request.is_json:
        return jsonify({"error": "Not JSON"}), 415

    data = request.get_json()
    uplink = data.get('uplink_message', {})
    raw_payload = uplink.get('frm_payload', '')
    received_at = data.get('received_at', 'Just now')
    
    clean_time_str = get_clean_time(received_at)
    
    if raw_payload:
        # Decode everything including ID
        device_id, status_text, color = decode_mailbox_data(raw_payload)
        
        if device_id is not None:
            # 1. Resolve Name
            device_name = DEVICE_NAMES.get(device_id, f"Device ID: {device_id}")
            
            # 2. Update Global Dashboard Data
            dashboard_data["device_name"] = device_name
            dashboard_data["status_text"] = status_text
            dashboard_data["status_color"] = color
            dashboard_data["timestamp"] = clean_time_str
            
            # 3. Add to History
            new_event = {"time": clean_time_str, "status": status_text, "color": color, "device": device_name}
            dashboard_data["history"].insert(0, new_event)
            if len(dashboard_data["history"]) > 10: 
                dashboard_data["history"].pop()

            # 4. Email Logic with 15-Minute Cooldown
            if "NEW MAIL" in status_text or "TAMPERING" in status_text:
                current_time = datetime.now()
                cooldown_key = (device_id, status_text) 
                
                last_sent = alert_cooldowns.get(cooldown_key)
                
                # Check if we should send email (First time OR >15 mins since last time)
                if not last_sent or (current_time - last_sent) > timedelta(minutes=15):
                    send_email_alert(device_name, status_text, clean_time_str)
                    alert_cooldowns[cooldown_key] = current_time 
                    logger.info(f"Email sent for {device_name}.")
                else:
                    logger.info(f"Email suppressed (Cooldown active). Next email allowed in {15 - (current_time - last_sent).seconds//60} mins.")
            
            logger.info(f"Updated Dashboard: {device_name} -> {status_text}")

    return jsonify({"message": "OK"}), 200

# --- ROUTE: SHOW DASHBOARD (GET) ---
@app.route('/', methods=['GET'])
def show_dashboard():
    # Helper variables for HTML template
    d_name = dashboard_data["device_name"]
    d_status = dashboard_data["status_text"]
    d_color = dashboard_data["status_color"]
    d_time = dashboard_data["timestamp"]
    d_history = dashboard_data["history"]

    html = """
    <!DOCTYPE html>
    <html>
    <head>
        <title>Mailbox Monitor</title>
        <meta http-equiv="refresh" content="3">
        <style>
            body { font-family: 'Segoe UI', sans-serif; text-align: center; padding: 40px; background-color: #f0f2f5; }
            .card { 
                background: white; padding: 40px; border-radius: 15px; 
                display: inline-block; box-shadow: 0 4px 12px rgba(0,0,0,0.1); width: 450px;
            }
            h1 { color: #333; margin-top: 0; margin-bottom: 5px; }
            h2 { color: #666; font-weight: normal; margin-top: 0; font-size: 18px; } 
            .status-box { 
                font-size: 32px; font-weight: bold; margin: 20px 0; padding: 20px;
                color: white; border-radius: 10px; background-color: {{ color }};
            }
            .meta { color: #888; font-size: 13px; margin-bottom: 30px; }
            table { width: 100%; border-collapse: collapse; margin-top: 20px; text-align: left; }
            th { border-bottom: 2px solid #ddd; padding: 10px; color: #555; }
            td { border-bottom: 1px solid #eee; padding: 10px; font-size: 14px; }
            .dot { height: 10px; width: 10px; border-radius: 50%; display: inline-block; margin-right: 5px; }
        </style>
    </head>
    <body>
        <div class="card">
            <h1>Smart Mailbox</h1>
            <h2>📍 {{ name }}</h2> 
            <div class="status-box">{{ status }}</div>
            <p class="meta">Last Update: {{ time }}</p>

            <h3>Recent Activity</h3>
            <table>
                <tr>
                    <th>Time</th>
                    <th>Event</th>
                </tr>
                {% for event in history %}
                <tr>
                    <td>{{ event.time }}</td> 
                    <td>
                        <span class="dot" style="background-color: {{ event.color }}"></span>
                        {{ event.status }}
                    </td>
                </tr>
                {% endfor %}
            </table>
        </div>
    </body>
    </html>
    """
    return render_template_string(html, name=d_name, status=d_status, color=d_color, time=d_time, history=d_history)

if __name__ == "__main__":
    logger.info(f"Server running locally on port {PORT}")
    logger.info(f"Public URL: {PUBLIC_URL}") 
    app.run(host='0.0.0.0', port=PORT)