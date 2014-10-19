/* 
 * File:   ATCInfo.h
 * Author: user
 *
 * Created on 16. Oktober 2014, 11:54
 */

#include <vector>
#include <string.h>

#ifndef ATCINFO_H
#define	ATCINFO_H

#define BufferSize 1024

#define DeviceValueType_string 0
#define DeviceValueType_float 1
#define DeviceValueType_defloat 2
#define DeviceValueType_int32 3
#define DeviceValueType_multidataset 4
#define DeviceValueType_octalint32 5
#define DeviceValueType_hexint32 6

class MultiDataSetClass;

struct EndsignalData {
    char *cp;
    int size;
};

class ATCInfo {
public:
    ATCInfo();
    //    ATCInfo(const ATCInfo& orig);
    virtual ~ATCInfo();

    int get_Network_technology_currently_in_use(char s[], int len);
    int get_Available_technologies_on_current_network(char s[], int len);
    int get_Operational_status(char s[], int len);
    int get_Current_active_radio_access_technology(char s[], int len);
    int get_Current_service_domain(char s[], int len);
    int get_Signal_Quality(char s[], int len);
    int get_WCDMA_Active_Set(char s[], int len);
    int get_WCDMA_Sync_Neighbour(char s[], int len);
    int get_WCDMA_Async_Neighbour(char s[], int len);
private:
    char *filename;
    int fd; // filedescriptor
    char buffer[BufferSize];

    std::vector<EndsignalData> EndSignalOkList;
    std::vector<EndsignalData> EndSignalErrorList;

    int endswithOK(const char s [], int len);
    int endswithERROR(const char s [], int len);

    int myWrite(const char s [], int len);
    int myRead(char buffer [], int len);

};

#endif	/* ATCINFO_H */

