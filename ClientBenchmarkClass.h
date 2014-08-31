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

#define LOCAL_Mess_PORT 8000
#define MAX_PAKETSIZE 65535 // Max. UDP Paketgroeße (2^16) 
/* 
 * Prüfen: http://openbook.galileocomputing.de/linux_unix_programmierung/Kap11-016.htm#RxxKap11016040003951F04F100
 * Trotzdem hat auch ein UDP-Paket eine maximale Größe, spätestens ab der Hardware (Ethernet: 1500 Bytes).
 */



class ClientBenchmarkClass {
public:
    ClientBenchmarkClass(char * _server_ip, int _server_rec_port, int _paket_size);
/*    
    ClientBenchmarkClass(const ClientBenchmarkClass& orig);
    virtual ~ClientBenchmarkClass();
*/
    
    pthread_t rec_thread;
private:
    struct sockaddr_in serverAddr;
    struct sockaddr_in meineAddr;
    socklen_t serverAddrSize;
    int server_mess_socket;

    int server_rec_port;
    int udp_rec_port;

    static void * rec_threadStart(void * vdata);
    void rec_threadRun();

    bool stop;

    /*
     * Paketgröße = Paket Header (36 Bytes) + Dummy Daten
     */
    int mess_paket_size;

    timespec timespec_diff_timespec(timespec start, timespec end);
    double timespec_diff_double(timespec start, timespec end);
};

#endif	/* CLIENTBENCHMARKCLASS_H */

