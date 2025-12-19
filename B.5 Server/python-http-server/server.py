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

# --- Device Mapping ---
DEVICE_NAMES = {
    33: "PostBox SAN", # Our first test box
    # In case of more boxes, add them here as shown above
}

# --- Email Mapping ---
# Define who gets alerts for which device here
DEVICE_RECIPIENTS = {
    33: "abhinavkothari02@gmail.com, jc.chincheong@gmail.com"       # Our SAN Test Box
    # Add emails depenmding on device IDs    
}

# Email Config
EMAIL_SENDER = os.environ.get("EMAIL_SENDER")
EMAIL_PASSWORD = os.environ.get("EMAIL_PASSWORD")
EMAIL_RECIPIENT_STRING = os.environ.get("EMAIL_RECIPIENT") 

# --- Logging ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(message)s')
logger = logging.getLogger(__name__)

app = Flask(__name__)

# --- Storage ---
dashboard_data = {
    "device_name": "Unknown Device",
    "status_text": "Waiting for data...",
    "status_color": "gray", 
    "battery_level": "N/A",
    "battery_color": "gray",
    "timestamp": "Never",
    "history": [] 
}

# Store the last time we sent an email for specific events, only works when server is continously running
alert_cooldowns = {} 

# --- FORMAT TIME ---
def get_clean_time(iso_string):
    if not iso_string or "T" not in iso_string:
        return iso_string 
    try:
        return iso_string.split('T')[1].split('.')[0].replace('Z', '')
    except:
        return iso_string

# --- SEND EMAIL ---
def send_email_alert(device_id, device_name, status, timestamp, battery):
    if not EMAIL_SENDER or not EMAIL_PASSWORD:
        logger.warning("Email configuration missing, skipping alert.") # If no emails set in .env
        return
        
    target_emails_str = DEVICE_RECIPIENTS.get(device_id)
    
    if not target_emails_str:
        logger.warning(f"No recipient defined for Device {device_id}, skipping email.")
        return

    try:
        # To allow multiple recievers
        recipients_list = [email.strip() for email in target_emails_str.split(',') if email.strip()]
        recipients_header = ", ".join(recipients_list)

        msg = EmailMessage()
        if(int(battery.strip('%')) < 25):
            content = content + (
                        f"Battery is critically low ({battery}). Please change it soon!"
                        f"")
        else:
            content = (f"")
        
        content = content + (f"Your Smart Mailbox ({device_name}) detected a new event:\n\n"
                   f"Status: {status}\n"
                   f"Battery: {battery}\n"
                   f"Time: {timestamp}\n\n"
                   f"View Dashboard: {PUBLIC_URL}")

        msg.set_content(content)
        msg['Subject'] = f"📬 Alert: {status} - {device_name}"
        msg['From'] = EMAIL_SENDER
        msg['To'] = recipients_header 

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
            return None, "Error: Short Data", "red", None

        # 1. FIX ID: Convert hex digits to int (0x33 -> "33" -> 33)
        try:
            device_id = int(f"{raw_bytes[0]:x}") 
        except:
            device_id = raw_bytes[0]

        state_byte = raw_bytes[1]

        # 2. DECODE STATUS
        if state_byte == 0x04:
            status, color = "⚠️ TAMPERING DETECTED", "red"
        elif state_byte == 0x05:
            status, color = "📬 NEW MAIL!", "green"
        elif state_byte == 0x06:
            status, color = "📭 No Mail", "blue"
        elif state_byte == 0x07:
            status, color = "✅ System Ready", "purple"
        else:
            status, color = f"Unknown Code: {hex(state_byte)}", "orange"

        # 3. DECODE BATTERY (NEW)
        # We look for 4 bytes: [ID, State, Type(0x08), Value]
        battery = None
        if len(raw_bytes) >= 4 and raw_bytes[2] == 0x08:
            battery = raw_bytes[3] # The 4th byte is the percentage

        return device_id, status, color, battery
            
    except Exception as e:
        logger.error(f"Decoding error: {e}")
        return None, "Decoding Failed", "black", None

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
        # Decode everything including ID and Battery
        device_id, status_text, color, battery_val = decode_mailbox_data(raw_payload)
        
        if device_id is not None:
            # 1. Resolve Name
            device_name = DEVICE_NAMES.get(device_id, f"Device ID: {device_id}")
            
            # 2. Determine Battery Color
            bat_color = "gray"
            bat_text = "N/A"
            if battery_val is not None:
                bat_text = f"{battery_val}%"
                if battery_val > 60: bat_color = "#4CAF50" # Green
                elif battery_val > 20: bat_color = "#FF9800" # Orange
                else: bat_color = "#F44336" # Red

            # 3. Update Global Dashboard Data
            dashboard_data["device_name"] = device_name
            dashboard_data["status_text"] = status_text
            dashboard_data["status_color"] = color
            dashboard_data["timestamp"] = clean_time_str
            dashboard_data["battery_level"] = bat_text
            dashboard_data["battery_color"] = bat_color
            
            # 4. Add to History
            new_event = {
                "time": clean_time_str, 
                "status": status_text, 
                "color": color, 
                "device": device_name,
                "battery": bat_text
            }
            dashboard_data["history"].insert(0, new_event)
            if len(dashboard_data["history"]) > 10: 
                dashboard_data["history"].pop()

            # 5. Email Logic
            if "NEW MAIL" in status_text or "TAMPERING" in status_text:
                current_time = datetime.now()
                cooldown_key = (device_id, status_text) 
                last_sent = alert_cooldowns.get(cooldown_key)
                
                if not last_sent or (current_time - last_sent) > timedelta(minutes=15):
                    # Pass battery info to email too
                    send_email_alert(device_id, device_name, status_text, clean_time_str, bat_text)
                    alert_cooldowns[cooldown_key] = current_time 
                    logger.info(f"Email sent for {device_name}.")
                else:
                    logger.info(f"Email suppressed (Cooldown).")
            
            logger.info(f"Updated: {device_name} -> {status_text} | Bat: {bat_text}")

    return jsonify({"message": "OK"}), 200

# --- ROUTE: SHOW DASHBOARD (GET) ---
@app.route('/', methods=['GET'])
def show_dashboard():
    # Helper variables
    d = dashboard_data

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
                color: white; border-radius: 10px; background-color: {{ d.status_color }};
            }
            
            .battery-indicator {
                font-weight: bold; font-size: 18px; margin-bottom: 10px;
                padding: 5px 15px; border-radius: 20px; display: inline-block;
                color: white; background-color: {{ d.battery_color }};
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
            <h2>📍 {{ d.device_name }}</h2> 
            
            <div class="status-box">{{ d.status_text }}</div>
            
            <div class="battery-indicator">🔋 {{ d.battery_level }}</div>

            <p class="meta">Last Update: {{ d.timestamp }}</p>

            <h3>Recent Activity</h3>
            <table>
                <tr>
                    <th>Time</th>
                    <th>Event</th>
                    <th>Bat</th>
                </tr>
                {% for event in d.history %}
                <tr>
                    <td>{{ event.time }}</td> 
                    <td>
                        <span class="dot" style="background-color: {{ event.color }}"></span>
                        {{ event.status }}
                    </td>
                    <td>{{ event.battery }}</td>
                </tr>
                {% endfor %}
            </table>
        </div>
    </body>
    </html>
    """
    return render_template_string(html, d=d)

if __name__ == "__main__":
    logger.info(f"Server running locally on port {PORT}")
    logger.info(f"Public URL: {PUBLIC_URL}") 
    app.run(host='0.0.0.0', port=PORT)