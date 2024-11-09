#include <cmath>
#include "sx1278.h"
#include <chrono>
#include <thread>


bool sx1278_module::init() {
    if (gpioInitialise() < 0) {
        std::cout << "PI gpio init error!" << std::endl;
        return false;
    }

    reset();

    if ((spi_handle_ = spiOpen(SPI_HW_CH, 32000, 0x000020)) < 0) { //0x000020
        std::cout << "'spiOpen' error code: " << spi_handle_ << std::endl;
        return false;
    }
    //read_byte_from_reg(0x42);

    set_lora_mode();

    set_header(op_config_.header);

    if (op_config_.header == IMPLICIT) {
        lora_set_payload(op_config_.payloadLen);
    }

    //set_errorcr(op_config_.ecr);
    //set_bandwidth(op_config_.bw);
    //set_spreading_factor(op_config_.sf);

    if (op_config_.CRC) {
        enableCrc();
    } else {
        disableCrc();
    }

    set_tx_power(op_config_.outPower);
    //set_sync_word(op_config_.syncWord);
    //set_preamble(op_config_.preambleLen);
    set_agc(op_config_.AGC);
    //set_lna(op_config_.lnaGain, op_config_.lnaBoost);

    write_byte_to_reg(REG_FIFO_TX_BASE_ADDR, TX_BASE_ADDR);
    write_byte_to_reg(REG_FIFO_RX_BASE_ADDR, RX_BASE_ADDR);
    write_byte_to_reg(REG_DETECT_OPTIMIZE, 0xc3);
    write_byte_to_reg(REG_DETECTION_THRESHOLD, 0x0a);

    set_frequency(op_config_.freq);
    return true;
}

void sx1278_module::end() {
    gpioTerminate();
}

void sx1278_module::send(std::vector<uint8_t> &buffer) {

}

void sx1278_module::receive() {
    calculate_packet_t();
    if(op_config_.lowDataRateOptimize){
        set_low_datarate_optimize_on();
    }
    else{
        set_low_datarate_optimize_off();
    }
    if(get_op_mode() != STDBY_MODE){
        set_standy_mode();
    }
    set_dio_rx_mapping();
    set_rx_done_dioISR();
    set_rx_cont_mode();
}

