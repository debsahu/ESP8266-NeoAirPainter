#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>           //https://github.com/bblanchon/ArduinoJson
#include <Hash.h>
#include <ESPAsyncTCP.h>           //https://github.com/me-no-dev/ESPAsyncTCP
#include <ESPAsyncWebServer.h>     //https://github.com/me-no-dev/ESPAsyncWebServer
#include <SPIFFSEditor.h>
#include <ESPAsyncDNSServer.h>     //https://github.com/devyte/ESPAsyncDNSServer
// #include <DNSServer.h>          //https://github.com/me-no-dev/ESPAsyncUDP
#include "src/dependencies/ESPAsyncWiFiManager/ESPAsyncWiFiManager.h" //Copy of https://github.com/alanswx/ESPAsyncWiFiManager with #define USE_EADNS as of 1/4/19
#include <Ticker.h>
#include <algorithm>
#include <NeoPixelBrightnessBus.h> //https://github.com/Makuna/NeoPixelBus
#include <NeoPixelAnimator.h>
#include "version.h"

// Soft AP is at 192.168.1.1
char AP_PASS[32] = ""; //passowrd for Soft AP mode connection

//BMP image scrolling too slow? read below ▼
//Uncomment the line below to display image from top to bottom instead of default left to right
//Top to Bottom BMP image read is FASTER and can give higher control over the animation speed
//If using top to bottom approach, remember to rotate BMP by 90deg clockwise before upload!!!!!
#define TOP_TO_BOTTOM_METHOD

#define HOSTNAME "NeoAirPainter"
#define HTTP_PORT 80
#define DNS_PORT 53

const uint16_t PixelCount = 120; // the sample images are meant for 144 pixels
const uint16_t AnimCount = 1;    // we only need one
typedef NeoGrbFeature MyPixelColorFeature;

NeoPixelBrightnessBus<MyPixelColorFeature, NeoEsp8266Dma800KbpsMethod> strip(PixelCount);
NeoPixelAnimator animations(AnimCount, NEO_MILLISECONDS);
NeoBitmapFile<MyPixelColorFeature, File> image;
NeoGamma<NeoGammaTableMethod> gammaColor;
uint16_t animState;
uint8_t brightness = 128;
uint16_t speed = 30;

Ticker animStart;

bool startMDNS = true;
AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");
AsyncDNSServer dns;
// DNSServer dns;

