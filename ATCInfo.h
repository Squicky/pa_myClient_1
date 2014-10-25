/* 
 * File:   ATCInfo.h
 * Author: user
 *
 * Created on 16. Oktober 2014, 11:54
 */

#include <vector>
#include <string.h>
#include <time.h>

#ifndef ATCINFO_H
#define	ATCINFO_H

#define BufferSize 10240

#define DeviceValueType_string 0
#define DeviceValueType_float 1
#define DeviceValueType_defloat 2
#define DeviceValueType_int32 3
#define DeviceValueType_multidataset 4
#define DeviceValueType_octalint32 5
#define DeviceValueType_hexint32 6

#define awNetwork_technology_currently_in_use "AT*CNTI=0\r\n"
#define awNetwork_technology_currently_in_use_len 11
#define awAvailable_technologies_on_current_network "AT*CNTI=1\r\n"
#define awAvailable_technologies_on_current_network_len 11
#define awOperational_status "AT!GSTATUS?\r\n"
#define awOperational_status_len 13
#define awCurrent_active_radio_access_technology "AT!GETRAT?\r\n"
#define awCurrent_active_radio_access_technology_len 12
#define awCurrent_service_domain "AT!SELMODE?\r\n"
#define awCurrent_service_domain_len 13
#define awSignal_Quality "AT+CSQ\r\n"
#define awSignal_Quality_len 8
#define awWCDMA_Active_Set "AT+USET?0\r\n"
#define awWCDMA_Active_Set_len 11
#define awWCDMA_Sync_Neighbour "AT+USET?1\r\n"
#define awWCDMA_Sync_Neighbour_len 11
#define awWCDMA_Async_Neighbour "AT+USET?2\r\n"
#define awWCDMA_Async_Neighbour_len 11

struct AT_Set {
    char arNetwork_technology_currently_in_use[BufferSize];
    char arAvailable_technologies_on_current_network[BufferSize];
    char arOperational_status[BufferSize];
    char arCurrent_active_radio_access_technology[BufferSize];
    char arCurrent_service_domain[BufferSize];
    char arSignal_Quality[BufferSize];
    char arWCDMA_Active_Set[BufferSize];
    char arWCDMA_Sync_Neighbour[BufferSize];
    char arWCDMA_Async_Neighbour[BufferSize];

    int readedNetwork_technology_currently_in_use;
    int readedAvailable_technologies_on_current_network;
    int readedOperational_status;
    int readedCurrent_active_radio_access_technology;
    int readedCurrent_service_domain;
    int readedSignal_Quality;
    int readedWCDMA_Active_Set;
    int readedWCDMA_Sync_Neighbour;
    int readedWCDMA_Async_Neighbour;

    struct timespec timeNetwork_technology_currently_in_use;
    struct timespec timeAvailable_technologies_on_current_network;
    struct timespec timeOperational_status;
    struct timespec timeCurrent_active_radio_access_technology;
    struct timespec timeCurrent_service_domain;
    struct timespec timeSignal_Quality;
    struct timespec timeWCDMA_Active_Set;
    struct timespec timeWCDMA_Sync_Neighbour;
    struct timespec timeWCDMA_Async_Neighbour;
};

class MultiDataSetClass;

struct EndsignalData {
    char *cp;
    int size;
};

class ATCInfo {
public:
    ATCInfo(char *_zeit_dateiname);
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

    int save_to_file();

    int do_step();
    int train_id;
    int retransfer_train_id;
    int step_index;
    int last_step_index;
    
    struct AT_Set *AT_Set_new;
    struct AT_Set *AT_Set_old;
private:
    int File_Deskriptor_tty; // filedescriptor
    char *filename_tty;
    char buffer[BufferSize];

    std::vector<EndsignalData> EndSignalOkList;
    std::vector<EndsignalData> EndSignalErrorList;

    int endswithOK(const char s [], int len);
    int endswithERROR(const char s [], int len);

    int myWrite(const char s [], int len);
    int myRead(char buffer [], int len);
    int timespec2str(char *buf, int len, struct timespec *ts);
    int myReadOne(char buffer [], int len, int *readed, struct timespec *time);

    int count_myReadOne;
    
    int File_Deskriptor_file;
    char filename_file[1024];    
};

#endif	/* ATCINFO_H */

