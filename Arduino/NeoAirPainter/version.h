#define SKETCH_VERSION "1.2.0"

/*
v1.0.0 (Initial version)
- Reads (image.bmp) BMP file from SPIFFS and displays the columns on NeoPixel LED string
- Completely Async
- WiFiManager Captive Portal to get WiFi credentials (Compile with -DUSE_EADNS)
- Waits 180s on WiFiManager to conenct to AP, then fails into stand-alone AP mode
- Connect RX/GPIO3 to DIN of NeoPixel strip
- Upload included new firmware at http://<IP_ADDRESS>/update

v1.1.0
- Mode major changes to read image from top to bottom, now image is read left to right

v1.2.0
- Arduino IDE Compatibility (use PIO instead!!!!)

*/