import struct
import lib.comms.comms as comms
import json
import time
from rpi.screens import ScreenType

from dataclasses import dataclass
from typing import Any

from lib.headphones.HeadphoneDriver import HeadphoneDriver
from lib.headphones import create

from lib.HostManager import HostManager
from lib.EmulatedHostManager import EmulatedHostManager

MAX_HEADPHONE_RETRY = 5

@dataclass
class HeadphoneInstance:
    driver: HeadphoneDriver
    config: Any
    available: bool = 0
    retry_cnt: int = 0



class HeadphoneQuery:
    def __init__(self, mgr: HostManager, headphone_config):
        self.mgr = mgr
        self.headphone_config = headphone_config["headphones"]
        self.headphone_instances: dict[int, HeadphoneInstance] = {}
        self.screen_available = False

        for config in self.headphone_config:
            inst = create(config["type"], *config["args"])
            if inst is not None: 
                self.headphone_instances[config["id"]] = HeadphoneInstance(driver=inst, config=config)


    def query(self):

        """
        Packet structure: 
            0: id
            1: state (available/not available)
            2: headphone type (int)
            3: batt
            4+: headphone-specific data
        """
        packets = []
        for headphone in self.headphone_instances.values():
            try:
                if headphone.driver.is_available():
                    pkt = headphone.driver.get_data_packet()
                    packets.append(struct.pack("<3B", headphone.config["id"], 1, headphone.config["type"]) + pkt)
                    headphone.available = True
                    headphone.retry_cnt = 0
                else:
                    raise RuntimeError() # for control flow purposes
            except Exception as e:
                print(f"Error!: {e}")
                # driver is not available
                headphone.retry_cnt += 1
                if headphone.retry_cnt > MAX_HEADPHONE_RETRY:
                    headphone.available = False
                packets.append(struct.pack("<2B", headphone.config["id"], headphone.available)) # headphone availability is determined after 5 retries, in case of a timed out packet
        
        if not self.screen_available and len(packets) > 0:
            # TODO: make this an actual implemented func on the HostManager
            self.mgr.send_uart_message(comms.PI_MSG, bytes([comms.PI_ADD_TMP_SCRN, ScreenType.SCREEN_HEADPHONES.value]))
            self.mgr.send_uart_message(comms.PI_MSG, bytes([comms.PI_ADD_TMP_NTF, ScreenType.SCREEN_HEADPHONES.value]))
            self.screen_available = True

        if self.screen_available and len(packets) == 0:
            self.mgr.send_uart_message(comms.PI_MSG, bytes([comms.PI_RMV_TMP_SCRN, ScreenType.SCREEN_HEADPHONES.value]))
            self.screen_available = False


        # TODO: send packets one at a time
        for packet in packets:
            self.mgr.send_uart_message(comms.HEADPHONE_UPDATE, packet)




if __name__ == "__main__":
    with open("../headphone_config.json", "r", encoding="utf-8") as file:
        data = json.load(file)
        headphones = HeadphoneQuery(EmulatedHostManager(), data)

        headphones.query()