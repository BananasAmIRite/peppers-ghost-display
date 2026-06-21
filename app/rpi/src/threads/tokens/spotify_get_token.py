import base64
import secrets
import threading
import urllib.parse
import webbrowser
from http.server import BaseHTTPRequestHandler, HTTPServer
import os
import requests
from dotenv import load_dotenv, set_key, find_dotenv

load_dotenv()

CLIENT_ID = os.getenv("SPOTIFY_CLIENT_ID")
CLIENT_SECRET = os.getenv("SPOTIFY_CLIENT_SECRET")
REDIRECT_URI = "http://127.0.0.1:8888/callback"


dotenv_path = '.env'

SCOPES = (
    "user-read-currently-playing "
    "user-read-playback-state"
)

auth_code = None
state = secrets.token_urlsafe(16)


class CallbackHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        global auth_code

        parsed = urllib.parse.urlparse(self.path)

        if parsed.path != "/callback":
            self.send_response(404)
            self.end_headers()
            return

        params = urllib.parse.parse_qs(parsed.query)

        if params.get("state", [""])[0] != state:
            self.send_response(400)
            self.end_headers()
            self.wfile.write(b"State mismatch")
            return

        auth_code = params.get("code", [""])[0]

        self.send_response(200)
        self.end_headers()
        self.wfile.write(
            b"Success! You can close this window."
        )


def run_server():
    server = HTTPServer(("localhost", 8888), CallbackHandler)
    server.handle_request()


threading.Thread(target=run_server, daemon=True).start()

params = {
    "client_id": CLIENT_ID,
    "response_type": "code",
    "redirect_uri": REDIRECT_URI,
    "scope": SCOPES,
    "state": state,
}

auth_url = (
    "https://accounts.spotify.com/authorize?"
    + urllib.parse.urlencode(params)
)

print("Opening browser...")
webbrowser.open(auth_url)

while auth_code is None:
    pass

print("Authorization code received.")

credentials = f"{CLIENT_ID}:{CLIENT_SECRET}"
auth_header = base64.b64encode(
    credentials.encode()
).decode()

response = requests.post(
    "https://accounts.spotify.com/api/token",
    headers={
        "Authorization": f"Basic {auth_header}",
        "Content-Type": "application/x-www-form-urlencoded",
    },
    data={
        "grant_type": "authorization_code",
        "code": auth_code,
        "redirect_uri": REDIRECT_URI,
    },
)

response.raise_for_status()

tokens = response.json()

print("\nACCESS TOKEN:")
print(tokens["access_token"])

print("\nREFRESH TOKEN:")
print(tokens["refresh_token"])

set_key(find_dotenv(), "SPOTIFY_REFRESH_TOKEN", tokens["refresh_token"])
