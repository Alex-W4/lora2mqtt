#include <pigpio.h>
#include "LoRa_chip.h"

#define REG_FIFO 0x00
#define REG_OP_MODE 0x01
#define REG_FR_MSB 0x06
#define REG_FR_MID 0x07
#define REG_FR_LSB 0x08
#define REG_PA_CONFIG 0x09
#define REG_OCP 0x0B
#define REG_LNA 0x0C
#define REG_FIFO_ADDR_PTR 0x0D
#define REG_FIFO_TX_BASE_ADDR 0x0E
#define REG_FIFO_RX_BASE_ADDR 0x0F
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS 0x12
#define REG_RX_NB_BYTES 0x13
#define REG_RX_HEADER_CNT_VALUE_MSB 0x14
#define REG_RX_HEADER_CNT_VALUE_LSB 0x15
#define REG_RX_PACKET_CNT_VALUE_MSB 0x16
#define REG_RX_PACKET_CNT_VALUE_LSB 0x17
#define REG_PKT_SNR_VALUE 0x19
#define REG_PKT_RSSI_VALUE 0x1A
#define REG_RSSI_VALUE 0x1B
#define REG_MODEM_CONFIG_1 0x1D
#define REG_MODEM_CONFIG_2 0x1E
#define REG_MODEM_CONFIG_3 0x26
#define REG_PAYLOAD_LENGTH 0x22
#define REG_FIFO_RX_BYTE_ADDR 0x25
#define REG_PA_DAC 0x4d
#define REG_DIO_MAPPING_1 0x40
#define REG_DIO_MAPPING_2 0x41
#define REG_TEMP 0x3c
#define REG_SYNC_WORD 0x39
#define REG_PREAMBLE_MSB 0x20
#define REG_PREAMBLE_LSB 0x21
#define REG_DETECT_OPTIMIZE 0x31
#define REG_DETECTION_THRESHOLD 0x37

#define TX_BASE_ADDR 0x00
#define RX_BASE_ADDR 0x00

#define LORA_MODE 0x80

#define SLEEP_MODE 0x00
#define STDBY_MODE 0x01
#define TX_MODE 0x03
#define RXCONT_MODE 0x05

#define IRQ_RXDONE 0x40
#define IRQ_TXDONE 0x08

typedef enum BandWidth{
    BW7_8 =0,
    BW10_4 = 1<<4,
    BW15_6 = 2<<4,
    BW20_8 = 3<<4,
    BW31_25 = 4<<4,
    BW41_7 = 5<<4,
    BW62_5 = 6<<4,
    BW125 = 7<<4,
    BW250 = 8<<4,
    BW500 = 9<<4,
} BandWidth;

typedef enum SpreadingFactor{
    SF7 = 7<<4,
    SF8 = 8<<4,
    SF9 = 9<<4,
    SF10 = 10<<4,
    SF11 = 11<<4,
    SF12 = 12<<4,
} SpreadingFactor;

typedef enum ErrorCodingRate{
    CR5 = 1<<1,
    CR6 = 2<<1,
    CR7 = 3<<1,
    CR8 = 4<<1,
} ErrorCodingRate;

typedef enum OutputPower{
    OP0 = 0,
    OP1 = 1,
    OP2 = 2,
    OP3 = 3,
    OP4 = 4,
    OP5 = 5,
    OP6 = 6,
    OP7 = 7,
    OP8 = 8,
    OP9 = 9,
    OP10 = 10,
    OP11 = 11,
    OP12 = 12,
    OP13 = 13,
    OP14 = 14,
    OP15 = 15,
    OP16 = 16,
    OP17 = 17,
    OP20 = 20,
} OutputPower;

typedef enum PowerAmplifireOutputPin{
    RFO = 0x70,
    PA_BOOST = 0xf0,
} PowerAmplifireOutputPin;

typedef enum LnaGain{
    G1 = 1,
    G2 = 2,
    G3 = 3,
    G4 = 4,
    G5 = 5,
    G6 = 6,
} LnaGain;

typedef enum header_mode{
    IMPLICIT,
    EXPLICIT
} header_mode;

typedef struct{
    char *buf;
    unsigned char size;//Size of buffer. Used in Explicit header mode. 255 MAX size
    struct timeval last_time;
    double Tsym;
    double Tpkt;
    unsigned payloadSymbNb;
    void *userPtr;//user pointer passing to user callback
} txData;

