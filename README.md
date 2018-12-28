# ESP8266-NeoAirPainter

[![Build Status](https://travis-ci.com/debsahu/ESP8266-NeoAirPainter.svg?branch=master)](https://travis-ci.com/debsahu/ESP8266-NeoAirPainter)

Based on the idea of [Minimalistic Async code around Async E131 for ESP8266/ESP32](https://github.com/debsahu/E131_PixelPusher) but for offline purpose using 24-bit BMP files

- Reads (image.bmp) BMP file from SPIFFS and displays the rows on NeoPixel LED string
- Completely Async
- WiFiManager Captive Portal to get WiFi credentials (Compile with -DUSE_EADNS)
- Waits 180s on WiFiManager to conenct to AP, then fails into stand-alone AP mode
- Connect RX/GPIO3 to DIN of NeoPixel strip
- Upload included new firmware at http://<IP_ADDRESS>/update

[![ESP8266-NeoAirPainter](https://img.youtube.com/vi/xxxxxxxxx/0.jpg)](https://www.youtube.com/watch?v=xxxxxxxxxxxx)

## Libraries Needed

[platformio.ini](https://github.com/debsahu/ESP8266-NeoAirPainter/blob/master/platformio.ini) is included, use [PlatformIO](https://platformio.org/platformio-ide) and it will take care of installing the following libraries.

| Library                   | Link                                                       |
|---------------------------|------------------------------------------------------------|
|ESPAsyncUDP                |https://github.com/me-no-dev/ESPAsyncUDP                    |
|ESPAsyncTCP                |https://github.com/me-no-dev/ESPAsyncTCP                    |
|NeoPixelBus                |https://github.com/Makuna/NeoPixelBus                       |
|ESPAsyncWiFiManager        |https://github.com/alanswx/ESPAsyncWiFiManager              |
|ESPAsyncDNSServer          |https://github.com/devyte/ESPAsyncDNSServer                 |
|ESP Async WebServer        |https://github.com/me-no-dev/ESPAsyncWebServer              |
|ArduinoJson                |https://github.com/bblanchon/ArduinoJson                    |

## Hardware Setup

![Hardware Setup](https://github.com/debsahu/ESP8266-NeoAirPainter/blob/master/hardware-setup.png)
