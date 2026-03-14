print(f"1/12...", end='\r')
import time
print(f"2/12...", end='\r')
import threading
print(f"3/12...", end='\r')
import tty
print(f"4/12...", end='\r')
import termios
print(f"5/12...", end='\r')
import fcntl
print(f"6/12...", end='\r')
import sys
print(f"7/12...", end='\r')
import os
print(f"8/12...", end='\r')
import pynq
print(f"9/12...", end='\r')
import ctypes
print(f"10/12...", end='\r')
import numpy as np
print(f"11/12...", end='\r')
from pynq.overlays.base import BaseOverlay
print(f"12/12...", end='\r')
base = BaseOverlay("base.bit")

pwm = pynq.lib.pmod.pmod_pwm.Pmod_PWM(base.PMODB, 3)
dsp_engine = ctypes.CDLL('./dsp_engine.so')

dsp_engine.detect_and_decode_loop.argtypes = [
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_int,
    np.ctypeslib.ndpointer(dtype=np.int32, ndim=1, flags='C_CONTIGUOUS'), 
    np.ctypeslib.ndpointer(dtype=np.float64, ndim=1, flags='C_CONTIGUOUS'),
    np.ctypeslib.ndpointer(dtype=np.float64, ndim=1, flags='C_CONTIGUOUS'),
    ctypes.POINTER(ctypes.c_int),
    ctypes.POINTER(ctypes.c_int),
]

write_buffer = [] # List of chars
freq1 = 3937
freq2 = 4734
fs = 48000
N = 480
input_buffer = np.zeros(480*2, dtype=np.int32)
output1 = np.array([0], dtype=np.float64)
output2 = np.array([0], dtype=np.float64)
start_flag = ctypes.c_int(0)
end_flag = ctypes.c_int(1)

def getch():
    global end_flag
    while(end_flag.value):
        """Reads a single character from standard input."""
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(fd)
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        write_buffer.append(ch)
        print(ch, end='', flush=True)
        if(ch == '\x03'):
             end_flag.value = False

def transmit():
    """from the list of chars `write_buffer`, convert byte to bit and transmit"""
    global end_flag
    global write_buffer
    period = 0.054
    while(end_flag.value):
        if len(write_buffer) > 0:
            byte_string = format(ord(write_buffer[0]), '08b')
            pwm.generate(250, 50)
            time.sleep(period)
            for bit in byte_string:
                pwm.generate(250 - int(bit)*42, 50)
                time.sleep(period)
            pwm.generate(208, 50)
            time.sleep(period)
            write_buffer = write_buffer[1:]
        else:
            pwm.stop()
            time.sleep(0.01)
    pwm.stop()

def receive():
    """Record 0.01s snippets to buffer. When written, set `start_flag` to 1 to indicate processing is possible."""
    global end_flag
    base.audio.select_microphone()
    base.leds[1].on()
    while(end_flag.value):
        base.audio.record(0.01)
        input_buffer[:] = base.audio.buffer
        start_flag.value = 1
        if(base.btns_gpio[0].read()): end_flag.value = 0
    base.leds[1].off()
    
t4 = threading.Thread(target=dsp_engine.detect_and_decode_loop, args=(
    freq1,
    freq2,
    fs,
    N,
    input_buffer,
    output1,
    output2,
    ctypes.byref(start_flag),
    ctypes.byref(end_flag)
))
t4.start()

t3 = threading.Thread(target=receive)
t3.start()

t1 = threading.Thread(target=getch, args=())
t1.start()

t2 = threading.Thread(target=transmit, args=())
t2.start()

print("Type to transmit (Ctrl-C to exit)...")

#getch()

# Example usage
"""print("Press a key (Ctrl-C to exit)...")
while True:
    char = getch()
    if char == '\x03': # '\x03' is the ASCII code for Ctrl-C
        print("Exiting...")
        break
    print(f"Key pressed: {repr(char)}")"""

