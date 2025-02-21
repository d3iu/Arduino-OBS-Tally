/*:
TALLY by DEIU
*/

#include <ESP_EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ArduinoWebsockets.h>      // https://github.com/gilmaimon/ArduinoWebsockets
#include <ArduinoJson.h>            // https://arduinojson.org/v6/doc/
#include <Adafruit_NeoPixel.h>      // https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use

#include "FS.h"

// Constant
const int OnceSet=10;
const int SsidMaxLength = 64;
const int PassMaxLength = 64;
const int HostNameMaxLength = 64;
const int IdSceneMaxLength = 64;
bool TERSAMBUNG=false;
const char* websockets_server_host = "192.168.0.255";   
const uint16_t websockets_server_port = 4444;           
const char* prefix;    

IPAddress Ip(192, 168, 4, 1);
IPAddress NMask(255, 255, 255, 0);

#define LED_PIN    D5      // GPIO14 connected to NeoPixels
#define LED_COUNT  1
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
uint8_t LEDbrightness = 255;
const uint32_t merah = strip.Color(220, 0, 0);
const uint32_t hijau = strip.Color(0, 220, 0);
const uint32_t biru = strip.Color(0, 0, 220);
const uint32_t cyan = strip.Color(50, 220, 220);
const uint32_t kuning = strip.Color(150,150,0);

const uint32_t putih = strip.Color(50, 50, 50);

uint32_t ping_last = 0;     
const uint16_t PING_interval = 2500;
bool status_update = false;

bool live_active = false;
bool preview_active = false;

using namespace websockets;
WebsocketsClient Sclient;
StaticJsonDocument<2048> doc;
StaticJsonDocument<256> filter;

void ledMessage(uint16_t startLED, uint16_t countLED, uint32_t colorLED, uint16_t countFlashes, uint16_t delayFlashes);
void onMessageCallback(WebsocketsMessage message);
void onEventsCallback(WebsocketsEvent event, String data);
void Websocket_handler(void);
//======================================================================================= Setari structura
struct Settings
{
  char ssid[SsidMaxLength];
  char pass[PassMaxLength];
  char hostName[HostNameMaxLength];
  char IdScene[IdSceneMaxLength];
};

//======================================================================================= Setari initiale
Settings defaultSettings = {
  "Nume retea wifi",
  "Parola Wifi",
  "192.168.0.255","Tally"
};
//======================================================================================= Setari
Settings settings;

//======================================================================================= HTTP Server
ESP8266WebServer httpServer(80);
char deviceName[32];
int status = WL_IDLE_STATUS;
bool apEnabled = false;
char apPass[64];


//======================================================================================= Client Wifi
WiFiClient client;
int timeout = 20;
int delayTime = 10000;

//======================================================================================= Incarca setari in EEPROM
void restart()
{
  
  Serial.println();
  Serial.println();
  PrintSekat();
  PrintSekat();
  Serial.println(F("RESTART"));
  PrintSekat();
  PrintSekat();
  Serial.println();
  Serial.println();

  ESP.restart();
}
void start()
{
  
  loadSettings();
  sprintf(deviceName, "OBS - %s", settings.IdScene);
  sprintf(apPass, "%s%s", "deiu", "tally");
  connectToWifi();

  if (WiFi.status() == WL_CONNECTED)
  {
    TERSAMBUNG=true;
  } else {
    TERSAMBUNG=false;
  }
}
void PrintSekat(){
  Serial.println (F("-------------------------"));
}

