Librarii necesare pentru functionare cod
Board manager ce trebuie adaugat pentru a descarca driverele necesare pentru compilare pe wemos si nodemcu
http://arduino.esp8266.com/stable/package_esp8266com_index.json

ESP_EEPROM https://github.com/jwrw/ESP_EEPROM

ArduinoJson https://arduinojson.org/v6/doc/

Arduino Websockets https://github.com/gilmaimon/ArduinoWebsockets

Neopixel https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use


Pentru instalare: Se foloseste Arduino IDE
Sketch -> Include librarby -> Add .zip File

Programul functioneaza cu Wemos D1 Mini si cu NodeMCU pe portul D5 care poate fi schimbat! 
Functioneaza cu led-uri neopixel care isi schimba culoarea in functie de scena selectata in OBS, Numarul pixelilor poate fi schimbat in cod

Datele de conectare implicite la Wifi-ul AP pe care il creeaza imediat dupa instalare sunt:
SSID: Obs-Tally
Parola: deiutally
https://192.168.4.1 - Interfata web pentru setari

Dupa conectare se vor introduce datele catre websockets de pe PC-ul unde functioneaza OBS (Websockets fara parola) SSID si Parola wifi-ului din casa pentru a se conecta la reteaua locala si numele scenei pentru care dorim ca led-ul sa afiseze (Rosu - Live , Verde - Preview, Alb - Neselectat)
Dupa conectarea la wifi-ul local, aflati IP-ul Obs-Tally verificand setarile DHCP ale routerului si apoi va puteti conecta la Obs-Tally folosind ip-ul din reteaua locala pentru schimbari ulterioare ale setarilor

Pentru a functiona este nevoie de websockets 4.9.1 si testat cu OBS 27.1.3

https://github.com/obsproject/obs-studio/releases/download/27.1.3/OBS-Studio-27.1.3-Full-Installer-x64.exe
https://github.com/obsproject/obs-websocket/releases/download/4.9.1/obs-websocket-4.9.1-Windows-Installer.exe
