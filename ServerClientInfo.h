/* 
 * File:   ServerClientInfo.h
 * Author: user
 *
 * Created on 19. August 2014, 10:40
 */

#ifndef SERVERCLIENTINFO_H
#define	SERVERCLIENTINFO_H

#include <time.h>


#define SERVER_IP "192.168.120.241"
#define CLIENT_IP "192.168.120.234"

//#define paket_puffer_size 2048

struct init_info_client_to_server {
    int paket_size;
};

struct init_info_server_to_client {
    int port;
};

struct paket_header {
//    int token;
    int train_id;
    int train_send_countid;
    int paket_id;
    int count_pakets_in_train;
    struct timespec recv_time;
    struct timespec send_time;
    int recv_data_rate; // Bytes per Sek
};

struct paket {
    struct paket_header header;
    char *puffer;
};


#endif	/* SERVERCLIENTINFO_H */

