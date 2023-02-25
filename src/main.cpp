#include <ctime>
#include <iostream>
#include <iomanip>
#include <string>
#include "radio.hpp"
#include <chrono>
#include <thread>
#include "lockingqueue.hpp"
#include "tuntap.hpp"

using namespace std;

bool verbose = false;

void read_from_tun(TUNDevice *device, LockingQueue<vector<uint8_t>>& send_queue);
void write_to_tun(TUNDevice *device, LockingQueue<vector<uint8_t>>& write_queue);
void radio_transmit(Radio* radio, LockingQueue<vector<uint8_t>>& send_queue);
void radio_recieve(Radio* radio, LockingQueue<vector<uint8_t>>& write_queue);

int main(int argc, char** argv)
{
    bool radioNumber = 1;

    cout << argv[0] << endl;
    uint8_t address[4][6] = {"1Node", "2Node", "3Node", "4Node"};

    cout << "Which radio is this? Enter '0' or '1'. Defaults to '0' ";
    string input;
    getline(cin, input);
    radioNumber = input.length() > 0 && (uint8_t)input[0] == 49;

    Radio radio_tx(17, address[radioNumber * 2], address[!radioNumber * 2 + 1], verbose);
    Radio radio_rx(27, address[radioNumber * 2 + 1], address[!radioNumber * 2], verbose);
    TUNDevice device("tun0", mode::TUN, 2);
    radio_tx.setListening(false);
    radio_rx.setListening(true);

    LockingQueue<vector<uint8_t>> send_queue;
    LockingQueue<vector<uint8_t>> write_queue;

    thread read_thread(read_from_tun, &device, send_queue);
    thread write_thread(write_to_tun, &device, write_queue);
    thread rx_thread(radio_recieve, &radio_rx, write_queue);
    read_thread.detach();
    write_thread.detach();
    rx_thread.detach();

    radio_transmit(&radio_tx, send_queue);

    return 0;
}

void read_from_tun(TUNDevice* device, LockingQueue<vector<uint8_t>>& send_queue)
{
    unsigned char payload[258];

    while (true) {
        size_t bytes_read = device->read(&payload, 258);
        vector<uint8_t> data(payload, payload + bytes_read);

        if (data.size() > 0) {
            send_queue.push(data);
        }
    }
}

void write_to_tun(TUNDevice* device, LockingQueue<vector<uint8_t>>& write_queue)
{
    while (true) {
        std::vector<uint8_t> ip_packet;

        write_queue.waitAndPop(ip_packet);

        size_t bytes_written = device->write(ip_packet.data(), ip_packet.size());

        if (verbose) {
            cout << "ip_packet sent to tun0, bytes: " << dec << bytes_written << endl;
        }
    }
}

void radio_transmit(Radio* radio, LockingQueue<vector<uint8_t>>& send_queue)
{
    while (true) {
        std::vector<uint8_t> data;

        send_queue.waitAndPop(data);

        radio->transmit(data);
    }
}

void radio_recieve(Radio* radio, LockingQueue<vector<uint8_t>>& write_queue)
{
    while (true) {
        vector<uint8_t> ip_packet = radio->recieve();

        if (ip_packet.size() > 0) {
            if (verbose) {
                cout << "IP Packet: ";
                for (int i = 0; i < ip_packet.size(); i++) {
                    cout << setfill('0') << setw(2) << uppercase << hex << int(ip_packet[i]);
                }
                cout << endl;
            }
            write_queue.push(ip_packet);
        }
    }
}
