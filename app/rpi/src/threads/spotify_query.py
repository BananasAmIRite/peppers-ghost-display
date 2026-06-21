import lib.comms.comms as comms
import base64
from PIL import Image
from io import BytesIO
import struct
from utils.image2buf import image_to_buf
import os
import requests
from dotenv import load_dotenv
from screens import ScreenType

class SpotifyQuery:
    def __init__(self, mgr, spotify_client_id: str, spotify_secret: str):
        self.access_token = ""
        self.last_played_id = ""
        self.playing = False
        self.mgr = mgr
        self.spotify_client_id = spotify_client_id
        self.spotify_secret = spotify_secret
        

    def set_access_token(self, token):
        self.access_token = token
    def get_access_token(self):
        return self.access_token
    
    def set_last_id(self, id):
        self.last_played_id = id
    def get_last_played_id(self):
        return self.last_played_id
    
    def set_playing(self, playing):
        self.playing = playing

    def get_playing(self):
        return self.playing
    
    
    def query(self):
        if self.get_access_token() == "":
            print("getting new access token")
            self.set_access_token(spotify_get_access_token(self.spotify_client_id, self.spotify_secret))
            
        url = "https://api.spotify.com/v1/me/player/currently-playing"
        custom_headers = {
            "Authorization": f"Bearer {self.get_access_token()}",
        }
        response = requests.get(url, headers=custom_headers)

        if response.status_code == 204:
            # TODO: implement not playing anything
            print("Not playing anything...")
            if self.get_playing():
                # transition from playing to not playing
                self.mgr.remove_temp_screen(ScreenType.SCREEN_SPOTIFY)
                self.set_playing(False)
            pass
        elif response.status_code != 200:
            # error, assume expired access token
            self.set_access_token("")
            return
        else:
            # update state and temp screens
            if not self.get_playing():
                self.mgr.add_temp_screen(ScreenType.SCREEN_SPOTIFY)
                self.set_playing(True)

            json = response.json()
            name = ((json["item"]["artists"][0]["name"] + " - ") if len(json["item"]["artists"]) > 0 else "") + json["item"]["name"]
            duration = int(json["item"]["duration_ms"] / 1000)
            progress = int(json["progress_ms"] / 1000)

            name_bytes = name.encode("utf-8")


            if (json["item"]["id"] != self.get_last_played_id()): # new song, update image
                    
                payload = (
                    struct.pack(
                        "<HHH",
                        len(name_bytes),
                        duration,
                        progress,
                    )
                    + name_bytes
                )

                self.mgr.send_uart_message(comms.SPOTIFY_SET_SONG, payload)
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
                    
                    self.mgr.send_spi_message(comms.SPOTIFY_SET_IMAGE, struct.pack("<HH", width, height) + bytes(buf))
            else:
                self.mgr.send_uart_message(comms.SPOTIFY_UPDATE_SONG, struct.pack("<H", progress))

            
            self.set_last_id(json["item"]["id"])
    


def spotify_get_access_token(spotify_client_id: str, spotify_secret: str):
    token = os.getenv('SPOTIFY_REFRESH_TOKEN')
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


if __name__ == '__main__':
    load_dotenv()
    
    client_id = os.getenv("SPOTIFY_CLIENT_ID")
    client_secret = os.getenv("SPOTIFY_CLIENT_SECRET")
    spotify_query = SpotifyQuery(None, client_id, client_secret)

    spotify_query.query()
