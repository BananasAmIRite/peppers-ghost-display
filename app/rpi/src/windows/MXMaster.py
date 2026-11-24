"""
Logitech MX Master thumb-button gestures -> cube swipes.

Logi Options+ doesn't expose the thumb button to Windows as a mouse button;
it injects keyboard shortcuts instead:

    Ctrl+Win+Left/Right (switch desktop) -> swipe left/right
    Win tap             (Start menu)     -> swipe up
    Win+D               (show desktop)   -> swipe down

While `is_active()` is true those shortcuts are swallowed and turned into
swipes. Otherwise (or for the physical keyboard, in the Win / Win+D case)
everything passes through untouched.

Usage:
    mx = MXMasterThumbButton(on_swipe=bridge.swipe, is_active=lambda: ...)
    listener = keyboard.Listener(..., win32_event_filter=mx.win32_event_filter)
    mx.attach(listener)
"""

import threading

from pynput import keyboard

# Win32 low-level keyboard hook constants
WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN, WM_SYSKEYUP = 0x100, 0x101, 0x104, 0x105
LLKHF_INJECTED = 0x10    # event came from SendInput (e.g. Logi Options+)
VK_LEFT, VK_RIGHT, VK_D = 0x25, 0x27, 0x44
VK_CTRL = {0xA2, 0xA3}   # L/R control
VK_WIN = {0x5B, 0x5C}    # L/R windows
# Unassigned VK. Injected after swallowing an arrow so releasing Win
# doesn't pop the Start menu (same trick AutoHotkey uses).
VK_MASK = 0xE8

# (direction byte, label) passed to on_swipe
SWIPE_LEFT, SWIPE_RIGHT, SWIPE_UP, SWIPE_DOWN = (
    (0x00, "Left"), (0x01, "Right"), (0x02, "Up"), (0x03, "Down"))

DEBUG = False


class MXMasterThumbButton:
    def __init__(self, on_swipe, is_active):
        """
        on_swipe(direction: int, label: str) - called on a background thread.
        is_active() -> bool - whether to intercept right now (called in the
            hook thread, so it must be cheap).
        """
        self.on_swipe = on_swipe
        self.is_active = is_active

        self.listener = None
        self.controller = keyboard.Controller()

        # modifier state tracked from the keyboard hook
        self.held_mods = set()
        # keys whose key-down we swallowed (so we swallow their key-up too)
        self.swallowed_keys = set()
        # Options+ Win press we're holding back (vk), until we see what follows
        self.pending_win = None
        self.pending_combo = False  # D was pressed during the held-back Win
        # (vk, is_down) events we injected ourselves and must let through
        self.replay = []

    def attach(self, listener: keyboard.Listener):
        """Give us the listener whose win32_event_filter is ours (for suppress_event)."""
        self.listener = listener

    # ----------------------------
    # helpers
    # ----------------------------
    def _swipe_async(self, swipe):
        # serial I/O off the hook thread (Windows drops slow hooks)
        threading.Thread(target=self.on_swipe, args=swipe, daemon=True).start()

    def _tap_mask_key(self):
        mask = keyboard.KeyCode.from_vk(VK_MASK)
        self.controller.press(mask)
        self.controller.release(mask)

    def _track_mods(self, vk, down, up):
        if vk in VK_WIN or vk in VK_CTRL:
            if down:
                self.held_mods.add(vk)
            elif up:
                self.held_mods.discard(vk)

    def _suppress(self):
        # raises inside pynput; nothing after this runs
        self.listener.suppress_event()

    # ----------------------------
    # pynput win32_event_filter. Runs in the Win32 hook thread: keep it fast.
    # ----------------------------
    def win32_event_filter(self, msg, data):
        if self.listener is None:
            return True

        vk = data.vkCode
        down = msg in (WM_KEYDOWN, WM_SYSKEYDOWN)
        up = msg in (WM_KEYUP, WM_SYSKEYUP)
        injected = bool(data.flags & LLKHF_INJECTED)

        if DEBUG:
            print(f"[key] vk={vk:#04x} msg={msg:#05x} flags={data.flags:#04x} "
                  f"held={sorted(hex(m) for m in self.held_mods)} "
                  f"pending_win={self.pending_win}")

        # our own replayed events: just track them
        if self.replay and self.replay[0] == (vk, down):
            self.replay.pop(0)
            self._track_mods(vk, down, up)
            return True

        # Options+ "Start menu" (Win tap) and "Show desktop" (Win+D) arrive as
        # an injected Win press with no Ctrl. Windows can't be stopped from
        # opening Start once it has seen Win, so hold the press back entirely
        # until we know what it is.
        if self.pending_win is not None:
            if vk == self.pending_win:
                if up:
                    if not self.pending_combo:
                        self._swipe_async(SWIPE_UP)
                    self.pending_win = None
                self._suppress()  # up, or auto-repeat
            if vk == VK_D and down:
                self._swipe_async(SWIPE_DOWN)
                self.pending_combo = True
                self.swallowed_keys.add(vk)
                self._suppress()
            if down:
                # some other Win+<key> combo: release the held-back Win, then the key
                win, self.pending_win = self.pending_win, None
                self.replay += [(win, True), (vk, True)]
                self.controller.press(keyboard.KeyCode.from_vk(win))
                self.controller.press(keyboard.KeyCode.from_vk(vk))
                self._suppress()

        if (vk in VK_WIN and down and injected and self.is_active()
                and not self.held_mods & (VK_WIN | VK_CTRL)):
            self.pending_win = vk
            self.pending_combo = False
            self._suppress()

        if vk in VK_WIN or vk in VK_CTRL:
            self._track_mods(vk, down, up)
            return True

        if up and vk in self.swallowed_keys:
            self.swallowed_keys.discard(vk)
            self._suppress()

        # Ctrl+Win+Left/Right (switch desktop)
        if not (down and self.held_mods & VK_WIN and self.held_mods & VK_CTRL):
            return True
        if vk == VK_LEFT:
            swipe = SWIPE_LEFT
        elif vk == VK_RIGHT:
            swipe = SWIPE_RIGHT
        else:
            return True
        if not self.is_active():
            return True

        self._swipe_async(swipe)
        self.swallowed_keys.add(vk)
        self._tap_mask_key()
        self._suppress()
