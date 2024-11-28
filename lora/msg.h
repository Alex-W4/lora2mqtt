#include <cstdint>
#include <string>
#include <vector>
#include <array>


#define ZISTERNENSENSOR static_cast<uint8_t>('Z')
#define FENSTERSENSOR static_cast<uint8_t>('F')


#define MAC_LENGTH 6
#define MAX_NAME_LENGTH 64

typedef struct{
    char *buf;
    unsigned char size;
    struct timeval last_time;
    double Tsym;
    double Tpkt;
    unsigned payload_symb_nb;
} tx_data;

typedef struct{
    std::vector<uint8_t> buf;
    unsigned char size;
    struct timeval last_time;
    double SNR;
    int RSSI;
    bool CRC;
} rx_data;

enum msg_datatype {
    UINT8_T = 0,
    UINT16_T = 1,
    UINT32_T = 2,
    UINT64_T = 3,
    INT8_T = 4,
    INT16_T = 5,
    INT32_T = 6,
    INT64_T = 7,
    CHAR = 8,
    FLOAT = 9,
    DOUBLE = 10,
};

enum payload_type {
    mac = 0,
    battery_voltage = 1,
    distance = 2,
};

struct {
    const uint8_t length = sizeof(uint8_t) * MAC_LENGTH;
} mac_handle;

struct {
    uint8_t length = sizeof(uint8_t);
} version_handle;

struct {
    uint8_t length = sizeof(float);
} battery_voltage_handle;

struct {
    uint8_t length = sizeof(int);
} distance_handle;

struct cistern_data{
    int32_t distance{-1};
    float battery_voltage{0};
    std::array<uint8_t, MAC_LENGTH> mac{0};
    uint8_t version{0};
    int rssi{0};
};