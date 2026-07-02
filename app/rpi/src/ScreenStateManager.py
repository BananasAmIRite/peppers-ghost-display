import lib.PeriodicThread as PeriodicThread
from threads.weather_query import weather_query
from threads.spotify_query import SpotifyQuery
from threads.tasks_query import TasksQuery
from threads.serial_loop import UARTComms
from lib.comms.ESPSerial import ESPSerial
from lib.comms.ESPSPI import ESPSPI
from enum import Enum
import lib.comms.comms as comms
from dotenv import load_dotenv
import os
import time
import tty
import termios
import sys
import select
from screens import ScreenType
import threading


class ScreenLayer(Enum):
    PERMANENT = 0
    TEMPORARY = 1




# Cambridge, MA — change to your location
# TODO: remake how this is calculated
LATITUDE  = 42.3601
LONGITUDE = -71.0589

class ScreenStateManager:
    def __init__(self, ser: ESPSerial, spi: ESPSPI):
        load_dotenv()

        self.perm_screens = [ScreenType.SCREEN_IDLE, ScreenType.SCREEN_WEATHER, ScreenType.SCREEN_TASKS]
        self.temp_screens = []
        self.screen_threads = []
        # uart to esp Serial1
        self.ser = ser
        # spi to esp
        self.spi = spi

        # screen state
        self.cur_screen_state = {
            "type": ScreenLayer.PERMANENT, 
            "idx_perm": 0, # screen idx of permanent screen
            "idx_temp": 0
        }
        self.cur_notif_id = -1

        # screen-specific states

        # spotify
        client_id = os.getenv("SPOTIFY_CLIENT_ID")
        client_secret = os.getenv("SPOTIFY_CLIENT_SECRET")

        self.spotify_query = SpotifyQuery(self, client_id, client_secret)

        # notion / tasks
        ntn_secret = os.getenv("NOTION_CLIENT_SECRET")
        notion_view_id = os.getenv("NOTION_VIEW_ID")
        self.tasks_query = TasksQuery(self, ntn_secret, notion_view_id)


        # async serial read
        self.uart_comms = UARTComms("ESP UART", ser)
        self.uart_comms.add_uart_handler(self.uart_handler)


    # initialization
    def init(self):
        self.ser.open()
        self.init_threads()
        self.start_threads()
        self.update_screen()

    def init_threads(self):
        
        # TODO: add more threads here
        # represents update threads for each of the screens (camera update thread will be separate from this!)
        thread_weather = PeriodicThread.PeriodicThread(60*30, weather_query, self, LATITUDE, LONGITUDE)
        thread_spotify = PeriodicThread.PeriodicThread(10, self.spotify_query.query)
        thread_tasks = PeriodicThread.PeriodicThread(60*5, self.tasks_query.query)


        self.screen_threads.append(thread_weather)
        self.screen_threads.append(thread_spotify)
        self.screen_threads.append(thread_tasks)

        thread_serialread = PeriodicThread.PeriodicThread(0.1, self.uart_comms.loop)
        self.screen_threads.append(thread_serialread)

    def start_threads(self):
        for t in self.screen_threads: 
            t.start()

    def wait_program(self):
        # try:
        #     while True:
        #         time.sleep(1)
        # except KeyboardInterrupt:
        #     print("\nProgram terminated gracefully.")

        # for t in self.screen_threads: 
        #     t.stop()
        #     t.join()
        print("\nControl device screens using WASD keys (Ctrl+C to exit):")
        print("  W -> Swipe Up    |  A -> Swipe Left")
        print("  S -> Swipe Down  |  D -> Swipe Right\n")

        # Save old terminal settings to restore later
        old_settings = termios.tcgetattr(sys.stdin)
        try:
            # Put terminal into cbreak mode (reads keys instantly without waiting for Enter)
            tty.setcbreak(sys.stdin.fileno())
            
            while True:
                # Check if there is data waiting to be read in stdin (timeout of 0.1s)
                if select.select([sys.stdin], [], [], 0.1)[0]:
                    key = sys.stdin.read(1).lower()
                    
                    if key == 'a':
                        print("Key 'A' pressed: Swiping Left")
                        self.swipe_left()
                    elif key == 'd':
                        print("Key 'D' pressed: Swiping Right")
                        self.swipe_right()
                    elif key == 'w':
                        print("Key 'W' pressed: Swiping Up")
                        self.swipe_up()
                    elif key == 's':
                        print("Key 'S' pressed: Swiping Down")
                        self.swipe_down()
                        
                # Minimal sleep to prevent CPU spiking
                time.sleep(0.01)
                
        except KeyboardInterrupt:
            print("\nProgram terminated gracefully.")
        finally:
            # Crucial: Reset terminal settings back to normal
            termios.tcsetattr(sys.stdin, termios.TCSADRAIN, old_settings)

        # Stop and join threads
        for t in self.screen_threads: 
            t.stop()
            t.join()

    # state management
    def add_temp_screen(self, temp_screen: ScreenType):
        if temp_screen in self.temp_screens: return
        print("Added temp screen:" , temp_screen)
        self.temp_screens.append(temp_screen)

    def add_temp_notif(self, temp_screen: ScreenType):
        self.cur_notif_id = temp_screen
        self.send_uart_message(comms.SET_SCREEN_NOTIF, bytearray([0, temp_screen.value]))

        threading.Timer(5.0, self.clear_temp_notif).start() # clear notification after 5 seconds

    def clear_temp_notif(self):
        self.cur_notif_id = -1
        self.send_uart_message(comms.SET_SCREEN_NOTIF, bytearray([1, 0]))


    def remove_temp_screen(self, temp_screen: ScreenType):
        if temp_screen not in self.temp_screens: return
        print("Removed temp screen:" , temp_screen)

        # notif is active, remove it, since the screen is gone
        if self.cur_notif_id == temp_screen:
            self.clear_temp_notif()

        temp_scrn_idx = self.temp_screens.index(temp_screen)
        
        # auto update state to make sure nothing weird happens (we want it to go to the last screen)
        if self.cur_screen_state["idx_temp"] == temp_scrn_idx:
            # make sure this is correct
            self.cur_screen_state["idx_temp"] -= 1

        self.temp_screens.remove(temp_screen)

        self.update_screen()

    def update_screen(self):
        # make sure state is valid (if not, make it valid)
        if self.cur_screen_state["type"] == ScreenLayer.TEMPORARY:
            if len(self.temp_screens) <= 0: 
                self.cur_screen_state["type"] = ScreenLayer.PERMANENT # transition to permanent screens (temp screens expired :/)

        
        if self.cur_screen_state["idx_temp"] >= len(self.temp_screens):
                self.cur_screen_state["idx_temp"] = len(self.temp_screens) - 1 # snap to end of screens
        elif self.cur_screen_state["idx_temp"] < 0:
            self.cur_screen_state["idx_temp"] = 0
        
        # do the same bounds checks for permanent screens
        if self.cur_screen_state["idx_perm"] >= len(self.perm_screens):
            self.cur_screen_state["idx_perm"] = len(self.perm_screens) - 1 # snap to end of screens
        elif self.cur_screen_state["idx_perm"] < 0:
            self.cur_screen_state["idx_perm"] = 0

        # now determine screen state
        screen_id = self.perm_screens[self.cur_screen_state["idx_perm"]] if self.cur_screen_state["type"] == ScreenLayer.PERMANENT else self.temp_screens[self.cur_screen_state["idx_temp"]]

        self.send_uart_message(comms.SET_SCREEN, bytearray([screen_id.value]))

    # gestures
    def swipe_left(self):
        if self.cur_screen_state["type"] == ScreenLayer.TEMPORARY:
            self.cur_screen_state["idx_temp"] -= 1
        else:
            self.cur_screen_state["idx_perm"] -= 1
        
        self.update_screen()

    def swipe_down(self):
        if self.cur_screen_state["type"] == ScreenLayer.TEMPORARY:
            self.cur_screen_state["type"] = ScreenLayer.PERMANENT
            self.update_screen()

    def swipe_up(self):
        if self.cur_screen_state["type"] == ScreenLayer.PERMANENT:
            # case 1: we have notification
            if self.cur_notif_id != -1 and self.temp_screens.index(self.cur_notif_id) != -1:
                # swipe up to the current notificated screen and clear notification
                self.cur_screen_state["type"] = ScreenLayer.TEMPORARY
                self.cur_screen_state["idx_temp"] = self.temp_screens.index(self.cur_notif_id)
                self.clear_temp_notif()
            else:
                # just swipe up
                self.cur_screen_state["type"] = ScreenLayer.TEMPORARY
            self.update_screen()

    def swipe_right(self):
        if self.cur_screen_state["type"] == ScreenLayer.TEMPORARY:
            # case 1: notification
            if self.cur_notif_id != -1 and self.temp_screens.index(self.cur_notif_id) != -1:
                self.cur_screen_state["idx_temp"] = self.temp_screens.index(self.cur_notif_id)
                self.clear_temp_notif()
            else:
                self.cur_screen_state["idx_temp"] += 1
        else:
            self.cur_screen_state["idx_perm"] += 1
        
        self.update_screen()

    # uart handlers from esp
    def uart_handler(self, type, data, len):
        if type == comms.PI_SWIPE and len >= 1:
            swipe_dir = data[0]
            if swipe_dir == 0:
                self.swipe_left()
            elif swipe_dir == 1:
                self.swipe_right()
            elif swipe_dir == 2:
                self.swipe_up()
            elif swipe_dir == 3:
                self.swipe_down()

    # thread helpers
    def send_uart_message(self, type: int, payload: bytearray):
        self.ser.write_message(type, payload)
    
    def send_spi_message(self, data_type: int, body: bytes):
        self.spi.send_packet(data_type, body)
    
