/* 
 * File:   ListArrayClass.cpp
 * Author: user
 * 
 * Created on 28. August 2014, 11:47
 */

#include "ListArrayClass.h"
#include <stdlib.h>
#include <string.h>

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
}

ListArrayClass::~ListArrayClass() {
    free(array_paket_header);
    if (nextListArrayClass != NULL) {
        free(this->nextListArrayClass);
        //        nextListArrayClass->~ListArrayClass();
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
    count_paket_headers = 0;

    first_paket_header = NULL;
    last_paket_header = NULL;
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

    for (i=0; i<count_paket_header_in_this_array; i++) {
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


