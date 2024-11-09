#include "sx1278/sx1278.h"
#include "LoRa_module.h"
#include <pigpio.h>

class WE_lora_receiver_v1: public virtual LoRa_module {
    public:
    WE_lora_receiver_v1();
    bool test_connection() override;
    bool start() override;
    void end() override;
    bool send_message(std::vector<uint8_t> &buffer) override;
    void set_receive_callback() override;
    void receive() override;

    sx1278_module *module;
};