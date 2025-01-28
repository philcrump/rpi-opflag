#!/usr/bin/env python3

# openssl req -x509 -newkey rsa:2048 -keyout key.pem -nodes -out cert.pem -days 365

from http.server import HTTPServer, BaseHTTPRequestHandler
import ssl
import json
from datetime import datetime, timedelta

HTTPS_ADDRESS = 'localhost'
HTTPS_PORT = 4443

TYPE_NONE = 0
TYPE_DOWNLINK_START = 1
TYPE_DOWNLINK_STOP = 2
TYPE_UPLINK_START = 3
TYPE_UPLINK_STOP = 4

print("Generating event list..")

json_array = list()

current_datetime = datetime.utcnow()

json_array.append({'description': 'AoS HAKR', 'type': TYPE_DOWNLINK_START, 'time': "2023-04-25T16:18:11.000Z"})

json_array.append({'description': 'Braking burn', 'type': TYPE_NONE, 'time': "2023-04-25T16:26:51.000Z"})
json_array.append({'description': 'Touchdown', 'type': TYPE_NONE, 'time': "2023-04-25T16:40:00.000Z"})


json_bytes = json.dumps(json_array).encode()

print(json.dumps(json_array, indent=4))

class RequestHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        self.wfile.write(json_bytes)        

httpd = HTTPServer((HTTPS_ADDRESS, HTTPS_PORT), RequestHandler)

httpd.socket = ssl.wrap_socket (httpd.socket, keyfile="key.pem", certfile='cert.pem', server_side=True)

print(f"Listening on https://{HTTPS_ADDRESS}:{HTTPS_PORT}/ ..")

httpd.serve_forever()
