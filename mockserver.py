import http.server
import random
import json
import os

aps = [
    {"ssid": "Mirror", "rssi": -32},
    {"ssid": "Door", "rssi": -64},
    {"ssid": "Lamp", "rssi": -45},
    {"ssid": "YingYang", "rssi": -66},
    {"ssid": "Famous", "rssi": -79},
    {"ssid": "Bonnie", "rssi": -80},
    {"ssid": "Cindy", "rssi": -94},
    {"ssid": "MOTO", "rssi": -88},
    {"ssid": "DIRECT-LWHL-DW", "rssi": -95},
    {"ssid": "Roku TV", "rssi": -84},
    {"ssid": "Living Room Speaker", "rssi": -89},
    {"ssid": "Living Room TV", "rssi": -92},
    {"ssid": "Spinbike", "rssi": -95},
    {"ssid": "Hollywood", "rssi": -35},
    {"ssid": "DELL DW 2048", "rssi": -37},
    {"ssid": "DELL DW 3072", "rssi": -45},
    {"ssid": "Brother 4096", "rssi": -55},
]

class MockServer(http.server.BaseHTTPRequestHandler):

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
            self.wfile.write(json.dumps(random.sample(aps, 10)).encode())

        else:
            file_path = "fs" + self.path
            if(os.path.exists(file_path)):
                extension = self.path.split(".")[-1]
                content_type = "text/html"

                if extension == "css": 
                    content_type = "text/css"  
                elif extension == "js":
                    content_type = "application/javascript"

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