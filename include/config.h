#pragma once
#include <message.h>
#include <stdint.h>
/*-------------------------------------------------------------------------------
General Configuration
--------------------------------------------------------------------------------*/
#define VERSION "v1.9.0" // internal program version

#define WIFI_RECONNECT 30000 // Delay between wifi reconnection tries
#define MQTT_RECONNECT 10000 // Delay between mqtt reconnection tries

struct s_cfg_jaro {
  unsigned long masterMSB;
  unsigned long masterLSB;
  bool learn_mode;
  uint32_t serial;
  char ch_name[16][64]{"\0"};
  bool ch_enable[16];
  char grp_name[6][64]{"\0"};
  bool grp_enable[6];
  uint16_t grp_mask[6];
  char remote_name[16][64]{"\0"};
  uint32_t remote_serial[16];
  bool remote_enable[16];
  uint16_t remote_mask[16];
};

struct s_cfg_timer {
  bool enable;          // Timer enable
  uint8_t type;         // 0 = fixed time, 2 = sunrise, 3 = sunset
  char time_value[6];   // fixed Time value (hh:mm)
  int16_t offset_value; // offset value in minutes for sunrise/sunset
  uint8_t cmd;          // 0 = up, 1 = down, 2=shade
  bool monday;
  bool tuesday;
  bool wednesday;
  bool thursday;
  bool friday;
  bool saturday;
  bool sunday;
  uint16_t grp_mask; // Group mask for included channels
  bool use_min_time;
  char min_time_value[6]; // fixed Time value (hh:mm)
  bool use_max_time;
  char max_time_value[6]; // fixed Time value (hh:mm)
};

struct s_cfg_geo {
  float latitude;
  float longitude;
};

struct s_cfg_wifi {
  bool enable = false;
  char ssid[128];
  char password[128];
  char hostname[128];
  bool static_ip = false;
  char ipaddress[17];
  char subnet[17];
  char gateway[17];
  char dns[17];
};

struct s_cfg_eth {
  bool enable = false;
  char hostname[128];
  bool static_ip = false;
  char ipaddress[17];
  char subnet[17];
  char gateway[17];
  char dns[17];
  int gpio_sck;
  int gpio_mosi;
  int gpio_miso;
  int gpio_cs;
  int gpio_irq;
  int gpio_rst;
};

struct s_cfg_mqtt {
  bool enable;
  char server[128];
  char user[128];
  char password[128];
  char topic[128];
  uint16_t port = 1883;
  bool ha_enable;
  char ha_topic[64];
  char ha_device[32];
};

struct s_cfg_ntp {
  bool enable = true;
  char server[128] = {"de.pool.ntp.org"};
  char tz[128] = {"CET-1CEST,M3.5.0,M10.5.0/3"};
};

struct s_cfg_gpio {
  int led_setup;
  int gdo0; // TX
  int gdo2; // RX
  int sck;
  int mosi;
  int miso;
  int cs;
};

struct s_cfg_auth {
  bool enable = true;
  char user[64];
  char password[64];
};

struct s_cfg_log {
  bool enable = true;
  int level = 3;
  int order = 0;
};

struct s_config {
  int version;
  int lang;
  s_cfg_wifi wifi;
  s_cfg_eth eth;
  s_cfg_mqtt mqtt;
  s_cfg_ntp ntp;
  s_cfg_gpio gpio;
  s_cfg_auth auth;
  s_cfg_log log;
  s_cfg_jaro jaro;
  s_cfg_timer timer[6];
  s_cfg_geo geo;
};

extern s_config config;
extern bool setupMode;
void configSetup();
void configCyclic();
void configSaveToFile();
void configLoadFromFile();
void configInitValue();
void configGPIO();