#include <vector>
#include <iostream>

typedef enum {
    INVALID,
    WAG_ELEC_V1
} MODULES;

class LoRa_module {
    public:
    MODULES type = MODULES::INVALID;
    virtual bool test_connection() = 0;
    virtual bool start() = 0;
    virtual void end() = 0;
    virtual bool send_message(std::vector<uint8_t> &buffer) = 0;
    //virtual void set_receive_callback(std::function<void (rx_data&)> func) = 0;
    virtual void receive() = 0;
};
