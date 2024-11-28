#include "message_handler.h"

// Modul List
std::vector<LoRa_module*> module_list;
std::vector<mac_lookup> mac_resolution_list;
std::vector<discovered_device> discovered_macs;

std::string mqtt_protocol_ = "mqtt";
const std::string mqtt_protocol_filler_ = "://";
std::string mqtt_server_adress_ = "localhost";
std::string mqtt_port_filler_ = ":";
std::string mqtt_port_ = "1883";
std::string mqtt_client_id_ = "lora2mqtt";
std::string mqtt_user_name_;
std::string mqtt_password_;

mqtt::async_client *mqtt_client_ = new mqtt::async_client(mqtt_protocol_ + mqtt_protocol_filler_ + mqtt_server_adress_ + mqtt_port_filler_ + mqtt_port_, mqtt_client_id_);
mqtt::connect_options *mqtt_con_opt_ = new mqtt::connect_options();


/*
 *  _  _  ___  ___  ___      _ ___
 * | \| |/ _ \|   \| __|  _ | / __|
 * | .` | (_) | |) | _|  | || \__ \
 * |_|\_|\___/|___/|___|  \__/|___/
 *
 */

napi_value set_device(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];

    // Parse input arguments
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok || argc < 1) {
        napi_throw_error(env, nullptr, "Expected one argument");
        return nullptr;
    }

    // Convert the input to DeviceType
    int32_t device_nr;
    status = napi_get_value_int32(env, args[0], &device_nr);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid enum value");
        return nullptr;
    }
    auto type = static_cast<MODULES>(device_nr);

    switch (type) {
        case MODULES::INVALID:
            napi_value napi_null;
            napi_get_null(env, &napi_null);
            return napi_null;
        case MODULES::WAG_ELEC_V1:
            LoRa_module *wag_elec_v1 = new WE_lora_receiver_v1(&message_parser);
            wag_elec_v1->start();
            wag_elec_v1->receive();
            module_list.emplace_back(wag_elec_v1);
            break;

    }
    return nullptr;
}

napi_value set_mac_lookup(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value args[4];

    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok || argc != 4) {
        napi_throw_error(env, nullptr, "Expected two arguments");
        return nullptr;
    }

    size_t result;
    char mac_char_buf[MAC_LENGTH*3-1];
    status = napi_get_value_string_utf8(env, args[0], mac_char_buf, MAC_LENGTH*3, &result);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[0]");
        return nullptr;
    }

    char name_char_buf[MAX_NAME_LENGTH];
    status = napi_get_value_string_utf8(env, args[1], name_char_buf, MAX_NAME_LENGTH, &result);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[1]");
        return nullptr;
    }

    char type_char_buf[2];
    status = napi_get_value_string_utf8(env, args[2], type_char_buf, 2, &result);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[2]");
        return nullptr;
    }

    uint32_t version;
    status = napi_get_value_uint32(env, args[3], &version);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[3]");
        return nullptr;
    }

    mac_lookup new_entry;

    //std::cout << "mac_char_buf: " << mac_char_buf << std::endl;

    string_to_mac(mac_char_buf, new_entry.mac);
    new_entry.name = name_char_buf;
    new_entry.type = type_char_buf[1];
    new_entry.version = version;
    if (std::find_if(mac_resolution_list.begin(), mac_resolution_list.end(),[&new_entry](const mac_lookup& entry) {return entry.mac == new_entry.mac;}) != mac_resolution_list.end()) {
        auto erase_element = std::remove_if(mac_resolution_list.begin(), mac_resolution_list.end(),[&new_entry](const mac_lookup& entry) {return entry.mac == new_entry.mac;});
        mac_resolution_list.erase(erase_element);
    }
    mac_resolution_list.emplace_back(new_entry);
    //std::cout << "mac_resolution_list[0].mac: " << mac_to_string(mac_resolution_list[0].mac) << std::endl;
    napi_value true_val;
    napi_create_uint32(env, 1, &true_val);
    return true_val;
}

