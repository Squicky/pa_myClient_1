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

    char filenamecsv[1024];
    filenamecsv[0] = 0;
    strncat(filenamecsv, filename, 1024);
    strncat(filenamecsv, "_.csv", 1024);
    if ((File_Deskriptor_csv = open(filenamecsv, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG, S_IRWXO)) == -1) {
        printf("ERROR:\n  Fehler beim Öffnen / Erstellen der Datei \"%s\" \n(%s)\n ", filename, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    // O_WRONLY nur zum Schreiben öffnen
    // O_RDWR zum Lesen und Schreiben öffnen
    // O_RDONLY nur zum Lesen öffnen
    // O_CREAT Falls die Datei nicht existiert, wird sie neu angelegt. Falls die Datei existiert, ist O_CREAT ohne Wirkung.
    // O_APPEND Datei öffnen zum Schreiben am Ende
    // O_EXCL O_EXCL kombiniert mit O_CREAT bedeutet, dass die Datei nicht geöffnet werden kann, wenn sie bereits existiert und open() den Wert –1 zurückliefert (–1 == Fehler).
    // O_TRUNC Eine Datei, die zum Schreiben geöffnet wird, wird geleert. Darauffolgendes Schreiben bewirkt erneutes Beschreiben der Datei von Anfang an. Die Attribute der Datei bleiben erhalten.
    if ((File_Deskriptor = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG, S_IRWXO)) == -1) {
        printf("ERROR:\n  Fehler beim Öffnen / Erstellen der Datei \"%s\" \n(%s)\n ", filename, strerror(errno));
        fflush(stdout);
        exit(EXIT_FAILURE);
    } else {

        printf("Datei \"%s\" erstellt & geöffnet \n ", filename);

        goto SchreibFehlerUeberspringen;

SchreibFehler:
        printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n ", filename, strerror(errno));
        fflush(stdout);
        //        exit(EXIT_FAILURE);

SchreibFehlerUeberspringen:


        FILE *f_csv = fdopen(File_Deskriptor_csv, "w");
        fprintf(f_csv, "train_id;train_send_countid;paket_id;count_pakets_in_train;recv_data_rate;recv_timeout_wait;last_recv_train_id;last_recv_train_send_countid;last_recv_paket_id;recv_time;send_time\n\n\n");
        fflush(f_csv);


        FILE *f = fdopen(File_Deskriptor, "w");
        fprintf(f, "train_id;train_send_countid;paket_id;count_pakets_in_train;recv_data_rate;recv_timeout_wait;last_recv_train_id;last_recv_train_send_countid;last_recv_paket_id;recv_time;send_time\n\n\n");
        fflush(f);
        /*        
                char c = 10;
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;

                c = 0;

                char puffer0[] = "train_id_int";
                char *puffer = puffer0;
                if (write(File_Deskriptor, puffer, strlen(puffer)) != strlen(puffer)) goto SchreibFehler;
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;

                char puffer1[] = "train_send_countid_int";
                puffer = puffer1;
                if (write(File_Deskriptor, puffer, strlen(puffer)) != strlen(puffer)) goto SchreibFehler;
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;

                char puffer2[] = "paket_id_int";
                puffer = puffer2;
                if (write(File_Deskriptor, puffer, strlen(puffer)) != strlen(puffer)) goto SchreibFehler;
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;

                char puffer3[] = "count_pakets_in_train_int";
                puffer = puffer3;
                if (write(File_Deskriptor, puffer, strlen(puffer)) != strlen(puffer)) goto SchreibFehler;
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;

                char puffer4[] = "recv_time_timespec";
                puffer = puffer4;
                if (write(File_Deskriptor, puffer, strlen(puffer)) != strlen(puffer)) goto SchreibFehler;
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;

                char puffer5[] = "send_time_timespec";
                puffer = puffer5;
                if (write(File_Deskriptor, puffer, strlen(puffer)) != strlen(puffer)) goto SchreibFehler;
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;

                char puffer6[] = "recv_data_rate_int";
                puffer = puffer6;
                if (write(File_Deskriptor, puffer, strlen(puffer)) != strlen(puffer)) goto SchreibFehler;
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;

                char puffer7[] = "last_recv_train_id_int";
                puffer = puffer7;
                if (write(File_Deskriptor, puffer, strlen(puffer)) != strlen(puffer)) goto SchreibFehler;
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;

                char puffer8[] = "last_recv_train_send_countid_int";
                puffer = puffer8;
                if (write(File_Deskriptor, puffer, strlen(puffer)) != strlen(puffer)) goto SchreibFehler;
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;

                char puffer9[] = "last_recv_paket_id_int";
                puffer = puffer9;
                if (write(File_Deskriptor, puffer, strlen(puffer)) != strlen(puffer)) goto SchreibFehler;
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;

                c = '\n';
                if (write(File_Deskriptor, &c, 1) != 1) goto SchreibFehler;
         * */


        log_file_ok = true;
    }
}

ListArrayClass::~ListArrayClass() {
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

void ListArrayClass::save_to_file_and_clear() {

    ListArrayClass *lac;

    FILE *f = fdopen(File_Deskriptor, "w");

    FILE *f_csv = fdopen(File_Deskriptor_csv, "w");

    for (lac = this; lac != NULL; lac = lac->nextListArrayClass) {
        if (lac->array_paket_header != NULL) {

            for (int i = 0; i < lac->count_paket_headers; i++) {
                
                fprintf(f_csv, "%d;%d;%d;%d;%d;%d;%d;%d;%d;%ld.%.9ld;%ld.%.9ld;;;\n",
                        lac->array_paket_header[i].train_id,
                        lac->array_paket_header[i].train_send_countid,
                        lac->array_paket_header[i].paket_id,
                        lac->array_paket_header[i].count_pakets_in_train,
                        lac->array_paket_header[i].recv_data_rate,
                        lac->array_paket_header[i].recv_timeout_wait,
                        lac->array_paket_header[i].last_recv_train_id,
                        lac->array_paket_header[i].last_recv_train_send_countid,
                        lac->array_paket_header[i].last_recv_paket_id,
                        lac->array_paket_header[i].recv_time.tv_sec,
                        lac->array_paket_header[i].recv_time.tv_nsec,
                        lac->array_paket_header[i].send_time.tv_sec,
                        lac->array_paket_header[i].send_time.tv_nsec
                        );
                
                //fflush(f_csv);
                
            }


            int bytezahl = lac->count_paket_headers * lac->paket_header_size;
            if (bytezahl != write(File_Deskriptor, lac->array_paket_header, bytezahl)) {
                printf("ERROR:\n  Fehler beim Schreiben der Datei \"%s\" \n(%s)\n ", lac->filename, strerror(errno));
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

paket_header *ListArrayClass::give_paket_header(int train_id, int train_send_countid, int paket_id) {
    int count_paket_header_in_this_array;
    int i;

    if (count_paket_headers < count_paket_header_in_one_array) {
        count_paket_header_in_this_array = count_paket_headers;
    } else {
        count_paket_header_in_this_array = count_paket_header_in_one_array;
    }

    for (i = 0; i < count_paket_header_in_this_array; i++) {
        if (array_paket_header[i].train_id == train_id &&
                array_paket_header[i].train_send_countid == train_send_countid &&
                array_paket_header[i].paket_id == paket_id) {

            return &array_paket_header[i];
        }
    }

    if (nextListArrayClass != NULL) {
        return nextListArrayClass->give_paket_header(train_id, train_send_countid, paket_id);
    }

    return NULL;
}


