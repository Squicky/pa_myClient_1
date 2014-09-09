/* 
 * File:   ClientBenchmarkClass.cpp
 * Author: user
 * 
 * Created on 18. August 2014, 18:10
 */

#include "ClientBenchmarkClass.h"
#include "ServerClientInfo.h"
#include "ListArrayClass.h"

#include <errno.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

ClientBenchmarkClass::ClientBenchmarkClass(char * _server_ip, int _server_rec_port, int _mess_paket_size, char _zeit_dateiname[]) {

    stop = false;
    mess_paket_size = _mess_paket_size;

    zeit_dateiname[0] = 0;
    strncat(zeit_dateiname, "/home/user/pa_log_data/", sizeof (zeit_dateiname));
    strncat(zeit_dateiname, _zeit_dateiname, sizeof (zeit_dateiname));
    //    memcpy(zeit_dateiname, _zeit_dateiname, sizeof (zeit_dateiname));

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

    //    meineAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (6 < strlen(CLIENT_IP) && strlen(CLIENT_IP) < 16) {
        meineAddr.sin_addr.s_addr = inet_addr(CLIENT_IP);
    } else {
        meineAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    printf("UDP Mess-Socket (UMS): IP: %s   UDP Port: %d \n", inet_ntoa(meineAddr.sin_addr), meineAddr.sin_port);

    // serverAddr konfigurieren: IPv4, Port, Empfaenger IP
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(server_rec_port);
    //    remote_Server_Addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // IP Adresse in int auflösen:
    if (inet_aton(_server_ip, &serverAddr.sin_addr) == 0) {
        printf("ERROR:\n  Server IP %s kann nicht aufgelöst werden:\n  (%s)\n", _server_ip, strerror(errno));
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
            printf("IP %s und Port %d an UDP Mess-Socket (UMS %d) gebunden :-) \n", inet_ntoa(meineAddr.sin_addr), udp_rec_port, udp_rec_port);
            break;
        }
    }

    // Wenn Port an Socket "bind" erfolgreich, dann Empfang (rec) Socket starten
    if (rc < 0) {
        printf("ERROR:\n  Es konnte kein Port an UDP Mess-Socket (UMS) gebunden werden \n");
        exit(EXIT_FAILURE);
    } else {
        int thread = pthread_create(&rec_thread, NULL, &rec_threadStart, this);

        if (thread != 0) {
            printf("ERROR:\n  Kann pthread nicht erstellen: \n(%s)\n", strerror(errno));
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
    }
}