void sx1278_module::reset() {
    gpioSetMode(RESET_PIN, PI_OUTPUT);
    gpioWrite(RESET_PIN, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    gpioWrite(RESET_PIN, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

int sx1278_module::write_byte_to_reg(uint8_t reg, uint8_t byte) {
    char rx[2], tx[2];
    tx[0]=(reg | 0x80);
    tx[1]=byte;

    rx[0]=0x00;
    rx[1]=0x00;

    return spiXfer(spi_handle_, tx, rx, 2);
}

int sx1278_module::write_buffer_to_reg(uint8_t reg, uint8_t *buffer, size_t size) {
    char tx[257];
    char rx[257];
    memset(tx, '\0', 257);
    memset(rx, '\0', 257);

    tx[0]=(reg | 0x80);
    memcpy(&tx[1], buffer, size);
    return spiXfer(spi_handle_, tx, rx, size+1);
}

uint8_t sx1278_module::read_byte_from_reg(uint8_t reg) {
    int ret;
    char rx[2], tx[2];
    tx[0]=reg;
    tx[1]=0x00;

    rx[0]=0x00;
    rx[1]=0x00;

    ret = spiXfer(spi_handle_, tx, rx, 2);
    if(ret<0)
        return ret;

    if(ret<=1)
        return -1;
    //std::printf("Read reg 0x%02x: %02x\n",reg, static_cast<uint8_t>(rx[1]));
    return rx[1];
}

int sx1278_module::read_buffer_from_reg(uint8_t reg, uint8_t *buffer, size_t size){
    int ret;
    char tx[257];
    char rx[257];

    memset(tx, '\0', 257);
    memset(rx, '\0', 257);
    memset(buffer, '\0', size);
    tx[0]=reg;
    ret = spiXfer(spi_handle_, tx, rx, size+1);
    memcpy(buffer, &rx[1], ret-1);
    //std::cout << "Read bytes: " << buffer << std::endl;
    return ret;
}


void sx1278_module::set_lora_mode() {
    set_sleep_mode();
    write_byte_to_reg(REG_OP_MODE, (read_byte_from_reg(REG_OP_MODE) & 0x7f) | LORA_MODE );
}

void sx1278_module::set_sleep_mode() {
    write_byte_to_reg( REG_OP_MODE, (read_byte_from_reg(REG_OP_MODE) & 0xf8) | SLEEP_MODE );
}

void sx1278_module::set_header(header_mode mode) {
    if (mode == EXPLICIT) {
        write_byte_to_reg(REG_MODEM_CONFIG_1, read_byte_from_reg(REG_MODEM_CONFIG_1) & 0xfe );
    } else {
        write_byte_to_reg(REG_MODEM_CONFIG_1, read_byte_from_reg(REG_MODEM_CONFIG_1) | 0x01 );
    }
}

void sx1278_module::lora_set_payload(unsigned char payload_length){
    write_byte_to_reg(REG_PAYLOAD_LENGTH, payload_length);
}

void sx1278_module::set_errorcr(ErrorCodingRate cr){
    write_byte_to_reg(REG_MODEM_CONFIG_1, (read_byte_from_reg(REG_MODEM_CONFIG_1)& 0xf1) | cr );
}

void sx1278_module::set_bandwidth(BandWidth bw) {
    write_byte_to_reg(REG_MODEM_CONFIG_1, (read_byte_from_reg(REG_MODEM_CONFIG_1)& 0x0f) | bw );
}

void sx1278_module::set_spreading_factor(SpreadingFactor sf) {
    write_byte_to_reg(REG_MODEM_CONFIG_2, (read_byte_from_reg(REG_MODEM_CONFIG_2)& 0x0f) | sf );
}

void sx1278_module::enableCrc() {
    write_byte_to_reg(REG_MODEM_CONFIG_2, (read_byte_from_reg(REG_MODEM_CONFIG_2)& 0xfb) | 0x01<<2 );
}

void sx1278_module::disableCrc() {
    write_byte_to_reg(REG_MODEM_CONFIG_2, (read_byte_from_reg(REG_MODEM_CONFIG_2)& 0xfb));
}

void sx1278_module::set_tx_power(OutputPower level) {
    write_byte_to_reg(REG_OCP, 0x1f);//Disable over current protection

    if(op_config_.powerOutPin == RFO){
        level = level >= OP15 ? OP15 : ( level <= OP0 ? OP0 : level);
        write_byte_to_reg(REG_PA_DAC, 0x84);//default val to +17dBm
        write_byte_to_reg(REG_PA_CONFIG, op_config_.powerOutPin | level);
        return;
    }else if(op_config_.powerOutPin == PA_BOOST){
        if(level == OP20){
            write_byte_to_reg(REG_PA_DAC, 0x87);//Max 20dBm power
            write_byte_to_reg(REG_PA_CONFIG, op_config_.powerOutPin | (level -2));
            return;
        }
        else{
            level = level >= OP17 ? OP17 : ( level <= OP2 ? OP2 : level);
            write_byte_to_reg(REG_PA_DAC, 0x84);//default val to +17dBm
            write_byte_to_reg(REG_PA_CONFIG, op_config_.powerOutPin | (level -2));
            return;
        }
    }
}

void sx1278_module::set_sync_word(unsigned char sw) {
    write_byte_to_reg(REG_SYNC_WORD, sw);
}

void sx1278_module::set_preamble(uint32_t preamble_length) {
    if(preamble_length < 6){
        preamble_length = 6;
    }
    else if(preamble_length > 65535){
        preamble_length = 65535;
    }
    unsigned len_revers=0;
    len_revers += ((unsigned char)(preamble_length>>0))<<8;
    len_revers += ((unsigned char)(preamble_length>>8))<<0;
    write_buffer_to_reg(REG_PREAMBLE_MSB, reinterpret_cast<uint8_t*>(&len_revers), 2);
}

void sx1278_module::set_agc(bool agc) {
    write_byte_to_reg(REG_MODEM_CONFIG_3, (agc << 2));
}

void sx1278_module::set_lna(LnaGain gain, bool boost) {
    write_byte_to_reg(REG_LNA,  ( (gain << 5) + boost) );
}

void sx1278_module::set_ocp(uint8_t ocp) {
    uint8_t OcpTrim;
    if(ocp == 0) {//turn off OCP
        write_byte_to_reg(REG_OCP, (read_byte_from_reg(REG_OCP) & 0xdf));
    }
    else if(ocp > 0 && ocp <= 120) {
        if(ocp < 50){ocp = 50;}

        OcpTrim = (ocp-45)/5 + 0x20;
        write_byte_to_reg(REG_OCP, OcpTrim);
    }
    else if(ocp > 120){
        if(ocp < 130){ocp = 130;}

        OcpTrim = (ocp+30)/10 + 0x20;
        write_byte_to_reg(REG_OCP, OcpTrim);
    }
}

void sx1278_module::set_frequency(uint64_t frequency) {
    uint64_t frf, frf_revers=0;
    frf = (frequency << 19) / 32000000;
    write_byte_to_reg(REG_FR_MSB, static_cast<uint8_t>(frf >> 16));
    write_byte_to_reg(REG_FR_MID, static_cast<uint8_t>(frf >> 8));
    write_byte_to_reg(REG_FR_LSB, static_cast<uint8_t>(frf >> 0));
}

void sx1278_module::calculate_packet_t() {
    unsigned BW_VAL[10] = {7800, 10400, 15600, 20800, 31250, 41700, 62500, 125000, 250000, 500000};

    double Tsym, Tpreamle, Tpayload, Tpacket;
    unsigned payloadSymbNb;
    int tmpPoly;

    unsigned bw = BW_VAL[(op_config_.bw>>4)];
    int sf = op_config_.sf>>4;
    unsigned char ecr = 4+(op_config_.ecr/2);
    int payload;
    if(op_config_.header == IMPLICIT){
        payload = op_config_.payloadLen;
    }
    else{
        payload = tx_data_.size;
    }

    Tsym = (pow(2, sf)/bw)*1000;
    op_config_.lowDataRateOptimize = (Tsym > 16);

    Tpreamle = (op_config_.preambleLen+4.25)*Tsym;
    tmpPoly = (8*payload - 4*sf + 28 + 16 - 20*(op_config_.header == IMPLICIT ? 1 : 0));
    if(tmpPoly<0){
        tmpPoly=0;
    }
    payloadSymbNb = 8+ceil(((double)tmpPoly)/(4*(sf - 2*op_config_.lowDataRateOptimize)))*ecr;
    Tpayload = payloadSymbNb*Tsym;
    Tpacket = Tpayload+Tpreamle;

    tx_data_.Tsym = Tsym;
    tx_data_.Tpkt = Tpacket;
    tx_data_.payloadSymbNb = payloadSymbNb;
}

void sx1278_module::set_low_datarate_optimize_on() {
    write_byte_to_reg(REG_MODEM_CONFIG_3, (read_byte_from_reg(REG_MODEM_CONFIG_3) & 0xf7) | (0x01<<3));
}

void sx1278_module::set_low_datarate_optimize_off() {
    write_byte_to_reg(REG_MODEM_CONFIG_3, (read_byte_from_reg(REG_MODEM_CONFIG_3) & 0xf7));
}

uint8_t sx1278_module::get_op_mode() {
    return (read_byte_from_reg(REG_OP_MODE) & 0x07);
}

void sx1278_module::set_standy_mode() {
    write_byte_to_reg(REG_OP_MODE, (read_byte_from_reg(REG_OP_MODE) & 0xf8) | STDBY_MODE );
}

void sx1278_module::set_dio_rx_mapping() {
    write_byte_to_reg(REG_DIO_MAPPING_1, 0<<6);
}

void sx1278_module::set_rx_done_dioISR() {
    printf("DIO_0_PIN: %i\n", DIO_0_PIN);
    gpioSetMode(DIO_0_PIN, PI_INPUT);
    printf("gpioSetISRFuncEx: %i\n", gpioSetAlertFuncEx(DIO_0_PIN, rxDoneISRf, this));
    //gpioSetISRFuncEx(DIO_0_PIN, RISING_EDGE, 0, reinterpret_cast<gpioISRFuncEx_t>(std::printf("Test callback! 0\n")), this);
}

void sx1278_module::rxDoneISRf(int gpio, int level, uint32_t tick, void* userdata){
    //sx1278_module *handle = new sx1278_module(pin_config{1, 433000000, 7, 22, 23, 24, 25, 0}, operation_config{});
    sx1278_module *handle = static_cast<sx1278_module *>(userdata);
    unsigned char rx_nb_bytes;

    if(handle->read_byte_from_reg(REG_IRQ_FLAGS) & IRQ_RXDONE){
        handle->write_byte_to_reg(REG_FIFO_ADDR_PTR, handle->read_byte_from_reg(REG_FIFO_RX_CURRENT_ADDR));

        gettimeofday(&handle->rx_data_.last_time, nullptr);

        if(handle->op_config_.header == IMPLICIT){
            handle->read_buffer_from_reg(REG_FIFO, reinterpret_cast<uint8_t *>(handle->rx_data_.buf), handle->op_config_.payloadLen);
            handle->rx_data_.size = handle->op_config_.payloadLen;
        }
        else{
            rx_nb_bytes = handle->read_byte_from_reg(REG_RX_NB_BYTES);
            handle->read_buffer_from_reg(REG_FIFO, reinterpret_cast<uint8_t *>(handle->rx_data_.buf), rx_nb_bytes);
            handle->rx_data_.size = rx_nb_bytes;
        }
        handle->rx_data_.CRC = (handle->read_byte_from_reg(REG_IRQ_FLAGS) & 0x20);
        handle->get_rssi_pkt();
        handle->get_snr();
        handle->reset_irq_flags();

        if (handle->rx_data_.size > 0) {
            rxData *temp = (rxData *)malloc(sizeof(rx_data_));

            if (!temp)
                return;

            memcpy(temp, &handle->rx_data_, sizeof(rx_data_));
            //rx_f(temp);
            pthread_create(&handle->receiver_thread, nullptr, rx_f, temp);
            pthread_detach(handle->receiver_thread);
        }
    }
}

void sx1278_module::set_rx_cont_mode() {
    write_byte_to_reg(REG_OP_MODE, (read_byte_from_reg(REG_OP_MODE) & 0xf8) | RXCONT_MODE );
}

void sx1278_module::get_rssi_pkt() {
    rx_data_.RSSI = read_byte_from_reg(REG_PKT_RSSI_VALUE) - (static_cast<double>(op_config_.freq) < 779E6 ? 164 : 157);
}

void sx1278_module::get_snr() {
    rx_data_.SNR = (read_byte_from_reg(REG_PKT_SNR_VALUE))*0.25;
}

void sx1278_module::reset_irq_flags() {
    write_byte_to_reg(REG_IRQ_FLAGS, 0xff);
}

void * sx1278_module::rx_f(void *p){
    rxData *rx = (rxData *)p;
    printf("rx done \n");
    printf("CRC error: %d\n", rx->CRC);
    printf("string: %s\n", rx->buf);//Data we'v received
    printf("RSSI: %d\n", rx->RSSI);
    printf("SNR: %f\n", rx->SNR);
    free(p);
    return NULL;
}

void test_static(int gpio, int level, uint32_t tick, void* userdata) {
    printf("test 0\n");
}


