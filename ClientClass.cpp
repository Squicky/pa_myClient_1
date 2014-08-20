/* 
 * File:   ClientClass.cpp
 * Author: user
 * 
 * Created on 17. August 2014, 14:56
 */

#include "ClientClass.h"
#include "ServerClientInfo.h"

#include <errno.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

ClientClass::ClientClass() {
    server_ip = (char*) SERVER;

    struct sockaddr_in serverAddr;
    struct sockaddr_in meineAddr;
    int server_socket;
    //    int remote_Server_Addr_Size = sizeof (remote_Server_Addr);

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        printf("ERROR:\n  Kann Server Socket nicht oeffnen: \n(%s)\n", strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    } else {
        printf("UDP Socket an Server erstellt :-) \n");
    }

    // meineAddr & remote_Server_Addr mit "0" füllen
    memset((char *) &serverAddr, 0, sizeof (serverAddr));
    memset((char *) &meineAddr, 0, sizeof (meineAddr));

    // meineAddr konfigurieren: IPv4, Port, jeder Absender
    meineAddr.sin_family = AF_INET;
    meineAddr.sin_port = htons(LOCAL_Control_SERVER_PORT);
    meineAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // serverAddr konfigurieren: IPv4, Port, Empfaenger IP
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Remote_Control_SERVER_PORT);
    //    remote_Server_Addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // IP Adresse in int auflösen:
    if (inet_aton(server_ip, &serverAddr.sin_addr) == 0) {
        printf("ERROR:\n  Server IP %s kann nicht aufgelöst werden:\n (%s)\n", server_ip, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    } else {
        printf("Server IP %s wurde aufgelöst: %s \n", server_ip, inet_ntoa(serverAddr.sin_addr));
    }

    long rc;


    rc = bind(server_socket, (struct sockaddr*) &meineAddr, sizeof (meineAddr));
    if (rc < 0) {
        printf("ERROR:\n  Port %d kann nicht an Control UDP Socket gebunden werden:\n (%s)\n", LOCAL_Control_SERVER_PORT, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    } else {
        printf("Port %d an Control UDP Socket gebunden :-) \n", LOCAL_Control_SERVER_PORT);
    }

    struct init_info_client_to_server info_c2s;
    info_c2s.paket_size = PAKETSIZE;

    socklen_t serverAddrSize = sizeof (serverAddr);

    rc = sendto(server_socket, &info_c2s, sizeof (info_c2s), 0, (struct sockaddr*) &serverAddr, serverAddrSize);
    //    buf[rc] = 0;
    if (rc < 0) {
        printf("ERROR:\n  %ld Bytes gesendet (%s)\n", rc, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    printf("Client (%s:%d) hat %ld Bytes an Server (%s:%d) gesendet\n", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port), rc, inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
    //    printf("  Daten: |%s|\n", buf);


    struct init_info_server_to_client info_s2c;

    rc = recvfrom(server_socket, &info_s2c, sizeof(info_s2c), 0, (struct sockaddr *) &serverAddr, &serverAddrSize);
    //    buf[rc] = 0;
    printf("Client (%s:%d) hat %ld Bytes von Server (%s:%d) empfangen\n", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port), rc, inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
    //    printf("  Daten: |%s|\n", buf);

    int server_mess_port = 0;

    // Empfangene Nachricht vergleichen & untersuchen
    if (rc != sizeof (info_s2c)) {

        printf("ERROR:\n  Verbindungs/Empfangs Probleme: Kein udp_rec_port");
        fflush(stdout);
        exit(EXIT_FAILURE);

    } else if (0 == info_s2c.port) {

        printf("ERROR:\n  Server Error: Kein udp_rec_port");
        fflush(stdout);
        exit(EXIT_FAILURE);

    } else if (0 < info_s2c.port) {

        server_mess_port = info_s2c.port;

    } else {

        printf("ERROR:\n  Unbekannte Server-Antwort \n ");
        fflush(stdout);
        exit(EXIT_FAILURE);

    }

    if (server_mess_port > 0) {
        printf("server_mess_port: %d \n", server_mess_port);

        clientBenchmarkClass = new ClientBenchmarkClass(server_ip, server_mess_port, PAKETSIZE);

        pthread_join(clientBenchmarkClass->rec_thread, NULL);
    }
}

ClientClass::ClientClass(const ClientClass& orig) {
}

ClientClass::~ClientClass() {
}

