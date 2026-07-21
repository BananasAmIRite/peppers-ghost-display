from lib.headphones.HeadphoneDriver import HeadphoneDriver
import struct
import bluetooth
from bluetooth import Protocols
from dataclasses import dataclass

# bose QC headphone (codename: prince) driver over Bluetooth Classic 
# specs: https://github.com/aaronsb/bosectl/pull/21/changes#diff-b38981a3a6dde1df88bedcfc9f70a4feb26db51a1fd63b15986651d2055a5fed

@dataclass
class BoseModeStatus:
    mode_idx: int
    voice_prompt: int
    
    # flags
    editable: bool
    configured: bool
    # unknown flag

    mode_name: str

    cnc_level: int
    auto_cnc: bool 
    spatial: bool
    wind_block: bool
    


class BoseHeadphones(HeadphoneDriver):
    def __init__(self, address):
        self.device = bluetooth.BluetoothSocket(Protocols.RFCOMM)
        self.address = address
        self.channel = 8

        self.device.settimeout(2)

    def start(self):
        return self.device.connect_ex((self.address, self.channel))

    def is_connected(self):
        try:
            # Request the remote address
            self.device.getpeername()
            return True
        except (bluetooth.BluetoothError, Exception):
            # Raises an error if disconnected or never connected
            return False

    def is_available(self):
        devices = bluetooth.discover_devices(lookup_names=False)
        return self.address in devices

    def stop(self):
        pass
        # self.device.disconnect()

    def create_packet(self, function_block, function_id, device, port, operator, payload: bytes):
        return struct.pack("<4B", function_block, function_id, 
                           (device << 6) | (port << 4) | operator
                           , len(payload)) + payload
    
    def create_simple_packet(self, function_block, function_id, operator, payload: bytes):
        return self.create_packet(function_block, function_id, 0, 0, operator, payload)

    def write_value(self, packet: bytes):
        if not self.is_connected():
            res = self.start()
            if res != 0: 
                return -1

        self.device.send(packet)
        return self.device.recv(1024)
    
    
    def get_battery(self):
        return self.write_value(self.create_simple_packet(0x02, 0x02, 0x01, bytes()))[4]
    
    def get_extra_info(self):
        name = self.get_prod_name()
        cur_mode = self.get_current_mode()
        modes = self.get_all_modes()
        mode_idxs = list(map(lambda m: m.mode_idx, modes))

        cur_mode_idx = mode_idxs.index(cur_mode)
        
        return bytes([len(mode_idxs), cur_mode_idx, len(name)]) + name
    
    
    def get_prod_name(self):
        return self.write_value(self.create_simple_packet(0x01, 0x02, 0x01, bytes()))[5:]
    
    def get_firmware(self):
        return self.write_value(self.create_simple_packet(0x00, 0x05, 0x01, bytes()))[4:]

    
    def get_all_modes(self) -> list[BoseModeStatus]:
        res = self.write_value(self.create_simple_packet(0x1f, 0x01, 0x05, bytes()))[28:]
        # print(res)
        # print(res.split(b'\x1f'))
        modes: BoseModeStatus = []
        for s in res.split(b'\x1f'):
            if len(s) < 50: continue
            data = s[3:]
            modes.append(
                BoseModeStatus(
                    mode_idx = data[0], 
                    voice_prompt=int.from_bytes(data[1:3], byteorder='big'), 
                    editable=data[3] & 0x01, 
                    configured=data[4] & 0x01, 
                    mode_name=data[6:38].split(b"\x00")[0].decode("utf-8"), 
                    cnc_level=data[42], 
                    auto_cnc=bool(data[43]), 
                    spatial=data[44], 
                    wind_block=bool(data[46])
                )
            )

        return modes
    
    def get_current_mode(self):
        return self.write_value(self.create_simple_packet(0x1f, 0x03, 0x01, bytes()))[4]
    
    def set_mode(self, mode, announce):
        self.write_value(self.create_simple_packet(0x1f, 0x03, 0x05, bytes([mode, announce])))