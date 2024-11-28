#include <vector>
#include <iostream>
#include <csignal>
#include <cstring>
#include <vector>
#include <functional>
//#include <node_api.h>
#include "msg.h"



#pragma once
class LoRa_chip {
    public:
    virtual bool init() = 0;
    virtual void end() = 0;
    virtual void send(std::vector<uint8_t> &buffer) = 0;
    //virtual void set_receive_callback(std::function<void (rx_data&)> func) = 0;
    virtual void receive() = 0;

    /*
    virtual void setTxPower(uint8_t level) = 0;
    virtual void setFrequency(uint64_t frequency) = 0;
    virtual void setSpreadingFactor(uint32_t sf) = 0;
    virtual void setSignalBandwidth(uint64_t sbw) = 0;
    virtual void setCodingRate4(uint32_t denominator) = 0;
    virtual void setPreambleLength(uint64_t length) = 0;
    virtual void setSyncWord(uint32_t sw) = 0;

    virtual void enableCrc() = 0;
    virtual void disableCrc() = 0;

    virtual void setOCP(uint8_t mA) = 0;

    virtual void setGain(uint8_t gain) = 0;*/
};

