/* 
 * File:   ClientBenchmarkClass.cpp
 * Author: user
 * 
 * Created on 18. August 2014, 18:10
 */

#include "ClientBenchmarkClass.h"
#include "ServerClientInfo.h"

#include <errno.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>

ClientBenchmarkClass::ClientBenchmarkClass(char * _server_ip, int _server_rec_port, int _mess_paket_size) {
    stop = false;
    mess_paket_size = _mess_paket_size;

    if (sizeof (paket_header) < mess_paket_size && mess_paket_size < (sizeof (paket))) {

    } else {
        printf("ERROR:\n  paket_size muss zwischen %d und %d sein \n", sizeof (paket_header), (paket_puffer_size + sizeof (paket_header)));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    server_rec_port = _server_rec_port;

    udp_rec_port = LOCAL_Mess_PORT;

    server_mess_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_mess_socket < 0) {
        printf("ERROR:\n  Kann UDP Mess-Socket (UMS) für Server nicht oeffnen: \n(%s)\n", strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    } else {
        printf("UDP Mess-Socket (UMS) für Server erstellt :-) \n");
    }

    // meineAddr & clientAddr mit "0" füllen
    memset((char *) &serverAddr, 0, sizeof (serverAddr));
    memset((char *) &meineAddr, 0, sizeof (meineAddr));

    // meineAddr konfigurieren: IPv4, Port, jeder Absender
    meineAddr.sin_family = AF_INET;
    meineAddr.sin_port = htons(udp_rec_port);
    meineAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // serverAddr konfigurieren: IPv4, Port, Empfaenger IP
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server_rec_port);
    //    remote_Server_Addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // IP Adresse in int auflösen:
    if (inet_aton(_server_ip, &serverAddr.sin_addr) == 0) {
        printf("ERROR:\n  Server IP %s kann nicht aufgelöst werden:\n (%s)\n", _server_ip, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    } else {
        printf("Server IP %s wurde aufgelöst: %s \n", _server_ip, inet_ntoa(serverAddr.sin_addr));
    }

    long rc;

    int i;
    for (i = 0; i < 10; i++) {
        rc = bind(server_mess_socket, (struct sockaddr*) &meineAddr, sizeof (meineAddr));
        if (rc < 0) {
            printf("ERROR:\n  Port %d kann nicht an UDP Mess-Socket (UMS) gebunden werden:\n (%s)\n", udp_rec_port, strerror(errno));

            udp_rec_port++;
            meineAddr.sin_port = htons(udp_rec_port);
        } else {
            printf("Port %d an UDP Mess-Socket (UMS %d) gebunden :-) \n", udp_rec_port, udp_rec_port);

            i = udp_rec_port;
            break;
        }
    }

    // Wenn Port an Socket "bind" erfolgreich, dann Empfang (rec) Socket starten
    if (i == udp_rec_port) {
        int thread = pthread_create(&rec_thread, NULL, &rec_threadStart, this);

        if (thread != 0) {
            printf("ERROR:\n  Kann pthread nicht erstellen: \n(%s)\n", strerror(errno));
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
    } else {
        printf("ERROR:\n  Es konnte kein Port an UDP Mess-Socket (UMS) gebunden werden \n");
        //        exit(EXIT_FAILURE);
    }
}

ClientBenchmarkClass::ClientBenchmarkClass(const ClientBenchmarkClass& orig) {
}

ClientBenchmarkClass::~ClientBenchmarkClass() {
}

void * ClientBenchmarkClass::rec_threadStart(void * data) {
    ((ClientBenchmarkClass*) data)->rec_threadRun();
}

void ClientBenchmarkClass::rec_threadRun() {
    printf("Empfangs-pthread für UDP Mess-Socket (UMS) (%s:%d) gestartet \n", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port));

    serverAddrSize = sizeof (serverAddr);

    // Startwerte und init

    // Berechne Speichergröße für Array für Header eines ganzen Trains
    // bei max. UMTS Geschwindigkeit 
    // Da wir nur 500 ms lange senden wollen sollte das Array groß genug sein (fast doppelte Größe))
    int array_paket_header_size = max_umts_data_rate / mess_paket_size;
    int paket_header_size = sizeof (paket_header);
    array_paket_header_size = array_paket_header_size * paket_header_size;

    struct paket_header *array_paket_header_recv = (paket_header*) malloc(array_paket_header_size);
    struct paket_header *array_paket_header_send = (paket_header*) malloc(array_paket_header_size);

    int paket_size = sizeof (paket);
    struct paket *arbeits_paket = (struct paket *) malloc(paket_size);
    struct paket_header *arbeits_paket_header = &(arbeits_paket->header);

    if (array_paket_header_recv == NULL || array_paket_header_send == NULL || arbeits_paket == NULL) {
        printf("ERROR:\n  Kein virtueller RAM mehr verfügbar \n");
        printf("  array_paket_header_recv: %p \n", array_paket_header_recv);
        printf("  array_paket_header_send: %p \n", array_paket_header_send);
        printf("  mein_paket: %p \n", arbeits_paket);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    // mein_paket->puffer mit dezimal "83" (binär "01010011") füllen
    memset((char *) &(arbeits_paket->puffer), 83, sizeof (arbeits_paket->puffer));

    // Es soll nur 1/2 Sek gesendet werden
    int mess_paket_size_doppelt = 2 * mess_paket_size;

    //    struct paket_header meinPaket;
    arbeits_paket_header->token = -1;
    arbeits_paket_header->train_id = 0;
    arbeits_paket_header->paket_id = 0;
    arbeits_paket_header->recv_time.tv_nsec = 0;
    arbeits_paket_header->recv_time.tv_sec = 0;
    arbeits_paket_header->send_time.tv_nsec = 0;
    arbeits_paket_header->send_time.tv_sec = 0;
    arbeits_paket_header->recv_data_rate = 64000 / 8; // 64 kBit/Sek.
    arbeits_paket_header->count_pakets_in_train = arbeits_paket_header->recv_data_rate / mess_paket_size_doppelt; // es soll nur 500 ms gesendet werden


    long countBytes;
    int i;
    int index_paket = 0;

    if (arbeits_paket_header->token == -1) {

        arbeits_paket_header->token++;

        printf("sende %d Pakete # token: %d\n", arbeits_paket_header->count_pakets_in_train, arbeits_paket_header->token);

        for (i = 0; i < arbeits_paket_header->count_pakets_in_train; i++) {
            arbeits_paket_header->paket_id = i;
            clock_gettime(CLOCK_REALTIME, &(arbeits_paket_header->send_time));

            countBytes = sendto(server_mess_socket, arbeits_paket, mess_paket_size, 0, (struct sockaddr*) &serverAddr, serverAddrSize);
            usleep(2000);

            if (countBytes != mess_paket_size) {
                printf("ERROR:\n  %ld Bytes gesendet (%s)\n", countBytes, strerror(errno));
            }
        }

    }

    /* Daten in While Schleife empfangen */
    printf("UDP Mess-Socket (UMS) (%s:%d) wartet auf Daten ... \n", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port));
    int last_paket_id = -1;
    while (stop == false) {

        /*
        i++;
        snprintf(puffer, puffer_size, "ClientBenchmarkClass sendet :-) %d", i);
        recvBytes = sendto(server_mess_socket, puffer, strlen(puffer), 0, (struct sockaddr*) &serverAddr, serverAddrSize);
        if (recvBytes < 0) {
            printf("ERROR:\n  %ld Bytes gesendet (%s)\n", recvBytes, strerror(errno));
        }
        printf("Client (%s:%d) hat %ld Bytes von Server (%s:%d) empfangen\n", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port), recvBytes, inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
        printf("  Daten: |%s|\n", puffer);
         */


        countBytes = recvfrom(server_mess_socket, arbeits_paket, paket_size, 0, (struct sockaddr *) &serverAddr, &serverAddrSize);
        clock_gettime(CLOCK_REALTIME, &(arbeits_paket->header.recv_time));
        //        usleep(5000);
        if ((last_paket_id + 1) != arbeits_paket->header.paket_id) {
            printf("paket empfangen id: %d # ", arbeits_paket->header.paket_id);
            printf("index_paket: %d # ", index_paket);
            printf("count_pakets_in_train: %d # ", arbeits_paket->header.count_pakets_in_train);
            printf("countBytes: %ld # ", countBytes);
            printf("token: %d ", arbeits_paket->header.token);
            printf(" ############# \n");
        } else {
//            if ((index_paket % 100) == 0 || arbeits_paket_header->paket_id == (arbeits_paket_header->count_pakets_in_train - 1)) {
            if (arbeits_paket_header->paket_id == (arbeits_paket_header->count_pakets_in_train - 1)) {
                printf("paket empfangen id: %d # ", arbeits_paket->header.paket_id);
                printf("index_paket: %d # ", index_paket);
                printf("count_pakets_in_train: %d # ", arbeits_paket->header.count_pakets_in_train);
                printf("countBytes: %ld # ", countBytes);
                printf("token: %d ", arbeits_paket->header.token);
                printf(" \n");
            }
        }
        last_paket_id = arbeits_paket->header.paket_id;


        if (countBytes != mess_paket_size) {
            printf("ERROR:\n  %ld Bytes empfangen (%s)\n", countBytes, strerror(errno));
        }

        // Header des empfangenen Pakets in das Array sichern 
        memcpy(&(array_paket_header_recv[index_paket]), arbeits_paket_header, paket_header_size);

        // wenn leztes Paket vom Paket Train empfangen, dann Antwort Train senden
        if (arbeits_paket_header->paket_id == (arbeits_paket_header->count_pakets_in_train - 1)) {

            arbeits_paket_header->count_pakets_in_train = arbeits_paket_header->recv_data_rate / mess_paket_size_doppelt;

            if (52000 < arbeits_paket_header->count_pakets_in_train) {
                arbeits_paket_header->count_pakets_in_train = 52000;
            }

            // berechne neue Empfangsrate
            double time_diff = timespec_diff_double(array_paket_header_recv[0].recv_time, array_paket_header_recv[index_paket].recv_time);
            double count_all_bytes = index_paket * mess_paket_size;
            double bytes_per_sek = count_all_bytes / time_diff;
            arbeits_paket_header->recv_data_rate = bytes_per_sek;

            printf("Last index_paket: %d # ", index_paket);
            printf("count_all_bytes: %f # ", count_all_bytes);
            printf("time_diff: %f # ", time_diff);
            printf("my new data_rate: %f \n", bytes_per_sek);

            arbeits_paket_header->token++;

            printf("sende %d Pakete # token: %d\n", arbeits_paket_header->count_pakets_in_train, arbeits_paket_header->token);
            for (i = 0; i < arbeits_paket_header->count_pakets_in_train; i++) {
                arbeits_paket_header->paket_id = i;
                clock_gettime(CLOCK_REALTIME, &(arbeits_paket_header->send_time));

                countBytes = sendto(server_mess_socket, arbeits_paket, mess_paket_size, 0, (struct sockaddr*) &serverAddr, serverAddrSize);

                usleep(2000);

                if (countBytes != mess_paket_size) {
                    printf("ERROR:\n  %ld Bytes gesendet (%s)\n", countBytes, strerror(errno));
                }
            }

            index_paket = -1;
            last_paket_id = -1;
        }

        index_paket++;

        /*
        // rc = Anzahl empfangener Bytes
        recvBytes = recvfrom(server_mess_socket, puffer, strlen(puffer), 0, (struct sockaddr *) &serverAddr, &serverAddrSize);
        puffer[rc] = 0;
        printf("Client (%s:%d) hat %ld Bytes an Server (%s:%d) gesendet\n", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port), rc, inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
        printf("  Daten: |%s|\n", puffer);
         */
    }

    free(array_paket_header_recv);
    free(array_paket_header_send);
    free(arbeits_paket);
}

/*
 * Berechnet aus zwei timespec de Zeitdifferenz
 * 
 * 1 Sek =         1.000 Millisekunden
 * 1 Sek =     1.000.000 Mikrosekunden 
 * 1 Sek = 1.000.000.000 Nanosekunden 
 */
timespec ClientBenchmarkClass::timespec_diff_timespec(timespec start, timespec end) {
    timespec temp;

    if (end.tv_nsec < start.tv_nsec) {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

double ClientBenchmarkClass::timespec_diff_double(timespec start, timespec end) {
    timespec temp = timespec_diff_timespec(start, end);

    double temp2 = temp.tv_nsec;
    double temp3 = 1000000000;
    temp2 = temp2 / temp3;
    temp3 = temp.tv_sec;

    return (temp2 + temp3);
}