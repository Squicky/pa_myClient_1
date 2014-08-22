/* 
 * File:   ServerClientInfo.h
 * Author: user
 *
 * Created on 19. August 2014, 10:40
 */

#ifndef SERVERCLIENTINFO_H
#define	SERVERCLIENTINFO_H

#include <time.h>

#define paket_puffer_size 2048

struct init_info_client_to_server {
    int paket_size;
};

struct init_info_server_to_client {
    int port;
};

struct paket_header {
    int token;
    int train_id;
    int paket_id;
    int train_send_countid;
    int count_pakets_in_train;
    struct timespec recv_time;
    struct timespec send_time;
    int recv_data_rate; // Bytes per Sek
};

struct paket {
    struct paket_header header;
    char puffer[paket_puffer_size];
};

/*
 * max. UMTS Datenrate: HSPA+ 42 MBits/Sek
 * 42 MBits/Sek = 42000000 Bits/Sek = 5250000 Bytes/Sek
 */
static int max_umts_data_rate = 15250000;

#endif	/* SERVERCLIENTINFO_H */