#define favicon_ico_gz_len 278
const uint8_t favicon_ico_gz[] PROGMEM = {
  0x1f, 0x8b, 0x08, 0x08, 0xb9, 0xc2, 0x25, 0x5c, 0x00, 0x03, 0x66, 0x61,
  0x76, 0x69, 0x63, 0x6f, 0x6e, 0x2e, 0x69, 0x63, 0x6f, 0x00, 0xbd, 0x92,
  0x31, 0x6e, 0xc3, 0x30, 0x0c, 0x45, 0x19, 0x24, 0x40, 0x56, 0x4f, 0x99,
  0x0b, 0x04, 0x86, 0x3c, 0xe5, 0x0a, 0x1d, 0xd3, 0xa3, 0x74, 0xe8, 0x94,
  0x0b, 0x14, 0x3e, 0x40, 0xa7, 0x9c, 0x83, 0x77, 0xc8, 0x51, 0x7c, 0x03,
  0x43, 0xf6, 0xd2, 0x4d, 0xf9, 0xb4, 0xbf, 0x10, 0xc4, 0x90, 0x14, 0x4f,
  0x91, 0xf0, 0x0c, 0xe3, 0x93, 0x9f, 0x24, 0x68, 0x8b, 0x6c, 0x70, 0xab,
  0x4a, 0xf0, 0xfc, 0x90, 0xef, 0x9d, 0xc8, 0x41, 0x44, 0x1a, 0x00, 0x09,
  0xca, 0xac, 0x4f, 0x07, 0xb1, 0xe3, 0x69, 0x26, 0x9e, 0x10, 0x82, 0xec,
  0xf7, 0xff, 0x11, 0x07, 0x14, 0x0c, 0x44, 0xa9, 0x4d, 0x71, 0xcb, 0x4d,
  0xc1, 0x78, 0x03, 0x7a, 0x10, 0x16, 0xf4, 0x8c, 0xbd, 0xf2, 0x6b, 0xc2,
  0x1b, 0xd1, 0x15, 0x7e, 0x5f, 0xf0, 0x8f, 0x6f, 0xf0, 0x17, 0xe7, 0x0f,
  0xb6, 0xeb, 0x14, 0x0f, 0xbf, 0xcb, 0xed, 0xef, 0x6f, 0x7b, 0xf9, 0x42,
  0xae, 0xd5, 0x18, 0x88, 0xbd, 0xbb, 0x20, 0xcf, 0xc5, 0x90, 0x5b, 0x73,
  0x0e, 0x4f, 0xf4, 0xba, 0xfd, 0x39, 0x23, 0xd4, 0x07, 0x6b, 0xf5, 0x8c,
  0x69, 0x0d, 0xbd, 0x0e, 0x28, 0x18, 0x88, 0x52, 0x13, 0xf6, 0x5a, 0x7a,
  0x23, 0x96, 0x67, 0x35, 0x7a, 0xb0, 0xbc, 0xa6, 0x35, 0xc8, 0xf1, 0x05,
  0xff, 0xc8, 0x5e, 0xb9, 0xab, 0x2b, 0xfc, 0xbe, 0xe0, 0x1f, 0x57, 0xcc,
  0xff, 0xca, 0xef, 0x0a, 0xfb, 0x8b, 0x7b, 0xcb, 0xce, 0xcf, 0x1d, 0xd6,
  0x9c, 0xc3, 0x13, 0xa5, 0x16, 0x77, 0x9f, 0xdb, 0x5f, 0xfc, 0x06, 0x59,
  0xf8, 0x52, 0xb3, 0x97, 0x27, 0x4a, 0x6d, 0x8a, 0xe7, 0xfe, 0x5b, 0xa3,
  0xfb, 0x15, 0xb9, 0x7d, 0x26, 0xd8, 0x88, 0x74, 0xb0, 0xb7, 0x92, 0x38,
  0xed, 0x5c, 0xb7, 0xc5, 0x00, 0x77, 0x0c, 0x84, 0x24, 0x03, 0x7e, 0x04,
  0x00, 0x00
};

File fsUploadFile;
bool shouldReboot = false;
extern const char update_html[];
extern const char root_html[];

