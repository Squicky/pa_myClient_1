/* 
 * File:   ClientBenchmarkClass.h
 * Author: user
 *
 * Created on 18. August 2014, 18:10
 * test
 */

#ifndef CLIENTBENCHMARKCLASS_H
#define	CLIENTBENCHMARKCLASS_H

#include <arpa/inet.h>
#include <stdbool.h>

#include "ClientBenchmarkClass.h"
#include "ATCInfo.h"

#define LOCAL_Mess_PORT 8000

class ClientBenchmarkClass {
public:
    ClientBenchmarkClass(char * _server_ip, int _server_rec_port, int _paket_size, char _zeit_dateiname[]);
    /*    
        ClientBenchmarkClass(const ClientBenchmarkClass& orig);
        virtual ~ClientBenchmarkClass();
     */

    pthread_t rec_thread;

    static timespec timespec_diff_timespec(timespec *start, timespec *end);
    static double timespec_diff_double(timespec *start, timespec *end);

private:
    struct sockaddr_in otherAddr;
    struct sockaddr_in meineAddr;
    socklen_t otherAddrSize;
    int other_mess_socket;

    uint other_ip;
    int other_port;

    int server_rec_port;
    int udp_rec_port;

    static void * rec_threadStart(void * vdata);
    void rec_threadRun();

    bool stop;
    timespec end_time;

    /*
     * Paketgröße = Paket Header (36 Bytes) + Dummy Daten
     */
    int mess_paket_size;

    char zeit_dateiname[256];
    
    ATCInfo *atcInfo;
};

#endif	/* CLIENTBENCHMARKCLASS_H */

