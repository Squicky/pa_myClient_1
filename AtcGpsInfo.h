/* 
 * File:   ATCInfo.h
 * Author: user
 *
 * Created on 16. Oktober 2014, 11:54
 */

#include <vector>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <list>

#ifndef ATCINFO_H
#define	ATCINFO_H

#define BufferSize 102400

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

struct AT_Set;

struct AT_Sets;

struct EndsignalData;


class AtcGpsInfo {
public:
    AtcGpsInfo(char *_zeit_dateiname);
    //    ATCInfo(const ATCInfo& orig);
    virtual ~AtcGpsInfo();

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
    int myReadOne(AT_Set *ats, int len);

    int train_id;
    int retransfer_train_id;
    int step_index;
    int last_step_index;

    struct AT_Sets *AT_Sets_v;
    
    bool all_steps_done;
    bool steps_after_save;
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
    
    int read_buffer_gpsd();

    int File_Deskriptor_atc;
    char filename_atc[1024];

    struct sockaddr_in address_gpsd;   
    int socket_gpsd;
    char buffer_pgsd[BufferSize];
    int buffer_pgsd_readed;
    int File_Deskriptor_gps;
    char filename_gps[1024];
    
    int step_index_by_last_save;
    
    std::list<AT_Set *> AT_set_List;
};

struct AT_Set {
    char *ar_saved;
    int readed_saved;

    char *ar_write;
    int readed_write;
    
    char *aw;
    int aw_len;
    
    struct timespec time_write;
    struct timespec time_read_start;
    struct timespec time_read_ende;

    bool new_readed;
    
    int count_read;
};

struct AT_Sets {
    AT_Set Network_technology_currently_in_use;
    AT_Set Available_technologies_on_current_network;
    AT_Set Operational_status;
    AT_Set Current_active_radio_access_technology;
    AT_Set Current_service_domain;
    AT_Set Signal_Quality;
    AT_Set WCDMA_Active_Set;
    AT_Set WCDMA_Sync_Neighbour;
    AT_Set WCDMA_Async_Neighbour;
};

struct EndsignalData {
    char *cp;
    int size;
};

#endif	/* ATCINFO_H */

