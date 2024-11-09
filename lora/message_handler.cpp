#include "module_handler.h"
#include <cstdio>

int main(int argc, char *argv[]) {
    LoRa_module *module = new WE_lora_receiver_v1();
    module->start();
    std::printf("Hello World!\n");
    module->receive();
    module->end();
    return 0;
}