napi_value get_discovered_macs(napi_env env, napi_callback_info info) {
    napi_status status;
    (void) info;
    // Create a new JavaScript object
    napi_value result;
    status = napi_create_object(env, &result);
    if (status != napi_ok) return nullptr;

    // Create a JavaScript array
    napi_value array;
    status = napi_create_array_with_length(env, discovered_macs.size(), &array);
    if (status != napi_ok) return nullptr;

    // Fill the array with strings from the vector
    for (size_t i = 0; i < discovered_macs.size(); ++i) {
        napi_value disc_obj;
        napi_value str, dev, version;

        // Create napi values
        status = napi_create_string_utf8(env, mac_to_string(discovered_macs[i].mac).c_str(), NAPI_AUTO_LENGTH, &str);
        if (status != napi_ok) return nullptr;
        status = napi_create_string_utf8(env, &discovered_macs[i].type, 1, &dev);
        if (status != napi_ok) return nullptr;
        status = napi_create_uint32(env, discovered_macs[i].version, &version);
        if (status != napi_ok) return nullptr;

        // Create object
        status = napi_create_object(env, &disc_obj);
        if (status != napi_ok) return nullptr;

        // Assign properties to object
        status = napi_set_named_property(env, disc_obj, "mac", str);
        if (status != napi_ok) return nullptr;
        status = napi_set_named_property(env, disc_obj, "device", dev);
        if (status != napi_ok) return nullptr;
        status = napi_set_named_property(env, disc_obj, "version", version);
        if (status != napi_ok) return nullptr;

        // Add the string to the array
        status = napi_set_element(env, array, i, disc_obj);
        if (status != napi_ok) return nullptr;
    }

    // Add the array to the result object with the key "discoveredMacs"
    status = napi_set_named_property(env, result, "macs", array);
    if (status != napi_ok) return nullptr;

    return result;
}

napi_value get_registered_devices(napi_env env, napi_callback_info info) {
    napi_status status;
    (void) info;
    // Create a new JavaScript object
    napi_value result;
    status = napi_create_object(env, &result);
    if (status != napi_ok) return nullptr;

    // Create a JavaScript array
    napi_value array;
    status = napi_create_array_with_length(env, mac_resolution_list.size(), &array);
    if (status != napi_ok) return nullptr;

    // Fill the array with strings from the vector
    for (size_t i = 0; i < mac_resolution_list.size(); ++i) {
        napi_value disc_obj;
        napi_value str, name, dev, version;

        // Create napi values
        status = napi_create_string_utf8(env, mac_to_string(mac_resolution_list[i].mac).c_str(), NAPI_AUTO_LENGTH, &str);
        if (status != napi_ok) return nullptr;
        status = napi_create_string_utf8(env, mac_resolution_list[i].name.c_str(), NAPI_AUTO_LENGTH, &name);
        if (status != napi_ok) return nullptr;
        status = napi_create_string_utf8(env, &mac_resolution_list[i].type, 1, &dev);
        if (status != napi_ok) return nullptr;
        status = napi_create_uint32(env, mac_resolution_list[i].version, &version);
        if (status != napi_ok) return nullptr;

        // Create object
        status = napi_create_object(env, &disc_obj);
        if (status != napi_ok) return nullptr;

        // Assign properties to object
        status = napi_set_named_property(env, disc_obj, "mac", str);
        if (status != napi_ok) return nullptr;
        status = napi_set_named_property(env, disc_obj, "name", name);
        if (status != napi_ok) return nullptr;
        status = napi_set_named_property(env, disc_obj, "device", dev);
        if (status != napi_ok) return nullptr;
        status = napi_set_named_property(env, disc_obj, "version", version);
        if (status != napi_ok) return nullptr;

        // Add the string to the array
        status = napi_set_element(env, array, i, disc_obj);
        if (status != napi_ok) return nullptr;
    }

    // Add the array to the result object with the key "discoveredMacs"
    status = napi_set_named_property(env, result, "macs", array);
    if (status != napi_ok) return nullptr;

    return result;
}

napi_value reset_discovery_list(napi_env env, napi_callback_info info) {
    (void) env;
    (void) info;
    discovered_macs.clear();
    return nullptr;
}
napi_value remove_from_discovery_list(napi_env env, napi_callback_info info) {
    (void) info;
    size_t argc = 1;
    napi_value args[1];

    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok || argc != 1) {
        napi_throw_error(env, nullptr, "Expected one argument");
        return nullptr;
    }

    size_t result;
    char mac_char_buf[MAC_LENGTH*3-1];
    status = napi_get_value_string_utf8(env, args[0], mac_char_buf, MAC_LENGTH*3, &result);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[0]");
        return nullptr;
    }
    std::array<uint8_t, MAC_LENGTH> erase_mac{};
    string_to_mac(mac_char_buf, erase_mac);

    remove_discovered_mac(erase_mac);
    return nullptr;
}

