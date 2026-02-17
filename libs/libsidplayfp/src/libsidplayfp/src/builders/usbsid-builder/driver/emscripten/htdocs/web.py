#!/usr/bin/env python

try:
    from http import server  # Python 3
except ImportError:
    import SimpleHTTPServer as server  # Python 2

class CustomHTTPRequestHandler(server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.add_custom_headers()
        super().end_headers()  # Call the superclass's method

    def add_custom_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")  # Allow all domains
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")  # Allow all domains
        self.send_header("Access-Control-Allow-Origin", "*")  # Allow all domains

if __name__ == '__main__':
    server.test(HandlerClass=CustomHTTPRequestHandler)
