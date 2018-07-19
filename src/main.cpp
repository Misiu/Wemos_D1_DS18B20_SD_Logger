#include <Arduino.h>

#define FS_NO_GLOBALS
#include <FS.h> //this needs to be first, or it all crashes and burns...

#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

#include <TimeLib.h>
#include <NtpClientLib.h>
#include <WiFiClient.h>
#include <Ticker.h>

unsigned long _ntp_start = 0;
bool _ntp_update = false;
bool _ntp_configure = false;

#include "JustWifi.h"

#include "config.h"
#include "main.h"

//////////////////////////variables//////////////////////////
//AsyncWebSocket ws("/ws");
//AsyncWebServer server(80);
//DNSServer dns;

Sd2Card card;
const int sdCardPin = D8;
const char configFileName[20] = "config.cfg";

char _ssid[40];
char _password[40];
bool _ap = false;

char mqtt_port[6] = "8080";
char blynk_token[34] = "YOUR_BLYNK_TOKEN";

uint32_t _wifi_scan_client_id = 0;
bool _wifi_wps_running = false;
bool _wifi_smartconfig_running = false;

bool _loadDefaultSettings = false;

///////////////////////////////////////////////////////////////////////////

void wifiReconnectCheck()
{
    bool connected = false;
    jw.setReconnectTimeout(connected ? 0 : WIFI_RECONNECT_INTERVAL);
}

void wifiRegister(wifi_callback_f callback)
{
    jw.subscribe(callback);
}

uint8_t _wifi_ap_mode = WIFI_AP_FALLBACK;

void wifiSetup()
{
    WiFi.setSleepMode(WIFI_SLEEP_MODE);

    //jw.setHostname("TJ_TEST");
    //jw.setSoftAP("TEST", "qwerty");
    //jw.setSoftAP("TJ_TEST");

    jw.setConnectTimeout(WIFI_CONNECT_TIMEOUT);
    wifiReconnectCheck();
    jw.enableAPFallback(true);

    jw.cleanNetworks();
    jw.addNetwork(_ssid, _password);
    //jw.enableScan(true);

    //wifiRegister(_wifiCallback);
    //wifiRegister(_wifiCaptivePortal);
    //wifiRegister(_wifiDebugCallback);
}

void ntpSetup()
{
    NTP.onNTPSyncEvent([](NTPSyncEvent_t error) {
        if (error)
        {
            if (error == noResponse)
            {
                Serial.println("[NTP] Error: NTP server not reachable");
            }
            else if (error == invalidAddress)
            {
                Serial.println("[NTP] Error: Invalid NTP server address");
            }
        }
        else
        {
            _ntp_update = true;
        }
    });

    wifiRegister([](justwifi_messages_t code, char *parameter) {
        if (code == MESSAGE_CONNECTED)
            _ntp_start = millis() + NTP_START_DELAY;
    });
}

void _wifiCallback(justwifi_messages_t code, char *parameter)
{
}

#include "DNSServer.h"

DNSServer _wifi_dnsServer;

void _wifiCaptivePortal(justwifi_messages_t code, char *parameter)
{

    if (MESSAGE_ACCESSPOINT_CREATED == code)
    {
        _wifi_dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        _wifi_dnsServer.start(53, "*", WiFi.softAPIP());
        Serial.println("[WIFI] Captive portal enabled");
    }

    if (MESSAGE_CONNECTED == code)
    {
        _wifi_dnsServer.stop();
        Serial.println("[WIFI] Captive portal disabled\n");
    }
}

