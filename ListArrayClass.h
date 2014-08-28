/* 
 * File:   ListArrayClass.h
 * Author: user
 *
 * Created on 28. August 2014, 11:47
 */

#ifndef LISTARRAYCLASS_H
#define	LISTARRAYCLASS_H


#include "ServerClientInfo.h"

class ListArrayClass {
public:
    ListArrayClass(int _mess_paket_size);
    virtual ~ListArrayClass();

    int count_paket_headers;

    unsigned int count_paket_header_in_one_array;
    unsigned int last_index_of_paket_header_in_one_array;
    unsigned int count_arrays;

    paket_header *copy_paket_header(struct paket_header *ph);
    struct paket_header *first_paket_header;
    struct paket_header *last_paket_header;
    
    void save_to_file_and_clear();

private:

    struct paket_header *array_paket_header;
    ListArrayClass *nextListArrayClass;
    int mess_paket_size;
    int paket_header_size;
};

#endif	/* LISTARRAYCLASS_H */

