#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>           //https://github.com/bblanchon/ArduinoJson
#include <Hash.h>
#include <ESPAsyncTCP.h>           //https://github.com/me-no-dev/ESPAsyncTCP
#include <ESPAsyncWebServer.h>     //https://github.com/me-no-dev/ESPAsyncWebServer
#include <SPIFFSEditor.h>
#include <ESPAsyncWiFiManager.h>   //https://github.com/alanswx/ESPAsyncWiFiManager
#include <ESPAsyncDNSServer.h>     //https://github.com/devyte/ESPAsyncDNSServer
// #include <DNSServer.h>
#include <Ticker.h>
#include <pgmspace.h>
#include <NeoPixelBrightnessBus.h> //https://github.com/Makuna/NeoPixelBus
#include <NeoPixelAnimator.h>

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

AsyncWebServer server(HTTP_PORT);
AsyncWebSocket ws("/ws");
AsyncDNSServer dns;
// DNSServer dns;

File fsUploadFile;
bool shouldReboot = false;
const char update_html[] PROGMEM = R"=====(<!DOCTYPE html><html lang="en"><head><title>Firware Update</title><meta http-equiv="Content-Type" content="text/html; charset=utf-8"><meta name="viewport" content="width=device-width"><link rel="shortcut icon" type="image/x-icon" href="favicon.ico"></head><body><h3>Update Firmware</h3><br><form method="POST" action="/update" enctype="multipart/form-data"><input type="file" name="update"> <input type="submit" value="Update"></form></body></html>)=====";
const char root_html[] PROGMEM = R"=====(
<html><head><head><title>NeoAirPainter</title><meta http-equiv="Content-Type" content="text/html; charset=utf-8"><meta name="viewport" content="width=device-width"><script>!function(e,n){"function"==typeof define&&define.amd?define([],n):"undefined"!=typeof module&&module.exports?module.exports=n():e.ReconnectingWebSocket=n()}(this,function(){function e(n,t,o){function c(e,n){var t=document.createEvent("CustomEvent");return t.initCustomEvent(e,!1,!1,n),t}var i={debug:!1,automaticOpen:!0,reconnectInterval:1e3,maxReconnectInterval:3e4,reconnectDecay:1.5,timeoutInterval:2e3};o||(o={});for(var r in i)this[r]="undefined"!=typeof o[r]?o[r]:i[r];this.url=n,this.reconnectAttempts=0,this.readyState=WebSocket.CONNECTING,this.protocol=null;var s,u=this,d=!1,a=!1,l=document.createElement("div");l.addEventListener("open",function(e){u.onopen(e)}),l.addEventListener("close",function(e){u.onclose(e)}),l.addEventListener("connecting",function(e){u.onconnecting(e)}),l.addEventListener("message",function(e){u.onmessage(e)}),l.addEventListener("error",function(e){u.onerror(e)}),this.addEventListener=l.addEventListener.bind(l),this.removeEventListener=l.removeEventListener.bind(l),this.dispatchEvent=l.dispatchEvent.bind(l),this.open=function(n){s=new WebSocket(u.url,t||[]),n||l.dispatchEvent(c("connecting")),(u.debug||e.debugAll)&&console.debug("ReconnectingWebSocket","attempt-connect",u.url);var o=s,i=setTimeout(function(){(u.debug||e.debugAll)&&console.debug("ReconnectingWebSocket","connection-timeout",u.url),a=!0,o.close(),a=!1},u.timeoutInterval);s.onopen=function(){clearTimeout(i),(u.debug||e.debugAll)&&console.debug("ReconnectingWebSocket","onopen",u.url),u.protocol=s.protocol,u.readyState=WebSocket.OPEN,u.reconnectAttempts=0;var t=c("open");t.isReconnect=n,n=!1,l.dispatchEvent(t)},s.onclose=function(t){if(clearTimeout(i),s=null,d)u.readyState=WebSocket.CLOSED,l.dispatchEvent(c("close"));else{u.readyState=WebSocket.CONNECTING;var o=c("connecting");o.code=t.code,o.reason=t.reason,o.wasClean=t.wasClean,l.dispatchEvent(o),n||a||((u.debug||e.debugAll)&&console.debug("ReconnectingWebSocket","onclose",u.url),l.dispatchEvent(c("close")));var i=u.reconnectInterval*Math.pow(u.reconnectDecay,u.reconnectAttempts);setTimeout(function(){u.reconnectAttempts++,u.open(!0)},i>u.maxReconnectInterval?u.maxReconnectInterval:i)}},s.onmessage=function(n){(u.debug||e.debugAll)&&console.debug("ReconnectingWebSocket","onmessage",u.url,n.data);var t=c("message");t.data=n.data,l.dispatchEvent(t)},s.onerror=function(n){(u.debug||e.debugAll)&&console.debug("ReconnectingWebSocket","onerror",u.url,n),l.dispatchEvent(c("error"))}},1==this.automaticOpen&&this.open(!1),this.send=function(n){if(s)return(u.debug||e.debugAll)&&console.debug("ReconnectingWebSocket","send",u.url,n),s.send(n);throw"INVALID_STATE_ERR : Pausing to reconnect websocket"},this.close=function(e,n){"undefined"==typeof e&&(e=1e3),d=!0,s&&s.close(e,n)},this.refresh=function(){s&&s.close()}}return e.prototype.onopen=function(){},e.prototype.onclose=function(){},e.prototype.onconnecting=function(){},e.prototype.onmessage=function(){},e.prototype.onerror=function(){},e.debugAll=!1,e.CONNECTING=WebSocket.CONNECTING,e.OPEN=WebSocket.OPEN,e.CLOSING=WebSocket.CLOSING,e.CLOSED=WebSocket.CLOSED,e})</script><script>function LoadBody(){wsc=new ReconnectingWebSocket("ws://"+window.location.hostname+"/ws"),wsc.timeoutInterval=3e3,wsc.onopen=function(e){console.log("WebSocketClient connected:",e)}}function previewFile(){var e=document.querySelector("img"),n=document.querySelector("input[type=file]").files[0],s=new FileReader;s.onloadend=function(){e.src=s.result},n?s.readAsDataURL(n):e.src=""}function BrightnessUp(){var e={brightness:"up"};wsc.send(JSON.stringify(e))}function BrightnessDown(){var e={brightness:"down"};wsc.send(JSON.stringify(e))}function SpeedUp(){var e={speed:"up"};wsc.send(JSON.stringify(e))}function SpeedDown(){var e={speed:"down"};wsc.send(JSON.stringify(e))}var wsc;previewFile()</script><style>#bribt,#speedbt{padding:15px 32px}body{width:100%;height:100%;margin:auto;text-align:center;background-color:#251758}#wrapper{width:350px;height:575px;border:2px solid #605293;border-radius:8px;margin:25px auto auto;background-color:#877CB0}#bribt,#subbt{border:none;color:#fff;text-align:center;margin:auto;display:inline-block;font-size:20px;text-decoration:none}#subbt{background-color:#008CBA;padding:15px 50px;border-radius:8px}#bribt{background-color:#781424;border-radius:8px}#speedbt{text-align:center;margin:auto;background-color:#FAA;border:none;color:#000;text-decoration:none;display:inline-block;font-size:20px;border-radius:8px}.upload-btn-wrapper{position:relative;overflow:hidden;display:inline-block}.btn{border:2px solid gray;color:gray;background-color:#fff;padding:8px 20px;border-radius:8px;font-size:20px;font-weight:700}.upload-btn-wrapper input[type=file]{font-size:100px;position:absolute;left:0;top:0;opacity:0}a{color:#fff;text-decoration:none}#github a{color:#ff69b4;text-decoration:none}</style></head><body onload="LoadBody()"><div id="wrapper"><form method="POST" action="/upload" enctype="multipart/form-data"><br><br><img src="/image.bmp" height="200" alt="Upload BMP File to see preview"><br><br><div class="upload-btn-wrapper"><button class="btn">Select BMP file</button> <input type="file" name="myfile" type="file" onchange="previewFile()" accept="image/bmp"></div><br><br><input id="subbt" type="submit" value="Upload"><br><br></form><input id="bribt" type="button" value="Bri(+)" onclick="BrightnessUp()">&nbsp;&nbsp;&nbsp;<input id="bribt" type="button" value="Bri(-)" onclick="BrightnessDown()"><br><br><input id="speedbt" type="button" value="Speed(+)" onclick="SpeedUp()">&nbsp;&nbsp;&nbsp;<input id="speedbt" type="button" value="Speed(-)" onclick="SpeedDown()"></div><br><a href="/update">Firmware Update</a><br><br><div id="github"><a href="https://github.com/debsahu/ESP8266-NeoAirPainter">ESP8266 NeoAirPainter</a></div></body></head></html>
)=====";

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

