#!/usr/bin/env python3
"""
push_image_to_python - PC viewer for the PicoCamera push_image_to_python sketch.

Protocol (binary, over USB serial):
    RGB565 frame:  b"SRGB" + width*height*2 raw bytes + b"ERGB"
    JPEG frame:    b"SJPG" + raw JPEG bytes (FFD8...FFD9) + b"EJPG"

Dependencies (the only two external packages):
    pip install pyserial Pillow

tkinter comes with the standard Python installer.
"""

import io
import queue
import threading
import time
import tkinter as tk
from tkinter import ttk

import serial
import serial.tools.list_ports
from PIL import Image, ImageTk

# Must match the sketch: frame_size FRAMESIZE_QVGA = 320x240
RGB_WIDTH = 320
RGB_HEIGHT = 240
RGB_FRAME_BYTES = RGB_WIDTH * RGB_HEIGHT * 2

HDR_RGB, TAIL_RGB = b"SRGB", b"ERGB"
HDR_JPG, TAIL_JPG = b"SJPG", b"EJPG"

BAUD = 115200  # ignored by USB CDC, but pyserial wants one


def rgb565_be_to_image(data: bytes) -> Image.Image:
    """Convert big-endian RGB565 (OV2640 wire order) to a Pillow image."""
    swapped = bytearray(len(data))
    swapped[0::2] = data[1::2]   # Pillow "BGR;16" wants little-endian 565
    swapped[1::2] = data[0::2]
    return Image.frombytes("RGB", (RGB_WIDTH, RGB_HEIGHT),
                           bytes(swapped), "raw", "BGR;16")


class SerialReader(threading.Thread):
    """Accumulates serial bytes and cuts out complete frames."""

    def __init__(self, port: str, out: queue.Queue):
        super().__init__(daemon=True)
        self.out = out
        self.stop_event = threading.Event()
        self.ser = serial.Serial(port, BAUD, timeout=0.1)

    def run(self):
        buf = bytearray()
        while not self.stop_event.is_set():
            chunk = self.ser.read(4096)
            if chunk:
                buf += chunk
            # extract as many complete frames as available
            while True:
                frame = self._pop_frame(buf)
                if frame is None:
                    break
                try:
                    self.out.put_nowait(frame)
                except queue.Full:
                    pass  # GUI is busy; drop the frame, keep the stream live

    def _pop_frame(self, buf: bytearray):
        # find the nearest header of either kind
        i_rgb = buf.find(HDR_RGB)
        i_jpg = buf.find(HDR_JPG)
        if i_rgb == -1 and i_jpg == -1:
            buf.clear()
            return None
        is_rgb = i_rgb != -1 and (i_jpg == -1 or i_rgb < i_jpg)
        start = i_rgb if is_rgb else i_jpg
        del buf[:start]

        if is_rgb:
            need = 4 + RGB_FRAME_BYTES + 4
            if len(buf) < need:
                return None
            payload = bytes(buf[4:4 + RGB_FRAME_BYTES])
            if bytes(buf[4 + RGB_FRAME_BYTES:need]) != TAIL_RGB:
                del buf[:4]  # bad trailer: resync after this header
                return None
            del buf[:need]
            return ("rgb", payload)

        # JPEG: payload ends at the EJPG trailer
        end = buf.find(TAIL_JPG, 4)
        if end == -1:
            if len(buf) > 4 * 1024 * 1024:  # trailer never came, drop garbage
                del buf[:4]
            return None
        payload = bytes(buf[4:end])
        del buf[:end + 4]
        if payload[:2] != b"\xff\xd8":
            return None  # false trailer match inside entropy data; resync
        return ("jpg", payload)

    def close(self):
        self.stop_event.set()
        try:
            self.ser.close()
        except serial.SerialException:
            pass


class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("PicoCamera viewer")
        self.resizable(False, False)

        bar = ttk.Frame(self)
        bar.pack(fill="x", padx=8, pady=6)

        ttk.Label(bar, text="Port:").pack(side="left")
        self.port_var = tk.StringVar()
        self.port_box = ttk.Combobox(bar, textvariable=self.port_var,
                                     width=18, state="readonly")
        self.port_box.pack(side="left", padx=4)
        ttk.Button(bar, text="Refresh", command=self.refresh_ports).pack(side="left")
        self.connect_btn = ttk.Button(bar, text="Connect", command=self.toggle_connect)
        self.connect_btn.pack(side="left", padx=8)

        self.image_label = ttk.Label(self)
        self.image_label.pack(padx=8, pady=4)

        self.status_var = tk.StringVar(value="not connected")
        ttk.Label(self, textvariable=self.status_var).pack(fill="x", padx=8, pady=6)

        self.reader = None
        self.frames = queue.Queue(maxsize=4)
        self.photo = None
        self.frames_done = 0
        self.fps_since = time.monotonic()

        self.refresh_ports()
        self.after(30, self.poll_frames)
        self.protocol("WM_DELETE_WINDOW", self.on_close)

    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_box["values"] = ports
        if ports and not self.port_var.get():
            self.port_var.set(ports[0])

    def toggle_connect(self):
        if self.reader:
            self.disconnect()
            return
        port = self.port_var.get()
        if not port:
            self.status_var.set("no port selected")
            return
        try:
            self.reader = SerialReader(port, self.frames)
        except serial.SerialException as e:
            self.status_var.set(f"open failed: {e}")
            self.reader = None
            return
        self.reader.start()
        self.connect_btn.config(text="Disconnect")
        self.status_var.set(f"connected: {port}")
        self.frames_done = 0
        self.fps_since = time.monotonic()

    def disconnect(self):
        if self.reader:
            self.reader.close()
            self.reader.join(timeout=1)
            self.reader = None
        self.connect_btn.config(text="Connect")
        self.status_var.set("not connected")

    def poll_frames(self):
        try:
            while True:
                kind, payload = self.frames.get_nowait()
                try:
                    if kind == "rgb":
                        img = rgb565_be_to_image(payload)
                    else:
                        img = Image.open(io.BytesIO(payload))
                except Exception as e:
                    self.status_var.set(f"decode error: {e}")
                    continue
                self.photo = ImageTk.PhotoImage(img)
                self.image_label.config(image=self.photo)
                self.frames_done += 1
        except queue.Empty:
            pass

        now = time.monotonic()
        if now - self.fps_since >= 1.0 and self.reader:
            fps = self.frames_done / (now - self.fps_since)
            self.status_var.set(f"{fps:.1f} fps")
            self.frames_done = 0
            self.fps_since = now

        self.after(30, self.poll_frames)

    def on_close(self):
        self.disconnect()
        self.destroy()


if __name__ == "__main__":
    App().mainloop()
