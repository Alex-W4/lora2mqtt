#include <vector>
#include <iostream>

struct LoRa_message {
    char buffer[256];
    size_t size;
};

class LoRa_module {
    public:
    virtual bool test_connection() = 0;
    virtual bool start() = 0;
    virtual void end() = 0;
    virtual bool send_message(std::vector<uint8_t> &buffer) = 0;
    virtual void set_receive_callback() = 0;
    virtual void receive() = 0;
};
