/* 
 * File:   ClientClass.h
 * Author: user
 *
 * Created on 17. August 2014, 14:56
 * test
 */

#ifndef CLIENTCLASS_H
#define	CLIENTCLASS_H

#include "ClientBenchmarkClass.h"

#define Remote_Control_SERVER_PORT 5000
#define LOCAL_Control_SERVER_PORT 6000
#define PAKETSIZE 1400

class ClientClass {
public:
    ClientClass();
    ClientClass(const ClientClass& orig);
    virtual ~ClientClass();
    
private:
    ClientBenchmarkClass *clientBenchmarkClass;
    
    char *server_ip;
};

#endif	/* CLIENTCLASS_H */

