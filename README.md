# Acoustic Audio-Modem: Real-Time FSK Transceiver on PYNQ-Z2

### UC San Diego - WES 237a (Winter 2026) Final Project ###

### Contributors: ###
  - Genaro Salazar Ruiz
  - Alex McGee

## 📌 Project Overview
Reliable digital communication typically relies on RF or wired connections, leaving a gap in environments where RF is severely attenuated (e.g., underwater) or strictly prohibited (e.g., secure air-gapped facilities). 

This project establishes a robust, air-gapped data link using purely acoustic signals. We built a real-time Frequency Shift Keying (FSK) transceiver using a PYNQ-Z2 board, a passive buzzer, and a commercial microphone. The system encodes logic states into distinct audio tones (4 kHz to 6 kHz range), framing the data using a standard 20-baud 8-N-1 UART protocol.

## ⚙️ Hardware Architecture
The system proves that a reliable communication link can be engineered using strictly low-cost, low-fidelity components.
* **Development Board:** [PYNQ-Z2](https://www.amd.com/en/corporate/university-program/aup-boards/pynq-z2.html) (Zynq-7000 SoC)
* **Audio Codec:** Onboard ADAU1761 (Capturing audio via the 3.5mm TRRS jack at 48 kHz).
* **Transmitter:** HW-508 Passive Piezoelectric Buzzer (Driven via PMODB GPIO using PWM).
* **Microphone:** Commercial-grade headset microphone.

## 💻 Software Architecture
To overcome execution overhead and latency inherent to Python, we developed a hybrid hardware/software co-design utilizing a 4-thread architecture. 

* **Python (High-Level Management):** Handles multi-thread management, raw terminal I/O (`termios`), and hardware abstraction via the `pynq` library.
* **C/C++ (Real-Time DSP):** Computationally heavy DSP (the Goertzel algorithm) and bit-level state-machine decoding are offloaded to custom C++ shared libraries (`dsp_engine.so` and `decoder.so`).

**Zero-Copy Memory:** A critical design choice was passing memory pointers directly from the Python audio buffer to the C++ DSP engine using `numpy` arrays and the `ctypes` library. This prevents CPU starvation and buffer overruns.

## 🚀 Key DSP Features
1. **Real-Time Mean Subtraction:** Eliminates hardware DC bias from the microphone by zero-centering the waveform before running the Goertzel loop.
2. **Relative Ratio Thresholding:** Abandons absolute magnitude thresholds. A bit is only considered valid if its tone is at least 3x stronger than the competing frequency, mitigating ambient room noise.
3. **UART Center-Sampling:** The C++ state machine hunts for the Start Bit's falling edge and delays its read until the exact center of the bit window to avoid acoustic phase-shift noise at the boundaries.
4. **Hardware Drift Calibration:** The Goertzel engine is actively tuned to hunt for real-world frequencies (~4500 Hz ± 300 Hz in this setup) to compensate for PYNQ-specific PWM hardware drift.

## 📈 Results & Known Limitations
The system successfully functions as a full-duplex, air-gapped acoustic modem, capable of transmitting and decoding text across a room in real-time. 

**Known Issues:**
* **Bit Rejection:** The raw 8-N-1 UART stream lacks error correction. Prolonged room reverberation can induce bit-flips
* **RTOS Memory Fault:** The system consistently encounters a segmentation fault/core dump after ~60 seconds of continuous keyboard input. We suspect this is due to unmanaged thread collisions between blocking terminal inputs and the high-speed audio DMA streams.

## 🔮 Future Work
* **Packet Overhead:** Transitioning from raw UART to structured data packets utilizing synchronization words (preambles) and checksums for error rejection.
* **OFDM Modulation:** Upgrading from strict two-tone FSK to Orthogonal Frequency-Division Multiplexing (OFDM) to distribute the data stream across a wider spectrum, mitigating room echoes and exponentially increasing the data rate.

---
*Developed for the Master of Advanced Studies in Wireless Embedded Systems program at UC San Diego.*

---

## 📄 License
Licensed under the MIT License – see the [LICENSE](LICENSE) file for details.