void gammaBrightness(uint8_t _brightness)
{
    for (uint16_t index = 0; index < strip.PixelCount() - 1; index++)
    {
        RgbColor color = strip.GetPixelColor(index);
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
        image.Blt(strip, 0, 0, animState, image.Width()); // draw the complete row at animState to the complete strip
        animState = (animState + 1) % image.Height();     // increment and wrap
        // image.Blt(strip, 0, 0, image.Height(), animState); // draw the complete row at animState to the complete strip
        // animState = (animState + 1) % image.Width();     // increment and wrap
        gammaBrightness(brightness);
    }
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

    WiFi.mode(WIFI_STA);
    WiFi.hostname(const_cast<char *>(NameChipId));
    AsyncWiFiManager wifiManager(&server, &dns); //Local intialization. Once its business is done, there is no need to keep it around
    wifiManager.setConfigPortalTimeout(180);      //sets timeout until configuration portal gets turned off, useful to make it all retry or go to sleep in seconds

    if (!wifiManager.autoConnect(NameChipId))
    {
        Serial.println("Failed to connect and hit timeout\nEntering Station Mode");
        WiFi.mode(WIFI_AP);
        WiFi.hostname(const_cast<char *>(NameChipId));
        IPAddress apIP(192, 168, 1, 1);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP(NameChipId);
        Serial.println("");
        Serial.print("HotSpt IP: ");
        Serial.println(WiFi.softAPIP());
        dns=AsyncDNSServer();
        dns.setTTL(300);
        dns.setErrorReplyCode(AsyncDNSReplyCode::ServerFailure);
        dns.start(DNS_PORT, "*", WiFi.softAPIP());
    }
    else
    {
        Serial.println("");
        Serial.print(F("Connected with IP: "));
        Serial.println(WiFi.localIP());
    }

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
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", "<META http-equiv='refresh' content='2;URL=/'>Update Success, redirecting to main page ...");
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
                      if (SPIFFS.exists("image.bmp"))
                          SPIFFS.remove("image.bmp");
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

        request->send(404);
    });
    server.addHandler(new SPIFFSEditor("admin", "admin"));

    MDNS.setInstanceName(String(HOSTNAME " (" + String(chipId) + ")").c_str());
    if (MDNS.begin(NameChipId))
    {
        MDNS.addService("http", "tcp", HTTP_PORT);
        Serial.printf(">>> MDNS Started: http://%s.local/\n", NameChipId);
    }
    else
    {
        Serial.println(F(">>> Error setting up mDNS responder <<<"));
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