void _wifiDebugCallback(justwifi_messages_t code, char *parameter)
{

    // -------------------------------------------------------------------------

    if (code == MESSAGE_SCANNING)
    {
        Serial.println("[WIFI] Scanning");
    }

    if (code == MESSAGE_SCAN_FAILED)
    {
        Serial.println("[WIFI] Scan failed");
    }

    if (code == MESSAGE_NO_NETWORKS)
    {
        Serial.println("[WIFI] No networks found");
    }

    if (code == MESSAGE_NO_KNOWN_NETWORKS)
    {
        Serial.println("[WIFI] No known networks found\n");
    }

    if (code == MESSAGE_FOUND_NETWORK)
    {
        Serial.print("[WIFI] ");
        Serial.println(parameter);
    }

    // -------------------------------------------------------------------------

    if (code == MESSAGE_CONNECTING)
    {
        Serial.print("[WIFI] Connecting to ");
        Serial.println(parameter);
    }

    if (code == MESSAGE_CONNECT_WAITING)
    {
        // too much noise
    }

    if (code == MESSAGE_CONNECT_FAILED)
    {
        Serial.print("[WIFI] Could not connect to ");
        Serial.println(parameter);
    }

    // if (code == MESSAGE_CONNECTED)
    // {
    //     wifiDebug(WIFI_STA);
    // }

    if (code == MESSAGE_DISCONNECTED)
    {
        Serial.println(PSTR("[WIFI] Disconnected\n"));
    }

    // -------------------------------------------------------------------------

    if (code == MESSAGE_ACCESSPOINT_CREATING)
    {
        Serial.println(PSTR("[WIFI] Creating access point\n"));
    }

    // if (code == MESSAGE_ACCESSPOINT_CREATED)
    // {
    //     wifiDebug(WIFI_AP);
    // }

    if (code == MESSAGE_ACCESSPOINT_FAILED)
    {
        Serial.println(PSTR("[WIFI] Could not create access point\n"));
    }

    if (code == MESSAGE_ACCESSPOINT_DESTROYED)
    {
        Serial.println(PSTR("[WIFI] Access point destroyed\n"));
    }

    // -------------------------------------------------------------------------

    if (code == MESSAGE_WPS_START)
    {
        Serial.println(PSTR("[WIFI] WPS started\n"));
    }

    if (code == MESSAGE_WPS_SUCCESS)
    {
        Serial.println(PSTR("[WIFI] WPS succeded!\n"));
    }

    if (code == MESSAGE_WPS_ERROR)
    {
        Serial.println(PSTR("[WIFI] WPS failed\n"));
    }

    // ------------------------------------------------------------------------

    if (code == MESSAGE_SMARTCONFIG_START)
    {
        Serial.println(PSTR("[WIFI] Smart Config started\n"));
    }

    if (code == MESSAGE_SMARTCONFIG_SUCCESS)
    {
        Serial.println(PSTR("[WIFI] Smart Config succeded!\n"));
    }

    if (code == MESSAGE_SMARTCONFIG_ERROR)
    {
        Serial.println(PSTR("[WIFI] Smart Config failed\n"));
    }
}

const char *getSdTypeString()
{
    if (SD.type() == SD_CARD_TYPE_SD1)
    {
        return "SD1";
    }
    else if (SD.type() == SD_CARD_TYPE_SD2)
    {
        return "SD2";
    }
    else if (SD.type() == SD_CARD_TYPE_SDHC)
    {
        return "SDHC";
    }
    else
    {
        return "UNKNOWN";
    }
}

