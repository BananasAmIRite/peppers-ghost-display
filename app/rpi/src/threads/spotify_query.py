import lib.comms.ESPSerial as ESPSerial
from lib.comms.ESPSPI import ESPSPI
import lib.comms.comms as comms
import base64
from PIL import Image
from io import BytesIO
import struct
from src.utils.image2buf import image_to_buf
import os
import requests
from dotenv import load_dotenv



class SpotifyState:
    def __init__(self):
        self.access_token = ""
        self.last_played_id = ""

    def set_access_token(self, token):
        self.access_token = token
    def get_access_token(self):
        return self.access_token
    
    def set_last_id(self, id):
        self.last_played_id = id
    def get_last_played_id(self):
        return self.last_played_id

def spotify_get_access_token(spotify_client_id: str, spotify_secret: str):
    with open('./src/threads/spotify/refresh_token.txt', 'r') as f:
        token = f.read().strip()
        auth = base64.b64encode(
            f"{spotify_client_id}:{spotify_secret}".encode()
        ).decode()

        response = requests.post(
            "https://accounts.spotify.com/api/token",
            headers={
                "Authorization": f"Basic {auth}",
                "Content-Type": "application/x-www-form-urlencoded",
            },
            data={
                "grant_type": "refresh_token",
                "refresh_token": token,
            },
        )

        return response.json()['access_token']

def spotify_query(state: SpotifyState, ser: ESPSerial.ESPSerial, spi, spotify_client_id: str, spotify_secret: str):
    if state.get_access_token() == "":
        print("getting new access token")
        state.set_access_token(spotify_get_access_token(spotify_client_id, spotify_secret))
        
    url = "https://api.spotify.com/v1/me/player/currently-playing"
    custom_headers = {
        "Authorization": f"Bearer {state.get_access_token()}",
    }
    response = requests.get(url, headers=custom_headers)



    if response.status_code == 204:
        # TODO: implement not playing anything
        pass
    elif response.status_code != 200:
        # error, assume expired access token
        state.set_access_token("")
        return
    else:
        json = response.json()
        name = ((json["item"]["artists"][0]["name"] + " - ") if len(json["item"]["artists"]) > 0 else "") + json["item"]["name"]
        duration = int(json["item"]["duration_ms"] / 1000)
        progress = int(json["progress_ms"] / 1000)

        name_bytes = name.encode("utf-8")

        payload = (
            struct.pack(
                "<HHH",
                len(name_bytes),
                duration,
                progress,
            )
            + name_bytes
        )

        ser.write_message(comms.SPOTIFY_SET_SONG, payload)

        if (json["item"]["id"] != state.get_last_played_id()): # new song, update image
            imgs = json["item"]["album"]["images"]
            i_64s = [img for img in imgs if img['height'] == 64 and img['width'] == 64]
            if len(i_64s) > 0:
                i_64 = i_64s[0]
                print(i_64['url'])
                # get image
                img_res = requests.get(i_64['url'])

                img = Image.open(BytesIO(img_res.content)).convert("RGB")

                width, height, buf = image_to_buf(img)

                print(len(buf), width, height)
                spi.send_packet(comms.SPOTIFY_SET_IMAGE, struct.pack("<HH", width, height) + bytes(buf))



    state.set_last_id(json["item"]["id"])


if __name__ == '__main__':
    load_dotenv()
    state = SpotifyState()
    ser = ESPSerial.ESPSerial("/dev/serial0", 115200)
    spi = ESPSPI()
    ser.open()
    client_id = os.getenv("SPOTIFY_CLIENT_ID")
    client_secret = os.getenv("SPOTIFY_CLIENT_SECRET")

    spotify_query(state, ser, spi, client_id, client_secret)
