#!/usr/bin/env python3
#
# This file is part of the PicoCamera project.
# https://github.com/umeiko/PicoCamera
#
# Author: umeko <umeko@stu.xmu.edu.cn>
# License: MIT
"""
push_image_to_python - PC viewer for the PicoCamera push_image_to_python sketch.

Protocol (binary, over USB serial):
    RGB565 frame:  b"SRGB" + width*height*2 raw bytes + b"ERGB"
    JPEG frame:    b"SJPG" + raw JPEG bytes (FFD8...FFD9) + b"EJPG"
    YUV422 frame:  b"SYUV" + width*height*2 packed YUYV bytes + b"EYUV"
    GRAYSCALE:     b"SGRY" + width*height raw Y bytes + b"EGRY"

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
WIDTH = 320
HEIGHT = 240
FRAME_BYTES = WIDTH * HEIGHT * 2  # RGB565 and YUV422 are both 2 bytes/pixel

HDR_RGB, TAIL_RGB = b"SRGB", b"ERGB"
HDR_JPG, TAIL_JPG = b"SJPG", b"EJPG"
HDR_YUV, TAIL_YUV = b"SYUV", b"EYUV"
HDR_GRY, TAIL_GRY = b"SGRY", b"EGRY"

BAUD = 115200  # ignored by USB CDC, but pyserial wants one


def rgb565_be_to_image(data: bytes) -> Image.Image:
    """Convert big-endian RGB565 (OV2640 wire order) to a Pillow image."""
    swapped = bytearray(len(data))
    swapped[0::2] = data[1::2]   # Pillow "BGR;16" wants little-endian 565
    swapped[1::2] = data[0::2]
    return Image.frombytes("RGB", (WIDTH, HEIGHT),
                           bytes(swapped), "raw", "BGR;16")


def yuyv_to_image(data: bytes) -> Image.Image:
    """Convert packed YUYV (YUV422 wire order) to a Pillow RGB image."""
    y = bytearray(WIDTH * HEIGHT)
    cb = bytearray(WIDTH * HEIGHT)
    cr = bytearray(WIDTH * HEIGHT)
    y[0::2] = data[0::4]
    y[1::2] = data[2::4]
    u, v = data[1::4], data[3::4]
    cb[0::2] = u  # one chroma sample covers two pixels
    cb[1::2] = u
    cr[0::2] = v
    cr[1::2] = v
    # NB: build the YCbCr image via merge() of three L planes.
    # Image.frombytes("YCbCr", ...) misinterprets the buffer layout
    # (Pillow 12) and produces a scrambled picture.
    return Image.merge("YCbCr", (
        Image.frombytes("L", (WIDTH, HEIGHT), bytes(y)),
        Image.frombytes("L", (WIDTH, HEIGHT), bytes(cb)),
        Image.frombytes("L", (WIDTH, HEIGHT), bytes(cr)),
    )).convert("RGB")


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
        # find the nearest header of any kind
        starts = [(i, k) for i, k in
                  ((buf.find(HDR_RGB), "rgb"),
                   (buf.find(HDR_JPG), "jpg"),
                   (buf.find(HDR_YUV), "yuv"),
                   (buf.find(HDR_GRY), "gray")) if i != -1]
        if not starts:
            buf.clear()
            return None
        start, kind = min(starts)
        del buf[:start]

        if kind in ("rgb", "yuv", "gray"):
            # fixed-length payload: header + payload + trailer
            nbytes = FRAME_BYTES if kind != "gray" else WIDTH * HEIGHT
            need = 4 + nbytes + 4
            if len(buf) < need:
                return None
            payload = bytes(buf[4:4 + nbytes])
            tail = {"rgb": TAIL_RGB, "yuv": TAIL_YUV, "gray": TAIL_GRY}[kind]
            if bytes(buf[4 + nbytes:need]) != tail:
                del buf[:4]  # bad trailer: resync after this header
                return None
            del buf[:need]
            return (kind, payload)

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
        self.last_kind = None
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
                    elif kind == "yuv":
                        img = yuyv_to_image(payload)
                    elif kind == "gray":
                        img = Image.frombytes("L", (WIDTH, HEIGHT), payload)
                    else:
                        img = Image.open(io.BytesIO(payload))
                except Exception as e:
                    self.status_var.set(f"decode error: {e}")
                    continue
                self.photo = ImageTk.PhotoImage(img)
                self.image_label.config(image=self.photo)
                self.last_kind = kind
                self.frames_done += 1
        except queue.Empty:
            pass

        now = time.monotonic()
        if now - self.fps_since >= 1.0 and self.reader:
            fps = self.frames_done / (now - self.fps_since)
            proto = {"rgb": "RGB565", "yuv": "YUV422", "gray": "GRAYSCALE",
                     "jpg": "JPEG"}.get(self.last_kind)
            if proto:
                self.status_var.set(f"{proto} {WIDTH}x{HEIGHT} | {fps:.1f} fps")
            else:
                self.status_var.set(f"{fps:.1f} fps")
            self.frames_done = 0
            self.fps_since = now

        self.after(30, self.poll_frames)

    def on_close(self):
        self.disconnect()
        self.destroy()


if __name__ == "__main__":
    App().mainloop()
