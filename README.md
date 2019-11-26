Bluetooth Speaker
=================

Bluetooth Speaker based on ESP32 chip with dynamic vision effects output.

## Main Features

* A2DP Audio Streaming
* I2S & PDM Input / I2S Output
* VFX Output (Audio FFT / Rainbow / Stars / ...)
* BLE Control Interface (for VFX Output & Audio Input)
* Audio Prompt (Connected / Disconnected / WakeUp / Sleep)
* OTA Firmware Update (via SPP Profile)
* Sleep & WakeUp Key

## Obtaining

```
git clone --recursive https://github.com/redchenjs/bluetooth_speaker_esp32.git
```

* Set up the Toolchain: <https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html>
* Tested toolchain version: `esp32-2019r1`

## Configure

```
./esp-idf/tools/idf.py menuconfig
```

* All project configurations are under the `Bluetooth Speaker` menu.

## Build & Flash & Monitor

```
./esp-idf/tools/idf.py flash monitor
```

## VFX on ST7735 80x160 LCD Panel (Linear Spectrum)

<img src="docs/st7735lin.png">

## VFX on ST7735 80x160 LCD Panel (CUBE0414 Simulation)

<img src="docs/st7735sim.png">

## VFX on ST7789 135x240 LCD Panel (Logarithmic Spectrum)

<img src="docs/st7789log.png">

## VFX on CUBE0414 8x8x8 RGB Light Cube

<img src="docs/cube0414.png">

## Videos Links

* [音乐全彩光立方演示](https://www.bilibili.com/video/av25188707)
