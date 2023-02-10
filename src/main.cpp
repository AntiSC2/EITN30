#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <iomanip>
#include <string>      // string, getline()
#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
#include <RF24.h>      // RF24, RF24_PA_LOW, delay()
#include <chrono>
#include "tuntap.hpp"

using namespace std;

RF24 radio(17, 0);

void setRole(); // prototype to set the node's role
void master();  // prototype of the TX node's behavior
void slave();   // prototype of the RX node's behavior

// custom defined timer for evaluating transmission time in microseconds
struct timespec startTimer, endTimer;
uint32_t getMicros(); // prototype to get ellapsed time in microseconds

int main(int argc, char** argv)
{

    // perform hardware check
    if (!radio.begin()) {
        cout << "radio hardware is not responding!!" << endl;
        return 0; // quit now
    }

    bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

    // print example's name
    cout << argv[0] << endl;

    // Let these addresses be used for the pair
    uint8_t address[2][6] = {"1Node", "2Node"};

    cout << "Which radio is this? Enter '0' or '1'. Defaults to '0' ";
    string input;
    getline(cin, input);
    radioNumber = input.length() > 0 && (uint8_t)input[0] == 49;

    radio.setPayloadSize(32); // float datatype occupies 4 bytes

    radio.setPALevel(RF24_PA_MAX); // RF24_PA_MAX is default.

    radio.setDataRate(RF24_2MBPS);

    radio.setAutoAck(false);

    // set channel above wifi spectrum
    radio.setChannel(105);

    // set the TX address of the RX node into the TX pipe
    radio.openWritingPipe(address[radioNumber]); // always uses pipe 0

    // set the RX address of the TX node into a RX pipe
    radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1

    setRole(); // calls master() or slave() based on user input
    return 0;
}

void setRole()
{
    string input = "";
    while (!input.length()) {
        cout << "*** PRESS 'T' to begin transmitting to the other node\n";
        cout << "*** PRESS 'R' to begin receiving from the other node\n";
        cout << "*** PRESS 'Q' to exit" << endl;
        getline(cin, input);
        if (input.length() >= 1) {
            if (input[0] == 'T' || input[0] == 't')
                master();
            else if (input[0] == 'R' || input[0] == 'r')
                slave();
            else if (input[0] == 'Q' || input[0] == 'q')
                break;
            else
                cout << input[0] << " is an invalid input. Please try again." << endl;
        }
        input = ""; // stay in the while loop
    }               // while
} // setRole()

void master()
{
    radio.stopListening(); // put radio in TX mode
    TUNDevice device("tun0", mode::TUN, 2);

    chrono::time_point<chrono::system_clock> start, end;

    unsigned int packets_sent = 0;
    unsigned int failure = 0; // keep track of failures
    start = chrono::system_clock::now();

    unsigned char payload[1024];

    while (packets_sent < 30000) {
        size_t bytes_read = device.read(&payload, 1024);
        size_t offset = 0;

        while (bytes_read > 0) {
            size_t amount_of_bytes = bytes_read;

            if (amount_of_bytes > 32) {
                amount_of_bytes = 32;
                bytes_read -= 32;
            } else {
                bytes_read = 0;
            }

            bool report = radio.write(&payload[offset], amount_of_bytes); // transmit & save the report
            if (report) {
                cout << "Transmission successful!" << endl;

                cout << "Sent: ";
                for (int i = 0; i < amount_of_bytes; i++) {
                    cout << setfill('0') << setw(2) << uppercase << hex
                         << int(payload[offset + i]);
                }
                cout << endl;
                packets_sent += 1;
            }
            else {
                // payload was not delivered
                cout << "Transmission failed or timed out" << endl;
                failure++;
            }

            offset += amount_of_bytes;
        }
    }
    end = chrono::system_clock::now();
    chrono::duration<double> elapsed_s = end - start;

    int total_payload_b = packets_sent * 32 * 8;
    auto speed_bps = total_payload_b / elapsed_s.count();

    cout << "Speed: " << speed_bps << " bps" << endl;
    cout << packets_sent << " packets sent." << endl;
    cout << failure << " failures detected. Leaving TX role." << endl;
}

void slave()
{

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
}