typedef struct{
    char buf[256];
    unsigned char size;
    struct timeval last_time;
    double SNR;
    int RSSI;
    bool CRC;
    void *userPtr;//user pointer passing to user callback
} rxData;

struct operation_config {
    BandWidth bw = BW500;
    SpreadingFactor sf = SF7; //only from SF7 to SF12. SF6 not support yet.
    ErrorCodingRate ecr = CR8;
    uint64_t freq = 433000000;// Frequency in Hz. Example 434000000
    unsigned int preambleLen = 6;
    bool lowDataRateOptimize = false;// Dont touch it sets automatically
    OutputPower outPower = OP17;
    PowerAmplifireOutputPin powerOutPin = PA_BOOST;//This chips has to outputs for signal "High power" and regular.
    unsigned char syncWord = 0x12;
    LnaGain lnaGain = G1;
    bool lnaBoost = true;//On/Off LNA boost
    bool AGC = true;// On/Off AGC. If AGC is on, LNAGain not used
    unsigned char OCP = 240;//Over Current Protection. 0 to turn OFF. Else reduces current from 45mA to 240mA
    header_mode header = EXPLICIT;
    unsigned char payloadLen = 10;//Payload len that used in implicit mode. In Explicit header mode not used.
    bool CRC = false;//1 - add CRC data and checking. 0 - remove CRC data and checking
};

void test_static(int gpio, int level, uint32_t tick, void* userdata);

class sx1278_module: public LoRa_chip {
    public:
    sx1278_module(pin_config pin_cfg, operation_config op_cfg): SPI_HW_CH(pin_cfg.SPI_HW_CH), SPI_FREQUENCY(pin_cfg.SPI_FREQUENCY), SPI_SS_PIN(pin_cfg.SPI_SS_PIN), RESET_PIN(pin_cfg.RESET_PIN), DIO_0_PIN(pin_cfg.DIO_0_PIN), DIO_1_PIN(pin_cfg.DIO_1_PIN), DIO_2_PIN(pin_cfg.DIO_2_PIN), DIO_3_PIN(pin_cfg.DIO_3_PIN), op_config_(op_cfg) {

    };

    bool init() override;
    void end() override;
    void send(std::vector<uint8_t> &buffer) override;
    void receive() override;
    static void rxDoneISRf(int gpio, int level, uint32_t tick, void* userdata);
    static void * rx_f(void *p);


    //private:
    const uint32_t SPI_HW_CH;
    const uint32_t SPI_FREQUENCY;
    const uint32_t SPI_SS_PIN;
    const uint32_t RESET_PIN;
    const uint32_t DIO_0_PIN;
    const uint32_t DIO_1_PIN;
    const uint32_t DIO_2_PIN;
    const uint32_t DIO_3_PIN;
    int32_t spi_handle_;
    operation_config op_config_;
    txData tx_data_;
    rxData rx_data_;
    pthread_t receiver_thread;

    void reset();
    int write_byte_to_reg(uint8_t reg, uint8_t byte);
    int write_buffer_to_reg(uint8_t reg, uint8_t *buffer, size_t size);
    uint8_t read_byte_from_reg(uint8_t reg);
    int read_buffer_from_reg(uint8_t reg, uint8_t *buffer, size_t size);
    void set_lora_mode();
    void set_sleep_mode();
    void set_tx_power(OutputPower level);
    void set_frequency(uint64_t frequency);
    void set_spreading_factor(SpreadingFactor sf);
    void set_bandwidth(BandWidth bw);
    void set_sync_word(unsigned char sw);
    void set_header(header_mode mode);
    void lora_set_payload(unsigned char payload_length);
    void set_errorcr(ErrorCodingRate cr);
    void set_preamble(uint32_t preamble_length);
    void set_agc(bool agc);
    void set_lna(LnaGain gain, bool boost);
    void set_ocp(uint8_t ocp);
    void calculate_packet_t();
    void set_low_datarate_optimize_on();
    void set_low_datarate_optimize_off();
    uint8_t get_op_mode();
    void set_standy_mode();
    void set_dio_rx_mapping();
    void set_rx_done_dioISR();
    void get_rssi_pkt();
    void get_snr();
    void reset_irq_flags();

    void set_rx_cont_mode();

    void enableCrc();
    void disableCrc();
};