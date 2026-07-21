from lib.headphones.BoseHeadphones import BoseHeadphones

HEADPHONE_TYPE_BOSE = 0x00

def create(type: str, *args):
    if type == HEADPHONE_TYPE_BOSE:
        return BoseHeadphones(*args)