napi_value remove_from_device_list(napi_env env, napi_callback_info info) {
    (void) info;
    size_t argc = 1;
    napi_value args[1];

    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok || argc != 1) {
        napi_throw_error(env, nullptr, "Expected one argument");
        return nullptr;
    }

    size_t result;
    char mac_char_buf[MAC_LENGTH*3-1];
    status = napi_get_value_string_utf8(env, args[0], mac_char_buf, MAC_LENGTH*3, &result);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[0]");
        return nullptr;
    }
    std::array<uint8_t, MAC_LENGTH> erase_mac{};
    string_to_mac(mac_char_buf, erase_mac);

    remove_device_by_mac(erase_mac);
    return nullptr;
}


napi_value set_mqtt_settings(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value args[4];

    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok || argc != 4) {
        napi_throw_error(env, nullptr, "Expected four arguments");
        return nullptr;
    }

    // Server address
    size_t server_adress_size;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &server_adress_size);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[0]");
        return nullptr;
    }
    std::string server_adress(server_adress_size + 1, '\0');
    status = napi_get_value_string_utf8(env, args[0], &server_adress[0], server_adress_size+1, &server_adress_size);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[0]");
        return nullptr;
    }

    // Port
    size_t port_size;
    status = napi_get_value_string_utf8(env, args[1], nullptr, 0, &port_size);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[1]");
        return nullptr;
    }
    std::string port( port_size + 1, '\0');
    status = napi_get_value_string_utf8(env, args[1], &port[0], port_size + 1, &port_size);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[1]");
        return nullptr;
    }

    // User name
    size_t user_name_size;
    status = napi_get_value_string_utf8(env, args[2], nullptr, 0, &user_name_size);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[2]");
        return nullptr;
    }
    std::string user_name( user_name_size + 1, '\0');
    status = napi_get_value_string_utf8(env, args[2], &user_name[0], user_name_size + 1, &user_name_size);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[2]");
        return nullptr;
    }

    // Password
    size_t password_size;
    status = napi_get_value_string_utf8(env, args[3], nullptr, 0, &password_size);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[3]");
        return nullptr;
    }
    std::string password( password_size + 1, '\0');
    status = napi_get_value_string_utf8(env, args[3], &password[0], password_size + 1, &password_size);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Invalid string value args[3]");
        return nullptr;
    }

    mqtt_server_adress_ = server_adress;
    mqtt_port_ = port;
    mqtt_user_name_ = user_name;
    mqtt_password_ = password;

    if (mqtt_client_->is_connected()) {
        mqtt_client_->disconnect();
    }
    mqtt_client_ = new mqtt::async_client(mqtt_protocol_ + mqtt_protocol_filler_ + mqtt_server_adress_ + mqtt_port_filler_ + mqtt_port_, mqtt_client_id_);
    if (mqtt_user_name_.length() > 1 && mqtt_password_.length() > 1) {
        mqtt_con_opt_->set_user_name(mqtt_user_name_);
        mqtt_con_opt_->set_password(mqtt_password_);
    }
    mqtt_con_opt_->set_automatic_reconnect(false);
    mqtt_con_opt_->set_clean_session(true);
    try {
        mqtt_client_->connect(*mqtt_con_opt_)->wait();
    } catch (const mqtt::exception& exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        napi_value true_val;
        napi_create_uint32(env, -1, &true_val);
        return true_val;
    }

    if (!mqtt_client_->is_connected()) {
        napi_throw_error(env, nullptr, "MQTT connection failed!");
        return nullptr;
    }
    mqtt_client_->disconnect();
    mqtt_con_opt_->set_automatic_reconnect(true);
    try {
        mqtt_client_->connect(*mqtt_con_opt_)->wait();
    } catch (const mqtt::exception& exc){
        std::cerr << "Error: " << exc.what() << std::endl;
        napi_value true_val;
        napi_create_uint32(env, -1, &true_val);
        return true_val;
    }

    napi_value true_val;
    napi_create_uint32(env, 1, &true_val);
    return true_val;
}

napi_value mqtt_is_connected(napi_env env, napi_callback_info info) {
    (void) env;
    (void) info;
    napi_value return_val;
    if (mqtt_client_->is_connected()) {
        napi_create_uint32(env, 1, &return_val);
    } else {
        napi_create_uint32(env, 0, &return_val);
    }
    return return_val;
}

napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor properties[] = {
            { "set_device", nullptr, set_device, nullptr, nullptr, nullptr, napi_default, nullptr },
            { "set_mac_lookup", nullptr, set_mac_lookup, nullptr, nullptr, nullptr, napi_default, nullptr },
            { "get_discovered_macs", nullptr, get_discovered_macs, nullptr, nullptr, nullptr, napi_default, nullptr },
            { "get_registered_devices", nullptr, get_registered_devices, nullptr, nullptr, nullptr, napi_default, nullptr },
            { "reset_discovery_list", nullptr, reset_discovery_list, nullptr, nullptr, nullptr, napi_default, nullptr },
            { "set_mqtt_settings", nullptr, set_mqtt_settings, nullptr, nullptr, nullptr, napi_default, nullptr },
            { "mqtt_is_connected", nullptr, mqtt_is_connected, nullptr, nullptr, nullptr, napi_default, nullptr },
            { "remove_from_discovery_list", nullptr, remove_from_discovery_list, nullptr, nullptr, nullptr, napi_default, nullptr },
            { "remove_from_device_list", nullptr, remove_from_device_list, nullptr, nullptr, nullptr, napi_default, nullptr }
    };

    napi_status status = napi_define_properties(env, exports, 9, properties);
    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Failed to define properties");
    }
    return exports;
}

NAPI_MODULE(lora2mqtt, Init)


/*
 *  __  __ ___  ___    _  _   _   _  _ ___  _    ___
 * |  \/  / __|/ __|  | || | /_\ | \| |   \| |  | __|
 * | |\/| \__ \ (_ |  | __ |/ _ \| .` | |) | |__| _|
 * |_|  |_|___/\___|  |_||_/_/ \_\_|\_|___/|____|___|
 *
 */

void message_parser(rx_data& packet) {
    const std::string PACKET_KEY_STR = "WaEl";
    const std::vector<uint8_t> PACKET_KEY(PACKET_KEY_STR.begin(), PACKET_KEY_STR.end());

    uint8_t device_byte = PACKET_KEY_STR.length();
    uint8_t device_version = PACKET_KEY_STR.length() + 1;

    //std::cout << "device_byte: " << packet.buf[device_byte] << ", " << static_cast<uint32_t>(device_byte) << ", device_version: " << packet.buf[device_version] << ", " << static_cast<uint32_t>(device_version) << std::endl;

    if (!std::equal(packet.buf.begin(), packet.buf.begin() + static_cast<uint32_t>(PACKET_KEY_STR.length()), PACKET_KEY.begin(), PACKET_KEY.begin() + static_cast<uint32_t>(PACKET_KEY_STR.length()))) {
        //printf("Unknown sender!\n");
        return;
    }
    cistern_data rx_cist;
    std::vector<uint8_t> temp_vec(packet.buf.begin() + device_version + 1, packet.buf.end());
    switch (packet.buf[device_byte]) {
        case ZISTERNENSENSOR:
            rx_cist = cistern_parser(temp_vec);
            rx_cist.rssi = packet.RSSI;
            rx_cist.version = packet.buf[device_version];
            publish_data(rx_cist);
            break;
        case FENSTERSENSOR:
            break;
        default:
            return;
    }
}

cistern_data cistern_parser(std::vector<uint8_t> &data) {
    //printf("data: %s\n", std::string(data.begin(), data.end()).c_str());
    cistern_data z_data;
    size_t pos = 0;
    while (data.size() > pos) {
        //std::cout << "pos: " << pos << ", data[pos]: " << static_cast<uint32_t>(data[pos]) << std::endl;
        switch (data[pos]) {
            case payload_type::mac:
                if (pos + mac_handle.length <= data.size()) {
                    for (int i = 0; i < mac_handle.length; ++i) {
                        z_data.mac[i] = data[i + pos + 1];
                    }
                }
                pos += mac_handle.length + 1;
                break;
            case payload_type::battery_voltage:
                if (pos + battery_voltage_handle.length <= data.size()) {
                    uint8_t byte_set[4];
                    byte_set[0] = data[pos + 1];
                    byte_set[1] = data[pos + 2];
                    byte_set[2] = data[pos + 3];
                    byte_set[3] = data[pos + 4];
                    //std::cout << "Float: " << static_cast<uint32_t>(byte_set[0]) << ", " << static_cast<uint32_t>(byte_set[1]) << ", " << static_cast<uint32_t>(byte_set[2]) << ", " << static_cast<uint32_t>(byte_set[3]) << std::endl;
                    z_data.battery_voltage = *reinterpret_cast<float*>(byte_set);
                }
                pos += battery_voltage_handle.length + 1;
                break;
            case payload_type::distance:
                if (pos + distance_handle.length <= data.size()) {
                    uint8_t byte_set[4];
                    byte_set[0] = data[pos + 1];
                    byte_set[1] = data[pos + 2];
                    byte_set[2] = data[pos + 3];
                    byte_set[3] = data[pos + 4];
                    z_data.distance = *reinterpret_cast<int32_t*>(byte_set);
                }
                pos += distance_handle.length + 1;
                break;
            default:
                pos = data.size();
                break;
        }
    }
    return z_data;
}

