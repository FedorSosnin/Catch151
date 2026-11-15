#!/usr/bin/env python3
"""Listens for a Windows test copy's timings (windows/build.sh with PERF_REPORT=<this Mac's address>) and hands
the test copy out on the home network, so a PC or handheld can download it from its browser.

  python3 tools/perf_listen.py <folder with the test .exe>     then, on the PC: http://<this Mac's address>:47399/

Each report is one line, printed with the time: ticks, frames and message-loop turns a second, then each part's
average and longest time in ms (tilt, game, frame, crt, compose, present, the whole tick), the motion sensor's mode
and readings a second, and the window's size and scaling. Nothing is sent anywhere but this terminal."""
import html
import http.server
import os
import sys
import time

FOLDER = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else ".")


class Handler(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        n = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(min(n, 4096)).decode("utf-8", "replace")
        print(f"{time.strftime('%H:%M:%S')} {self.client_address[0]} {body}", flush=True)
        self.send_response(204)
        self.end_headers()

    def do_GET(self):
        files = sorted(f for f in os.listdir(FOLDER) if f.lower().endswith(".exe"))
        name = self.path.lstrip("/")
        if name in files:
            path = os.path.join(FOLDER, name)
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Length", str(os.path.getsize(path)))
            self.send_header("Content-Disposition", f'attachment; filename="{name}"')
            self.end_headers()
            with open(path, "rb") as f:
                self.wfile.write(f.read())
            print(f"{time.strftime('%H:%M:%S')} {self.client_address[0]} downloaded {name}", flush=True)
            return
        links = "".join(f'<p><a href="/{html.escape(f)}">{html.escape(f)}</a></p>' for f in files)
        page = f"<!doctype html><meta name=viewport content='width=device-width'><h2>Catch 151 test copies</h2>{links}"
        data = page.encode()
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, *args):  # only the reports and downloads
        pass


print(f"serving {FOLDER} on port 47399", flush=True)
http.server.ThreadingHTTPServer(("", 47399), Handler).serve_forever()
