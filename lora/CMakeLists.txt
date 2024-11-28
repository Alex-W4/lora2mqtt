cmake_minimum_required(VERSION 3.22)
set(BUILD_SHARED_LIBS OFF)

project(lora2mqtt)

add_definitions(-DNAPI_VERSION=7)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

#include_directories(modules
#        chips
#        /usr/include/node)

#add_executable(lora2mqtt
#        modules/wagner_electronics/lora_receiver_v1/lora_receiver_v1.cpp
#        chips/sx1278/sx1278.cpp
#        message_handler.cpp)

#target_link_libraries(lora2mqtt pigpio)

find_package(PahoMqttCpp REQUIRED)

set(CMAKE_EXE_LINKER_FLAGS "-static")

# Build a shared library named after the project from the files in `src/`
file(GLOB SOURCE_FILES "*.cc" "*.h")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC}
        message_handler.cpp
        modules/wagner_electronics/lora_receiver_v1/lora_receiver_v1.cpp
        chips/sx1278/sx1278.cpp
        msg.h.cpp)

# Gives our library file a .node extension without any "lib" prefix
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node" LINKER_LANGUAGE CXX)

# Essential include files to build a node addon,
# You should add this line in every CMake.js based project
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC}
        modules
        chips
        /usr/include/node
        .)

# Essential library files to link to a node addon
# You should add this line in every CMake.js based project
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} pigpio PahoMqttCpp::paho-mqttpp3)



