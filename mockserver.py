"""
A mock server for development and testing of the WIFI web portal.
"""

import http.server
import random
import json
import os

aps = [
    {"ssid": "Mirror", "rssi": -32, "auth": 0},
    {"ssid": "Door", "rssi": -64, "auth": 0},
    {"ssid": "Lamp", "rssi": -45, "auth": 3} ,
    {"ssid": "YingYang", "rssi": -66, "auth": 3},
    {"ssid": "Famous", "rssi": -79, "auth": 3},
    {"ssid": "Bonnie", "rssi": -80, "auth": 3},
    {"ssid": "Cindy", "rssi": -94, "auth": 3},
    {"ssid": "MOTO", "rssi": -88, "auth": 3},
    {"ssid": "DIRECT-LWHL-DW", "rssi": -95, "auth": 0},
    {"ssid": "Roku TV", "rssi": -84, "auth": 3},
    {"ssid": "Living Room Speaker", "rssi": -89, "auth": 3},
    {"ssid": "Living Room TV", "rssi": -92, "auth": 3},
    {"ssid": "Spinbike", "rssi": -95, "auth": 3},
    {"ssid": "Hollywood", "rssi": -35, "auth": 3},
    {"ssid": "DELL DW 2048", "rssi": -37, "auth": 3},
    {"ssid": "DELL DW 3072", "rssi": -45, "auth": 3},
    {"ssid": "Brother 4096", "rssi": -55, "auth": 3}
]


class MockServer(http.server.BaseHTTPRequestHandler):

    def __init__(self, request: bytes, client_address: tuple[str, int], server: socketserver.BaseServer) -> None:
        super().__init__(request, client_address, server)
        

    """
    An implementation of the request handler.
    """

    def do_POST(self):
        if self.path == "/api/connect":
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            
            content_length = int(self.headers['Content-Length'])
            data_input = bytes.decode(self.rfile.read(content_length))
            print("Content recieved: " + data_input)

            self.wfile.write(json.dumps({"success": False}).encode())

    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            with open("fs/portal_index.html", "r") as f:
                self.wfile.write(f.read().encode())

        elif self.path == "/api/scan":
            self.send_response(200)
            self.send_header("Content-type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(sorted(random.sample(aps, 10), key=lambda x: x['rssi'], reverse=True)).encode())

        else:
            file_path = "fs" + self.path
            if(os.path.exists(file_path)):
                extension = self.path.split(".")[-1]
                content_type = "text/html"

                if extension == "css": 
                    content_type = "text/css"  
                elif extension == "js":
                    content_type = "application/javascript"
                elif extension == "svg":
                    content_type = "image/svg+xml"

                self.send_response(200)
                self.send_header("Content-type", content_type)
                self.end_headers()
                with open(file_path, "r") as f:
                    self.wfile.write(f.read().encode())
            else:
                self.send_response(404)
                self.send_header("Content-type", "text/html")
                self.end_headers()
                self.wfile.write("404".encode())

if __name__ == '__main__':
    server_address = ('', 8080)
    httpd = http.server.HTTPServer(server_address, MockServer)
    httpd.serve_forever()    


# """
# A function that serves content for route /api/scan
# """
# def api_scan():
#     selected_aps = random.sample(aps, random.randint(1, 8))