#include <cstdio>
#include <cassert>
#include <node_api.h>
#include <mqtt/async_client.h>
#include <array>
#include <iomanip>

// Modules
#include "wagner_electronics/lora_receiver_v1/lora_receiver_v1.h"

typedef struct {
    std::array<uint8_t, MAC_LENGTH> mac;
    std::string name;
    char type;
    uint8_t version;
} mac_lookup;

typedef struct {
    std::array<uint8_t, MAC_LENGTH> mac;
    char type;
    uint8_t version;
} discovered_device;

//cistern_data cistern_parser(std::vector<uint8_t> data);
napi_value set_device(napi_env env, napi_callback_info info);
napi_value set_mac_lookup(napi_env env, napi_callback_info info);
napi_value get_discovered_macs(napi_env env, napi_value exports);
napi_value get_registered_devices(napi_env env, napi_callback_info info);
napi_value reset_discovery_list(napi_env env, napi_callback_info info);
napi_value set_mqtt_settings(napi_env env, napi_callback_info info);
napi_value mqtt_is_connected(napi_env env, napi_callback_info info);
napi_value remove_from_discovery_list(napi_env env, napi_callback_info info);
napi_value Init(napi_env env, napi_value exports);

void message_parser(rx_data& packet);
cistern_data cistern_parser(std::vector<uint8_t> &data);

void publish_message(const std::string& topic, const std::string& payload);
void publish_data(cistern_data &rx_data);

std::string mac_to_string(std::array<uint8_t, MAC_LENGTH> mac);
void string_to_mac(std::string mac_string, std::array<uint8_t, MAC_LENGTH> &mac);
void add_discovered_mac(std::array<uint8_t, MAC_LENGTH> mac, char dev_type, uint8_t version);
void remove_discovered_mac(std::array<uint8_t, MAC_LENGTH> mac);
void remove_device_by_mac(std::array<uint8_t, MAC_LENGTH> mac);
std::string lookup_mac(std::array<uint8_t, MAC_LENGTH> mac);