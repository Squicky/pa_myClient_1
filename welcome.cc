
// save: 21.09.2014 18:45


#include <iostream>

#include <cstdlib>
#include <cstdio>

#include "ClientClass.h"
#include "ServerClientInfo.h"
#include "ATCInfo.h"

int main(int argc, char**argv) {
    // Prints welcome message...
    std::cout << "Welcome ..." << std::endl;

    ATCInfo *a = new ATCInfo();

    int buffer_len = 1024;
    char buffer[buffer_len];
    timespec t1, t2;
    clock_gettime(CLOCK_REALTIME, &t1);
    
    a->get_Network_technology_currently_in_use(buffer, buffer_len);
    a->get_Available_technologies_on_current_network(buffer, buffer_len);
    a->get_Current_active_radio_access_technology(buffer, buffer_len);
    a->get_Current_service_domain(buffer, buffer_len);
    a->get_Network_technology_currently_in_use(buffer, buffer_len);
    a->get_Operational_status(buffer, buffer_len);
    a->get_Signal_Quality(buffer, buffer_len);
    a->get_WCDMA_Active_Set(buffer, buffer_len);
    a->get_WCDMA_Async_Neighbour(buffer, buffer_len);
    a->get_WCDMA_Sync_Neighbour(buffer, buffer_len);
    
    clock_gettime(CLOCK_REALTIME, &t2);
    
    double d = ClientBenchmarkClass::timespec_diff_double(&t1, &t2);


    return EXIT_SUCCESS;

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

}

timespec timespec_diff_timespec(timespec *start, timespec *end) {
    timespec temp;

    if (end->tv_nsec < start->tv_nsec) {
        temp.tv_sec = end->tv_sec - start->tv_sec - 1;
        temp.tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
    } else {
        temp.tv_sec = end->tv_sec - start->tv_sec;
        temp.tv_nsec = end->tv_nsec - start->tv_nsec;
    }
    return temp;
}

double timespec_diff_double(timespec *start, timespec *end) {
    timespec temp = timespec_diff_timespec(start, end);

    double temp2 = temp.tv_nsec;
    double temp3 = 1000000000;
    temp2 = temp2 / temp3;
    temp3 = temp.tv_sec;

    return (temp2 + temp3);
}