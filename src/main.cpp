#include <RF24.h>
 
RF24 radio(17, 0); // pin numbers connected to the radio's CE and CSN pins (respectively)
 
int main()
{
    if (!radio.begin()) {
        printf("Radio hardware is not responding!\n");
	return 1;
    }
    printf("Is this real life?\n");
    return 0;
    // continue with program as normal ...
}