/*
 *  __  __  ___ _____ _____
 * |  \/  |/ _ \_   _|_   _|
 * | |\/| | (_) || |   | |
 * |_|  |_|\__\_\|_|   |_|
 *
 */

void publish_data(cistern_data &rx_data) {
    //printf("publish" );
    if (lookup_mac(rx_data.mac).empty()) {
        add_discovered_mac(rx_data.mac, ZISTERNENSENSOR, rx_data.version);
        return;
    }
    if (!mqtt_client_->is_connected()) {
        return;
    }

    const int TIMEOUT = 1000; // Milliseconds
    try {
        // Publish the message
        std::string topic = "lora2mqtt/cistern/";
        topic.append(lookup_mac(rx_data.mac));
        topic.append("/");


        mqtt::message_ptr pubmsg = mqtt::make_message(topic + ("distance"), std::to_string(rx_data.distance));
        //std::cout << "distance: " << rx_data.distance << std::endl;
        pubmsg->set_qos(1);
        mqtt_client_->publish(pubmsg)->wait_for(TIMEOUT);

        pubmsg = mqtt::make_message(topic + ("battery_voltage"), std::to_string(rx_data.battery_voltage));
        //std::cout << "battery_voltage: " << rx_data.battery_voltage << std::endl;
        pubmsg->set_qos(1);
        mqtt_client_->publish(pubmsg)->wait_for(TIMEOUT);

        pubmsg = mqtt::make_message(topic + ("rssi"), std::to_string(rx_data.rssi));
        //std::cout << "rssi: " << rx_data.rssi << std::endl <<std::endl;
        pubmsg->set_qos(1);
        mqtt_client_->publish(pubmsg)->wait_for(TIMEOUT);

    } catch (const mqtt::exception& exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
    }
}


std::string mac_to_string(std::array<uint8_t, MAC_LENGTH> mac) {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');
    ss << std::hex << std::setw(2) << static_cast<int>(mac[0]);

    for (int i = 1; i < MAC_LENGTH; i++) {
        ss << ":";
        ss << std::hex << std::setw(2) << static_cast<int>(mac[i]);
    }

    return ss.str();
}

void string_to_mac(std::string mac_string, std::array<uint8_t, MAC_LENGTH> &mac) {

    std::stringstream ss;

    std::remove( mac_string.begin(), mac_string.end(), ':');

    ss << std::hex << mac_string;

    size_t i;
    int value;
    for (i = 0; i < MAC_LENGTH && sscanf(ss.str().c_str() + i * 2, "%2x", &value) == 1; i++) {
        mac[i] = value;
    }
}

void add_discovered_mac(std::array<uint8_t, MAC_LENGTH> mac, char dev_type, uint8_t version) {
    bool all_zero = true;
    for (int i = 0; i < MAC_LENGTH; ++i) {
        if (mac[i] != 0) {
            all_zero = false;
        }
    }
    if (!all_zero && std::find_if(discovered_macs.begin(), discovered_macs.end(),[&mac](const discovered_device entry) {return entry.mac == mac;}) == discovered_macs.end()){
        discovered_device new_entry;
        new_entry.mac = mac;
        new_entry.type = dev_type;
        new_entry.version = version;
        discovered_macs.emplace_back(new_entry);
    }
}

void remove_discovered_mac(std::array<uint8_t, MAC_LENGTH> mac) {
    if (std::find_if(discovered_macs.begin(), discovered_macs.end(),[&mac](const discovered_device entry) {return entry.mac == mac;}) != discovered_macs.end()) {
        discovered_macs.erase(std::find_if(discovered_macs.begin(), discovered_macs.end(),[&mac](const discovered_device entry) {return entry.mac == mac;}));
    }
}

void remove_device_by_mac(std::array<uint8_t, MAC_LENGTH> mac) {
    if (std::find_if(mac_resolution_list.begin(), mac_resolution_list.end(),[&mac](const mac_lookup entry) {return entry.mac == mac;}) != mac_resolution_list.end()) {
        mac_resolution_list.erase(std::find_if(mac_resolution_list.begin(), mac_resolution_list.end(),[&mac](const mac_lookup entry) {return entry.mac == mac;}));
    }
}

std::string lookup_mac(std::array<uint8_t, MAC_LENGTH> mac) {
    for (auto & lookup_entry : mac_resolution_list) {
        if (lookup_entry.mac == mac) {
            return lookup_entry.name;
        }
    }
    return "";
}



/*
int main(int argc, char *argv[]) {
    LoRa_module *module = new WE_lora_receiver_v1();
    if (!module->start()) {
        return 1;
    }
    module->receive();
    module->end();
    return 0;
}*/


