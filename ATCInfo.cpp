/* 
 * File:   ATCInfo.cpp
 * Author: user
 * 
 * Created on 16. Oktober 2014, 11:54
 */

#include "ATCInfo.h"
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

ATCInfo::ATCInfo() {

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

    filename = (char*) malloc(filename_len + 1);

    strncpy(filename, ATC_FILENAME, filename_len);

    // Geraetedatei oeffnen
    fd = open(filename, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        printf("ERROR:\n  Geraetedatei \"%s\" nicht oeffnen!!\n(%s)\n", filename, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    } else {
        // Prüfen ob Gerätedatei serielle Schnittstelle ist
        if (isatty(fd) == 0) {
            printf("ERROR:\n  \"%s\" ist keine tty\n", filename);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }

        fcntl(fd, F_SETFL, FNDELAY); // lesen blockiert nicht
        //fcntl(fd, F_SETFL, 0); // lesen blockiert

        struct termios options;

        /* Attribute des tty erhalten */
        tcgetattr(fd, &options);

        /* Timeout der Leseblockierung setzen */
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_oflag &= ~OPOST;
        options.c_cc[VMIN] = 0;

        unsigned char timeout = 1;
        options.c_cc[VTIME] = timeout;

        /* Attribute speichern */
        tcsetattr(fd, TCSANOW, &options);

        int a = myWrite("AT\r", 3);

        a = myRead(buffer, BufferSize);

        if (a != 0) {
            printf("ERROR:\n \"%s\" hat auf AT nicht mit \"OK\" geantwortet.\n  \"%s\" geantwortet \n", filename, buffer);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }

    }

}


//ATCInfo::ATCInfo(const ATCInfo& orig) {}

ATCInfo::~ATCInfo() {
}

int ATCInfo::myWrite(const char s [], int len) {
    int count_written_bytes = write(fd, s, len);
    if (len != count_written_bytes) {
        printf("ERROR:\n  Write %i Bytes: \"%s\" to \"%s\" \n", count_written_bytes, s, filename);
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
int ATCInfo::myRead(char buffer [], int len) {

    int count_readed_bytes = 0;

    int count_read = 0;
    int max_try_read = 5555;
    int new_count_readed_bytes = read(fd, buffer, len - 1);

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
        new_count_readed_bytes = read(fd, &(buffer[count_readed_bytes]), len - 1 - count_readed_bytes);

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
int ATCInfo::endswithOK(const char s [], int len) {
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
int ATCInfo::endswithERROR(const char s [], int len) {
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

int ATCInfo::get_Network_technology_currently_in_use(char s[], int len) {
    myWrite("AT*CNTI=0\r\n", 11);

    return myRead(s, len);
}

int ATCInfo::get_Available_technologies_on_current_network(char s[], int len) {
    myWrite("AT*CNTI=1\r\n", 11);

    return myRead(s, len);
}

int ATCInfo::get_Operational_status(char s[], int len) {
    myWrite("AT!GSTATUS?\r\n", 13);

    return myRead(s, len);
}

int ATCInfo::get_Current_active_radio_access_technology(char s[], int len) {
    myWrite("AT!GETRAT?\r\n", 12);

    return myRead(s, len);
}

int ATCInfo::get_Current_service_domain(char s[], int len) {
    myWrite("AT!SELMODE?\r\n", 13);

    return myRead(s, len);
}

int ATCInfo::get_Signal_Quality(char s[], int len) {
    myWrite("AT+CSQ\r\n", 8);

    return myRead(s, len);
}

int ATCInfo::get_WCDMA_Active_Set(char s[], int len) {
    myWrite("AT+USET?0\r\n", 11);

    return myRead(s, len);
}

int ATCInfo::get_WCDMA_Sync_Neighbour(char s[], int len) {
    myWrite("AT+USET?1\r\n", 11);

    return myRead(s, len);
}

int ATCInfo::get_WCDMA_Async_Neighbour(char s[], int len) {
    myWrite("AT+USET?2\r\n", 11);

    return myRead(s, len);
}

