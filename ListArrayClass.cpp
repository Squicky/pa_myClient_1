/* 
 * File:   ListArrayClass.cpp
 * Author: user
 * 
 * Created on 28. August 2014, 11:47
 */

#include "ListArrayClass.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

ListArrayClass::ListArrayClass(int _mess_paket_size) {

    mess_paket_size = _mess_paket_size;
    count_arrays = 1;
    count_paket_headers = 0;

    nextListArrayClass = NULL;
    first_paket_header = NULL;
    last_paket_header = NULL;

    count_paket_header_in_one_array = MAX_UMTS_DATA_RATE / _mess_paket_size;
    last_index_of_paket_header_in_one_array = count_paket_header_in_one_array - 1;
    paket_header_size = sizeof (paket_header);
    int array_paket_header_size = count_paket_header_in_one_array * paket_header_size;

    array_paket_header = (paket_header*) malloc(array_paket_header_size);
    log_file_ok = false;
}

ListArrayClass::ListArrayClass(int _mess_paket_size, char *_filename) {

    mess_paket_size = _mess_paket_size;
    count_arrays = 1;
    count_paket_headers = 0;

    File_Deskriptor = 0;
    file_csv = NULL;

    strncpy(filename, _filename, strlen(_filename));

    nextListArrayClass = NULL;
    first_paket_header = NULL;
    last_paket_header = NULL;

    count_paket_header_in_one_array = MAX_UMTS_DATA_RATE / _mess_paket_size;
    last_index_of_paket_header_in_one_array = count_paket_header_in_one_array - 1;
    paket_header_size = sizeof (paket_header);
    int array_paket_header_size = count_paket_header_in_one_array * paket_header_size;

    array_paket_header = (paket_header*) malloc(array_paket_header_size);

    log_file_ok = false;

    char filename_csv[1024];
    filename_csv[0] = 0;
    strncat(filename_csv, filename, 1024);
    strncat(filename_csv, "_.csv", 1024);

    // O_WRONLY nur zum Schreiben oeffnen
    // O_RDWR zum Lesen und Schreiben oeffnen
    // O_RDONLY nur zum Lesen oeffnen
    // O_CREAT Falls die Datei nicht existiert, wird sie neu angelegt. Falls die Datei existiert, ist O_CREAT ohne Wirkung.
    // O_APPEND Datei oeffnen zum Schreiben am Ende
    // O_EXCL O_EXCL kombiniert mit O_CREAT bedeutet, dass die Datei nicht geoeffnet werden kann, wenn sie bereits existiert und open() den Wert 1 zurueckliefert (1 == Fehler).
    // O_TRUNC Eine Datei, die zum Schreiben geoeffnet wird, wird geleert. Darauffolgendes Schreiben bewirkt erneutes Beschreiben der Datei von Anfang an. Die Attribute der Datei bleiben erhalten.
    File_Deskriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG, S_IRWXO);
    if (File_Deskriptor == -1) {
        printf("ERROR:\n  Fehler beim oeffnen / Erstellen der Datei \"%s\" \n(%s)\n", filename, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    printf("Datei \"%s\" erstellt & geoeffnet \n", filename);

    char firstlines[] = "train_id;retransfer_train_id;paket_id;count_pakets_in_train;recv_data_rate;first_recv_train_id;first_recv_retransfer_train_id;first_recv_paket_id;first_recv_recv_time;last_recv_paket_bytes;timeout_time_tv_sec;timeout_time_tv_usec;recv_time;send_time;rtt\n\n\n";
    int firstlines_len = strlen(firstlines);

    /*
    file_csv = fopen(filename_csv, "w");
    if (file_csv == NULL) {
        printf("ERROR:\n  Fehler beim oeffnen / Erstellen der Datei \"%s\" \n(%s)\n", filename_csv, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
    printf("Datei \"%s\" erstellt & geoeffnet \n", filename);
     * */

    if (write(File_Deskriptor, firstlines, firstlines_len) != firstlines_len) {
        printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if (file_csv != NULL) {
        if (fprintf(file_csv, "%s", firstlines) != firstlines_len) {
            printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename_csv, strerror(errno));
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
        fflush(file_csv);
    }

    log_file_ok = true;

}

ListArrayClass::~ListArrayClass() {
    if (File_Deskriptor != 0) {
        close(File_Deskriptor);
    }

    if (file_csv != NULL) {
        fclose(file_csv);
    }

    free(array_paket_header);
    array_paket_header = NULL;
    if (nextListArrayClass != NULL) {
        free(nextListArrayClass);
        nextListArrayClass = NULL;
    }
}

paket_header *ListArrayClass::copy_paket_header(struct paket_header *ph) {

    if (count_paket_headers < count_paket_header_in_one_array) {
        if (0 == count_paket_headers) {
            first_paket_header = &array_paket_header[count_paket_headers];
            last_paket_header = first_paket_header;

            memcpy(first_paket_header, ph, paket_header_size);

            count_paket_headers++;
            return first_paket_header;
        } else {
            last_paket_header = &array_paket_header[count_paket_headers];

            memcpy(last_paket_header, ph, paket_header_size);

            count_paket_headers++;
            return last_paket_header;
        }
    } else {
        if (this->nextListArrayClass == NULL) {
            nextListArrayClass = new ListArrayClass(mess_paket_size);
        }
        count_paket_headers++;
        last_paket_header = nextListArrayClass->copy_paket_header(ph);
        return last_paket_header;
    }

}

int timespec2str(char *buf, uint len, struct timespec *ts) {
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

void ListArrayClass::save_to_file_and_clear() {
    ListArrayClass *lac;

    //const uint timestr_size = strlen("2014-12-31 12:59:59.123456789") + 1;
    const uint timestr_size = 30;
    char timestr1[timestr_size];
    char timestr2[timestr_size];

    for (lac = this; lac != NULL; lac = lac->nextListArrayClass) {
        if (lac->array_paket_header != NULL) {

            int min;
            if (lac->count_paket_header_in_one_array < lac->count_paket_headers) {
                min = lac->count_paket_header_in_one_array;
            } else {
                min = lac->count_paket_headers;
            }

            if (file_csv != NULL) {
                for (int i = 0; i < min; i++) {

                    timespec2str(timestr1, timestr_size, &lac->array_paket_header[i].recv_time);
                    timespec2str(timestr2, timestr_size, &lac->array_paket_header[i].send_time);

                    fprintf(file_csv, "%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%d;%s;%s;%f\n",
                            lac->array_paket_header[i].train_id,
                            lac->array_paket_header[i].retransfer_train_id,
                            lac->array_paket_header[i].paket_id,
                            lac->array_paket_header[i].retransfer_train_id,
                            lac->array_paket_header[i].recv_data_rate,
                            lac->array_paket_header[i].first_recv_train_id,
                            lac->array_paket_header[i].first_recv_retransfer_train_id,
                            lac->array_paket_header[i].first_recv_paket_id,
                            lac->array_paket_header[i].last_recv_paket_bytes,
                            lac->array_paket_header[i].timeout_time_tv_sec,
                            lac->array_paket_header[i].timeout_time_tv_usec,
                            timestr1,
                            timestr2,
                            lac->array_paket_header[i].rtt
                            );

                    fflush(file_csv);
                }
            }

            int bytezahl = min * lac->paket_header_size;
            if (bytezahl != write(File_Deskriptor, lac->array_paket_header, bytezahl)) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n", filename, strerror(errno));
                fflush(stdout);
                exit(EXIT_FAILURE);
            }

        }
    }

    count_paket_headers = 0;

    first_paket_header = NULL;
    last_paket_header = NULL;

    if (nextListArrayClass != NULL) {
        return nextListArrayClass->clear();
    }
}

void ListArrayClass::clear() {
    count_paket_headers = 0;

    first_paket_header = NULL;
    last_paket_header = NULL;

    if (nextListArrayClass != NULL) {
        return nextListArrayClass->clear();
    }
}

paket_header *ListArrayClass::give_paket_header(int index) {
    if (index < count_paket_headers) {
        if (index < count_paket_header_in_one_array) {
            return &array_paket_header[index];
        } else {
            return (nextListArrayClass->give_paket_header(index));
        }
    }

    return NULL;
}

paket_header *ListArrayClass::give_paket_header(int train_id, int retransfer_train_id, int paket_id) {
    int count_paket_header_in_this_array;
    int i;

    if (count_paket_headers < count_paket_header_in_one_array) {
        count_paket_header_in_this_array = count_paket_headers;
    } else {
        count_paket_header_in_this_array = count_paket_header_in_one_array;
    }

    for (i = count_paket_header_in_this_array - 1; 0 <= i; i--) {

        if (array_paket_header[i].train_id == train_id &&
                array_paket_header[i].retransfer_train_id == retransfer_train_id &&
                array_paket_header[i].paket_id == paket_id) {

            return &array_paket_header[i];
        }
    }

    if (nextListArrayClass != NULL) {
        return nextListArrayClass->give_paket_header(train_id, retransfer_train_id, paket_id);
    }

    return NULL;
}

