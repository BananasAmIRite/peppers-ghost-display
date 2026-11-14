"""
Spectrum visualizer source for the Spotify screen's SPECTRUM page.

Captures whatever the default Windows output device is playing (WASAPI loopback),
runs an FFT, buckets it into NUM_BANDS log-spaced bands and streams one byte per band
to the cube as SPOTIFY_SPECTRUM packets.

Streaming is driven entirely by the cube: it sends SPOTIFY_VIZ_STATE [1] when the SPECTRUM
page becomes visible (and every second after that as a keepalive), and [0] when the page or
the Spotify screen is left. If keepalives stop arriving, capture stops on its own.
"""

import threading
import time
import warnings
from collections import deque

import numpy as np
import soundcard as sc

import lib.comms.comms as comms

NUM_BANDS = 32             # must match SPECTRUM_BANDS in SpotifyScreen.h
SAMPLE_RATE = 48000
FPS = 30
HOP = SAMPLE_RATE // FPS   # new samples per frame
FFT_SIZE = 4096            # analysis window (~85ms); needed so the lowest bands span >= 1 FFT bin
F_MIN = 80
F_MAX = 16000

DYNAMIC_RANGE_DB = 45      # dB below the running reference that maps to an empty bar
REF_DECAY_DB_PER_SEC = 6   # how fast auto-gain recovers after a loud section
SILENCE_DB = -90           # below this the reference stops falling (keeps silence looking silent)

# Delay before frames are sent, to line the bars up with Bluetooth headphones (loopback captures
# audio before it goes out over BT). ~0.15-0.25s is typical; 0 for wired speakers.
OUTPUT_DELAY_S = 0.0

KEEPALIVE_TIMEOUT_S = 3.0
DEVICE_CHECK_S = 2.0       # re-check the default output device this often (e.g. switching to headphones)


def _band_bins():
    """[start, end) FFT bin ranges for each log-spaced band, each at least one bin wide."""
    freqs = np.fft.rfftfreq(FFT_SIZE, 1 / SAMPLE_RATE)
    edges = np.geomspace(F_MIN, F_MAX, NUM_BANDS + 1)
    idx = np.searchsorted(freqs, edges)
    bins = []
    for i in range(NUM_BANDS):
        lo = int(idx[i])
        hi = max(int(idx[i + 1]), lo + 1)
        bins.append((lo, hi))
    return bins


class SpectrumVisualizer:
    def __init__(self, mgr):
        self.mgr = mgr
        self._active = False
        self._last_keepalive = 0.0
        self._wake = threading.Event()
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True, name="spectrum")

        self._bins = _band_bins()
        self._window = np.hanning(FFT_SIZE).astype(np.float32)
        self._ref_db = SILENCE_DB + DYNAMIC_RANGE_DB

    # ----------------------------
    # Control (called from the serial reader thread)
    # ----------------------------
    def set_active(self, active: bool):
        if active != self._active:
            print(f"Spectrum visualizer {'started' if active else 'stopped'}")
        self._active = active
        if active:
            self._last_keepalive = time.time()
            self._wake.set()

    def _should_run(self):
        return self._active and (time.time() - self._last_keepalive) < KEEPALIVE_TIMEOUT_S

    def start(self):
        self._thread.start()

    def stop(self):
        self._stop.set()
        self._wake.set()

    # ----------------------------
    # Capture loop
    # ----------------------------
    def _run(self):
        # soundcard warns on every loopback buffer gap (e.g. while nothing is playing)
        warnings.filterwarnings("ignore", category=sc.SoundcardRuntimeWarning)

        while not self._stop.is_set():
            if not self._should_run():
                self._active = False
                self._wake.wait(timeout=0.5)
                self._wake.clear()
                continue

            try:
                self._capture_session()
            except Exception as e:
                print(f"Spectrum capture error: {e}")
                self._stop.wait(1.0)

    def _capture_session(self):
        speaker = sc.default_speaker()
        mic = sc.get_microphone(id=str(speaker.name), include_loopback=True)
        print(f"Spectrum capturing from: {speaker.name}")

        buf = np.zeros(FFT_SIZE, dtype=np.float32)
        pending = deque()  # (send_time, payload) for OUTPUT_DELAY_S
        last_device_check = time.time()

        with mic.recorder(samplerate=SAMPLE_RATE, channels=2, blocksize=HOP) as rec:
            while self._should_run() and not self._stop.is_set():
                data = rec.record(numframes=HOP)  # blocks ~1/FPS
                mono = data.mean(axis=1).astype(np.float32)

                buf = np.roll(buf, -len(mono))
                buf[-len(mono):] = mono[-FFT_SIZE:]

                pending.append((time.time() + OUTPUT_DELAY_S, self._compute_bands(buf)))
                while pending and pending[0][0] <= time.time():
                    _, payload = pending.popleft()
                    self.mgr.send_uart_message(comms.SPOTIFY_SPECTRUM, payload, log=False)

                # output device changed (e.g. headphones connected): reopen on the new one
                if time.time() - last_device_check > DEVICE_CHECK_S:
                    last_device_check = time.time()
                    if sc.default_speaker().name != speaker.name:
                        return

    def _compute_bands(self, buf: np.ndarray) -> bytes:
        spectrum = np.fft.rfft(buf * self._window)
        power = (spectrum.real ** 2 + spectrum.imag ** 2) / FFT_SIZE

        # summing (not averaging) power over log-spaced bands gives wider high bands a
        # natural +3dB/octave boost, which offsets music's low-frequency-heavy balance
        band_power = np.array([power[lo:hi].sum() for lo, hi in self._bins])
        band_db = 10 * np.log10(band_power + 1e-12)

        # auto-gain: the reference jumps up to the loudest band and decays slowly
        peak = float(band_db.max())
        self._ref_db = max(peak, self._ref_db - REF_DECAY_DB_PER_SEC / FPS, SILENCE_DB + DYNAMIC_RANGE_DB)

        levels = np.clip((band_db - (self._ref_db - DYNAMIC_RANGE_DB)) / DYNAMIC_RANGE_DB, 0, 1)
        return bytes((levels * 255).astype(np.uint8))
