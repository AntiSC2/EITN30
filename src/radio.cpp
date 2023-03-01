#include "radio.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>
#include <exception>
#include <algorithm>

Radio::Radio(int ce_pin, uint8_t tx_address[6], uint8_t rx_address[6], bool verbose) : m_radio()
{
    int csn_pin = (ce_pin == 27) * 10;

    if (!m_radio.begin(ce_pin, csn_pin)) {
        throw std::runtime_error("radio hardware not responding! ce_pin " + std::to_string(ce_pin));
    }

    m_radio.disableDynamicPayloads();
    m_radio.setPALevel(RF24_PA_LOW);
    m_radio.setDataRate(RF24_2MBPS);
    m_radio.setAutoAck(true);
    m_radio.setAutoAck(0, 0);
    m_radio.setCRCLength(RF24_CRC_8);

    if ((rx_address[0] == '1' && tx_address[0] == '3') || (rx_address[0] == '3' && tx_address[0] == '1')) {
        m_radio.setChannel(120);
    } else {
        m_radio.setChannel(100);
    }

    m_radio.openWritingPipe(tx_address);
    m_radio.openReadingPipe(1, rx_address);

    m_verbose = verbose;
}

void Radio::setListening(bool listen)
{
    if (listen) {
        m_radio.startListening();
    } else {
        m_radio.stopListening();
    }
}
