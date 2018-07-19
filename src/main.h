typedef std::function<void(justwifi_messages_t code, char *parameter)> wifi_callback_f;
void _wifiCallback(justwifi_messages_t code, char * parameter);
void _wifiCaptivePortal(justwifi_messages_t code, char * parameter);
void _wifiDebugCallback(justwifi_messages_t code, char * parameter);