void BMPGammaBrightness(uint8_t _brightness)
{
#ifdef TOP_TO_BOTTOM_METHOD
    for (uint16_t index = 0; index < std::min(image.Width(), PixelCount); index++)
    {
        RgbColor color = strip.GetPixelColor(index);
#else
    for (uint16_t index = 0; index < std::min(image.Height(), PixelCount); index++)
    {
        RgbColor color = image.GetPixelColor(animState, index);
#endif
        color = gammaColor.Correct(color);
        strip.SetPixelColor(index, color);
    }
    strip.SetBrightness(_brightness);
}

void LoopAnimUpdate(const AnimationParam &param)
{
    if (param.state == AnimationState_Completed)
    {
        animations.RestartAnimation(param.index);         // done, time to restart this position tracking animation/timer

        #ifndef TOP_TO_BOTTOM_METHOD
        //new method reads left to right of BMP
        animState = (animState + 1) % image.Width();     // increment and wrap
        #else
        // old method reads top to bottom of BMP
        image.Blt(strip, 0, 0, animState, image.Width()); // draw the complete row at animState to the complete strip
        animState = (animState + 1) % image.Height();     // increment and wrap
        #endif
        BMPGammaBrightness(brightness);
    }
}

void readImage(void)
{
    File bitmapFile = SPIFFS.open("/image.bmp", "r"); // open the file
    if (!bitmapFile)
    {
        Serial.println("File open fail, or not present"); // don't do anything more:
        return;
    }

    if (!image.Begin(bitmapFile)) // initialize the image with the file
    {
        Serial.println("File format fail, not a supported bitmap"); // don't do anything more:
        return;
    }

    animState = 0; // we use the index 0 animation to time how often we rotate all the pixels
    animations.StartAnimation(0, speed, LoopAnimUpdate);
}

void changeAnimationSpeed(uint16_t _speed)
{
    animations.StopAnimation(0);
    animState = 0;
    animations.StartAnimation(0, _speed, LoopAnimUpdate);
}

bool processJson(String &message)
{
    //const size_t bufferSize = JSON_OBJECT_SIZE(1) + 20;
    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(message);

    if (!root.success())
    {
        Serial.println("parseObject() failed");
        return false;
    }

    if (root.containsKey("brightness"))
    {
        const char *brighness_str = root["brightness"];
        Serial.printf("Brighness JSON parsed: %s\n", brighness_str);
        if (strcmp(brighness_str, "up") == 0)
        {
            brightness = constrain(brightness * 2, 1, 255);
            Serial.printf("Brighness[+]: %d\n", brightness);
        }
        else if (strcmp(brighness_str, "down") == 0)
        {
            brightness = constrain(brightness / 1.5, 1, 255);
            Serial.printf("Brighness[-]: %d\n", brightness);
        }
        brightness = constrain(brightness, 1, 255);
    }

    if (root.containsKey("speed"))
    {
        const char *speed_str = root["speed"];
        Serial.printf("Speed JSON parsed: %s\n", speed_str);
        if (strcmp(speed_str, "up") == 0)
        {
            speed = constrain(speed - 1, 1, 32767); // Smaller number means faster animations
            Serial.printf("Speed[+]: %d\n", speed);
        }
        else if (strcmp(speed_str, "down") == 0)
        {
            speed = constrain(speed + 1, 1, 32767); // Larger number means slower animations
            Serial.printf("Speed[-]: %d\n", speed);
        }
        changeAnimationSpeed(speed);
    }

    jsonBuffer.clear();
    return true;
}

void setup()
{
    Serial.begin(115200);
    delay(10);

    Serial.println();
    Serial.println();
    Serial.println();

    if (SPIFFS.begin())
    {
        Dir dir = SPIFFS.openDir("/");
        while (dir.next())
        {
            String fileName = dir.fileName();
            size_t fileSize = dir.fileSize();
            Serial.printf("FS File: %s, size: %dB\n", fileName.c_str(), fileSize);
        }

        FSInfo fs_info;
        SPIFFS.info(fs_info);
        Serial.printf("FS Usage: %d/%d bytes\n\n", fs_info.usedBytes, fs_info.totalBytes);
    }

    strip.Begin();
    strip.Show();

    char NameChipId[64] = {0}, chipId[7] = {0};
    snprintf(chipId, sizeof(chipId), "%06x", ESP.getChipId());
    snprintf(NameChipId, sizeof(NameChipId), "%s_%06x", HOSTNAME, ESP.getChipId());

    WiFi.mode(WIFI_AP_STA);
    WiFi.hostname(const_cast<char *>(NameChipId));
    AsyncWiFiManager wifiManager(&server, &dns); //Local intialization. Once its business is done, there is no need to keep it around
    wifiManager.setConfigPortalTimeout(180);      //sets timeout until configuration portal gets turned off, useful to make it all retry or go to sleep in seconds

    if (!wifiManager.autoConnect(NameChipId))
    {
        Serial.println("Failed to connect and hit timeout\nEntering Station Mode");
    }
    else
    {
        Serial.println("---------------------------------------");
        Serial.print("Router IP: ");
        Serial.println(WiFi.localIP());
        //ESP.restart();
        //nothing to do here, entering AP mode
        startMDNS = false;
    }

    Serial.println("---------------------------------------");
    IPAddress apIP(192, 168, 1, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(NameChipId, AP_PASS);
    Serial.print("HotSpt IP: ");
    Serial.println(WiFi.softAPIP());
    dns=AsyncDNSServer();
    dns.start(DNS_PORT, "*", WiFi.softAPIP());

    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_CONNECT)
        {
            Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
            client->ping();
        }
        else if (type == WS_EVT_DISCONNECT)
        {
            Serial.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
        }
        else if (type == WS_EVT_ERROR)
        {
            Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
        }
        else if (type == WS_EVT_PONG)
        {
            Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
        }
        else if (type == WS_EVT_DATA)
        {
            AwsFrameInfo *info = (AwsFrameInfo *)arg;
            String msg = "";
            if (info->final && info->index == 0 && info->len == len)
            {
                if (info->opcode == WS_TEXT)
                {
                    for (size_t i = 0; i < info->len; i++)
                    {
                        msg += (char)data[i];
                    }
                    Serial.println(msg);
                    Serial.println(processJson(msg) ? "Success" : "Failed");
                }
            }
        }
    });
    server.addHandler(&ws);
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", root_html);
    });
    server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", SKETCH_VERSION);
    });
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("/restart");
        request->send(200, "text/html", "<META http-equiv='refresh' content='15;URL=/'> Restarting...");
        ESP.restart();
    });
    server.on("/reset_wlan", HTTP_GET, [](AsyncWebServerRequest *request) {
        Serial.println("/reset_wlan");
        request->send(200, "text/html", "<META http-equiv='refresh' content='15;URL=/'> Resetting WLAN and restarting...");
        server.reset();
        dns=AsyncDNSServer();
        AsyncWiFiManager wifiManager(&server, &dns);
        wifiManager.resetSettings();
        ESP.restart();
    });
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "image/x-icon", favicon_ico_gz, favicon_ico_gz_len);
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", update_html);
        request->send(response);
    });
    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
        shouldReboot = !Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", shouldReboot ? "<META http-equiv='refresh' content='15;URL=/'>Update Success, rebooting..." : "FAIL");
        response->addHeader("Connection", "close");
        request->send(response); },
              [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
                  if (!filename.endsWith(".bin"))
                  {
                      return;
                  }
                  if (!index)
                  {
                      Serial.printf("Update Start: %s\n", filename.c_str());
                      Update.runAsync(true);
                      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
                      {
                          Update.printError(Serial);
                      }
                  }
                  if (!Update.hasError())
                  {
                      if (Update.write(data, len) != len)
                      {
                          Update.printError(Serial);
                      }
                  }
                  if (final)
                  {
                      if (Update.end(true))
                          Serial.printf("Update Success: %uB\n", index + len);
                      else
                      {
                          Update.printError(Serial);
                      }
                  }
              });
    server.on("/start", HTTP_GET, [](AsyncWebServerRequest *request) {
        animStart.once(1, readImage);
        request->redirect("/");
    });
    server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
        animations.StopAnimation(0);
        animState = 0;
        request->redirect("/");
    });
    server.on("/brightnessup", HTTP_GET, [](AsyncWebServerRequest *request) {
        brightness = constrain(brightness * 2, 1, 255);
        request->redirect("/");
    });
    server.on("/brightnessdown", HTTP_GET, [](AsyncWebServerRequest *request) {
        brightness = constrain(brightness / 1.5, 1, 255);
        request->redirect("/");
    });
    server.on("/speedup", HTTP_GET, [](AsyncWebServerRequest *request) {
        speed = constrain(speed - 1, 1, 32767);
        request->redirect("/");
    });
    server.on("/speeddown", HTTP_GET, [](AsyncWebServerRequest *request) {
        speed = constrain(speed + 1, 1, 32767);
        request->redirect("/");
    });
    // server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
    //     request->redirect("/");
    //     //AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", uploadspiffs_html);
    //     //request->send(response);
    // });
    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
        animStart.once(1, readImage);
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "<META http-equiv='refresh' content='2;URL=/'>Upload Success, redirecting to main page ...");
        //response->addHeader("Connection", "close");
        request->send(response); },
              [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
                  if (!filename.endsWith(".bmp"))
                  {
                      Serial.println("Please only upload bmp file!!!");
                      return;
                  }
                  if (!index)
                  {
                      Serial.printf("UploadStart: %s\n", filename.c_str());
                      //if (!filename.startsWith("/")) filename = "/" + filename;
                      if (SPIFFS.exists("/image.bmp"))
                          SPIFFS.remove("/image.bmp");
                      fsUploadFile = SPIFFS.open("/image.bmp", "w"); //write all bmp file data to image.bmp
                  }
                  for (size_t i = 0; i < len; i++)
                      fsUploadFile.write(data[i]);
                  if (final)
                  {
                      fsUploadFile.close();
                      Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index + len);
                  }
              });

    server.on("/image.bmp", [](AsyncWebServerRequest *request) {
        if (SPIFFS.exists("/image.bmp"))
        {
            AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/image.bmp", "image/bmp");
            request->send(response);
        }
        else
        {
            request->send(404);
        }
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        String filename = request->url();
        String ContentType = "text/plain";

        if (filename.endsWith(".htm"))
            ContentType = "text/html";
        else if (filename.endsWith(".html"))
            ContentType = "text/html";
        else if (filename.endsWith(".css"))
            ContentType = "text/css";
        else if (filename.endsWith(".js"))
            ContentType = "application/javascript";
        else if (filename.endsWith(".png"))
            ContentType = "image/png";
        else if (filename.endsWith(".gif"))
            ContentType = "image/gif";
        else if (filename.endsWith(".jpg"))
            ContentType = "image/jpeg";
        else if (filename.endsWith(".ico"))
            ContentType = "image/x-icon";
        else if (filename.endsWith(".xml"))
            ContentType = "text/xml";
        else if (filename.endsWith(".pdf"))
            ContentType = "application/x-pdf";
        else if (filename.endsWith(".zip"))
            ContentType = "application/x-zip";
        else if (filename.endsWith(".gz"))
            ContentType = "application/x-gzip";
        else if (filename.endsWith("ico.gz"))
            ContentType = "image/x-icon";

        if (SPIFFS.exists(filename + ".gz") || SPIFFS.exists(filename))
        {
            if (SPIFFS.exists(filename + ".gz"))
                filename += ".gz";
            AsyncWebServerResponse *response = request->beginResponse(SPIFFS, filename, ContentType);
            if (filename.endsWith(".gz"))
                response->addHeader("Content-Encoding", "gzip");
            request->send(response);
            return;
        }

        Serial.print("NOT_FOUND: ");
        if (request->method() == HTTP_GET)
            Serial.print("GET");
        else if (request->method() == HTTP_POST)
            Serial.print("POST");
        else if (request->method() == HTTP_DELETE)
            Serial.print("DELETE");
        else if (request->method() == HTTP_PUT)
            Serial.print("PUT");
        else if (request->method() == HTTP_PATCH)
            Serial.print("PATCH");
        else if (request->method() == HTTP_HEAD)
            Serial.print("HEAD");
        else if (request->method() == HTTP_OPTIONS)
            Serial.print("OPTIONS");
        else
            Serial.print("UNKNOWN");

        Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

        if (request->contentLength())
        {
            Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
            Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
        }

        int headers = request->headers();
        int i;
        for (i = 0; i < headers; i++)
        {
            AsyncWebHeader *h = request->getHeader(i);
            Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
        }

        int params = request->params();
        for (i = 0; i < params; i++)
        {
            AsyncWebParameter *p = request->getParam(i);
            if (p->isFile())
            {
                Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
            }
            else if (p->isPost())
            {
                Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            }
            else
            {
                Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
            }
        }

        // request->send(404);
        request->redirect("/"); // send all DNS requests to root
        //request->send_P(200, "text/html", root_html);
    });
    server.addHandler(new SPIFFSEditor("admin", "admin"));

    if(startMDNS) {
        MDNS.setInstanceName(String(HOSTNAME " (" + String(chipId) + ")").c_str());
        if (MDNS.begin(HOSTNAME))
        {
            MDNS.addService("http", "tcp", HTTP_PORT);
            Serial.printf(">>> MDNS Started: http://%s.local/\n", NameChipId);
        }
        else
        {
            Serial.println(F(">>> Error setting up mDNS responder <<<"));
        }
    }

    server.begin();

    if (SPIFFS.exists("/image.bmp"))
        animStart.once(1, readImage);
}

void loop()
{
    MDNS.update();

    if (shouldReboot)
        ESP.reset();

    animations.UpdateAnimations();
    strip.Show();
}
