from lib.HostManager import HostManager
import lib.comms.comms as comms


def heartbeat(mgr: HostManager):
    print("Sending heartbeat...")
    mgr.send_uart_message(comms.DEBUG_HOST_HEARTBEAT, bytes())