String getSizeString(size_t bytes)
{
    if (bytes < 1024)
    {
        return String(String(bytes) + "B");
    }
    else if (bytes < (1024 * 1024))
    {
        return String(String(bytes / 1024.0) + "KB");
    }
    else if (bytes < (1024 * 1024 * 1024))
    {
        return String(String(bytes / 1024.0 / 1024.0) + "MB");
    }
    else
    {
        return String(String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB");
    }
}

void loadConfigFromSD()
{
    Serial.println("Initializing SD card...");
    if (!card.init(SPI_HALF_SPEED, sdCardPin))
    {
        Serial.println("initialization failed. Things to check:");
        Serial.println("* is a card inserted?");
        Serial.println("* is your wiring correct?");
        Serial.println("* did you change the chipSelect pin to match your shield or module?");
        _loadDefaultSettings = true;
    }

    if (!_loadDefaultSettings)
    {
        Serial.println("Wiring is correct and a card is present.");
        if (!SD.begin(sdCardPin))
        {
            Serial.println("initialization failed!");
            _loadDefaultSettings = true;
        }
    }

    if (!_loadDefaultSettings)
    {
        Serial.print("\n==== SD Card Info ====\n");
        Serial.printf("SD Type: %s FAT%d, Size: %s\n", getSdTypeString(), SD.fatType(), getSizeString(SD.size()).c_str());
        Serial.printf("SD Blocks: total: %d, size: %s\n", SD.totalBlocks(), getSizeString(SD.blockSize()).c_str());
        Serial.printf("SD Clusters: total: %d, blocks: %d, size: %s\n", SD.totalClusters(), SD.blocksPerCluster(), getSizeString(SD.clusterSize()).c_str());

        if (!SD.exists(configFileName))
        {
            Serial.println("no config file!");
            _loadDefaultSettings = true;
        }
    }

    if (!_loadDefaultSettings)
    {
        Serial.printf("%s exists.\n", configFileName);

        File configFile = SD.open(configFileName, FILE_READ);
        if (configFile)
        {
            Serial.println("opened config file");
            size_t size = configFile.size();
            // Allocate a buffer to store contents of the file.
            std::unique_ptr<char[]> buf(new char[size]);

            configFile.readBytes(buf.get(), size);
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(buf.get());
            json.printTo(Serial);
            if (json.success())
            {
                strcpy(_ssid, json["ssid"]);
                strcpy(_password, json["password"]);

                //free(configFile);
                //free(jsonBuffer);
                //free(json);
            }
            else
            {
                _loadDefaultSettings = true;
            }
        }
        else
        {
            _loadDefaultSettings = true;
        }
    }

    if (_loadDefaultSettings)
    {
        strcpy(_ssid, "WTF_2.4");
        strcpy(_password, "Korolcia");
    }

    Serial.print("SSID:");
    Serial.println(_ssid);
    Serial.print("PASSWORD:");
    Serial.println(_password);
}

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);

    loadConfigFromSD();

    wifiSetup();

    ntpSetup();

    // Wait for connection
    // AsyncWiFiManager wifiManager(&server, &dns);
    // //wifiManager.resetSettings();
    // wifiManager.setDebugOutput(true);

    // //wifiManager.setAPCallback(configModeCallback);
    // if (!wifiManager.autoConnect("TestAP", "password"))
    // {
    //     Serial.println("failed to connect, we should reset as see if it connects");
    //     delay(30000);
    //     ESP.reset();
    //     delay(5000);
    // }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    Serial.println("local ip");
    Serial.println(WiFi.localIP());

#ifdef OTA
    ArduinoOTA.setHostname(HOST_NAME);
    ArduinoOTA.begin();
    Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", HOST_NAME);
#endif

    // if (MDNS.begin(HOST_NAME))
    // {
    //     MDNS.addService("http", "tcp", 80);
    //     Serial.println("MDNS responder started");
    // }

    // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    //     request->send(200, "text/plain", "Hello World");
    // });

    // server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    //     ESP.restart();
    // });

    // server.onNotFound([](AsyncWebServerRequest *request) {
    //     request->send(404);
    // });

    // Start the webserver
    //
    //server.begin();
    //Serial.println("Webserver started ");

    pinMode(BUILTIN_LED, OUTPUT); // initialize onboard LED as output

    Serial.println("Led setup");
}

void _wifiCheckAP()
{

    if ((WIFI_AP_FALLBACK == _wifi_ap_mode) &&
        (jw.connected()) &&
        ((WiFi.getMode() & WIFI_AP) > 0) &&
        (WiFi.softAPgetStationNum() == 0))
    {
        jw.enableAP(false);
    }
}

void _wifiScan(uint32_t client_id = 0)
{

    Serial.println("[WIFI] Start scanning");

    unsigned char result = WiFi.scanNetworks();

    if (result == WIFI_SCAN_FAILED)
    {
        Serial.println("[WIFI] Scan failed");
    }
    else if (result == 0)
    {
        Serial.println("[WIFI] No networks found");
    }
    else
    {

        Serial.print("[WIFI] %d networks found: ");
        Serial.print(result);

        // Populate defined networks with scan data
        for (int8_t i = 0; i < result; ++i)
        {

            String ssid_scan;
            int32_t rssi_scan;
            uint8_t sec_scan;
            uint8_t *BSSID_scan;
            int32_t chan_scan;
            bool hidden_scan;
            char buffer[128];

            WiFi.getNetworkInfo(i, ssid_scan, sec_scan, rssi_scan, BSSID_scan, chan_scan, hidden_scan);

            snprintf_P(buffer, sizeof(buffer),
                       PSTR("BSSID: %02X:%02X:%02X:%02X:%02X:%02X SEC: %s RSSI: %3d CH: %2d SSID: %s"),
                       BSSID_scan[1], BSSID_scan[2], BSSID_scan[3], BSSID_scan[4], BSSID_scan[5], BSSID_scan[6],
                       (sec_scan != ENC_TYPE_NONE ? "YES" : "NO "),
                       rssi_scan,
                       chan_scan,
                       (char *)ssid_scan.c_str());

            Serial.print("[WIFI] > "),
                Serial.println(buffer);
        }
    }
    WiFi.scanDelete();
}