/*
ClientBenchmarkClass::ClientBenchmarkClass(const ClientBenchmarkClass& orig) {
}

ClientBenchmarkClass::~ClientBenchmarkClass() {
}
 */

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
    //    uint count_array_paket_header = MAX_UMTS_DATA_RATE / mess_paket_size;
    //    uint last_index_in_array_paket_header = count_array_paket_header - 1;
    //    uint paket_header_size = sizeof (paket_header);
    //    uint array_paket_header_size = count_array_paket_header * paket_header_size;

    //    struct paket_header *array_paket_header_recv = (paket_header*) malloc(array_paket_header_size);
    //    struct paket_header *array_paket_header_send = (paket_header*) malloc(array_paket_header_size);

    char puffer[256];
    sprintf(puffer, "%s_cl_recv.csv", this->zeit_dateiname);
    ListArrayClass *lac_recv = new ListArrayClass(mess_paket_size, puffer);

    sprintf(puffer, "%s_cl_send.csv", this->zeit_dateiname);

    ListArrayClass *lac_send1 = new ListArrayClass(mess_paket_size, puffer);
    ListArrayClass *lac_send2 = new ListArrayClass(mess_paket_size);
    lac_send2->File_Deskriptor = lac_send1->File_Deskriptor;
    ListArrayClass *lac_send3 = lac_send1;

    /*    
        if (array_paket_header_recv == NULL || array_paket_header_send == NULL) {
            printf("ERROR:\n  Kein virtueller RAM mehr verfügbar \n");
            printf("  array_paket_header_recv: %p \n", array_paket_header_recv);
            printf("  array_paket_header_send: %p \n", array_paket_header_send);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
     */

    //    uint array_paket_header_recv_start = (uint) array_paket_header_recv;
    //    uint array_paket_header_recv_ende = array_paket_header_recv_start + array_paket_header_size - 1;

    int paket_header_size = sizeof (paket_header);
    int paket_size = paket_header_size + mess_paket_size;
    struct paket *arbeits_paket_recv = (struct paket *) malloc(paket_size);
    struct paket_header *arbeits_paket_header_recv = &(arbeits_paket_recv->header);
    struct paket *arbeits_paket_send = (struct paket *) malloc(paket_size);
    struct paket_header *arbeits_paket_header_send = &(arbeits_paket_send->header);

    if (arbeits_paket_recv == NULL || arbeits_paket_send == NULL) {
        printf("ERROR:\n  Kein virtueller RAM mehr verfügbar \n  (%s)\n", strerror(errno));
        printf("  arbeits_paket_recv: %p \n", arbeits_paket_recv);
        printf("  arbeits_paket_send: %p \n", arbeits_paket_send);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    // mein_paket->puffer mit dezimal "83" (binär "01010011") füllen
    memset((char *) &(arbeits_paket_send->puffer), 83, mess_paket_size);

    // Es soll nur 1/2 Sek gesendet werden
    int mess_paket_size_doppelt = 2 * mess_paket_size;

    // Timeout fuer recvfrom auf 1 Sek setzen     
    struct timeval timeout_time;
    timeout_time.tv_sec = 0; // Anzahl Sekunden
    timeout_time.tv_usec = 300000; // Anzahl Mikrosekunden : 1 Sek. = 1.000.000 Mikrosekunden

    /*
    //    int abc;
    //    timespec a,b;
    //    clock_gettime(CLOCK_REALTIME, &a);
    //    for (abc = 1; abc < 25000000; abc++) {
    //        timeout_time.tv_usec = abc % 1000000;
    if (setsockopt(server_mess_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout_time, sizeof (timeout_time))) {
        //            printf(" %d \n", abc);
        printf("ERROR:\n  Kann Timeout fuer UDP Mess-Socket (UMS) nicht setzen: \n(%s)\n", strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    //    }
    //    clock_gettime(CLOCK_REALTIME, &b);
    printf(" time: %f \n", timespec_diff_double(&a, &b));
     */


    //    struct paket_header meinPaket;
    //    arbeits_paket_header_send->token = -1;
    arbeits_paket_header_send->train_id = 0;
    arbeits_paket_header_send->paket_id = 0;
    arbeits_paket_header_send->train_send_countid = 0;
    arbeits_paket_header_send->recv_time.tv_nsec = 0;
    arbeits_paket_header_send->recv_time.tv_sec = 0;
    arbeits_paket_header_send->send_time.tv_nsec = 0;
    arbeits_paket_header_send->send_time.tv_sec = 0;
    arbeits_paket_header_send->recv_data_rate = 64000 / 8; // 64 kBit/Sek.
    arbeits_paket_header_send->count_pakets_in_train = arbeits_paket_header_send->recv_data_rate / mess_paket_size_doppelt; // es soll nur 500 ms gesendet werden

    arbeits_paket_header_send->recv_timeout_wait = -1;

    if (arbeits_paket_header_send->count_pakets_in_train < 2) {
        arbeits_paket_header_send->count_pakets_in_train = 2;
    }

    long countBytes;
    int i;
    printf("sende %d Pakete # train_id: %d # send_countid: %d\n", arbeits_paket_header_send->count_pakets_in_train, arbeits_paket_header_send->train_id, arbeits_paket_header_send->train_send_countid);
    for (i = 0; i < arbeits_paket_header_send->count_pakets_in_train; i++) {
        arbeits_paket_header_send->paket_id = i;
        clock_gettime(CLOCK_REALTIME, &(arbeits_paket_header_send->send_time));

        countBytes = sendto(server_mess_socket, arbeits_paket_send, mess_paket_size - 8 - 20 - 26, 0, (struct sockaddr*) &serverAddr, serverAddrSize);

        if (countBytes != mess_paket_size - 8 - 20 - 26) {
            printf("ERROR:\n  %ld Bytes gesendet (%s)\n", countBytes, strerror(errno));
        } else {
            lac_send3->copy_paket_header(arbeits_paket_header_send);
        }
    }

    int my_last_send_train_id = -1;
    int my_recv_train_send_countid = -1;
    int my_send_train_send_countid = -1;
    int my_max_recv_train_id = -1;
    int my_max_send_train_id = -3;
    int my_max_train_id = -2;
    int my_bytes_per_sek = 64;

    int set_timeout = 0;

    /* Daten in While Schleife empfangen */
    printf("UDP Mess-Socket (UMS) (%s:%d) wartet auf Daten ... \n", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port));
    while (stop == false) {

        countBytes = recvfrom(server_mess_socket, arbeits_paket_recv, paket_size, 0, (struct sockaddr *) &serverAddr, &serverAddrSize);

        clock_gettime(CLOCK_REALTIME, &(arbeits_paket_header_recv->recv_time));

        if (set_timeout == 0) {
            set_timeout = 1;
            if (setsockopt(server_mess_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout_time, sizeof (timeout_time))) {
                printf("ERROR:\n  Kann Timeout fuer UDP Mess-Socket (UMS) nicht setzen: \n(%s)\n", strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }
        }

        if (countBytes == -1) {
            printf("Timeout recvfrom:\n  %ld Bytes empfangen (%s)\n", countBytes, strerror(errno));
        } else if (countBytes != mess_paket_size - 8 - 20 - 26) {
            printf("ERROR:\n  %ld Bytes empfangen (%s)\n", countBytes, strerror(errno));
            fflush(stdout);
            exit(EXIT_FAILURE);
        }

        if (countBytes == -1) {
            //          sleep(1);
        } else {

            if (arbeits_paket_header_recv->train_id < my_max_send_train_id) {
                lac_recv->copy_paket_header(arbeits_paket_header_recv);
                continue;
            }


            if (my_max_recv_train_id < arbeits_paket_header_recv->train_id) {
                my_recv_train_send_countid = arbeits_paket_header_recv->train_send_countid;

                printf("RECV neu train # train_id %d  # countid: %d #  count pakete: %d\n", arbeits_paket_header_recv->train_id, arbeits_paket_header_recv->train_send_countid, arbeits_paket_header_recv->count_pakets_in_train);


            } else if (my_max_recv_train_id == arbeits_paket_header_recv->train_id) {
                if (my_recv_train_send_countid < arbeits_paket_header_recv->train_send_countid) {
                    my_recv_train_send_countid = arbeits_paket_header_recv->train_send_countid;

                    printf("RECV alt train # train_id %d  # countid: %d # count pakete: %d \n", arbeits_paket_header_recv->train_id, arbeits_paket_header_recv->train_send_countid, arbeits_paket_header_recv->count_pakets_in_train);

                }
            } else {

                printf("ERROR:\n  arbeits_paket_header_recv->train_id: %d \n", arbeits_paket_header_recv->train_id);
                fflush(stdout);
                exit(EXIT_FAILURE);

            }

            if (my_max_recv_train_id < arbeits_paket_header_recv->train_id) {
                my_max_recv_train_id = arbeits_paket_header_recv->train_id;

                if (my_max_train_id < my_max_recv_train_id) {
                    my_max_train_id = my_max_recv_train_id;
                    printf("my_max_train_id recv: %d \n", my_max_train_id);
                }

            }


            // Header des empfangenen Pakets in das Array sichern 

            lac_recv->copy_paket_header(arbeits_paket_header_recv);

        }

        // wenn leztes Paket vom Paket Train empfangen, dann Antwort Train senden
        if (
                (countBytes == -1)
                ||
                (
                my_max_recv_train_id == arbeits_paket_header_recv->train_id
                && my_recv_train_send_countid == arbeits_paket_header_recv->train_send_countid
                && arbeits_paket_header_recv->paket_id == (arbeits_paket_header_recv->count_pakets_in_train - 1)
                )
                //                arbeits_paket_header_recv->paket_id == (arbeits_paket_header_recv->count_pakets_in_train - 1))
                ) {

            arbeits_paket_header_send->count_pakets_in_train = arbeits_paket_header_recv->recv_data_rate / mess_paket_size_doppelt;

            if (lac_recv->last_index_of_paket_header_in_one_array < arbeits_paket_header_send->count_pakets_in_train) {
                arbeits_paket_header_send->count_pakets_in_train = lac_recv->last_index_of_paket_header_in_one_array;
            } else if (arbeits_paket_header_send->count_pakets_in_train < 2) {
                arbeits_paket_header_send->count_pakets_in_train = 2;
            }

            // berechne neue Empfangsrate
            double time_diff_recv;
            double count_all_bytes_recv;
            double bytes_per_sek_recv;
            
            // prüfe/kontrolliere sende Datenrate
            double time_diff_send;
            double count_all_bytes_send;
            double bytes_per_sek_send;
            long send_sleep_total = 0;
            long send_sleep_count = 0;

            if (1 < lac_recv->count_paket_headers) {

                time_diff_recv = timespec_diff_double(&lac_recv->first_paket_header->recv_time, &lac_recv->last_paket_header->recv_time);

                if (time_diff_recv <= 0) {
                    printf("ERROR:\n  time_diff <= 0 \n");
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                }

                count_all_bytes_recv = lac_recv->count_paket_headers * mess_paket_size;
                bytes_per_sek_recv = count_all_bytes_recv / time_diff_recv;
                my_bytes_per_sek = bytes_per_sek_recv;
            } else {
                my_bytes_per_sek = mess_paket_size * 6;
            }

            arbeits_paket_header_send->recv_data_rate = my_bytes_per_sek;

            if (countBytes == -1) {
                printf("L -1: count: %d # ", lac_recv->count_paket_headers);

            } else {
                printf("Last: count: %d # ", lac_recv->count_paket_headers);

            }
            printf("train id: %d # ", arbeits_paket_header_recv->train_id);
            printf("train send countid : %d # ", arbeits_paket_header_recv->train_send_countid);
            printf("paket id: %d # ", arbeits_paket_header_recv->paket_id);
            printf("count in t: %d # ", arbeits_paket_header_recv->count_pakets_in_train);
            if (lac_recv->count_paket_headers == arbeits_paket_header_recv->count_pakets_in_train) {
                printf("RECV 100.00 %% # ");
            } else {
                printf("recv %.4f %% # ", (double) ((double) lac_recv->count_paket_headers / (double) arbeits_paket_header_recv->count_pakets_in_train) * 100.0);
            }
            printf("time_diff: %.4f # ", time_diff_recv);
            if (bytes_per_sek_recv >= 1024 * 1024) {
                printf("data_rate: %.4f MB / Sek \n", bytes_per_sek_recv / (1024 * 1024));
            } else if (bytes_per_sek_recv >= 1024) {
                printf("data_rate: %.4f KB / Sek \n", bytes_per_sek_recv / (1024));
            } else {
                printf("data_rate: %.4f B / Sek \n", bytes_per_sek_recv);
            }

            int my_bits_per_sek = 8 * my_bytes_per_sek;
            if (30000000 < my_bits_per_sek) {
                i = 2;
            }

            arbeits_paket_header_send->train_id = my_max_recv_train_id + 1;

            if (my_last_send_train_id < arbeits_paket_header_send->train_id) {
                my_send_train_send_countid = 0;
            } else {
                my_send_train_send_countid++;
            }

            arbeits_paket_header_send->train_send_countid = my_send_train_send_countid;

            my_last_send_train_id = arbeits_paket_header_send->train_id;


            if (0 < lac_recv->count_paket_headers) {
                arbeits_paket_header_send->last_recv_train_id = lac_recv->last_paket_header->train_id;
                arbeits_paket_header_send->last_recv_train_send_countid = lac_recv->last_paket_header->train_send_countid;
                arbeits_paket_header_send->last_recv_paket_id = lac_recv->last_paket_header->paket_id;
            }

            printf("sende %d Pakete # train_id: %d # send_countid: %d\n", arbeits_paket_header_send->count_pakets_in_train, arbeits_paket_header_send->train_id, arbeits_paket_header_send->train_send_countid);
            fflush(stdout);

            timespec x_timespec;
            for (i = 0; i < arbeits_paket_header_send->count_pakets_in_train; i++) {
                arbeits_paket_header_send->paket_id = i;
                clock_gettime(CLOCK_REALTIME, &(arbeits_paket_header_send->send_time));

                countBytes = sendto(server_mess_socket, arbeits_paket_send, mess_paket_size - 8 - 20 - 26, 0, (struct sockaddr*) &serverAddr, serverAddrSize);

                if (countBytes != mess_paket_size - 8 - 20 - 26) {
                    printf("ERROR:\n  %ld Bytes gesendet (%s)\n", countBytes, strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                } else {
                    lac_send3->copy_paket_header(arbeits_paket_header_send);
                }

                if (i == (arbeits_paket_header_send->count_pakets_in_train - 1)) {
                    i++;
                    i--;
                } else {
                    // Wenn Paket Train über 0,5 Sekunden gesendet wird, dann Paket Train kürzen
                    x_timespec = timespec_diff_timespec(&lac_send3->first_paket_header->send_time, &arbeits_paket_header_send->send_time);
                    if (500000000 < x_timespec.tv_nsec) {
                        arbeits_paket_header_send->count_pakets_in_train = i + 2;
                    } else if (1 < i) {

                        // Paket max. doppelt so schnell senden, wie vom Empfänger der Recv gewünscht
                        // sonst sleep
                        count_all_bytes_send = i * mess_paket_size;
                        time_diff_send = (double) x_timespec.tv_nsec / 1000000000.0;
                        bytes_per_sek_send = count_all_bytes_recv / time_diff_send;
                        if ((2 * arbeits_paket_header_recv->recv_data_rate) < bytes_per_sek_send) {
                            double soll_send_time = count_all_bytes_send / (2 * arbeits_paket_header_recv->recv_data_rate);

                            double sleep_time = soll_send_time - time_diff_send;

                            int sleep_time_microsec = 1000000 * sleep_time;
                            usleep(sleep_time_microsec);
                            
                            send_sleep_total = send_sleep_total + sleep_time_microsec;
                            send_sleep_count++;
                        }

                    }
                }
            }
            
            if (arbeits_paket_header_send->train_id == 2) {
                i++;
                i--;
            }

            timespec *b = &(lac_send3->first_paket_header->send_time);
            timespec *c = &(lac_send3->last_paket_header->send_time);
            struct timespec a = timespec_diff_timespec(b, c);

            if (a.tv_sec == 0) {
                //                timeout_time.tv_sec = 0;
                timeout_time.tv_usec = 1000000 - (a.tv_nsec / 1000);

                if (setsockopt(server_mess_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout_time, sizeof (timeout_time))) {
                    printf("ERROR:\n  Kann Timeout fuer UDP Mess-Socket (UMS) nicht setzen: \n(%s)\n", strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                }

            }

            printf("gesendet %d Pakete # train_id: %d # send_countid: %d  #  sendTime: %.5f  # RecvTimeout. %.5f   \n", 
                    arbeits_paket_header_send->count_pakets_in_train, 
                    arbeits_paket_header_send->train_id, 
                    arbeits_paket_header_send->train_send_countid, 
                    (double) x_timespec.tv_nsec / 1000000000.0, 
                    (double) timeout_time.tv_usec / 1000000.0 );
            
            fflush(stdout);

            if (0 < lac_recv->count_paket_headers) {
                struct paket_header *x;

                if (lac_send1 == lac_send3) {
                    x = lac_send2->give_paket_header(lac_recv->first_paket_header->last_recv_train_id, lac_recv->first_paket_header->last_recv_train_send_countid, lac_recv->first_paket_header->last_recv_paket_id);
                    if (x == NULL) {
                        x = lac_send1->give_paket_header(lac_recv->first_paket_header->last_recv_train_id, lac_recv->first_paket_header->last_recv_train_send_countid, lac_recv->first_paket_header->last_recv_paket_id);
                    }
                    //                    lac_send2->save_to_file_and_clear();
                    lac_send2->clear();
                    lac_send3 = lac_send2;
                } else {
                    x = lac_send1->give_paket_header(lac_recv->first_paket_header->last_recv_train_id, lac_recv->first_paket_header->last_recv_train_send_countid, lac_recv->first_paket_header->last_recv_paket_id);
                    if (x == NULL) {
                        x = lac_send2->give_paket_header(lac_recv->first_paket_header->last_recv_train_id, lac_recv->first_paket_header->last_recv_train_send_countid, lac_recv->first_paket_header->last_recv_paket_id);
                    }
                    //                    lac_send1->save_to_file_and_clear();
                    lac_send1->clear();
                    lac_send3 = lac_send1;
                }

                if (x != NULL) {

                    double t = timespec_diff_double(&x->send_time, &lac_recv->first_paket_header->recv_time);

                    printf("rtt ist %f  \n", t);
                }
            }


            if (my_max_send_train_id < my_last_send_train_id) {
                my_max_send_train_id = my_last_send_train_id;

                if (my_max_train_id < my_max_send_train_id) {
                    my_max_train_id = my_max_send_train_id;
                    printf("my_max_train_id send: %d \n", my_max_train_id);
                }
            }

            //            lac_recv->save_to_file_and_clear();
            lac_recv->clear();

        }

    }

    delete (lac_recv);
    delete (arbeits_paket_recv);
    delete (arbeits_paket_header_recv);
    delete (arbeits_paket_send);
    delete (arbeits_paket_header_send);
}

/*
 * Berechnet aus zwei timespec de Zeitdifferenz
 * 
 * 1 Sek =         1.000 Millisekunden
 * 1 Sek =     1.000.000 Mikrosekunden 
 * 1 Sek = 1.000.000.000 Nanosekunden 
 */
timespec ClientBenchmarkClass::timespec_diff_timespec(timespec *start, timespec *end) {
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

double ClientBenchmarkClass::timespec_diff_double(timespec *start, timespec *end) {
    timespec temp = timespec_diff_timespec(start, end);

    double temp2 = temp.tv_nsec;
    double temp3 = 1000000000;
    temp2 = temp2 / temp3;
    temp3 = temp.tv_sec;

    return (temp2 + temp3);
}

