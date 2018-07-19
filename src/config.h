//---------------------------------------------
// MNDS name
#define HOST_NAME "TJ"

//pin where the relay is connected to
#define RELAY_PIN 5

//pin where the thermometer is connected to
#define ONE_WIRE_BUS 14

//passwd for the SPIFFS editor/file manager
#define HTTP_USERNAME "admin"
#define HTTP_PASSWORD "admin"

//this is to enable OTA updates
//#define OTA 1

#ifndef WIFI_SLEEP_MODE
#define WIFI_SLEEP_MODE WIFI_NONE_SLEEP // WIFI_NONE_SLEEP, WIFI_LIGHT_SLEEP or WIFI_MODEM_SLEEP
#endif

#ifndef WIFI_CONNECT_TIMEOUT
#define WIFI_CONNECT_TIMEOUT 60000 // Connecting timeout for WIFI in ms
#endif

#ifndef WIFI_RECONNECT_INTERVAL
#define WIFI_RECONNECT_INTERVAL 180000 // If could not connect to WIFI, retry after this time in ms
#endif

#define WIFI_AP_FALLBACK            2

#ifndef NTP_START_DELAY
#define NTP_START_DELAY             3000            // Delay NTP start 1 second
#endif

#ifndef NTP_SERVER
#define NTP_SERVER                  "pool.ntp.org"  // Default NTP server
#endif

#ifndef NTP_SYNC_INTERVAL
#define NTP_SYNC_INTERVAL           10              // NTP initial check every minute
#endif

#ifndef NTP_UPDATE_INTERVAL
#define NTP_UPDATE_INTERVAL         1800            // NTP check every 30 minutes
#endif

#ifndef NTP_TIMEOUT
#define NTP_TIMEOUT                 2000            // Set NTP request timeout to 2 seconds (issue #452)
#endif

#ifndef NTP_DST_REGION
#define NTP_DST_REGION              0               // 0 for Europe, 1 for USA (defined in NtpClientLib)
#endif

#ifndef NTP_DAY_LIGHT
#define NTP_DAY_LIGHT               1               // Enable daylight time saving by default
#endif

#ifndef NTP_TIME_OFFSET
#define NTP_TIME_OFFSET             60              // Default timezone offset (GMT+1)
#endif