cmake_minimum_required(VERSION 3.22)
project(lora2mqtt)

set(CMAKE_CXX_STANDARD 17)


include_directories(.
        modules
        chips)

add_executable(lora2mqtt
        chips/sx1278/sx1278.cpp
        modules/wagner_electronics/lora_receiver_v1/lora_receiver_v1.cpp
        message_handler.cpp)

target_link_libraries(lora2mqtt pigpio)
