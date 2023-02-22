#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <iomanip>
#include <string>      // string, getline()
#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
#include "radio.hpp"      // RF24, RF24_PA_LOW, delay()
#include <chrono>
#include <thread>
#include "tuntap.hpp"

using namespace std;

void setRole(Radio* radio); // prototype to set the node's role
void radio_transmit(Radio* radio);  // prototype of the TX node's behavior
void radio_recieve(Radio* radio);   // prototype of the RX node's behavior

// custom defined timer for evaluating transmission time in microseconds
struct timespec startTimer, endTimer;
uint32_t getMicros(); // prototype to get ellapsed time in microseconds

int main(int argc, char** argv)
{
    bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

    // print example's name
    cout << argv[0] << endl;

    // Let these addresses be used for the pair
    uint8_t address[4][6] = {"1Node", "2Node", "3Node", "4Node"};

    cout << "Which radio is this? Enter '0' or '1'. Defaults to '0' ";
    string input;
    getline(cin, input);
    radioNumber = input.length() > 0 && (uint8_t)input[0] == 49;

    Radio radio_tx(17, address[radioNumber * 2], address[!radioNumber * 2 + 1], true);
    Radio radio_rx(27, address[radioNumber * 2 + 1], address[!radioNumber * 2], true);

    //setRole(&radio_tx, &radio_rx); // calls master() or slave() based on user input

    thread rx_thread (radio_recieve, &radio_rx);
    rx_thread.detach();

    radio_transmit(&radio_tx);

    return 0;
}

/*
void setRole(Radio* radio_tx, Radio *radio_rx)
{
    string input = "";
    while (!input.length()) {
        cout << "*** PRESS 'T' to begin transmitting to the other node\n";
        cout << "*** PRESS 'R' to begin receiving from the other node\n";
        cout << "*** PRESS 'Q' to exit" << endl;
        getline(cin, input);
        if (input.length() >= 1) {
            if (input[0] == 'T' || input[0] == 't')
                master(radio);
            else if (input[0] == 'R' || input[0] == 'r')
                slave(radio);
            else if (input[0] == 'Q' || input[0] == 'q')
                break;
            else
                cout << input[0] << " is an invalid input. Please try again." << endl;
        }
        input = ""; // stay in the while loop
    }               // while
} // setRole()
*/
void radio_transmit(Radio* radio)
{
    TUNDevice device("tun0", mode::TUN, 2);

    chrono::time_point<chrono::system_clock> start, end;

    unsigned int packets_sent = 0;
    start = chrono::system_clock::now();

    unsigned char payload[1024];

    while (packets_sent < 30000) {
        size_t bytes_read = device.read(&payload, 1024);
        vector<uint8_t> data(payload, payload + bytes_read);

        radio->transmit(data);
        packets_sent += 1;
    }
    end = chrono::system_clock::now();
    chrono::duration<double> elapsed_s = end - start;

    int total_payload_b = packets_sent * 32 * 8;
    auto speed_bps = total_payload_b / elapsed_s.count();

    cout << "Speed: " << speed_bps << " bps" << endl;
    cout << packets_sent << " packets sent." << endl;
}

void radio_recieve(Radio* radio)
{
    while (true) {
        std::vector<uint8_t> ip_packet = radio->recieve();
        cout << "IP Packet: ";
        for (int i = 0; i < ip_packet.size(); i++) {
            cout << setfill('0') << setw(2) << uppercase << std::hex << int(ip_packet[i]);
        }
        cout << endl;
    }
/*
    radio.startListening(); // put radio in RX mode

    unsigned int packets_received = 0;
    time_t startTimer = time(nullptr);       // start a timer
    unsigned char payload[32];
    while (time(nullptr) - startTimer < 240) { // use 240 second timeout
        uint8_t pipe;
        if (radio.available(&pipe)) {
            uint8_t bytes = radio.getPayloadSize();
            radio.read(&payload, bytes);
            cout << "Received " << (unsigned int)bytes;
            cout << " bytes on pipe " << (unsigned int)pipe;
            cout << ": ";

            for (int i = 0; i < 32; i++) {
                    cout << setfill('0') << setw(2) << uppercase << hex
                         << int(payload[i]);
            }

            cout << endl;

            startTimer = time(nullptr);
            packets_received += 1;
        }
    }
    cout << packets_received << " packets received." << endl;
    cout << "Nothing received in 6 seconds. Leaving RX role." << endl;
    radio.stopListening();
*/
}