void loadSettings()
{

  long ptr = 0;

  for (int i = 0; i < SsidMaxLength; i++)
  {
    settings.ssid[i] = EEPROM.read(ptr);
    ptr++;
  }

  for (int i = 0; i < PassMaxLength; i++)
  {
    settings.pass[i] = EEPROM.read(ptr);
    ptr++;
  }

  for (int i = 0; i < HostNameMaxLength; i++)
  {
    settings.hostName[i] = EEPROM.read(ptr);
    ptr++;
  }

  for (int i = 0; i < IdSceneMaxLength; i++)
  {
    settings.IdScene[i] = EEPROM.read(ptr);
    ptr++;
  }
  
  PrintSekat();
  Serial.println("Loading settings :");
  int loader=EEPROM.read(500);
  if (loader!=OnceSet){
    Serial.println("First Time booting");
    settings = defaultSettings;
    saveSettings();
    EEPROM.write(500,OnceSet);
    EEPROM.commit();
    Serial.println("Default settings Loaded");
  }
  else
  { 
    Serial.println("Settings loaded"); 
  }
  PrintSekat();
}
//======================================================================================= Salveaza setari in EEPROM
void saveSettings()
{
  PrintSekat();
  Serial.println("Saving settings");

  long ptr = 0;

  for (int i = 0; i < 490; i++)
  {
    EEPROM.write(i, 0);
  }

  for (int i = 0; i < SsidMaxLength; i++)
  {
    EEPROM.write(ptr, settings.ssid[i]);
    ptr++;
  }

  for (int i = 0; i < PassMaxLength; i++)
  {
    EEPROM.write(ptr, settings.pass[i]);
    ptr++;
  }

  for (int i = 0; i < HostNameMaxLength; i++)
  {
    EEPROM.write(ptr, settings.hostName[i]);
    ptr++;
  }

  for (int i = 0; i < IdSceneMaxLength; i++)
  {
    EEPROM.write(ptr, settings.IdScene[i]);
    ptr++;
  }

  EEPROM.commit();

  Serial.println("Settings saved");
  PrintSekat();
  printSettings();
  PrintSekat();
}

//======================================================================================= Print setari in monitorul serial
void printSettings()
{
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(settings.ssid);
  Serial.print("SSID Password: ");
  Serial.println(settings.pass);
  Serial.print("OBS IP: ");
  Serial.println(settings.hostName);
  Serial.print("ID Scene: ");
  Serial.println(settings.IdScene);
}

//======================================================================================= Start AP
void apStart()
{
  Serial.println("AP Start");
  Serial.print("AP SSID: ");
  Serial.println(deviceName);
  Serial.print("AP password: ");
  Serial.println(apPass);

  WiFi.mode(WIFI_AP);
  WiFi.hostname(deviceName);
  WiFi.softAPConfig(Ip, Ip, NMask);
  WiFi.softAP(deviceName, apPass);
  delay(100);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("IP address: ");
  Serial.println(myIP);
  strip.fill(kuning);
  strip.show();
  apEnabled = true;
}

