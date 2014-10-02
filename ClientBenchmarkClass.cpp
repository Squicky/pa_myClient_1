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

#define BinClient true
#define LOG_TYPE 1

ClientBenchmarkClass::ClientBenchmarkClass(char * _server_ip, int _server_rec_port, int _mess_paket_size, char _zeit_dateiname[]) {

    stop = false;
    mess_paket_size = _mess_paket_size;

    zeit_dateiname[0] = 0;
    strncat(zeit_dateiname, "../../log_data/", sizeof (zeit_dateiname));
    strncat(zeit_dateiname, _zeit_dateiname, strlen(_zeit_dateiname));
    //    memcpy(zeit_dateiname, _zeit_dateiname, sizeof (zeit_dateiname));

    server_rec_port = _server_rec_port;

    udp_rec_port = LOCAL_Mess_PORT;

    other_mess_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (other_mess_socket < 0) {
        printf("ERROR:\n  Kann UDP Mess-Socket (UMS) für Server nicht oeffnen: \n(%s)\n", strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    } else {
        printf("UDP Mess-Socket (UMS) für Server erstellt :-) \n");
    }

    // meineAddr & clientAddr mit "0" füllen
    memset((char *) &otherAddr, 0, sizeof (otherAddr));
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

    printf("UDP Mess-Socket (UMS): IP: %s   UDP Port: %d \n", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port));

    // serverAddr konfigurieren: IPv4, Port, Empfaenger IP
    otherAddr.sin_family = AF_INET;
    otherAddr.sin_port = htons(server_rec_port);
    //    remote_Server_Addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // IP Adresse in int auflösen:
    if (inet_aton(_server_ip, &otherAddr.sin_addr) == 0) {
        printf("ERROR:\n  Server IP %s kann nicht aufgelöst werden:\n  (%s)\n", _server_ip, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    } else {
        printf("Server IP %s wurde aufgelöst: %s \n", _server_ip, inet_ntoa(otherAddr.sin_addr));
        other_ip = otherAddr.sin_addr.s_addr;
        other_port = otherAddr.sin_port;
    }

    long rc;

    int i;
    for (i = 0; i < 10; i++) {
        rc = bind(other_mess_socket, (struct sockaddr*) &meineAddr, sizeof (meineAddr));
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

    otherAddrSize = sizeof (otherAddr);
}

/*
ClientBenchmarkClass::ClientBenchmarkClass(const ClientBenchmarkClass& orig) {}
ClientBenchmarkClass::~ClientBenchmarkClass() {}
 */

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

void * ClientBenchmarkClass::rec_threadStart(void * data) {
    ((ClientBenchmarkClass*) data)->rec_threadRun();
}

void ClientBenchmarkClass::rec_threadRun() {
    printf("Empfangs-pthread für UDP Mess-Socket (UMS) (%s:%d) gestartet \n", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port));

    int log_type = LOG_TYPE;

    // Startwerte und init

    // Berechne Speichergroeße fuer Array fuer Header eines ganzen Trains
    // bei max. UMTS Geschwindigkeit
    // Da wir nur 500 ms lange senden wollen sollte das Array groß genug sein (fast doppelte Groe�e))
    //    uint count_array_paket_header = MAX_UMTS_DATA_RATE / mess_paket_size;
    //    uint last_index_in_array_paket_header = count_array_paket_header - 1;
    //    uint paket_header_size = sizeof (paket_header);
    //    uint array_paket_header_size = count_array_paket_header * paket_header_size;

    //    struct paket_header *array_paket_header_recv = (paket_header*) malloc(array_paket_header_size);
    //    struct paket_header *array_paket_header_send = (paket_header*) malloc(array_paket_header_size);

    char puffer[256];
#ifdef BinClient
    sprintf(puffer, "%s_cl_recv.b", this->zeit_dateiname);
    ListArrayClass *lac_recv = new ListArrayClass(mess_paket_size, puffer);

    sprintf(puffer, "%s_cl_send.b", this->zeit_dateiname);
#endif
#ifdef BinServer
    sprintf(puffer, "%s_sv_recv.b", this->zeit_dateiname);
    ListArrayClass *lac_recv = new ListArrayClass(mess_paket_size, puffer);

    sprintf(puffer, "%s_sv_send.b", this->zeit_dateiname);
#endif

    ListArrayClass *lac_send1 = new ListArrayClass(mess_paket_size, puffer);
    ListArrayClass *lac_send2 = new ListArrayClass(mess_paket_size);
    lac_send2->File_Deskriptor = lac_send1->File_Deskriptor;
    lac_send2->file_csv = lac_send1->file_csv;
    ListArrayClass *lac_send3 = lac_send1;

#ifdef BinServer
    if (lac_recv->log_file_ok && lac_send1->log_file_ok) {
        log_files_ok = true;
    } else {
        log_files_ok = false;
    }
#endif

    struct paket *arbeits_paket_recv = (struct paket *) malloc(mess_paket_size);
    struct paket_header *arbeits_paket_header_recv = &(arbeits_paket_recv->header);
    struct paket *arbeits_paket_send = (struct paket *) malloc(mess_paket_size);
    struct paket_header *arbeits_paket_header_send = &(arbeits_paket_send->header);

    if (arbeits_paket_recv == NULL || arbeits_paket_send == NULL) {
        printf("ERROR:\n  Kein virtueller RAM mehr verfuegbar \n  (%s)\n", strerror(errno));
        printf("  arbeits_paket_recv: %p \n", arbeits_paket_recv);
        printf("  arbeits_paket_send: %p \n", arbeits_paket_send);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    // puffer mit dezimal "83" (binaer "01010011") fuellen :-)
    memset((char *) &(arbeits_paket_send->puffer), 83, mess_paket_size - sizeof (arbeits_paket_send->header));

    // Es soll nur 1/2 Sek gesendet werden
    int mess_paket_size_doppelt = 2 * mess_paket_size;

    // Timeout fuer recvfrom auf 1 Sek setzen     
    struct timeval timeout_time;
    timeout_time.tv_sec = 0; // Anzahl Sekunden
    timeout_time.tv_usec = 300000; // Anzahl Mikrosekunden : 1 Sek. = 1.000.000 Mikrosekunden

    arbeits_paket_header_send->train_id = 0;
    arbeits_paket_header_send->paket_id = 0;
    arbeits_paket_header_send->retransfer_train_id = 0;
    arbeits_paket_header_send->recv_time.tv_nsec = 0;
    arbeits_paket_header_send->recv_time.tv_sec = 0;
    arbeits_paket_header_send->send_time.tv_nsec = 0;
    arbeits_paket_header_send->send_time.tv_sec = 0;
    arbeits_paket_header_send->timeout_time_tv_sec = -1;
    arbeits_paket_header_send->timeout_time_tv_usec = -1;
    arbeits_paket_header_recv->timeout_time_tv_sec = -1;
    arbeits_paket_header_recv->timeout_time_tv_usec = -1;
    arbeits_paket_header_send->last_recv_paket_bytes = -2;
    arbeits_paket_header_recv->last_recv_paket_bytes = -2;
    arbeits_paket_header_send->rtt = -2;
    arbeits_paket_header_recv->rtt = -2;
    arbeits_paket_header_send->recv_data_rate = START_RECV_DATA_RATE / 8;
    arbeits_paket_header_recv->recv_data_rate = START_RECV_DATA_RATE / 8;
    arbeits_paket_header_send->mess_paket_size = mess_paket_size;
    arbeits_paket_header_recv->mess_paket_size = mess_paket_size;

    bool bremsen_datarate = true;
    bool bremsen_send = true;

    timespec *first_paket_train_send_time;
    timespec *last_paket_train_send_time;

    int my_last_send_train_id = -1;
    int my_recv_retransfer_train_id = -1;
    int my_send_retransfer_train_id = -1;
    int my_max_recv_train_id = -1;
    int my_max_send_train_id = -3;
    int my_max_train_id = -2;
    int my_bytes_per_sek = START_RECV_DATA_RATE / 8;

    // pruefe/kontrolliere sende Datenrate
    double time_diff_send;
    double count_all_bytes_send;
    double ist_train_bytes_per_sek_send;
    double max_send_faktor = 1.175;
    double max_send_faktor_mal_recv_data_rate;
    long send_sleep_total = 0;
    long send_sleep_count = 0;
    timespec train_sending_time;

    long countBytes;
    int i;
    int set_timeout = 0;
    uint count_recv_Timeout = 0;
    long last_sec_recv = 0;
    int log_zeile = 14;

#ifdef BinServer
    in_while_schleife = true;
#endif

#ifdef BinClient
    arbeits_paket_header_send->count_pakets_in_train = arbeits_paket_header_send->recv_data_rate / mess_paket_size_doppelt; // es soll nur 500 ms gesendet werden
    if (arbeits_paket_header_send->count_pakets_in_train < 5) {
        arbeits_paket_header_send->count_pakets_in_train = 5;
    }

    printf("START Messung: Client (%s:%d)  ", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port));
    printf("<----> Server (%s:%d)", inet_ntoa(otherAddr.sin_addr), ntohs(otherAddr.sin_port));
    
    printf("sende %d Pakete # train_id: %d # retransfer_train_id: %d\n", arbeits_paket_header_send->count_pakets_in_train, arbeits_paket_header_send->train_id, arbeits_paket_header_send->retransfer_train_id);
    for (i = 0; i < arbeits_paket_header_send->count_pakets_in_train; i++) {
        arbeits_paket_header_send->paket_id = i;
        clock_gettime(CLOCK_REALTIME, &(arbeits_paket_header_send->send_time));

        countBytes = sendto(other_mess_socket, arbeits_paket_send, mess_paket_size - HEADER_SIZES, 0, (struct sockaddr*) &otherAddr, otherAddrSize);

        if (countBytes != mess_paket_size - HEADER_SIZES) {
            printf("ERROR:\n  %ld Bytes gesendet (%s)\n", countBytes, strerror(errno));
        } else {
            lac_send3->copy_paket_header(arbeits_paket_header_send);
        }

        if (i == 0) {
            first_paket_train_send_time = &(lac_send3->last_paket_header->send_time);
        } else if (i == (arbeits_paket_header_send->count_pakets_in_train - 1)) {
            last_paket_train_send_time = &(lac_send3->last_paket_header->send_time);
        } else {
            // Wenn Paket Train ueber 0,5 Sekunden gesendet wird, dann Paket Train kuerzen
            train_sending_time = timespec_diff_timespec(first_paket_train_send_time, &(arbeits_paket_header_send->send_time));
            if (500000000 < train_sending_time.tv_nsec || 0 < train_sending_time.tv_sec) {
                if (4 < i) {
                    arbeits_paket_header_send->count_pakets_in_train = i + 2;
                }
            }
            if (1 < i) {

                if (bremsen_datarate) {
                    // Paket max. fak-mal so schnell senden, wie vom Empfaenger der Recv gewuenscht
                    // sonst sleep
                    count_all_bytes_send = i * mess_paket_size;
                    time_diff_send = (double) train_sending_time.tv_nsec / 1000000000.0;
                    time_diff_send = time_diff_send + train_sending_time.tv_sec;
                    ist_train_bytes_per_sek_send = count_all_bytes_send / time_diff_send;

                    max_send_faktor_mal_recv_data_rate = max_send_faktor * arbeits_paket_header_recv->recv_data_rate;
                    if (max_send_faktor_mal_recv_data_rate < ist_train_bytes_per_sek_send) {

                        double soll_send_time = count_all_bytes_send / max_send_faktor_mal_recv_data_rate;

                        double sleep_time = soll_send_time - time_diff_send;

                        int sleep_time_microsec = 1000000 * sleep_time;

                        if (sleep_time_microsec < 0 || 1000000 < sleep_time_microsec) {
                            //sleep_time_microsec++;
                            //sleep_time_microsec--;
                        } else {
                            usleep(sleep_time_microsec);
                        }

                        send_sleep_total = send_sleep_total + sleep_time_microsec;
                        send_sleep_count++;
                    }
                }
            }
        }
    }
#endif  


    /* Daten in While Schleife empfangen */
    printf("UDP Mess-Socket (UMS) (%s:%d) wartet auf Daten ... \n", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port));
    while (stop == false) {

        countBytes = recvfrom(other_mess_socket, arbeits_paket_recv, mess_paket_size - HEADER_SIZES, 0, (struct sockaddr *) &otherAddr, &otherAddrSize);

        clock_gettime(CLOCK_REALTIME, &(arbeits_paket_header_recv->recv_time));

        if (set_timeout == 0) {
            set_timeout = 1;

#ifdef BinClient
            printf("ERSTER Empfang: Client (%s:%d)  ", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port));
            printf("<----> Server (%s:%d)", inet_ntoa(otherAddr.sin_addr), ntohs(otherAddr.sin_port));
#endif

            arbeits_paket_header_send->timeout_time_tv_sec = timeout_time.tv_sec;
            arbeits_paket_header_send->timeout_time_tv_usec = timeout_time.tv_usec;

            if (setsockopt(other_mess_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout_time, sizeof (timeout_time))) {
                printf("ERROR:\n  Kann Timeout fuer UDP Mess-Socket (UMS) nicht setzen: \n(%s)\n", strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }
        }

        if (countBytes == -1) {
            count_recv_Timeout++;

            if (log_type == 1) {
                printf("\033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  ",
                        log_zeile, log_zeile + 1, log_zeile + 2, log_zeile + 3, log_zeile + 4, log_zeile + 5, log_zeile + 6, log_zeile + 7, log_zeile + 8);
                printf("\033[%d;0H# ", log_zeile);
            }
            if (log_type == 1 || log_type == 2) {

                printf("Timeout recvfrom:  %ld Bytes empfangen (%u)       ", countBytes, count_recv_Timeout);

                if (log_type == 1) {
                    printf("\033[%d;0H", log_zeile + 8);
                    fflush(stdout);
                } else if (log_type == 2) {
                    printf("\n");
                    fflush(stdout);
                }
            }

        } else if (countBytes != mess_paket_size - HEADER_SIZES) {
            printf("ERROR:\n  %ld Bytes empfangen (%s)\n", countBytes, strerror(errno));
            fflush(stdout);
            exit(EXIT_FAILURE);
        }

        if (countBytes == -1) {

            if (last_sec_recv != 0) {
                int sek = (arbeits_paket_header_recv->recv_time.tv_sec - last_sec_recv);
                if (120 < sek) {
                    stop = true;
                    printf("ERROR:\n  %d Sekunden nichts empfangen \n", sek);
                    fflush(stdout);
                    continue;
                }
            }

        } else {

            last_sec_recv = arbeits_paket_header_recv->recv_time.tv_sec;

            arbeits_paket_header_recv->timeout_time_tv_sec = timeout_time.tv_sec;
            arbeits_paket_header_recv->timeout_time_tv_usec = timeout_time.tv_usec;

            if (arbeits_paket_header_recv->train_id < my_max_send_train_id) {
                lac_recv->copy_paket_header(arbeits_paket_header_recv);
                continue;
            }

            if (my_max_recv_train_id < arbeits_paket_header_recv->train_id) {
                my_recv_retransfer_train_id = arbeits_paket_header_recv->retransfer_train_id;

                if (log_type == 1) {
                    printf("\033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  ",
                            log_zeile, log_zeile + 1, log_zeile + 2, log_zeile + 3, log_zeile + 4, log_zeile + 5, log_zeile + 6, log_zeile + 7, log_zeile + 8);
                    printf("\033[%d;0H# ", log_zeile + 1);
                }
                if (log_type == 1 || log_type == 2) {
                    printf("RECV neu train # train_id %d  # countid: %d #  count pakete: %d       ", arbeits_paket_header_recv->train_id, arbeits_paket_header_recv->retransfer_train_id, arbeits_paket_header_recv->count_pakets_in_train);

                    if (log_type == 1) {
                        printf("\033[%d;0H", log_zeile + 8);
                        fflush(stdout);
                    } else if (log_type == 2) {
                        printf("\n");
                        fflush(stdout);
                    }
                }

            } else if (my_max_recv_train_id == arbeits_paket_header_recv->train_id) {
                if (my_recv_retransfer_train_id < arbeits_paket_header_recv->retransfer_train_id) {
                    my_recv_retransfer_train_id = arbeits_paket_header_recv->retransfer_train_id;

                    if (log_type == 1) {
                        printf("\033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  ",
                                log_zeile, log_zeile + 1, log_zeile + 2, log_zeile + 3, log_zeile + 4, log_zeile + 5, log_zeile + 6, log_zeile + 7, log_zeile + 8);
                        printf("\033[%d;0H# ", log_zeile + 2);
                    }
                    if (log_type == 1 || log_type == 2) {

                        printf("RECV ALT train # train_id %d  # countid: %d # count pakete: %d        ", arbeits_paket_header_recv->train_id, arbeits_paket_header_recv->retransfer_train_id, arbeits_paket_header_recv->count_pakets_in_train);

                        if (log_type == 1) {
                            printf("\033[%d;0H", log_zeile + 8);
                            fflush(stdout);
                        } else if (log_type == 2) {
                            printf("\n");
                            fflush(stdout);
                        }
                    }
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

                    if (log_type == 1) {
                        printf("\033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  ",
                                log_zeile, log_zeile + 1, log_zeile + 2, log_zeile + 3, log_zeile + 4, log_zeile + 5, log_zeile + 6, log_zeile + 7, log_zeile + 8);
                        printf("\033[%d;0H# ", log_zeile + 3);
                    }
                    if (log_type == 1 || log_type == 2) {

                        printf("my_max_train_id recv: %d        ", my_max_train_id);

                        if (log_type == 1) {
                            printf("\033[%d;0H", log_zeile + 8);
                            fflush(stdout);
                        } else if (log_type == 2) {
                            printf("\n");
                            fflush(stdout);
                        }
                    }
                }

            }

            // Header des empfangenen Pakets in das Array sichern 
            lac_recv->copy_paket_header(arbeits_paket_header_recv);

        }

        // wenn leztes Paket (oder Recv-Timeout) vom Paket Train empfangen, dann Antwort Train senden
        if (
                (countBytes == -1)
                ||
                (
                my_max_recv_train_id == arbeits_paket_header_recv->train_id
                && my_recv_retransfer_train_id == arbeits_paket_header_recv->retransfer_train_id
                && arbeits_paket_header_recv->paket_id == (arbeits_paket_header_recv->count_pakets_in_train - 1)
                )
                ) {

            arbeits_paket_header_send->last_recv_paket_bytes = countBytes;

            arbeits_paket_header_send->count_pakets_in_train = arbeits_paket_header_recv->recv_data_rate / mess_paket_size_doppelt;

            if (lac_recv->last_index_of_paket_header_in_one_array < arbeits_paket_header_send->count_pakets_in_train) {
                arbeits_paket_header_send->count_pakets_in_train = lac_recv->last_index_of_paket_header_in_one_array;
            } else if (arbeits_paket_header_send->count_pakets_in_train < 5) {
                arbeits_paket_header_send->count_pakets_in_train = 5;
            }

            // berechne neue Empfangsrate
            double time_diff_recv;
            double count_all_bytes_recv;
            double bytes_per_sek_recv;

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


            // Datenrate bremsen :-)
            if (bremsen_datarate) {
                if (10000000 < (my_bytes_per_sek * 8)) {
                    my_bytes_per_sek = 10000000 / 8;
                }
            }

            arbeits_paket_header_send->recv_data_rate = my_bytes_per_sek;

            if (log_type == 1) {
                printf("\033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  ",
                        log_zeile, log_zeile + 1, log_zeile + 2, log_zeile + 3, log_zeile + 4, log_zeile + 5, log_zeile + 6, log_zeile + 7, log_zeile + 8);
                printf("\033[%d;0H# ", log_zeile + 4);
            }

            if (log_type == 1 || log_type == 2) {
                if (countBytes == -1) {
                    printf("L -1: count: %d # ", lac_recv->count_paket_headers);
                } else {
                    printf("Last: count: %d # ", lac_recv->count_paket_headers);
                }
                printf("train id: %d # ", arbeits_paket_header_recv->train_id);
                printf("train send countid : %d # ", arbeits_paket_header_recv->retransfer_train_id);
                printf("paket id: %d # ", arbeits_paket_header_recv->paket_id);
                printf("count in t: %d # ", arbeits_paket_header_recv->count_pakets_in_train);
                if (lac_recv->count_paket_headers == arbeits_paket_header_recv->count_pakets_in_train) {
                    printf("RECV 100.00 %% # ");
                } else {
                    printf("recv %.4f %% # ", (double) ((double) lac_recv->count_paket_headers / (double) arbeits_paket_header_recv->count_pakets_in_train) * 100.0);
                }
                printf("time_diff: %.4f # ", time_diff_recv);
                double mbits_per_sek_recv = bytes_per_sek_recv * 8;
                mbits_per_sek_recv = mbits_per_sek_recv / 1000000;
                printf("data_rate: %.4f # ", mbits_per_sek_recv);
                printf("%.4f MBits/Sek   ", (double) arbeits_paket_header_recv->recv_data_rate * 8 / 1000000);

                if (log_type == 1) {
                    printf("\033[%d;0H", log_zeile + 8);
                    fflush(stdout);
                } else if (log_type == 2) {
                    printf("\n");
                    fflush(stdout);
                }
            }

            arbeits_paket_header_send->train_id = my_max_recv_train_id + 1;

            if (my_last_send_train_id < arbeits_paket_header_send->train_id) {
                my_send_retransfer_train_id = 0;
            } else {
                my_send_retransfer_train_id++;
            }

            arbeits_paket_header_send->retransfer_train_id = my_send_retransfer_train_id;

            my_last_send_train_id = arbeits_paket_header_send->train_id;

            if (0 < lac_recv->count_paket_headers) {
                arbeits_paket_header_send->last_recv_train_id = lac_recv->last_paket_header->train_id;
                arbeits_paket_header_send->last_recv_retransfer_train_id = lac_recv->last_paket_header->retransfer_train_id;
                arbeits_paket_header_send->last_recv_paket_id = lac_recv->last_paket_header->paket_id;
            }

            if (log_type == 1) {
                printf("\033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  ",
                        log_zeile, log_zeile + 1, log_zeile + 2, log_zeile + 3, log_zeile + 4, log_zeile + 5, log_zeile + 6, log_zeile + 7, log_zeile + 8);
                printf("\033[%d;0H# ", log_zeile + 5);
            }
            if (log_type == 1 || log_type == 2) {

                printf("  sende  %d Pakete # train_id: %d # retransfer_train_id: %d                                       ### ", arbeits_paket_header_send->count_pakets_in_train, arbeits_paket_header_send->train_id, arbeits_paket_header_send->retransfer_train_id);

                if (log_type == 1) {
                    printf("\033[%d;0H", log_zeile + 8);
                    fflush(stdout);
                } else if (log_type == 2) {
                    printf("\n");
                    fflush(stdout);
                }
            }

            // rtt in ersten recv Packet eintraken
            {
                struct paket_header *x = NULL;
                if (0 < lac_recv->count_paket_headers) {
                    x = lac_send2->give_paket_header(lac_recv->first_paket_header->last_recv_train_id, lac_recv->first_paket_header->last_recv_retransfer_train_id, lac_recv->first_paket_header->last_recv_paket_id);
                    if (x == NULL) {
                        x = lac_send1->give_paket_header(lac_recv->first_paket_header->last_recv_train_id, lac_recv->first_paket_header->last_recv_retransfer_train_id, lac_recv->first_paket_header->last_recv_paket_id);
                    }
                }

                if (x != NULL) {
                    arbeits_paket_header_send->rtt = timespec_diff_double(&x->send_time, &lac_recv->first_paket_header->recv_time);
                } else {
                    arbeits_paket_header_send->rtt = -1;
                }
            }

            arbeits_paket_header_send->mess_paket_size = mess_paket_size;
            
            for (i = 0; i < arbeits_paket_header_send->count_pakets_in_train; i++) {

                arbeits_paket_header_send->paket_id = i;
                clock_gettime(CLOCK_REALTIME, &(arbeits_paket_header_send->send_time));

                countBytes = sendto(other_mess_socket, arbeits_paket_send, mess_paket_size - HEADER_SIZES, 0, (struct sockaddr*) &otherAddr, otherAddrSize);

                if (countBytes != mess_paket_size - HEADER_SIZES) {
                    printf("ERROR:\n  %ld Bytes gesendet (%s)\n", countBytes, strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                } else {
                    lac_send3->copy_paket_header(arbeits_paket_header_send);
                }

                if (i == 0) {
                    first_paket_train_send_time = &(lac_send3->last_paket_header->send_time);
                } else if (i == (arbeits_paket_header_send->count_pakets_in_train - 1)) {
                    last_paket_train_send_time = &(lac_send3->last_paket_header->send_time);
                } else {
                    // Wenn Paket Train ueber 0,5 Sekunden gesendet wird, dann Paket Train kuerzen
                    train_sending_time = timespec_diff_timespec(first_paket_train_send_time, &(arbeits_paket_header_send->send_time));
                    if (500000000 < train_sending_time.tv_nsec || 0 < train_sending_time.tv_sec) {
                        if (4 < i) {
                            arbeits_paket_header_send->count_pakets_in_train = i + 2;
                        }
                    } else if (1 < i) {

                        if (bremsen_send) {
                            // Paket max. fak-mal so schnell senden, wie vom Empfaenger der Recv gewuenscht
                            // sonst sleep
                            count_all_bytes_send = i * mess_paket_size;
                            time_diff_send = (double) train_sending_time.tv_nsec / 1000000000.0;
                            time_diff_send = time_diff_send + train_sending_time.tv_sec;
                            ist_train_bytes_per_sek_send = count_all_bytes_send / time_diff_send;

                            max_send_faktor_mal_recv_data_rate = max_send_faktor * arbeits_paket_header_recv->recv_data_rate;
                            if (max_send_faktor_mal_recv_data_rate < ist_train_bytes_per_sek_send) {

                                double soll_send_time = count_all_bytes_send / max_send_faktor_mal_recv_data_rate;

                                double sleep_time = soll_send_time - time_diff_send;

                                int sleep_time_microsec = 1000000 * sleep_time;

                                if (sleep_time_microsec < 0 || 1000000 < sleep_time_microsec) {
                                    //sleep_time_microsec++;
                                    //sleep_time_microsec--;
                                } else {
                                    usleep(sleep_time_microsec);
                                }

                                send_sleep_total = send_sleep_total + sleep_time_microsec;
                                send_sleep_count++;
                            }
                        }
                    }
                }
            }

            // Recv Timeout von 1 Sek berechnen
            // 1 Sek = Soll Zeit fuer letztes Paket vom nechsten recv. Train
            //            timespec *b = first_paket_train_time_send;
            train_sending_time = timespec_diff_timespec(first_paket_train_send_time, last_paket_train_send_time);
            if (train_sending_time.tv_sec == 0) {

                timeout_time.tv_sec = 0;
                timeout_time.tv_usec = 1000000 - (train_sending_time.tv_nsec / 1000);

                // Wenn keine Daten empfangen, dann Verdacht auch Train Lost
                // dann Recv Timeout verlaengern
                if (lac_recv->count_paket_headers == 0) {
                    if (1 < arbeits_paket_header_send->count_pakets_in_train) {
                        timeout_time.tv_usec = timeout_time.tv_usec * (1 + arbeits_paket_header_send->retransfer_train_id);
                        timeout_time.tv_sec = timeout_time.tv_usec / 1000000;
                        timeout_time.tv_usec = timeout_time.tv_usec % 1000000;

#ifdef BinServer
                        if (7 < timeout_time.tv_sec) {
                            timeout_time.tv_sec = 7;
                        }
#endif 
#ifdef BinClient
                        if (5 < timeout_time.tv_sec) {
                            timeout_time.tv_sec = 5;
                        }
#endif
                    }
                }

                arbeits_paket_header_send->timeout_time_tv_sec = timeout_time.tv_sec;
                arbeits_paket_header_send->timeout_time_tv_usec = timeout_time.tv_usec;

                if (setsockopt(other_mess_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout_time, sizeof (timeout_time))) {
                    printf("ERROR:\n  Kann Timeout fuer UDP Mess-Socket (UMS) nicht setzen: \n(%s)\n", strerror(errno));
                    fflush(stdout);
                    exit(EXIT_FAILURE);
                }
            }

            if (log_type == 1) {
                printf("\033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  ",
                        log_zeile, log_zeile + 1, log_zeile + 2, log_zeile + 3, log_zeile + 4, log_zeile + 5, log_zeile + 6, log_zeile + 7, log_zeile + 8);
                printf("\033[%d;0H# ", log_zeile + 5);
            }
            if (log_type == 1 || log_type == 2) {
                printf("gesendet %d Pakete # train_id: %d # send_count: %d # sendTime: %.5f # RecvTimeout. %ld,%.6ld # sleep: %ld | %ld          ",
                        arbeits_paket_header_send->count_pakets_in_train,
                        arbeits_paket_header_send->train_id,
                        arbeits_paket_header_send->retransfer_train_id,
                        (double) train_sending_time.tv_nsec / 1000000000.0,
                        timeout_time.tv_sec,
                        //                    (double) timeout_time.tv_usec / 1000000.0,
                        timeout_time.tv_usec,
                        send_sleep_count,
                        send_sleep_total);

                if (log_type == 1) {
                    printf("\033[%d;0H", log_zeile + 8);
                    fflush(stdout);
                } else if (log_type == 2) {
                    printf("\n");
                    fflush(stdout);
                }
            }


            if (0 < lac_recv->count_paket_headers) {
                struct paket_header *x;

                if (lac_send1 == lac_send3) {
                    x = lac_send2->give_paket_header(lac_recv->first_paket_header->last_recv_train_id, lac_recv->first_paket_header->last_recv_retransfer_train_id, lac_recv->first_paket_header->last_recv_paket_id);

                    if (x == NULL) {
                        x = lac_send1->give_paket_header(lac_recv->first_paket_header->last_recv_train_id, lac_recv->first_paket_header->last_recv_retransfer_train_id, lac_recv->first_paket_header->last_recv_paket_id);
                    }

                    lac_send2->save_to_file_and_clear();
                    //lac_send2->clear();
                    lac_send3 = lac_send2;
                } else {
                    x = lac_send1->give_paket_header(lac_recv->first_paket_header->last_recv_train_id, lac_recv->first_paket_header->last_recv_retransfer_train_id, lac_recv->first_paket_header->last_recv_paket_id);

                    if (x == NULL) {
                        x = lac_send2->give_paket_header(lac_recv->first_paket_header->last_recv_train_id, lac_recv->first_paket_header->last_recv_retransfer_train_id, lac_recv->first_paket_header->last_recv_paket_id);
                    }

                    lac_send1->save_to_file_and_clear();
                    //lac_send1->clear();
                    lac_send3 = lac_send1;
                }

                if (x != NULL) {

                    if (log_type == 1) {
                        printf("\033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  ",
                                log_zeile, log_zeile + 1, log_zeile + 2, log_zeile + 3, log_zeile + 4, log_zeile + 5, log_zeile + 6, log_zeile + 7, log_zeile + 8);
                        printf("\033[%d;0H# ", log_zeile + 7);
                    }
                    if (log_type == 1 || log_type == 2) {

                        printf("rtt ist %f          ", arbeits_paket_header_send->rtt);

                        if (log_type == 1) {
                            printf("\033[%d;0H", log_zeile + 8);
                            fflush(stdout);
                        } else if (log_type == 2) {
                            printf("\n");
                            fflush(stdout);
                        }
                    }

                }
            }

            if (my_max_send_train_id < my_last_send_train_id) {
                my_max_send_train_id = my_last_send_train_id;

                if (my_max_train_id < my_max_send_train_id) {
                    my_max_train_id = my_max_send_train_id;

                    if (log_type == 1) {
                        printf("\033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  \033[%d;0H  ",
                                log_zeile, log_zeile + 1, log_zeile + 2, log_zeile + 3, log_zeile + 4, log_zeile + 5, log_zeile + 6, log_zeile + 7, log_zeile + 8);
                        printf("\033[%d;0H# ", log_zeile + 6);
                    }
                    if (log_type == 1 || log_type == 2) {

                        printf("my_max_train_id send: %d    # sendTime: %.5f  # RecvTimeout. %.5f               ", my_max_train_id, (double) train_sending_time.tv_nsec / 1000000000.0, (double) timeout_time.tv_usec / 1000000.0);

                        if (log_type == 1) {
                            printf("\033[%d;0H", log_zeile + 8);
                            fflush(stdout);
                        } else if (log_type == 2) {
                            printf("\n");
                            fflush(stdout);
                        }
                    }
                }
            }

            lac_recv->save_to_file_and_clear();
            //lac_recv->clear();
        }
    }

#ifdef BinServer
    in_while_schleife = false;
#endif

    delete (lac_recv);
    delete (lac_send1);
    delete (lac_send2);

    delete (arbeits_paket_recv);
    delete (arbeits_paket_send);

    clock_gettime(CLOCK_REALTIME, &end_time);

    printf("Ende von %s:%d <--> ", inet_ntoa(meineAddr.sin_addr), ntohs(meineAddr.sin_port));
    printf("%s:%d \n", inet_ntoa(otherAddr.sin_addr), ntohs(otherAddr.sin_port));
}

