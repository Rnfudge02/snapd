#!/usr/bin/env python3
"""Run the real reporter against a loopback predictor for the fork smoke test."""

from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import os
import subprocess
import sys
from threading import Thread
from urllib.parse import urlsplit


def main():
    mode = os.environ.get("PREDICTOR_SMOKE_MODE", "deny")
    if mode not in ("deny", "allow", "unavailable"):
        raise ValueError(f"Unknown PREDICTOR_SMOKE_MODE: {mode}")
    if len(sys.argv) != 4:
        raise ValueError("Expected repository, workflow run ID and attempt")

    class PredictorHandler(BaseHTTPRequestHandler):
        def do_GET(self):
            if urlsplit(self.path).path != "/predict":
                self.send_error(404)
                return
            self.send_response(503 if mode == "unavailable" else 200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            response = {"success_probability": 0.9 if mode == "allow" else 0.1}
            self.wfile.write(json.dumps(response).encode())

    with HTTPServer(("127.0.0.1", 0), PredictorHandler) as server:
        thread = Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            environment = dict(os.environ, TEST_PREDICTOR_URL=f"http://127.0.0.1:{server.server_port}")
            result = subprocess.run(
                ["bash", "-x", ".github/scripts/test-predictor-reporter.sh", *sys.argv[1:]],
                env=environment,
                check=False,
            )
        finally:
            server.shutdown()
            thread.join()
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())