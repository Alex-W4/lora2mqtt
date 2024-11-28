#include <thread>
#include <utility>
#include "lora_receiver_v1.h"
#include "lora_receiver_v1_config.h"


WE_lora_receiver_v1::WE_lora_receiver_v1(std::function<void (rx_data&)> callback_func_) : module(new sx1278_module(pin_config{SPI_HW_CH, SPI_FREQUENCY, SPI_SS_PIN, RESET_PIN, DIO_0_PIN, DIO_1_PIN, DIO_2_PIN, DIO_3_PIN}, operation_config{}, std::move(callback_func_))) {
    this->type = MODULES::WAG_ELEC_V1;
}

bool WE_lora_receiver_v1::test_connection() {
    return false;
}

bool WE_lora_receiver_v1::start() {
    return module->init();
}

void WE_lora_receiver_v1::end() {
    module->end();
}

bool WE_lora_receiver_v1::send_message(std::vector<uint8_t> &buffer) {
    return false;
}

/*
void WE_lora_receiver_v1::set_receive_callback(std::function<void (rx_data&)> func) {

}*/

void WE_lora_receiver_v1::receive() {
    module->receive();
    /*
    for (int i = 0; i < 60; ++i) {
        //std::printf("DIO_0_PIN: %b\n", gpioRead(DIO_0_PIN));
        //sx1278_module::rxDoneISRf(0,0,0, module);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }*/
}

