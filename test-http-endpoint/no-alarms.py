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