//======================================================================================= Pagina http afisare client
void rootPageHandler()
{
  
  String response_message = "<!DOCTYPE html>";
  response_message += F("<html lang='en'>");
  response_message += "<head>";
  response_message += "<title>" + String(deviceName) + "</title>";
  response_message += "<meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>";
  response_message += "<meta charset='utf-8'>";
  response_message += F("<style>body {width: 100%;height: 100%;padding: 25px;}");
  response_message += F(":root{--blue:#007bff;--indigo:#6610f2;--purple:#6f42c1;--pink:#e83e8c;--red:#dc3545;--orange:#fd7e14;--yellow:#ffc107;--green:#28a745;--teal:#20c997;--cyan:#17a2b8;--white:#fff;--gray:#6c757d;--gray-dark:#343a40;--primary:#007bff;--secondary:#6c757d;--success:#28a745;--info:#17a2b8;--warning:#ffc107;--danger:#dc3545;--light:#f8f9fa;--dark:#343a40;--breakpoint-xs:0;--breakpoint-sm:576px;--breakpoint-md:768px;--breakpoint-lg:992px;--breakpoint-xl:1200px;--font-family-sans-serif:-apple-system,BlinkMacSystemFont,\"Segoe UI\",Roboto,\"Helvetica Neue\",Arial,\"Noto Sans\",sans-serif,\"Apple Color Emoji\",\"Segoe UI Emoji\",\"Segoe UI Symbol\",\"Noto Color Emoji\";--font-family-monospace:SFMono-Regular,Menlo,Monaco,Consolas,\"Liberation Mono\",\"Courier New\",monospace}");
  response_message += F("*,::after,::before{box-sizing:border-box}");
  response_message += F("html{font-family:sans-serif;line-height:1.15;-webkit-text-size-adjust:100%}body{margin:0;font-family:-apple-system,BlinkMacSystemFont,\"Segoe UI\",Roboto,\"Helvetica Neue\",Arial,\"Noto Sans\",sans-serif,\"Apple Color Emoji\",\"Segoe UI Emoji\",\"Segoe UI Symbol\",\"Noto Color Emoji\";font-size:1rem;font-weight:400;line-height:1.5;color:#212529;text-align:left;background-color:#fff}h1,h2{margin-top:0;margin-bottom:.5rem}table{border-collapse:collapse}th{text-align:inherit}label{display:inline-block;margin-bottom:.5rem}input{margin:0;font-family:inherit;font-size:inherit;line-height:inherit}input{overflow:visible}[type=submit]{-webkit-appearance:button}[type=submit]::-moz-focus-inner{padding:0;border-style:none}[type=number]::-webkit-inner-spin-button,[type=number]::-webkit-outer-spin-button{height:auto}::-webkit-file-upload-button{font:inherit;-webkit-appearance:button}h1,h2{margin-bottom:.5rem;font-family:inherit;font-weight:500;line-height:1.2;color:inherit}h1{font-size:2.5rem}h2{font-size:2rem}");
  response_message += F(".row{display:-ms-flexbox;display:flex;-ms-flex-wrap:wrap;flex-wrap:wrap;margin-right:-15px;margin-left:-15px}");
  response_message += F(".col-md-6,.col-sm-4,.col-sm-8{position:relative;width:100%;padding-right:15px;padding-left:15px}");
  response_message += F("@media (min-width:576px){.col-sm-4{-ms-flex:0 0 33.333333%;flex:0 0 33.333333%;max-width:33.333333%}.col-sm-8{-ms-flex:0 0 66.666667%;flex:0 0 66.666667%;max-width:66.666667%}}");
  response_message += F("@media (min-width:768px){.col-md-6{-ms-flex:0 0 50%;flex:0 0 50%;max-width:50%}}.table{width:100%;margin-bottom:1rem;background-color:transparent}");
  response_message += F(".table td,.table th{padding:.75rem;vertical-align:top;border-top:1px solid #dee2e6}");
  response_message += F(".form-control{display:block;width:100%;height:calc(2.25rem + 2px);padding:.375rem .75rem;font-size:1rem;font-weight:400;line-height:1.5;color:#495057;background-color:#fff;background-clip:padding-box;border:1px solid #ced4da;border-radius:.25rem}");
  response_message += F(".form-control::-ms-expand{background-color:transparent;border:0}");
  response_message += F(".form-control::-webkit-input-placeholder{color:#6c757d;opacity:1}");
  response_message += F(".form-control::-moz-placeholder{color:#6c757d;opacity:1}");
  response_message += F(".form-control:-ms-input-placeholder{color:#6c757d;opacity:1}");
  response_message += F(".form-control::-ms-input-placeholder{color:#6c757d;opacity:1}");
  response_message += F(".col-form-label{padding-top:calc(.375rem + 1px);padding-bottom:calc(.375rem + 1px);margin-bottom:0;font-size:inherit;line-height:1.5}");
  response_message += F(".form-group{margin-bottom:1rem}");
  response_message += F(".btn{display:inline-block;font-weight:400;color:#212529;text-align:center;vertical-align:middle;background-color:transparent;border:1px solid transparent;padding:.375rem .75rem;font-size:1rem;line-height:1.5;border-radius:.25rem}");
  response_message += F(".btn-primary{color:#fff;background-color:#007bff;border-color:#007bff;cursor: pointer}");
  response_message += F(".bg-light{background-color:#f8f9fa!important} </style>");
  response_message += "</head>";
  
  response_message += "<body class='bg-light'>";

  response_message += "<h1>OBS tally : " + String(settings.IdScene) + "</h1>";
  response_message += "<div data-role='content' class='row'>";

  response_message += "<div class='col-md-6'>";
  response_message += "<h2>Setari</h2>";
  response_message += "<form action='/save' method='post' enctype='multipart/form-data' data-ajax='false'>";

  response_message += "<div class='form-group row'>";
  response_message += "<label for='ssid' class='col-sm-4 col-form-label'>SSID</label>";
  response_message += "<div class='col-sm-8'>";
  response_message += "<input id='ssid' class='form-control' type='text' size='64' maxlength='64' name='ssid' value='" + String(settings.ssid) + "'>";
  response_message += "</div></div>";

  response_message += "<div class='form-group row'>";
  response_message += "<label for='ssidpass' class='col-sm-4 col-form-label'>Parola SSID</label>";
  response_message += "<div class='col-sm-8'>";
  response_message += "<input id='ssidpass' class='form-control' type='text' size='64' maxlength='64' name='ssidpass' value='" + String(settings.pass) + "'>";
  response_message += "</div></div>";

  response_message += "<div class='form-group row'>";
  response_message += "<label for='hostname' class='col-sm-4 col-form-label'>IP PC OBS (portul folosit este 4444)</label>";
  response_message += "<div class='col-sm-8'>";
  response_message += "<input id='hostname' class='form-control' type='text' size='64' maxlength='64' name='hostname' value='" + String(settings.hostName) + "'>";
  response_message += "</div></div>";

  response_message += "<div class='form-group row'>";
  response_message += "<label for='inputnumber' class='col-sm-4 col-form-label'>Nume Scena</label>";
  response_message += "<div class='col-sm-8'>";
  response_message += "<input id='inputnumber' class='form-control' type='text' size='64' maxlength='64' name='inputnumber' value='" + String(settings.IdScene) + "'>";
  response_message += "</div></div>";

  response_message += "<input type='submit' value='Salvare' class='btn btn-primary'></form>";
  response_message += "</div>";

  response_message += "<div class='col-md-6'>";
  response_message += "<h2>Informatii dispozitiv</h2>";
  response_message += "<table class='table'><tbody>";

  char ip[13];
  sprintf(ip, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  response_message += "<tr><th>IP</th><td>" + String(ip) + "</td></tr>";

  response_message += "<tr><th>MAC</th><td>" + String(WiFi.macAddress()) + "</td></tr>";
  response_message += "<tr><th>Putere semnal</th><td>" + String(WiFi.RSSI()) + " dBm</td></tr>";
  response_message += "<tr><th>Nume dispozitiv</th><td>" + String(deviceName) + "</td></tr>";

  if (WiFi.status() == WL_CONNECTED)
  {
    response_message += "<tr><th>Status</th><td>Conectat</td></tr>";
  }
  else
  {
    response_message += "<tr><th>Status</th><td>Deconectat</td></tr>";
  }

  if (apEnabled)
  {
    sprintf(ip, "%d.%d.%d.%d", WiFi.softAPIP()[0], WiFi.softAPIP()[1], WiFi.softAPIP()[2], WiFi.softAPIP()[3]);
    response_message += "<tr><th>AP</th><td>Active (" + String(ip) + ")</td></tr>";
  }
  else
  {
    response_message += "<tr><th>AP</th><td>Inactiv</td></tr>";
  }
  response_message += "</tbody></table>";
  response_message += "</div>";
  response_message += "</div>";
  response_message += "<div><center>";
  response_message += "Deiu 2025";
  response_message += "</center></div>";
  response_message += "</body>";
  response_message += "</html>";
  
  httpServer.sendHeader("Connection", "close");
  httpServer.send(200, "text/html", String(response_message));
}

//======================================================================================= Setari POST
void handleSave()
{
  bool doRestart = false;

  httpServer.sendHeader("Location", String("/"), true);
  httpServer.send(302, "text/plain", "Redirected to: /");

  if (httpServer.hasArg("ssid"))
  {
    if (httpServer.arg("ssid").length() <= SsidMaxLength)
    {
      httpServer.arg("ssid").toCharArray(settings.ssid, SsidMaxLength);
      doRestart = true;
    }
  }

  if (httpServer.hasArg("ssidpass"))
  {
    if (httpServer.arg("ssidpass").length() <= PassMaxLength)
    {
      httpServer.arg("ssidpass").toCharArray(settings.pass, PassMaxLength);
      doRestart = true;
    }
  }

  if (httpServer.hasArg("hostname"))
  {
    if (httpServer.arg("hostname").length() <= HostNameMaxLength)
    {
      httpServer.arg("hostname").toCharArray(settings.hostName, HostNameMaxLength);
      doRestart = true;
    }
  }

  if (httpServer.hasArg("inputnumber"))
  {
    if (httpServer.arg("inputnumber").length() <= IdSceneMaxLength)
    {
      httpServer.arg("inputnumber").toCharArray(settings.IdScene, IdSceneMaxLength);
      doRestart = true;
    }
  }
  

  if (doRestart == true)
  {
    saveSettings();
    restart();
  }
}

//======================================================================================= Conectare la WiFi
void connectToWifi()
{
  Serial.println("Conectare la WiFi");
  Serial.print("SSID: ");
  Serial.println(settings.ssid);
  Serial.print("Parola: ");
  Serial.println(settings.pass);

  int timeout = 15;

  WiFi.mode(WIFI_STA);
  WiFi.hostname(deviceName);
  WiFi.begin(settings.ssid, settings.pass);

  Serial.print("Se asteapta conexiunea.");
  while (WiFi.status() != WL_CONNECTED and timeout > 0)
  { 
    Serial.print(".");
    ledMessage(0, LED_COUNT, biru, 1, 300);
    timeout--;
    delay(300);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Succes!");
    Serial.print("Adresa IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Nume dispozitiv: ");
    Serial.println(deviceName);
    Serial.println("------------");
    strip.fill(cyan);
    strip.show();
    Serial.println("Connect to Websocket server");
    bool connected = Sclient.connect(settings.hostName, websockets_server_port, "/12345678");
      if (connected)
      {
        Serial.println("Conectat!");
      }
      else
      {
        Serial.println("Nu s-a conectat! Incearca mai tarziu");
      }
  Serial.println();
  }
  else
  {
    if (WiFi.status() == WL_IDLE_STATUS)
      Serial.println("Idle");
    else if (WiFi.status() == WL_NO_SSID_AVAIL)
      Serial.println("No SSID Available");
    else if (WiFi.status() == WL_SCAN_COMPLETED)
      Serial.println("Scan Completed");
    else if (WiFi.status() == WL_CONNECT_FAILED)
      Serial.println("Connection Failed");
    else if (WiFi.status() == WL_CONNECTION_LOST)
      Serial.println("Connection Lost");
    else if (WiFi.status() == WL_DISCONNECTED)
      Serial.println("Disconnected");
    else
      Serial.println("Unknown Failure");

    Serial.println("------------");
    apStart();
  }
}


void setup()
{
  Serial.begin(115200);
  EEPROM.begin(512);
  SPIFFS.begin();
  httpServer.on("/", HTTP_GET, rootPageHandler);
  httpServer.on("/save", HTTP_POST, handleSave);
  httpServer.serveStatic("/", SPIFFS, "/", "max-age=315360000");
  httpServer.begin();

  strip.begin();
  strip.show(); // Initializare pixeli 'opriti'
  ledMessage(0, 1, putih, 5, 100);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Initializare JSON");
  filter["sources"][0]["name"] = true;
  filter["scene-name"] = true;
  filter["update-type"] = true;
  filter["to-scene"] = true;
  
  Serial.println("JSON filtru initializat");
  Sclient.onMessage(onMessageCallback);
  Sclient.onEvent(onEventsCallback);
    

  start();
}

void loop()
{ 
  if (!TERSAMBUNG){
  httpServer.handleClient(); //Start server web mod AP

  } else {

 Websocket_handler();
 httpServer.handleClient(); //Start server web dupa conectare la wifi

  if (status_update)
  {
    if (live_active)
    {
      
     
      strip.fill(merah);
      strip.show();
    }
    else if (preview_active)
    {
      
      strip.fill(hijau);
      strip.show();
    }
    else if ((!live_active) && (!preview_active))
    {
      
      strip.fill(putih);
      strip.show();
    }

    status_update = false;
  }

   yield();
    
  }

  
}

void ledMessage(uint16_t startLED, uint16_t countLED, uint32_t colorLED, uint16_t countFlashes, uint16_t delayFlashes)
{
  for (uint16_t i = 0; i < countFlashes; i++)
  {
    strip.fill(colorLED, startLED, countLED);
    strip.show();
    delay(delayFlashes / 2);
    strip.clear();
    strip.show();
    delay(delayFlashes / 2);
  }
}

void onMessageCallback(WebsocketsMessage message)
{
  DeserializationError error = deserializeJson(doc, message.data(), DeserializationOption::Filter(filter));
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  else
  { //========================================================================= Activ scena
    if (strcmp(doc["update-type"], "SwitchScenes") == 0 )
    {
      bool live = false; 
        if (strcmp(doc["scene-name"], settings.IdScene) ==0)
        {
          live = true;
        }
      

      if (live)
      {
        if (!live_active)
        {
          live_active = true;
          status_update = true;
        }

      }
      else
      {
        if (live_active)
        {
          live_active = false;
          status_update = true;
        }
      }
    }//========================================================================= FADE
    /*else if (strcmp(doc["update-type"], "TransitionBegin") == 0)
    {
      bool live = false;
         
        if (strcmp(doc["to-scene"], settings.IdScene) == 0 )
        {
          live = true;
         
        }
     

      if (live)
      {
        if (!live_active)
        {
          live_active = true;
          status_update = true;
        }

      }
      else
      {
        if (live_active)
        {
          live_active = false;
          status_update = true;
        }
      }
    }*/
    //========================================================================= Preview
    else if (strcmp(doc["update-type"], "PreviewSceneChanged") == 0)
    {
      bool preview = false;
      
        if (strcmp(doc["scene-name"], settings.IdScene) == 0 )
        {
          preview = true;
         
        }
      
      if (preview)
      {
        if (!preview_active)
        {
          preview_active = true;
          status_update = true;
        }
      }
      else
      {
        if (preview_active)
        {
          preview_active = false;
          status_update = true;
         }
      }
    }

  }

}

void onEventsCallback(WebsocketsEvent event, String data)
{
  if (event == WebsocketsEvent::ConnectionOpened)
  {
    Serial.println("Conexiune stabilita");

  }
  else if (event == WebsocketsEvent::ConnectionClosed)
  {
    Serial.println("Conexiune inchisa");
    strip.clear();
    strip.show();
    live_active = false;
    preview_active = false;
    status_update = true;
  }

  
}


void Websocket_handler(void)
{
  if (Sclient.available())
  {
    Sclient.poll(); 
    if ((millis() - ping_last) >= PING_interval)
    {
      ping_last = millis();
      Sclient.ping();
    }

  }
  else
  {
    Serial.print("Conectare la server: ");
    Serial.print (settings.hostName)  ;   
    bool connected = Sclient.connect(settings.hostName, websockets_server_port, "/");
    if (connected)
    {
      strip.fill(putih);
      strip.show();
      Serial.println("Conectat!");
    }
    else
    {
      Serial.println("Nu s-a conectat!");
      strip.fill(putih);
      strip.show();
      delay(400);
      strip.clear();
      strip.show();
    }
   
  }
}
