#!/usr/bin/env python3

# openssl req -x509 -newkey rsa:2048 -keyout key.pem -nodes -out cert.pem -days 365

from http.server import HTTPServer, BaseHTTPRequestHandler
import ssl
import json
from datetime import datetime, timedelta

HTTPS_ADDRESS = 'localhost'
HTTPS_PORT = 4443

print("Generating event list..")

json_array = list()

json_array.append(
  {
    "host_name": "network-server-a",
    "host_display_name": "Server A",
    "host_state": 0,
    "service_description": "server-importantservice",
    "service_display_name": "Service 1",
    "service_state": 1,
    "service_in_downtime": 0,
    "service_acknowledged": 0,
    "service_handled": 0,
    "service_output": "High Load",
    "service_perfdata": "",
    "service_attempt": "1/5",
    "service_last_state_change": 1738238096,
    "service_icon_image": "",
    "service_icon_image_alt": "",
    "service_is_flapping": 0,
    "service_state_type": 1,
    "service_severity": 2112,
    "service_notifications_enabled": 0,
    "service_active_checks_enabled": 1,
    "service_passive_checks_enabled": 1,
    "service_check_command": "check_command1",
    "service_next_update": "1738455048.00002"
  }
)
json_array.append(
  {
    "host_name": "network-server-b",
    "host_display_name": "Server B",
    "host_state": 0,
    "service_description": "server-importantservice",
    "service_display_name": "Service 1",
    "service_state": 2,
    "service_in_downtime": 0,
    "service_acknowledged": 0,
    "service_handled": 0,
    "service_output": "Service Failed",
    "service_perfdata": "",
    "service_attempt": "1/5",
    "service_last_state_change": 1738238096,
    "service_icon_image": "",
    "service_icon_image_alt": "",
    "service_is_flapping": 0,
    "service_state_type": 1,
    "service_severity": 2112,
    "service_notifications_enabled": 0,
    "service_active_checks_enabled": 1,
    "service_passive_checks_enabled": 1,
    "service_check_command": "check_command2",
    "service_next_update": "1738455048.00002"
  }
)

json_bytes = json.dumps(json_array).encode()

print(json.dumps(json_array, indent=4))

class RequestHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        self.wfile.write(json_bytes)        

httpd = HTTPServer((HTTPS_ADDRESS, HTTPS_PORT), RequestHandler)

httpd.socket = ssl.wrap_socket(httpd.socket, keyfile="key.pem", certfile='cert.pem', server_side=True)

print(f"Listening on https://{HTTPS_ADDRESS}:{HTTPS_PORT}/ ..")

httpd.serve_forever()
