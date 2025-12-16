import os
import logging
import base64
import smtplib
from email.message import EmailMessage
from functools import wraps
from flask import Flask, request, jsonify, render_template_string
from dotenv import load_dotenv
from datetime import datetime

load_dotenv()

# --- CONFIG ---
PORT = int(os.environ.get("SERVER_PORT", 3000))
AUTH_TOKEN = os.environ.get("AUTH_TOKEN")
# Update this to your specific Ngrok URL
PUBLIC_URL = "https://unarithmetically-peppiest-libbie.ngrok-free.dev"

# Email Config
EMAIL_SENDER = os.environ.get("EMAIL_SENDER")
EMAIL_PASSWORD = os.environ.get("EMAIL_PASSWORD")
EMAIL_RECIPIENT = os.environ.get("EMAIL_RECIPIENT")

# --- LOGGING ---
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(message)s')
logger = logging.getLogger(__name__)

app = Flask(__name__)

# --- GLOBAL STORAGE ---
dashboard_data = {
    "status_text": "Waiting for data...",
    "status_color": "gray", 
    "timestamp": "Never",
    "history": [] 
}

# --- HELPER: FORMAT TIME ---
def get_clean_time(iso_string):
    if not iso_string or "T" not in iso_string:
        return iso_string 
    try:
        return iso_string.split('T')[1].split('.')[0].replace('Z', '')
    except:
        return iso_string

# --- HELPER: SEND EMAIL ---
def send_email_alert(status, timestamp):
    if not EMAIL_SENDER or not EMAIL_PASSWORD:
        logger.warning("Email not configured in .env, skipping alert.")
        return

    try:
        msg = EmailMessage()
        # Added the dashboard link to the email body
        content = (f"Your Smart Mailbox detected a new event:\n\n"
                   f"Status: {status}\n"
                   f"Time: {timestamp}\n\n"
                   f"View Dashboard: {PUBLIC_URL}")
                   
        msg.set_content(content)
        msg['Subject'] = f"📬 Mailbox Alert: {status}"
        msg['From'] = EMAIL_SENDER
        msg['To'] = EMAIL_RECIPIENT

        server = smtplib.SMTP_SSL('smtp.gmail.com', 465)
        server.login(EMAIL_SENDER, EMAIL_PASSWORD)
        server.send_message(msg)
        server.quit()
        logger.info(f"Email sent to {EMAIL_RECIPIENT}")
    except Exception as e:
        logger.error(f"Failed to send email: {e}")

# --- HELPER: DECODE MAILBOX BYTES ---
def decode_mailbox_data(base64_string):
    try:
        raw_bytes = base64.b64decode(base64_string)
        if len(raw_bytes) < 2:
            return "Error: Data too short", "red"

        state_byte = raw_bytes[1]

        if state_byte == 0x04:
            return "⚠️ TAMPERING DETECTED", "red"
        elif state_byte == 0x05:
            return "📬 NEW MAIL!", "green"
        elif state_byte == 0x06:
            return "📭 No Mail", "blue"
        elif state_byte == 0x07:
            return "✅ System Ready", "purple"
        else:
            return f"Unknown Code: {hex(state_byte)}", "orange"
            
    except Exception as e:
        logger.error(f"Decoding error: {e}")
        return "Decoding Failed", "black"

# --- AUTH DECORATOR ---
def require_bearer_token(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        auth_header = request.headers.get('Authorization')
        if not auth_header or not auth_header.startswith('Bearer '):
            return jsonify({"error": "Unauthorized"}), 401
        token = auth_header.split(' ')[1]
        if token != AUTH_TOKEN:
            return jsonify({"error": "Forbidden"}), 403
        return f(*args, **kwargs)
    return decorated

# --- ROUTE: RECEIVE DATA (POST) ---
@app.route('/', methods=['POST'])
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
        status_text, color = decode_mailbox_data(raw_payload)
        
        dashboard_data["status_text"] = status_text
        dashboard_data["status_color"] = color
        dashboard_data["timestamp"] = clean_time_str
        
        new_event = {"time": clean_time_str, "status": status_text, "color": color}
        dashboard_data["history"].insert(0, new_event)
        if len(dashboard_data["history"]) > 5:
            dashboard_data["history"].pop()

        if "NEW MAIL" in status_text or "TAMPERING" in status_text:
            send_email_alert(status_text, clean_time_str)
        
        logger.info(f"Updated Status: {status_text}")

    return jsonify({"message": "OK"}), 200

# --- ROUTE: SHOW DASHBOARD (GET) ---
@app.route('/', methods=['GET'])
def show_dashboard():
    current_status = dashboard_data["status_text"]
    current_color = dashboard_data["status_color"]
    current_time = dashboard_data["timestamp"]
    history_list = dashboard_data["history"]

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
                display: inline-block; box-shadow: 0 4px 12px rgba(0,0,0,0.1); width: 400px;
            }
            h1 { color: #333; margin-top: 0; }
            .status-box { 
                font-size: 32px; font-weight: bold; margin: 20px 0; padding: 20px;
                color: white; border-radius: 10px; background-color: {{ color }};
            }
            .meta { color: #666; font-size: 13px; margin-bottom: 30px; }
            table { width: 100%; border-collapse: collapse; margin-top: 20px; text-align: left; }
            th { border-bottom: 2px solid #ddd; padding: 10px; color: #555; }
            td { border-bottom: 1px solid #eee; padding: 10px; font-size: 14px; }
            .dot { height: 10px; width: 10px; border-radius: 50%; display: inline-block; margin-right: 5px; }
        </style>
    </head>
    <body>
        <div class="card">
            <h1>Smart Mailbox</h1>
            
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
    return render_template_string(html, status=current_status, color=current_color, time=current_time, history=history_list)

if __name__ == "__main__":
    logger.info(f"Server running locally on port {PORT}")
    logger.info(f"Public URL: {PUBLIC_URL}") 
    app.run(host='0.0.0.0', port=PORT)