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
#include <list>

#define ATC_FILENAME "/dev/ttyUSB3" 

#define END_SIGNAL_OK1 "OK\n\n"
#define END_SIGNAL_OK2 "OK\r\n"
#define END_SIGNAL_ERROR1 "ERROR\n\n"
#define END_SIGNAL_ERROR2 "ERROR\r\n"

#define PORT_GPSD 2947
#define IP_GPSD "192.168.0.32"
//#define IP_GPSD "192.168.2.180"

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
        printf("ERROR:\n  Geraetedatei \"%s\" kann nicht geeoffnet werden!!\n(%s)\n", filename_tty, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    } else {
        // Prüfen ob Gerätedatei serielle Schnittstelle ist
        if (isatty(File_Deskriptor_tty) == 0) {
            printf("ERROR:\n  \"%s\" ist keine tty\n", filename_tty);
            fflush(stdout);
            exit(EXIT_FAILURE);
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
    last_step_index = 18;

    AT_Sets_v = (AT_Sets*) malloc(sizeof (AT_Sets));

    AT_Sets_v->Network_technology_currently_in_use.ar_saved = (char*) malloc(BufferSize);
    AT_Sets_v->Network_technology_currently_in_use.ar_write = (char*) malloc(BufferSize);
    AT_Sets_v->Available_technologies_on_current_network.ar_saved = (char*) malloc(BufferSize);
    AT_Sets_v->Available_technologies_on_current_network.ar_write = (char*) malloc(BufferSize);
    AT_Sets_v->Operational_status.ar_saved = (char*) malloc(BufferSize);
    AT_Sets_v->Operational_status.ar_write = (char*) malloc(BufferSize);
    AT_Sets_v->Current_active_radio_access_technology.ar_saved = (char*) malloc(BufferSize);
    AT_Sets_v->Current_active_radio_access_technology.ar_write = (char*) malloc(BufferSize);
    AT_Sets_v->Current_service_domain.ar_saved = (char*) malloc(BufferSize);
    AT_Sets_v->Current_service_domain.ar_write = (char*) malloc(BufferSize);
    AT_Sets_v->Signal_Quality.ar_saved = (char*) malloc(BufferSize);
    AT_Sets_v->Signal_Quality.ar_write = (char*) malloc(BufferSize);
    AT_Sets_v->WCDMA_Active_Set.ar_saved = (char*) malloc(BufferSize);
    AT_Sets_v->WCDMA_Active_Set.ar_write = (char*) malloc(BufferSize);
    AT_Sets_v->WCDMA_Sync_Neighbour.ar_saved = (char*) malloc(BufferSize);
    AT_Sets_v->WCDMA_Sync_Neighbour.ar_write = (char*) malloc(BufferSize);
    AT_Sets_v->WCDMA_Async_Neighbour.ar_saved = (char*) malloc(BufferSize);
    AT_Sets_v->WCDMA_Async_Neighbour.ar_write = (char*) malloc(BufferSize);

    AT_Sets_v->Network_technology_currently_in_use.readed_saved = 0;
    AT_Sets_v->Available_technologies_on_current_network.readed_saved = 0;
    AT_Sets_v->Operational_status.readed_saved = 0;
    AT_Sets_v->Current_active_radio_access_technology.readed_saved = 0;
    AT_Sets_v->Current_service_domain.readed_saved = 0;
    AT_Sets_v->Signal_Quality.readed_saved = 0;
    AT_Sets_v->WCDMA_Active_Set.readed_saved = 0;
    AT_Sets_v->WCDMA_Sync_Neighbour.readed_saved = 0;
    AT_Sets_v->WCDMA_Async_Neighbour.readed_saved = 0;

    AT_Sets_v->Network_technology_currently_in_use.readed_write = 0;
    AT_Sets_v->Available_technologies_on_current_network.readed_write = 0;
    AT_Sets_v->Operational_status.readed_write = 0;
    AT_Sets_v->Current_active_radio_access_technology.readed_write = 0;
    AT_Sets_v->Current_service_domain.readed_write = 0;
    AT_Sets_v->Signal_Quality.readed_write = 0;
    AT_Sets_v->WCDMA_Active_Set.readed_write = 0;
    AT_Sets_v->WCDMA_Sync_Neighbour.readed_write = 0;
    AT_Sets_v->WCDMA_Async_Neighbour.readed_write = 0;

    AT_Sets_v->Network_technology_currently_in_use.new_readed = false;
    AT_Sets_v->Available_technologies_on_current_network.new_readed = false;
    AT_Sets_v->Operational_status.new_readed = false;
    AT_Sets_v->Current_active_radio_access_technology.new_readed = false;
    AT_Sets_v->Current_service_domain.new_readed = false;
    AT_Sets_v->Signal_Quality.new_readed = false;
    AT_Sets_v->WCDMA_Active_Set.new_readed = false;
    AT_Sets_v->WCDMA_Sync_Neighbour.new_readed = false;
    AT_Sets_v->WCDMA_Async_Neighbour.new_readed = false;

    AT_Sets_v->Network_technology_currently_in_use.aw = (char*) malloc(strlen(awNetwork_technology_currently_in_use) + 1);
    AT_Sets_v->Available_technologies_on_current_network.aw = (char*) malloc(strlen(awAvailable_technologies_on_current_network) + 1);
    AT_Sets_v->Operational_status.aw = (char*) malloc(strlen(awOperational_status) + 1);
    AT_Sets_v->Current_active_radio_access_technology.aw = (char*) malloc(strlen(awCurrent_active_radio_access_technology) + 1);
    AT_Sets_v->Current_service_domain.aw = (char*) malloc(strlen(awCurrent_service_domain) + 1);
    AT_Sets_v->Signal_Quality.aw = (char*) malloc(strlen(awSignal_Quality) + 1);
    AT_Sets_v->WCDMA_Active_Set.aw = (char*) malloc(strlen(awWCDMA_Active_Set) + 1);
    AT_Sets_v->WCDMA_Sync_Neighbour.aw = (char*) malloc(strlen(awWCDMA_Sync_Neighbour) + 1);
    AT_Sets_v->WCDMA_Async_Neighbour.aw = (char*) malloc(strlen(awWCDMA_Async_Neighbour) + 1);

    strcpy(AT_Sets_v->Network_technology_currently_in_use.aw, awNetwork_technology_currently_in_use);
    strcpy(AT_Sets_v->Available_technologies_on_current_network.aw, awAvailable_technologies_on_current_network);
    strcpy(AT_Sets_v->Operational_status.aw, awOperational_status);
    strcpy(AT_Sets_v->Current_active_radio_access_technology.aw, awCurrent_active_radio_access_technology);
    strcpy(AT_Sets_v->Current_service_domain.aw, awCurrent_service_domain);
    strcpy(AT_Sets_v->Signal_Quality.aw, awSignal_Quality);
    strcpy(AT_Sets_v->WCDMA_Active_Set.aw, awWCDMA_Active_Set);
    strcpy(AT_Sets_v->WCDMA_Sync_Neighbour.aw, awWCDMA_Sync_Neighbour);
    strcpy(AT_Sets_v->WCDMA_Async_Neighbour.aw, awWCDMA_Async_Neighbour);

    AT_Sets_v->Network_technology_currently_in_use.aw_len = awNetwork_technology_currently_in_use_len;
    AT_Sets_v->Available_technologies_on_current_network.aw_len = awAvailable_technologies_on_current_network_len;
    AT_Sets_v->Operational_status.aw_len = awOperational_status_len;
    AT_Sets_v->Current_active_radio_access_technology.aw_len = awCurrent_active_radio_access_technology_len;
    AT_Sets_v->Current_service_domain.aw_len = awCurrent_service_domain_len;
    AT_Sets_v->Signal_Quality.aw_len = awSignal_Quality_len;
    AT_Sets_v->WCDMA_Active_Set.aw_len = awWCDMA_Active_Set_len;
    AT_Sets_v->WCDMA_Sync_Neighbour.aw_len = awWCDMA_Sync_Neighbour_len;
    AT_Sets_v->WCDMA_Async_Neighbour.aw_len = awWCDMA_Async_Neighbour_len;

    File_Deskriptor_atc = 0;

    sprintf(filename_atc, "%s_cl_atc.b", _zeit_dateiname);

    // O_WRONLY nur zum Schreiben oeffnen
    // O_RDWR zum Lesen und Schreiben oeffnen
    // O_RDONLY nur zum Lesen oeffnen
    // O_CREAT Falls die Datei nicht existiert, wird sie neu angelegt. Falls die Datei existiert, ist O_CREAT ohne Wirkung.
    // O_APPEND Datei oeffnen zum Schreiben am Ende
    // O_EXCL O_EXCL kombiniert mit O_CREAT bedeutet, dass die Datei nicht geoeffnet werden kann, wenn sie bereits existiert und open() den Wert 1 zurueckliefert (1 == Fehler).
    // O_TRUNC Eine Datei, die zum Schreiben geoeffnet wird, wird geleert. Darauffolgendes Schreiben bewirkt erneutes Beschreiben der Datei von Anfang an. Die Attribute der Datei bleiben erhalten.
    File_Deskriptor_atc = open(filename_atc, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG, S_IRWXO, 0777);
    if (File_Deskriptor_atc == -1) {
        printf("ERROR:\n  Fehler beim Oeffnen / Erstellen der Datei \"%s\" \n(%s)\n", filename_atc, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    printf("Datei \"%s\" erstellt & geoeffnet \n", filename_atc);

    char firstlines[] = "atc Data \n\n";
    int firstlines_len = strlen(firstlines);

    if (write(File_Deskriptor_atc, firstlines, firstlines_len) != firstlines_len) {
        printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_atc, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }


    File_Deskriptor_gps = 0;

    sprintf(filename_gps, "%s_cl_gps.txt", _zeit_dateiname);

    File_Deskriptor_gps = open(filename_gps, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG, S_IRWXO, 0777);
    if (File_Deskriptor_gps == -1) {
        printf("ERROR:\n  Fehler beim oeffnen / Erstellen der Datei \"%s\" \n(%s)\n", filename_gps, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    printf("Datei \"%s\" erstellt & geoeffnet \n", filename_gps);

    char firstlines_gps[] = "gps Data \n\n";
    int firstlines_gps_len = strlen(firstlines_gps);

    if (write(File_Deskriptor_gps, firstlines_gps, firstlines_gps_len) != firstlines_gps_len) {
        printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", firstlines_gps, strerror(errno));
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
        printf("ERROR:\n  Fehler beim Connect vom gpsd Socket \n(%s)\n", strerror(errno));
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

    if (size < 1) {
        printf("ERROR:\n  Fehler beim Empfangen vom gpsd Socket \n(%s)\n", strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    buffer_pgsd_readed = 0;
    buffer_pgsd[buffer_pgsd_readed] = 0;

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

int AtcGpsInfo::myReadOne(AT_Set *ats, int len) {

    if (ats->count_read == 0) {
        (ats->time_read_start).tv_nsec = 0;
        (ats->time_read_start).tv_sec = 0;
        (ats->time_read_ende).tv_nsec = 0;
        (ats->time_read_ende).tv_sec = 0;
    }

    ats->count_read++;

    int new_count_readed_bytes;

    if (ats->readed_write < 0) {
        new_count_readed_bytes = read(File_Deskriptor_tty, ats->ar_write, len - 1);
    } else {
        new_count_readed_bytes = read(File_Deskriptor_tty, &ats->ar_write[ats->readed_write], len - ats->readed_write - 1);
    }

    if (0 < new_count_readed_bytes) {
        if (-1 == ats->readed_write) {
            ats->readed_write = 0;
            clock_gettime(CLOCK_REALTIME, &ats->time_read_start);
        }

        ats->readed_write = ats->readed_write + new_count_readed_bytes;

        ats->ar_write[ats->readed_write] = 0;

        if (endswithOK(ats->ar_write, ats->readed_write) == 0) {
            clock_gettime(CLOCK_REALTIME, &ats->time_read_ende);
            AT_set_List.push_back(ats);
            return 0;
        } else {
            if (endswithERROR(ats->ar_write, ats->readed_write) == 0) {
                clock_gettime(CLOCK_REALTIME, &ats->time_read_ende);
                AT_set_List.push_back(ats);
                return -1;
            }
        }
    } else {
        //        if (12500 < ats->count_read) {
        if (50000 < ats->count_read) {
            if (-1 == ats->readed_write) {
                ats->readed_write = 0;
            }
            ats->ar_write[ats->readed_write] = 0;
            clock_gettime(CLOCK_REALTIME, &ats->time_read_ende);
            AT_set_List.push_back(ats);
            return 2;
        }
        if (new_count_readed_bytes == -1) {
            return 1;
        }
    }

    return 1;
}

int AtcGpsInfo::do_step() {

    if (this->steps_after_save == false) {
        AT_Sets_v->Network_technology_currently_in_use.readed_write = 0;
        AT_Sets_v->Available_technologies_on_current_network.readed_write = 0;
        AT_Sets_v->Operational_status.readed_write = 0;
        AT_Sets_v->Current_active_radio_access_technology.readed_write = 0;
        AT_Sets_v->Current_service_domain.readed_write = 0;
        AT_Sets_v->Signal_Quality.readed_write = 0;
        AT_Sets_v->WCDMA_Active_Set.readed_write = 0;
        AT_Sets_v->WCDMA_Sync_Neighbour.readed_write = 0;
        AT_Sets_v->WCDMA_Async_Neighbour.readed_write = 0;
    }

    AT_Set *ats;

    switch (step_index) {
        case 0:

            ats = &(AT_Sets_v->Network_technology_currently_in_use);

            myWrite(ats->aw, ats->aw_len);
            clock_gettime(CLOCK_REALTIME, &ats->time_write);
            ats->readed_write = -1;
            ats->new_readed = false;
            step_index++;
            steps_after_save = true;

            ats->count_read = 0;
            break;

        case 1:
            ats = &(AT_Sets_v->Network_technology_currently_in_use);
            if (1 != myReadOne(ats, BufferSize)) {
                step_index++;
                steps_after_save = true;
                ats->new_readed = true;
            }

            break;

        case 2:
            ats = &(AT_Sets_v->Available_technologies_on_current_network);

            myWrite(ats->aw, ats->aw_len);
            clock_gettime(CLOCK_REALTIME, &ats->time_write);
            ats->readed_write = -1;
            ats->new_readed = false;
            step_index++;
            steps_after_save = true;

            ats->count_read = 0;
            break;

        case 3:
            ats = &(AT_Sets_v->Available_technologies_on_current_network);
            if (1 != myReadOne(ats, BufferSize)) {
                step_index++;
                steps_after_save = true;
                ats->new_readed = true;
            }

            break;

        case 4:
            ats = &(AT_Sets_v->Operational_status);

            myWrite(ats->aw, ats->aw_len);
            clock_gettime(CLOCK_REALTIME, &ats->time_write);
            ats->readed_write = -1;
            ats->new_readed = false;
            step_index++;
            steps_after_save = true;

            ats->count_read = 0;
            break;

        case 5:
            ats = &(AT_Sets_v->Operational_status);
            if (1 != myReadOne(ats, BufferSize)) {
                //                step_index++;

                step_index = 10;

                steps_after_save = true;
                ats->new_readed = true;
            }

            break;

        case 6:
            step_index++;
            break;

            /*            
                        ats = &(AT_Sets_v->Current_active_radio_access_technology);

                        myWrite(ats->aw, ats->aw_len);
                        clock_gettime(CLOCK_REALTIME, &ats->time_write);
                        ats->readed_write = -1;
                        ats->new_readed = false;
                        step_index++;
                        steps_after_save = true;

                        ats->count_read = 0;
                        break;
             */

        case 7:
            step_index++;
            break;

            /*  
                        ats = &(AT_Sets_v->Current_active_radio_access_technology);
                        if (1 != myReadOne(ats, BufferSize)) {
                            step_index++;
                            steps_after_save = true;
                            ats->new_readed = true;
                        }

                        break;
             */

        case 8:
            step_index++;
            break;

            /*  
                        ats = &(AT_Sets_v->Current_service_domain);

                        myWrite(ats->aw, ats->aw_len);
                        clock_gettime(CLOCK_REALTIME, &ats->time_write);
                        ats->readed_write = -1;
                        ats->new_readed = false;
                        step_index++;
                        steps_after_save = true;

                        ats->count_read = 0;
                        break;
             */
        case 9:
            step_index++;
            break;

            /*  
                        ats = &(AT_Sets_v->Current_service_domain);
                        if (1 != myReadOne(ats, BufferSize)) {
                            step_index++;
                            steps_after_save = true;
                            ats->new_readed = true;
                        }

                        break;
             */
        case 10:
            ats = &(AT_Sets_v->Signal_Quality);

            myWrite(ats->aw, ats->aw_len);
            clock_gettime(CLOCK_REALTIME, &ats->time_write);
            ats->readed_write = -1;
            ats->new_readed = false;
            step_index++;
            steps_after_save = true;

            ats->count_read = 0;
            break;

        case 11:
            ats = &(AT_Sets_v->Signal_Quality);
            if (1 != myReadOne(ats, BufferSize)) {
                step_index++;
                steps_after_save = true;
                ats->new_readed = true;
            }

            break;

        case 12:
            ats = &(AT_Sets_v->WCDMA_Active_Set);

            myWrite(ats->aw, ats->aw_len);
            clock_gettime(CLOCK_REALTIME, &ats->time_write);
            ats->readed_write = -1;
            ats->new_readed = false;
            step_index++;
            steps_after_save = true;

            ats->count_read = 0;
            break;

        case 13:
            ats = &(AT_Sets_v->WCDMA_Active_Set);
            if (1 != myReadOne(ats, BufferSize)) {
                step_index++;
                steps_after_save = true;
                ats->new_readed = true;
            }

            break;

        case 14:
            ats = &(AT_Sets_v->WCDMA_Sync_Neighbour);

            myWrite(ats->aw, ats->aw_len);
            clock_gettime(CLOCK_REALTIME, &ats->time_write);
            ats->readed_write = -1;
            ats->new_readed = false;
            step_index++;
            steps_after_save = true;

            ats->count_read = 0;
            break;

        case 15:
            ats = &(AT_Sets_v->WCDMA_Sync_Neighbour);
            if (1 != myReadOne(ats, BufferSize)) {
                step_index++;
                steps_after_save = true;
                ats->new_readed = true;
            }


            break;

        case 16:
            ats = &(AT_Sets_v->WCDMA_Async_Neighbour);

            myWrite(ats->aw, ats->aw_len);
            clock_gettime(CLOCK_REALTIME, &ats->time_write);
            ats->readed_write = -1;
            ats->new_readed = false;
            step_index++;
            steps_after_save = true;

            ats->count_read = 0;
            break;

        case 17:
            ats = &(AT_Sets_v->WCDMA_Async_Neighbour);
            if (1 != myReadOne(ats, BufferSize)) {
                step_index++;
                steps_after_save = true;
                ats->new_readed = true;
            }

            break;

        case 18:
            read_buffer_gpsd();
            steps_after_save = true;
            step_index = 0;
            break;

    }

    if (step_index_by_last_save == step_index) {
        if (steps_after_save) {
            all_steps_done = true;
            return 0;
        }
    }

    return 0;
}

int AtcGpsInfo::save_to_file() {

    if (steps_after_save == false) {
        return 0;
    }

    int timespec_size = sizeof (timespec);
    int aw_len_1 = 0;
    int ar_len_1 = 0;
    int int_size = sizeof (int);

    bool save_all = true;

    AT_Set *ats;

    char *cp;

    for (std::list<AT_Set *>::iterator it = AT_set_List.begin(); it != AT_set_List.end(); it++) {

        ats = *it;

        if (ats->new_readed == true &&
                (ats->readed_write != ats->readed_saved ||
                0 != strcmp(ats->ar_write, ats->ar_saved) ||
                save_all)
                ) {

            if (write(File_Deskriptor_atc, &(ats->time_write), timespec_size) != timespec_size) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_atc, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            if (write(File_Deskriptor_atc, &(ats->time_read_start), timespec_size) != timespec_size) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_atc, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            if (write(File_Deskriptor_atc, &(ats->time_read_ende), timespec_size) != timespec_size) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_atc, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            //            char ca[1024];
            //            timespec2str(ca, 1023, &(ats->time));
            //            printf("%s\n", ca);

            aw_len_1 = ats->aw_len + 1;
            if (write(File_Deskriptor_atc, ats->aw, aw_len_1) != aw_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_atc, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            //            printf("%s\n", ats->aw);

            if (write(File_Deskriptor_atc, &(ats->count_read), int_size) != int_size) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_atc, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            ar_len_1 = ats->readed_write + 1;
            if (write(File_Deskriptor_atc, ats->ar_write, ar_len_1) != ar_len_1) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_atc, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

            //            printf("%s\n", ats->ar_write);

            ar_len_1 = ats->readed_saved;
            ats->readed_saved = ats->readed_write;
            ats->readed_write = ar_len_1;

            cp = ats->ar_saved;
            ats->ar_saved = ats->ar_write;
            ats->ar_write = cp;

        }
    }

    AT_set_List.clear();


    if (write(File_Deskriptor_gps, buffer_pgsd, buffer_pgsd_readed) != buffer_pgsd_readed) {
        printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_gps, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    buffer_pgsd_readed = 0;

    all_steps_done = false;
    steps_after_save = false;
    step_index_by_last_save = step_index;

}

int AtcGpsInfo::read_buffer_gpsd() {

    int size = recv(socket_gpsd, &buffer_pgsd[buffer_pgsd_readed], BufferSize - buffer_pgsd_readed, 0);

    if (size < 0) {
        if (errno != 11) {
            printf("ERROR:\n  Fehler beim beim Empfangen vom gpsd Socket \n(%s)\n", strerror(errno));
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
    } else {
        buffer_pgsd_readed += size;
    }

}
