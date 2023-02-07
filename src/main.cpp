#include <ctime>       // time()
#include <iostream>    // cin, cout, endl
#include <string>      // string, getline()
#include <time.h>      // CLOCK_MONOTONIC_RAW, timespec, clock_gettime()
#include <RF24.h>      // RF24, RF24_PA_LOW, delay()
#include <chrono>

using namespace std;

RF24 radio(17, 0);
float payload = 0.0;

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

    radio.setPayloadSize(sizeof(payload)); // float datatype occupies 4 bytes

    radio.setPALevel(RF24_PA_MAX); // RF24_PA_MAX is default.

    radio.setDataRate(RF24_2MBPS);

    radio.setAutoAck(false);

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

    chrono::time_point<chrono::system_clock> start, end;

    unsigned int packets_sent = 0;
    unsigned int failure = 0; // keep track of failures
    start = chrono::system_clock::now();
    while (packets_sent < 30000) {
        bool report = radio.write(&payload, sizeof(float)); // transmit & save the report

        if (report) {
            // payload was delivered
            //cout << "Transmission successful! Time to transmit = ";
            //cout << timerEllapsed;                    // print the timer result
            //cout << " us. Sent: " << payload << endl; // print payload sent
            payload += 0.01;                          // increment float payload
            packets_sent += 1;
        }
        else {
            // payload was not delivered
            cout << "Transmission failed or timed out" << endl;
            failure++;
        }

        // to make this example readable in the terminal
        //delay(1000); // slow transmissions down by 1 second
    }
    end = chrono::system_clock::now();
    auto elapsed_ms = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    int total_payload_B = packets_sent * 32;
    auto speed_Bps = total_payload_B / 1000.0 * elapsed_ms;

    cout << "Speed: " << speed_Bps << "Bps" << endl;
    cout << packets_sent << " packets sent." << endl;
    cout << failure << " failures detected. Leaving TX role." << endl;
}

void slave()
{

    radio.startListening(); // put radio in RX mode

    unsigned int packets_received = 0;
    time_t startTimer = time(nullptr);       // start a timer
    while (time(nullptr) - startTimer < 6) { // use 6 second timeout
        uint8_t pipe;
        if (radio.available(&pipe)) {                        // is there a payload? get the pipe number that recieved it
            uint8_t bytes = radio.getPayloadSize();          // get the size of the payload
            radio.read(&payload, bytes);                     // fetch payload from FIFO
            cout << "Received " << (unsigned int)bytes;      // print the size of the payload
            cout << " bytes on pipe " << (unsigned int)pipe; // print the pipe number
            cout << ": " << payload << endl;                 // print the payload's value
            startTimer = time(nullptr);                      // reset timer
            packets_received += 1;        
        }
    }
    cout << packets_received << " packets received." << endl;
    cout << "Nothing received in 6 seconds. Leaving RX role." << endl;
    radio.stopListening();
}