static unsigned long last = 0;

void wifiLoop()
{

    // Main wifi loop
    jw.loop();

    // Process captrive portal DNS queries if in AP mode only
    if ((WiFi.getMode() & WIFI_AP) == WIFI_AP)
    {
        _wifi_dnsServer.processNextRequest();
    }

    // Do we have a pending scan?
    if (_wifi_scan_client_id > 0)
    {
        _wifiScan(_wifi_scan_client_id);
        _wifi_scan_client_id = 0;
    }

    // Check if we should disable AP
    if (millis() - last > 60000)
    {
        last = millis();
        _wifiCheckAP();
    }
}

void _ntpConfigure()
{

    _ntp_configure = false;

    int offset = NTP_TIME_OFFSET;
    int sign = offset > 0 ? 1 : -1;
    offset = abs(offset);
    int tz_hours = sign * (offset / 60);
    int tz_minutes = sign * (offset % 60);
    if (NTP.getTimeZone() != tz_hours || NTP.getTimeZoneMinutes() != tz_minutes)
    {
        NTP.setTimeZone(tz_hours, tz_minutes);
        _ntp_update = true;
    }

    bool daylight = NTP_DAY_LIGHT == 1;
    if (NTP.getDayLight() != daylight)
    {
        NTP.setDayLight(daylight);
        _ntp_update = true;
    }

    String server = NTP_SERVER;
    if (!NTP.getNtpServerName().equals(server))
    {
        NTP.setNtpServerName(server);
    }

    uint8_t dst_region = NTP_DST_REGION;
    NTP.setDSTZone(dst_region);
}

bool ntpSynced()
{
    return (year() > 2017);
}

String ntpDateTime(time_t t)
{
    char buffer[20];
    snprintf_P(buffer, sizeof(buffer),
               PSTR("%04d-%02d-%02d %02d:%02d:%02d"),
               year(t), month(t), day(t), hour(t), minute(t), second(t));
    return String(buffer);
}

String ntpDateTime() {
    if (ntpSynced()) return ntpDateTime(now());
    return String();
}

time_t ntpLocal2UTC(time_t local) {
    int offset = NTP_TIME_OFFSET;
    if (NTP.isSummerTime()) offset += 60;
    return local - offset * 60;
}

void _ntpUpdate()
{

    _ntp_update = false;

    if (ntpSynced())
    {
        time_t t = now();
        Serial.printf("[NTP] UTC Time  : %s\n", (char *)ntpDateTime(ntpLocal2UTC(t)).c_str());
        Serial.printf("[NTP] Local Time: %s\n", (char *)ntpDateTime(t).c_str());
    }
}

void _ntpStart()
{

    _ntp_start = 0;

    NTP.begin(NTP_SERVER);
    NTP.setInterval(NTP_SYNC_INTERVAL, NTP_UPDATE_INTERVAL);
    NTP.setNTPTimeout(NTP_TIMEOUT);
    _ntpConfigure();
}

void ntpLoop()
{

    if (0 < _ntp_start && _ntp_start < millis())
        _ntpStart();
    if (_ntp_configure)
        _ntpConfigure();
    if (_ntp_update)
        _ntpUpdate();

    now();
}

unsigned int led_1_blink_time = 200;
//we need this variable to store last time of blink in milliseconds
unsigned long led_1_last_blink;

int ntp_time = 1000;
//we need this variable to store last time of blink in milliseconds
long ntp_last;

void loop()
{
    if ((millis() - led_1_last_blink) >= led_1_blink_time)
    {
        if (digitalRead(BUILTIN_LED) == HIGH)
        {
            digitalWrite(BUILTIN_LED, LOW);
        }
        else
        {
            digitalWrite(BUILTIN_LED, HIGH);
        }
        led_1_last_blink = millis();
        led_1_blink_time += 100;
    }
    // let other modules update
    //MDNS.update();
    //ArduinoOTA.handle();

    wifiLoop();
    ntpLoop();

    if ((millis() - ntp_last) >= ntp_time)
    {
        Serial.printf("Time: %s", ntpDateTime().c_str());
        ntp_last = millis();
    }
}