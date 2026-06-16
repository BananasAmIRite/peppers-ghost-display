import src.lib.PeriodicThread as PeriodicThread
from src.threads.weather_query import weather_query
from src.lib.ESPSerial import ESPSerial
from src.lib.ESPSPI import ESPSPI
from enum import Enum
import lib.comms as comms

class ScreenLayer(Enum):
    PERMANENT = 0, 
    TEMPORARY = 1

class ScreenType(Enum):
    SCREEN_IDLE = 0x00, 
    SCREEN_WEATHER = 0x01,
    SCREEN_TASKS = 0x02,

    SCREEN_SPOTIFY = 0x10


# Cambridge, MA — change to your location
# TODO: remake how this is calculated
LATITUDE  = 42.3601
LONGITUDE = -71.0589

class ScreenStateManager:
    def __init__(self, ser: ESPSerial, spi: ESPSPI):
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

    # initialization
    def open_serial(self):
        self.ser.open()

    def init_threads(self):
        # TODO: add more threads here
        # represents update threads for each of the screens (camera update thread will be separate from this!)
        thread_weather = PeriodicThread.PeriodicThread(60*30, weather_query, self, LATITUDE, LONGITUDE)
        self.screen_threads.append(thread_weather)

    def start_threads(self):
        for t in self.screen_threads: 
            t.start()

    # state management
    def add_temp_screen(self, temp_screen: ScreenType):
        if temp_screen in self.temp_screens: return
        self.temp_screens.append(temp_screen)
        
        # TODO: notify new temp screen

    def remove_temp_screen(self, temp_screen: ScreenType):
        if temp_screen not in self.temp_screens: return
        temp_scrn_idx = self.temp_screens.index(temp_screen)
        
        # auto update state to make sure nothing weird happens (we want it to go to the last screen)
        if self.cur_screen_state["idx_temp"] == temp_scrn_idx:
            # make sure this is correct
            self.cur_screen_state["idx_temp"] -= 1
            self.update_screen()

    def update_screen(self):
        # make sure state is valid (if not, make it valid)
        if self.cur_screen_state["type"] == ScreenLayer.TEMPORARY:
            if len(self.perm_screens) <= 0: 
                self.cur_screen_state["type"] = ScreenLayer.PERMANENT # transition to permanent screens (temp screens expired :/)

        
        if self.cur_screen_state["idx_temp"] >= len(self.temp_screens):
                self.cur_screen_state["idx_temp"] = len(self.temp_screens - 1) # snap to end of screens
        elif self.cur_screen_state["idx_temp"] < 0:
            self.cur_screen_state["idx_temp"] = 0
        
        # do the same bounds checks for permanent screens
        if self.cur_screen_state["idx_perm"] >= len(self.perm_screens):
            self.cur_screen_state["idx_perm"] = len(self.perm_screens - 1) # snap to end of screens
        elif self.cur_screen_state["idx_perm"] < 0:
            self.cur_screen_state["idx_perm"] = 0

        # now determine screen state
        screen_id = self.perm_screens[self.cur_screen_state["idx_perm"]] if self.cur_screen_state["type"] == ScreenLayer.PERMANENT else self.temp_screens[self.cur_screen_state["idx_temp"]]

        self.send_uart_message(comms.SET_SCREEN, bytearray(screen_id))

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
                self.cur_notif_id = -1
            else:
                # just swipe up
                self.cur_screen_state["type"] = ScreenLayer.TEMPORARY
            self.update_screen()

    def swipe_right(self):
        if self.cur_screen_state["type"] == ScreenLayer.TEMPORARY:
            # case 1: notification
            if self.cur_notif_id != -1 and self.temp_screens.index(self.cur_notif_id) != -1:
                self.cur_screen_state["idx_temp"] = self.temp_screens.index(self.cur_notif_id)
            else:
                self.cur_screen_state["idx_temp"] += 1
        else:
            self.cur_screen_state["idx_perm"] += 1
        
        self.update_screen()


    # thread helpers
    def send_uart_message(self, type: int, payload: bytearray):
        self.ser.write_message(type, payload)
    
    def send_spi_message(self, type: int, payload: bytearray):
        self.spi.write_message(type, payload)

    def send_spi_chunked_message(self, type_header: int, type_data: int, payload_header: bytearray, payload_data: bytearray):
        self.spi.write_chunked_message(type_header, type_data, payload_header, payload_data)
    