
// save: 21.09.2014 18:45


#include <iostream>

#include <cstdlib>
#include <cstdio>

#include "ClientClass.h"
#include "ServerClientInfo.h"

int main(int argc, char**argv) {
    // Prints welcome message...
    std::cout << "Welcome ..." << std::endl;

    // Prints arguments...
    /*
    if (argc > 1) {
        std::cout << std::endl << "Arguments:" << std::endl;
        for (int i = 1; i < argc; i++) {
            std::cout << i << ": " << argv[i] << std::endl;
        }
    }
    */

    /*
    int size_int = sizeof(int);
    int size_double = sizeof(double);
    int size_timespec = sizeof(timespec);
    
    int size_paket_header = sizeof (paket_header);
    
    paket_header ph;
    
    int size_count_pakets_in_train = sizeof (ph.count_pakets_in_train);
    int size_recv_time = sizeof (ph.recv_time);
    
    long int longint;
    
    int size_longint = sizeof(longint);

    struct timespec ts;
    int size_tstv_sec = sizeof(ts.tv_sec);
    int size_tstv_nsec = sizeof(ts.tv_nsec);
    */
    
    ClientClass *c = new ClientClass();

    printf("\nmain EXIT_SUCCESS\n");
    return EXIT_SUCCESS;
}
