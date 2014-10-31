/* 
 * File:   ATCInfo.cpp
 * Author: user
 * 
 * Created on 16. Oktober 2014, 11:54
 */

#include "AtcGpsInfo.h"
#include "ClientBenchmarkClass.h"

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#define ATC_FILENAME "/dev/ttyUSB3" 

#define END_SIGNAL_OK1 "OK\n\n"
#define END_SIGNAL_OK2 "OK\r\n"
#define END_SIGNAL_ERROR1 "ERROR\n\n"
#define END_SIGNAL_ERROR2 "ERROR\r\n"

#define PORT_GPSD 2947
#define IP_GPSD "192.168.0.32"

AtcGpsInfo::AtcGpsInfo(char *_zeit_dateiname) {

    struct EndsignalData esd;
    esd.size = strlen(END_SIGNAL_OK1);
    esd.cp = (char*) END_SIGNAL_OK1;
    EndSignalOkList.push_back(esd);
    esd.size = strlen(END_SIGNAL_OK2);
    esd.cp = (char*) END_SIGNAL_OK2;
    EndSignalOkList.push_back(esd);
    esd.size = strlen(END_SIGNAL_ERROR1);
    esd.cp = (char*) END_SIGNAL_ERROR1;
    EndSignalErrorList.push_back(esd);
    esd.size = strlen(END_SIGNAL_ERROR2);
    esd.cp = (char*) END_SIGNAL_ERROR2;
    EndSignalErrorList.push_back(esd);

    int filename_len = strlen(ATC_FILENAME);

    filename_tty = (char*) malloc(filename_len + 1);

    strncpy(filename_tty, ATC_FILENAME, filename_len);

    // Geraetedatei oeffnen
    File_Deskriptor_tty = open(filename_tty, O_RDWR | O_NOCTTY | O_NDELAY);
    if (File_Deskriptor_tty == -1) {
        printf("ERROR:\n  Geraetedatei \"%s\" nicht oeffnen!!\n(%s)\n", filename_tty, strerror(errno));
        fflush(stdout);
        //        exit(EXIT_FAILURE);
    } else {
        // Prüfen ob Gerätedatei serielle Schnittstelle ist
        if (isatty(File_Deskriptor_tty) == 0) {
            printf("ERROR:\n  \"%s\" ist keine tty\n", filename_tty);
            fflush(stdout);
            //            exit(EXIT_FAILURE);
        }

        fcntl(File_Deskriptor_tty, F_SETFL, FNDELAY); // lesen blockiert nicht
        //fcntl(fd, F_SETFL, 0); // lesen blockiert

        struct termios options;

        /* Attribute des tty erhalten */
        tcgetattr(File_Deskriptor_tty, &options);

        /* Timeout der Leseblockierung setzen */
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_oflag &= ~OPOST;
        options.c_cc[VMIN] = 0;

        unsigned char timeout = 1;
        options.c_cc[VTIME] = timeout;

        /* Attribute speichern */
        tcsetattr(File_Deskriptor_tty, TCSANOW, &options);

        int a = myWrite("AT\r", 3);

        if (a == -2) {
            sleep(1);
            a = myRead(buffer, BufferSize);
        }

        if (a != 0) {
            printf("ERROR:\n \"%s\" hat auf AT nicht mit \"OK\" geantwortet.\n  \"%s\" geantwortet \n", filename_tty, buffer);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }

        sleep(1);
        int b = myRead(buffer, BufferSize);
        int bb = strlen(buffer);
        int c = 0;
        while (0 < bb && c < 10) {
            sleep(1);
            b = myRead(buffer, BufferSize);
            bb = strlen(buffer);
            c++;
        }

        if (0 < bb) {
            printf("ERROR:\n Modem sendet per tty Daten \n");
            fflush(stdout);
            exit(EXIT_FAILURE);
        }

    }

    step_index = 0;
    last_step_index = 17;

    AT_Sets_new = (AT_Sets*) malloc(sizeof (AT_Sets));
    AT_Sets_old = (AT_Sets*) malloc(sizeof (AT_Sets));

    AT_Sets_old->Network_technology_currently_in_use.readed = 0;
    AT_Sets_old->Available_technologies_on_current_network.readed = 0;
    AT_Sets_old->Operational_status.readed = 0;
    AT_Sets_old->Current_active_radio_access_technology.readed = 0;
    AT_Sets_old->Current_service_domain.readed = 0;
    AT_Sets_old->Signal_Quality.readed = 0;
    AT_Sets_old->WCDMA_Active_Set.readed = 0;
    AT_Sets_old->WCDMA_Sync_Neighbour.readed = 0;
    AT_Sets_old->WCDMA_Async_Neighbour.readed = 0;

    File_Deskriptor_file = 0;

    sprintf(filename_file, "%s_cl_atc.b", _zeit_dateiname);

    // O_WRONLY nur zum Schreiben oeffnen
    // O_RDWR zum Lesen und Schreiben oeffnen
    // O_RDONLY nur zum Lesen oeffnen
    // O_CREAT Falls die Datei nicht existiert, wird sie neu angelegt. Falls die Datei existiert, ist O_CREAT ohne Wirkung.
    // O_APPEND Datei oeffnen zum Schreiben am Ende
    // O_EXCL O_EXCL kombiniert mit O_CREAT bedeutet, dass die Datei nicht geoeffnet werden kann, wenn sie bereits existiert und open() den Wert 1 zurueckliefert (1 == Fehler).
    // O_TRUNC Eine Datei, die zum Schreiben geoeffnet wird, wird geleert. Darauffolgendes Schreiben bewirkt erneutes Beschreiben der Datei von Anfang an. Die Attribute der Datei bleiben erhalten.
    File_Deskriptor_file = open(filename_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG, S_IRWXO, 0777);
    if (File_Deskriptor_file == -1) {
        printf("ERROR:\n  Fehler beim oeffnen / Erstellen der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    printf("Datei \"%s\" erstellt & geoeffnet \n", filename_file);

    char firstlines[] = "atc Data \n\n";
    int firstlines_len = strlen(firstlines);

    if (write(File_Deskriptor_file, firstlines, firstlines_len) != firstlines_len) {
        printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }


    if ((socket_gpsd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        printf("ERROR:\n  Fehler beim Erstellen vom gpsd Socket \n(%s)\n", strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    address_gpsd.sin_family = AF_INET;
    address_gpsd.sin_port = htons(PORT_GPSD);

    inet_aton(IP_GPSD, &address_gpsd.sin_addr);

    if (connect(socket_gpsd, (struct sockaddr *) &address_gpsd, sizeof (address_gpsd)) != 0) {
        printf("ERROR:\n  Fehler beim Erstellen vom gpsd Socket \n(%s)\n", strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    struct timeval tv_timeout;
    tv_timeout.tv_sec = 1; /* 30 Secs Timeout */
    tv_timeout.tv_usec = 0; // Not init'ing this can cause strange errors
    setsockopt(socket_gpsd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv_timeout, sizeof (struct timeval));

    if (fcntl(socket_gpsd, F_SETFL, fcntl(socket_gpsd, F_GETFL) | O_NONBLOCK) < 0) {
        printf("ERROR:\n  Fehler beim Setzen des NONBLOCK Flag vom gpsd Socket \n(%s)\n", strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    sleep(1);

    int size = recv(socket_gpsd, buffer_pgsd, BufferSize, 0);

    size++;
    size--;

    size = recv(socket_gpsd, buffer_pgsd, BufferSize, 0);

    if (size < 1) {
        printf("ERROR:\n  Fehler beim beim Empfangen vom gpsd Socket \n(%s)\n", strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    char buf2[] = "?WATCH={\"enable\":true,\"json\":true}";

    if (strlen(buf2) != send(socket_gpsd, buf2, strlen(buf2), 0)) {
        printf("ERROR:\n  Fehler beim beim Senden vom gpsd Socket \n(%s)\n", strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    all_steps_done = false;
    steps_after_save = false;
    step_index_by_last_save = 0;
}


//ATCInfo::ATCInfo(const ATCInfo& orig) {}

AtcGpsInfo::~AtcGpsInfo() {
}

int AtcGpsInfo::myWrite(const char s [], int len) {
    int count_written_bytes = write(File_Deskriptor_tty, s, len);
    if (len != count_written_bytes) {
        printf("ERROR:\n  Write %i Bytes: \"%s\" to \"%s\" \n", count_written_bytes, s, filename_tty);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    return 0;
}

/**
 * Liest AT-Befehl Antwort aus
 * 
 * @param buffer
 *      Zeiger auf Char Array für Antwort
 * @param len
 *      Länge von Char Array
 * @return 
 *      0 - erfolgreich (s endet mit einem OK End-Signal)
 *     -1 - erfolgreich (s endet mit einem Error End-Signal)
 *     -2 - nicht erfolgreich (es wurden 0 Bytes gelesen)
 *     >0 - nicht erfolgreich (es wurden @return Bytes gelesen)
 */
int AtcGpsInfo::myRead(char buffer [], int len) {

    int count_readed_bytes = 0;

    int count_read = 0;
    int max_try_read = 100;
    int new_count_readed_bytes = read(File_Deskriptor_tty, buffer, len - 1);

    /*
    if (new_count_readed_bytes < 0) {
        printf("ERROR:\n  Read %i Bytes from \"%s\"; read total %i Bytes \n", new_count_readed_bytes, filename, count_readed_bytes);
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
     */
    if (new_count_readed_bytes == -1) {
        new_count_readed_bytes = 0;
    }
    count_readed_bytes = count_readed_bytes + new_count_readed_bytes;

    buffer[count_readed_bytes] = '\0';

    if (endswithOK(buffer, count_readed_bytes) == 0) {
        return 0;
    } else {
        if (endswithERROR(buffer, count_readed_bytes) == 0) {
            return -1;
        }
    }

    while (count_read < max_try_read) {

        count_read++;
        new_count_readed_bytes = read(File_Deskriptor_tty, &(buffer[count_readed_bytes]), len - 1 - count_readed_bytes);

        /*
        if (new_count_readed_bytes < 0) {
            printf("ERROR:\n  Read %i Bytes from \"%s\"; read total %i Bytes \n", new_count_readed_bytes, filename, count_readed_bytes);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
         */
        if (new_count_readed_bytes == -1) {
            new_count_readed_bytes = 0;
        }
        if (0 < new_count_readed_bytes) {
            count_read = 0;
        }

        count_readed_bytes = count_readed_bytes + new_count_readed_bytes;

        buffer[count_readed_bytes] = '\0';

        if (endswithOK(buffer, count_readed_bytes) == 0) {
            return 0;
        } else {
            if (endswithERROR(buffer, count_readed_bytes) == 0) {
                return -1;
            }
        }
    }

    if (count_readed_bytes == 0) {
        return -2;
    }

    return count_readed_bytes;
}

int AtcGpsInfo::timespec2str(char *buf, int len, struct timespec *ts) {
    int ret;
    struct tm t;

    tzset();
    if (localtime_r(&(ts->tv_sec), &t) == NULL) {
        return 1;
    }

    ret = strftime(buf, len, "%F %T", &t);
    if (ret == 0) {
        return 2;
    }
    len = len - ret;

    ret = snprintf(&buf[ret], len, ".%09ld", ts->tv_nsec);
    if (ret >= len) {
        return 3;
    }

    return 0;
}

int AtcGpsInfo::myReadOne(AT_Set *ats, int len) {

    if (count_myReadOne == 0) {
        (ats->time).tv_nsec = 0;
        (ats->time).tv_sec = 0;
    }

    count_myReadOne++;

    int new_count_readed_bytes = read(File_Deskriptor_tty, &buffer[ats->readed], len - ats->readed - 1);

    if (0 < new_count_readed_bytes) {
        if (-1 == ats->readed) {
            ats->readed = 0;
            clock_gettime(CLOCK_REALTIME, &ats->time);
        }

        ats->readed = ats->readed + new_count_readed_bytes;

        buffer[ats->readed] = '\0';

        //    printf("# rr %d: %s #\n", new_count_readed_bytes, &buffer[(*readed) - new_count_readed_bytes]);

        if (endswithOK(buffer, ats->readed) == 0) {
            return 0;
        } else {
            if (endswithERROR(buffer, ats->readed) == 0) {
                return -1;
            }
        }
    } else {
        if (100000 < count_myReadOne) {
            return 2;
        }
        if (new_count_readed_bytes == -1) {
            return 1;
        }
    }

    return 1;
}

/**
 * Prüft ob Charr Array s mit einem OK End-Signal aus Devicekonfig endet
 * 
 * @param s
 *      Zieger auf Char Array
 * @param len
 *      Länge von Char Array
 * @return 
 *      0 - erfolgreich (s endet mit einem OK End-Signal)
 */
int AtcGpsInfo::endswithOK(const char s [], int len) {
    for (std::vector<EndsignalData>::iterator i = this->EndSignalOkList.begin(); i != this->EndSignalOkList.end(); i++) {

        //        char *c = (*i).cp;

        if ((*i).size <= len) {
            if (0 == strncmp((*i).cp, &(s[len - (*i).size]), (*i).size)) {
                return 0;
            }
        }
    }

    return -1;
}

/**
 * Prüft ob Charr Array s mit einem Error End-Signal aus Devicekonfig endet
 * 
 * @param s
 *      Zieger auf Char Array
 * @param len
 *      Länge von Char Array
 * @return 
 *      0 - erfolgreich (s endet mit einem Error End-Signal)
 */
int AtcGpsInfo::endswithERROR(const char s [], int len) {
    for (std::vector<EndsignalData>::iterator i = this->EndSignalErrorList.begin(); i != this->EndSignalErrorList.end(); i++) {

        //        char *c = (*i).cp;

        if ((*i).size <= len) {
            if (0 == strncmp((*i).cp, &(s[len - (*i).size]), (*i).size)) {
                return 0;
            }
        }
    }

    return -1;
}

int AtcGpsInfo::get_Network_technology_currently_in_use(char s[], int len) {
    myWrite(awNetwork_technology_currently_in_use, awNetwork_technology_currently_in_use_len);

    return myRead(s, len);
}

int AtcGpsInfo::get_Available_technologies_on_current_network(char s[], int len) {
    myWrite(awAvailable_technologies_on_current_network, awAvailable_technologies_on_current_network_len);

    return myRead(s, len);
}

int AtcGpsInfo::get_Operational_status(char s[], int len) {
    myWrite(awOperational_status, awOperational_status_len);

    return myRead(s, len);
}

int AtcGpsInfo::get_Current_active_radio_access_technology(char s[], int len) {
    myWrite(awCurrent_active_radio_access_technology, awCurrent_active_radio_access_technology_len);

    return myRead(s, len);
}

int AtcGpsInfo::get_Current_service_domain(char s[], int len) {
    myWrite(awCurrent_service_domain, awCurrent_service_domain_len);

    return myRead(s, len);
}

int AtcGpsInfo::get_Signal_Quality(char s[], int len) {
    myWrite(awSignal_Quality, awSignal_Quality_len);

    return myRead(s, len);
}

int AtcGpsInfo::get_WCDMA_Active_Set(char s[], int len) {
    myWrite(awWCDMA_Active_Set, awWCDMA_Active_Set_len);

    return myRead(s, len);
}

int AtcGpsInfo::get_WCDMA_Sync_Neighbour(char s[], int len) {
    myWrite(awWCDMA_Sync_Neighbour, awWCDMA_Sync_Neighbour_len);

    return myRead(s, len);
}

int AtcGpsInfo::get_WCDMA_Async_Neighbour(char s[], int len) {
    myWrite(awWCDMA_Async_Neighbour, awWCDMA_Async_Neighbour_len);

    return myRead(s, len);
}

int AtcGpsInfo::do_step() {

    if (step_index_by_last_save == step_index) {
        if (steps_after_save) {
            all_steps_done = true;
            return 0;
        }
    }

    if (step_index == 0) {
        AT_Sets_new->Network_technology_currently_in_use.readed = 0;
        AT_Sets_new->Available_technologies_on_current_network.readed = 0;
        AT_Sets_new->Operational_status.readed = 0;
        AT_Sets_new->Current_active_radio_access_technology.readed = 0;
        AT_Sets_new->Current_service_domain.readed = 0;
        AT_Sets_new->Signal_Quality.readed = 0;
        AT_Sets_new->WCDMA_Active_Set.readed = 0;
        AT_Sets_new->WCDMA_Sync_Neighbour.readed = 0;
        AT_Sets_new->WCDMA_Async_Neighbour.readed = 0;
    }

    switch (step_index) {
        case 0:
            myWrite(awNetwork_technology_currently_in_use, awNetwork_technology_currently_in_use_len);
            AT_Sets_new->Network_technology_currently_in_use.readed = -1;
            step_index++;
            steps_after_save = true;


            //            printf("#w: %s#\n", awNetwork_technology_currently_in_use);
            count_myReadOne = 0;
            break;

        case 1:
            if (1 != myReadOne(&AT_Sets_new->Network_technology_currently_in_use, BufferSize)) {
                step_index++;
                steps_after_save = true;
                /*
                                if (0 < AT_Set_new->readedNetwork_technology_currently_in_use) {
                                    printf("#r %d %d: %s #\n", count_myReadOne, AT_Set_new->readedNetwork_technology_currently_in_use, AT_Set_new->Network_technology_currently_in_use);
                                }
                 */
            }

            break;

        case 2:
            myWrite(awAvailable_technologies_on_current_network, awAvailable_technologies_on_current_network_len);
            AT_Sets_new->Available_technologies_on_current_network.readed = -1;
            step_index++;
            steps_after_save = true;

            //            printf("#w: %s#\n", awAvailable_technologies_on_current_network);
            count_myReadOne = 0;
            break;

        case 3:
            if (1 != myReadOne(&AT_Sets_new->Available_technologies_on_current_network, BufferSize)) {
                step_index++;
                steps_after_save = true;
                /*
                if (0 < AT_Set_new->readedAvailable_technologies_on_current_network) {
                    printf("#r %d %d: %s #\n", count_myReadOne, AT_Set_new->readedAvailable_technologies_on_current_network, AT_Set_new->Available_technologies_on_current_network);
                }
                 */
            }

            break;

        case 4:
            myWrite(awOperational_status, awOperational_status_len);
            AT_Sets_new->Operational_status.readed = -1;
            step_index++;
            steps_after_save = true;

            //            printf("#w: %s#\n", awOperational_status);
            count_myReadOne = 0;
            break;

        case 5:
            if (1 != myReadOne(&AT_Sets_new->Operational_status, BufferSize)) {
                step_index++;
                steps_after_save = true;
                /*
                if (0 < AT_Set_new->readedOperational_status) {
                    printf("#r %d %d: %s #\n", count_myReadOne, AT_Set_new->readedOperational_status, AT_Set_new->Operational_status);
                }
                 * */
            }

            break;

        case 6:
            myWrite(awCurrent_active_radio_access_technology, awCurrent_active_radio_access_technology_len);
            AT_Sets_new->Current_active_radio_access_technology.readed = -1;
            step_index++;
            steps_after_save = true;

            //            printf("#w: %s#\n", awCurrent_active_radio_access_technology);
            count_myReadOne = 0;
            break;

        case 7:
            if (1 != myReadOne(&AT_Sets_new->Current_active_radio_access_technology, BufferSize)) {
                step_index++;
                steps_after_save = true;
                /*
                if (0 < AT_Set_new->readedCurrent_active_radio_access_technology) {
                    printf("#r %d %d: %s #\n", count_myReadOne, AT_Set_new->readedCurrent_active_radio_access_technology, AT_Set_new->Current_active_radio_access_technology);
                }
                 */
            }

            break;

        case 8:
            myWrite(awCurrent_service_domain, awCurrent_service_domain_len);
            AT_Sets_new->Current_service_domain.readed = -1;
            step_index++;
            steps_after_save = true;

            //            printf("#w: %s#\n", awCurrent_service_domain);
            count_myReadOne = 0;
            break;

        case 9:
            if (1 != myReadOne(&AT_Sets_new->Current_service_domain, BufferSize)) {
                step_index++;
                steps_after_save = true;
                /*
                if (0 < AT_Set_new->readedCurrent_service_domain) {
                    printf("#r %d %d: %s #\n", count_myReadOne, AT_Set_new->readedCurrent_service_domain, AT_Set_new->Current_service_domain);
                }
                 * */
            }

            break;

        case 10:
            myWrite(awSignal_Quality, awSignal_Quality_len);
            AT_Sets_new->Signal_Quality.readed = -1;
            step_index++;
            steps_after_save = true;

            //printf("#w: %s#\n", awSignal_Quality);
            count_myReadOne = 0;
            break;

        case 11:
            if (1 != myReadOne(&AT_Sets_new->Signal_Quality, BufferSize)) {
                step_index++;
                steps_after_save = true;
                /*
                if (0 < AT_Set_new->readedSignal_Quality) {
                    printf("#r %d %d: %s #\n", count_myReadOne, AT_Set_new->readedSignal_Quality, AT_Set_new->Signal_Quality);
                }
                 * */
            }

            break;

        case 12:
            myWrite(awWCDMA_Active_Set, awWCDMA_Active_Set_len);
            AT_Sets_new->WCDMA_Active_Set.readed = -1;
            step_index++;
            steps_after_save = true;

            //            printf("#w: %s#\n", awWCDMA_Active_Set);
            count_myReadOne = 0;
            break;

        case 13:
            if (1 != myReadOne(&AT_Sets_new->WCDMA_Active_Set, BufferSize)) {
                step_index++;
                steps_after_save = true;
                /*
                if (0 < AT_Set_new->readedWCDMA_Active_Set) {
                    printf("#r %d %d: %s #\n", count_myReadOne, AT_Set_new->readedWCDMA_Active_Set, AT_Set_new->WCDMA_Active_Set);
                }
                 * */
            }

            break;

        case 14:
            myWrite(awWCDMA_Sync_Neighbour, awWCDMA_Sync_Neighbour_len);
            AT_Sets_new->WCDMA_Sync_Neighbour.readed = -1;
            step_index++;
            steps_after_save = true;

            //            printf("#w: %s#\n", awWCDMA_Sync_Neighbour);
            count_myReadOne = 0;
            break;

        case 15:
            if (1 != myReadOne(&AT_Sets_new->WCDMA_Sync_Neighbour, BufferSize)) {
                step_index++;
                steps_after_save = true;
                /*
                if (0 < AT_Set_new->readedWCDMA_Sync_Neighbour) {
                    printf("#r %d %d: %s #\n", count_myReadOne, AT_Set_new->readedWCDMA_Sync_Neighbour, AT_Set_new->WCDMA_Sync_Neighbour);
                }
                 * */
            }


            break;

        case 16:
            myWrite(awWCDMA_Async_Neighbour, awWCDMA_Async_Neighbour_len);
            AT_Sets_new->WCDMA_Async_Neighbour.readed = -1;
            step_index++;
            steps_after_save = true;

            //            printf("#w: %s#\n", awWCDMA_Async_Neighbour);
            count_myReadOne = 0;
            break;

        case 17:
            if (1 != myReadOne(&AT_Sets_new->WCDMA_Async_Neighbour, BufferSize)) {
                step_index = 0;
                /*
                if (0 < AT_Set_new->readedWCDMA_Async_Neighbour) {
                    printf("#r %d %d: %s #\n", count_myReadOne, AT_Set_new->readedWCDMA_Async_Neighbour, AT_Set_new->WCDMA_Async_Neighbour);
                }
                 * */
            }

            break;
    }

    return 0;
}

int AtcGpsInfo::save_to_file() {

    if (steps_after_save == false) {
        return 0;
    }
    
    this->step_index = 0;

    int timespec_len = sizeof (timespec);
    int aw_len_1 = 0;
    int ar_len_1 = 0;

    bool save_all = true;

    AT_Set ats_temp;

    if (AT_Sets_new->Network_technology_currently_in_use.readed == -1) {
        ats_temp = AT_Sets_new->Network_technology_currently_in_use;
        AT_Sets_new->Network_technology_currently_in_use = AT_Sets_old->Network_technology_currently_in_use;
        AT_Sets_old->Network_technology_currently_in_use = ats_temp;
    } else {

        if (AT_Sets_old->Network_technology_currently_in_use.readed != AT_Sets_new->Network_technology_currently_in_use.readed ||
                0 != strcmp(AT_Sets_old->Network_technology_currently_in_use.ar, AT_Sets_new->Network_technology_currently_in_use.ar) ||
                save_all) {

            if (write(File_Deskriptor_file, &AT_Sets_new->Network_technology_currently_in_use.time, timespec_len) != timespec_len) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            aw_len_1 = awNetwork_technology_currently_in_use_len + 1;
            if (write(File_Deskriptor_file, awNetwork_technology_currently_in_use, aw_len_1) != aw_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            ar_len_1 = AT_Sets_new->Network_technology_currently_in_use.readed + 1;
            if (write(File_Deskriptor_file, AT_Sets_new->Network_technology_currently_in_use.ar, ar_len_1) != ar_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

        }

    }

    if (AT_Sets_new->Network_technology_currently_in_use.readed == -1) {
        ats_temp = AT_Sets_new->Network_technology_currently_in_use;
        AT_Sets_new->Network_technology_currently_in_use = AT_Sets_old->Network_technology_currently_in_use;
        AT_Sets_old->Network_technology_currently_in_use = ats_temp;
    } else {

        if (AT_Sets_old->Available_technologies_on_current_network.readed != AT_Sets_new->Available_technologies_on_current_network.readed ||
                0 != strcmp(AT_Sets_old->Available_technologies_on_current_network.ar, AT_Sets_new->Available_technologies_on_current_network.ar)
                || save_all) {

            if (write(File_Deskriptor_file, &AT_Sets_new->Available_technologies_on_current_network.time, timespec_len) != timespec_len) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            aw_len_1 = awAvailable_technologies_on_current_network_len + 1;
            if (write(File_Deskriptor_file, awAvailable_technologies_on_current_network, aw_len_1) != aw_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            ar_len_1 = AT_Sets_new->Available_technologies_on_current_network.readed + 1;
            if (write(File_Deskriptor_file, AT_Sets_new->Available_technologies_on_current_network.ar, ar_len_1) != ar_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

        }

    }

    if (AT_Sets_new->Network_technology_currently_in_use.readed == -1) {
        ats_temp = AT_Sets_new->Network_technology_currently_in_use;
        AT_Sets_new->Network_technology_currently_in_use = AT_Sets_old->Network_technology_currently_in_use;
        AT_Sets_old->Network_technology_currently_in_use = ats_temp;
    } else {


        if (AT_Sets_old->Operational_status.readed != AT_Sets_new->Operational_status.readed ||
                0 != strcmp(AT_Sets_old->Operational_status.ar, AT_Sets_new->Operational_status.ar) ||
                save_all) {

            if (write(File_Deskriptor_file, &AT_Sets_new->Operational_status.time, timespec_len) != timespec_len) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            aw_len_1 = awOperational_status_len + 1;
            if (write(File_Deskriptor_file, awOperational_status, aw_len_1) != aw_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            ar_len_1 = AT_Sets_new->Operational_status.readed + 1;
            if (write(File_Deskriptor_file, AT_Sets_new->Operational_status.ar, ar_len_1) != ar_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

        }

    }

    if (AT_Sets_new->Network_technology_currently_in_use.readed == -1) {
        ats_temp = AT_Sets_new->Network_technology_currently_in_use;
        AT_Sets_new->Network_technology_currently_in_use = AT_Sets_old->Network_technology_currently_in_use;
        AT_Sets_old->Network_technology_currently_in_use = ats_temp;
    } else {


        if (AT_Sets_old->Current_active_radio_access_technology.readed != AT_Sets_new->Current_active_radio_access_technology.readed ||
                0 != strcmp(AT_Sets_old->Current_active_radio_access_technology.ar, AT_Sets_new->Current_active_radio_access_technology.ar) ||
                save_all) {

            if (write(File_Deskriptor_file, &AT_Sets_new->Current_active_radio_access_technology.time, timespec_len) != timespec_len) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            aw_len_1 = awCurrent_active_radio_access_technology_len + 1;
            if (write(File_Deskriptor_file, awCurrent_active_radio_access_technology, aw_len_1) != aw_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            ar_len_1 = AT_Sets_new->Current_active_radio_access_technology.readed + 1;
            if (write(File_Deskriptor_file, AT_Sets_new->Current_active_radio_access_technology.ar, ar_len_1) != ar_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

        }

    }

    if (AT_Sets_new->Network_technology_currently_in_use.readed == -1) {
        ats_temp = AT_Sets_new->Network_technology_currently_in_use;
        AT_Sets_new->Network_technology_currently_in_use = AT_Sets_old->Network_technology_currently_in_use;
        AT_Sets_old->Network_technology_currently_in_use = ats_temp;
    } else {


        if (AT_Sets_old->Current_service_domain.readed != AT_Sets_new->Current_service_domain.readed ||
                0 != strcmp(AT_Sets_old->Current_service_domain.ar, AT_Sets_new->Current_service_domain.ar) ||
                save_all) {

            if (write(File_Deskriptor_file, &AT_Sets_new->Current_service_domain.time, timespec_len) != timespec_len) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            aw_len_1 = awCurrent_service_domain_len + 1;
            if (write(File_Deskriptor_file, awCurrent_service_domain, aw_len_1) != aw_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            ar_len_1 = AT_Sets_new->Current_service_domain.readed + 1;
            if (write(File_Deskriptor_file, AT_Sets_new->Current_service_domain.ar, ar_len_1) != ar_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

        }

    }

    if (AT_Sets_new->Network_technology_currently_in_use.readed == -1) {
        ats_temp = AT_Sets_new->Network_technology_currently_in_use;
        AT_Sets_new->Network_technology_currently_in_use = AT_Sets_old->Network_technology_currently_in_use;
        AT_Sets_old->Network_technology_currently_in_use = ats_temp;
    } else {


        if (AT_Sets_old->Signal_Quality.readed != AT_Sets_new->Signal_Quality.readed ||
                0 != strcmp(AT_Sets_old->Signal_Quality.ar, AT_Sets_new->Signal_Quality.ar) ||
                save_all) {

            if (write(File_Deskriptor_file, &AT_Sets_new->Signal_Quality.time, timespec_len) != timespec_len) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            aw_len_1 = awSignal_Quality_len + 1;
            if (write(File_Deskriptor_file, awSignal_Quality, aw_len_1) != aw_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            ar_len_1 = AT_Sets_new->Signal_Quality.readed + 1;
            if (write(File_Deskriptor_file, AT_Sets_new->Signal_Quality.ar, ar_len_1) != ar_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

        }

    }

    if (AT_Sets_new->Network_technology_currently_in_use.readed == -1) {
        ats_temp = AT_Sets_new->Network_technology_currently_in_use;
        AT_Sets_new->Network_technology_currently_in_use = AT_Sets_old->Network_technology_currently_in_use;
        AT_Sets_old->Network_technology_currently_in_use = ats_temp;
    } else {


        if (AT_Sets_old->WCDMA_Active_Set.readed != AT_Sets_new->WCDMA_Active_Set.readed ||
                0 != strcmp(AT_Sets_old->WCDMA_Active_Set.ar, AT_Sets_new->WCDMA_Active_Set.ar) ||
                save_all) {

            if (write(File_Deskriptor_file, &AT_Sets_new->WCDMA_Active_Set.time, timespec_len) != timespec_len) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            aw_len_1 = awWCDMA_Active_Set_len + 1;
            if (write(File_Deskriptor_file, awWCDMA_Active_Set, aw_len_1) != aw_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            ar_len_1 = AT_Sets_new->WCDMA_Active_Set.readed + 1;
            if (write(File_Deskriptor_file, AT_Sets_new->WCDMA_Active_Set.ar, ar_len_1) != ar_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

        }

    }

    if (AT_Sets_new->Network_technology_currently_in_use.readed == -1) {
        ats_temp = AT_Sets_new->Network_technology_currently_in_use;
        AT_Sets_new->Network_technology_currently_in_use = AT_Sets_old->Network_technology_currently_in_use;
        AT_Sets_old->Network_technology_currently_in_use = ats_temp;
    } else {


        if (AT_Sets_old->WCDMA_Sync_Neighbour.readed != AT_Sets_new->WCDMA_Sync_Neighbour.readed ||
                0 != strcmp(AT_Sets_old->WCDMA_Sync_Neighbour.ar, AT_Sets_new->WCDMA_Sync_Neighbour.ar) ||
                save_all) {

            if (write(File_Deskriptor_file, &AT_Sets_new->WCDMA_Sync_Neighbour.time, timespec_len) != timespec_len) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            aw_len_1 = awWCDMA_Sync_Neighbour_len + 1;
            if (write(File_Deskriptor_file, awWCDMA_Sync_Neighbour, aw_len_1) != aw_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            ar_len_1 = AT_Sets_new->WCDMA_Sync_Neighbour.readed + 1;
            if (write(File_Deskriptor_file, AT_Sets_new->WCDMA_Sync_Neighbour.ar, ar_len_1) != ar_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

        }

    }

    if (AT_Sets_new->Network_technology_currently_in_use.readed == -1) {
        ats_temp = AT_Sets_new->Network_technology_currently_in_use;
        AT_Sets_new->Network_technology_currently_in_use = AT_Sets_old->Network_technology_currently_in_use;
        AT_Sets_old->Network_technology_currently_in_use = ats_temp;
    } else {


        if (AT_Sets_old->WCDMA_Async_Neighbour.readed != AT_Sets_new->WCDMA_Async_Neighbour.readed ||
                0 != strcmp(AT_Sets_old->WCDMA_Async_Neighbour.ar, AT_Sets_new->WCDMA_Async_Neighbour.ar) ||
                save_all) {

            if (write(File_Deskriptor_file, &AT_Sets_new->WCDMA_Async_Neighbour.time, timespec_len) != timespec_len) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            aw_len_1 = awWCDMA_Async_Neighbour_len + 1;
            if (write(File_Deskriptor_file, awWCDMA_Async_Neighbour, aw_len_1) != aw_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            ar_len_1 = AT_Sets_new->WCDMA_Async_Neighbour.readed + 1;
            if (write(File_Deskriptor_file, AT_Sets_new->WCDMA_Async_Neighbour.ar, ar_len_1) != ar_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_file, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

        }

    }

    struct AT_Sets *AT_Set_temp = AT_Sets_new;
    AT_Sets_new = AT_Sets_old;
    AT_Sets_old = AT_Set_temp;

    all_steps_done = false;
    steps_after_save = false;
    step_index_by_last_save = step_index;

}
