#pragma once

// Copy to secrets.h and replace locally. The broker must provide MQTT over TLS.
inline constexpr char WIFI_SSID[] = "REPLACE_WITH_WIFI_SSID";
inline constexpr char WIFI_PASSWORD[] = "REPLACE_WITH_WIFI_PASSWORD";
inline constexpr char MQTT_HOST[] = "REPLACE_WITH_MQTT_HOSTNAME";
inline constexpr uint16_t MQTT_PORT = 8883;
inline constexpr char MQTT_USERNAME[] = "REPLACE_WITH_MQTT_USERNAME";
inline constexpr char MQTT_PASSWORD[] = "REPLACE_WITH_MQTT_PASSWORD";
inline constexpr char MQTT_TOPIC_ROOT[] = "REPLACE_WITH_UNIQUE_TOPIC_ROOT";
inline constexpr char MQTT_CLIENT_ID[] = "REPLACE_WITH_UNIQUE_CLIENT_ID";
inline constexpr char MQTT_ROOT_CA[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
REPLACE_WITH_BROKER_ROOT_CA_PEM
-----END CERTIFICATE-----
)PEM";
