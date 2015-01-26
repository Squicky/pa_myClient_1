/**
 * Publish-Subscribe Modem-Informations-Service
 * 
 * @author Adrian Skuballa
 */

#ifndef SERIALPORT_H
#define	SERIALPORT_H

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <list>
#include <vector>

#define BufferSize 1024

#define SerialPortStatus_OK 0
#define SerialPortStatus_Init 1
#define SerialPortStatus_OpenError 2
#define SerialPortStatus_isattyError 3
#define SerialPortStatus_InitError 4
#define SerialPortStatus_WriteError 5
#define SerialPortStatus_ReadError 6
#define SerialPortStatus_NoConfig 7
#define SerialPortStatus_Close 8

#define SerialPortATCommandStatus_OK 0
#define SerialPortATCommandStatus_Error 1
#define SerialPortATCommandStatus_Timeout 2
#define SerialPortATCommandStatus_DeviceError 3
#define SerialPortATCommandStatus_ParseError 4
#define SerialPortATCommandStatus_Init 5
#define SerialPortATCommandStatus_AnswerOK 6

#define TaskDataType_ATCommand 0
#define TaskDataType_ATDeviceValue 1
#define TaskDataType_ATDeviceConfig 2
#define TaskDataType_PeriodicallyTask 3
#define TaskDataType_addPeriodicallyDataRequest 4
#define TaskDataType_removePeriodicallyDataRequest 5


class SerialPortTaskDataClass {
public:
    SerialPortTaskDataClass();
    virtual ~SerialPortTaskDataClass();

    struct RemovePDR *removePDR;
    uint deviceValueIndex;
    int type;
    char *atcommand;
    int atcommandLength;
private:
};

struct writereaddata {
    char *write;
    int writeSize;
    std::string read;
    int status;
    unsigned long long timestemp;
    std::vector<char*> *answerVe; // necessary answer for correctly config AT-Command
    std::vector<int> *answerSizeVe;
};

class SerialPortClass {
public:
    SerialPortClass(MISClass *_mis, const char* _filename, unsigned char timeout);
    virtual ~SerialPortClass();
    
    void startSerialPortThread();
    void addTask(struct SerialPortTaskDataClass *task);
    pthread_mutex_t mutex_TaskData;
    pthread_cond_t condition_TaskData;
    ThreadClass *SerialPortThreadPo;
    int status;
    unsigned long long nextPriorityTaskTimeStemp;
    uint index;
    DeviceConfigClass *DeviceConfigPo;
    PriorityClass *PriorityPo;
    SerialPortTaskDataClass *TaskDataPriority;
    int fd; // filedescriptor
    char *getInfo();
    char *filename;
private:
    char *info;
    char buffer[BufferSize];
    static void * threadStart(void* par);
    std::list<SerialPortTaskDataClass *> TaskList;
    std::vector<char *> ReadBufferList;
    unsigned long baudrate_wert(speed_t baud);
    void writeread(writereaddata *data);
    int parseData(writereaddata *wrdata, union value *v, ATCoValueClass *atCommandValue, int *vssize);
    int sendParsedDataToClient(unsigned long long *TimeStemp, SerialPortTaskDataClass *TaskData, value *v,
            int ValueType, int vssize, unsigned long long *WorkTime);
    int sendParsingErrorToClient(int parseDataReturn, writereaddata *wrdata, SerialPortTaskDataClass *TaskData,
            int atCommandValueType);
    int sendPeriodicallyDataToClients(unsigned long long *TimeStemp, SerialPortTaskDataClass *TaskData,
            value *v, int ValueType, int vssize, int i_atcv_array);
    int sendPeriodicallyErrorToClients(int parseDataReturn, writereaddata *wrdata,
            SerialPortTaskDataClass *TaskData, int atCommandValueType, int i_atcv_array);
    struct timespec nexttask_timespec;
    struct timeval nexttask_timeval;
    int defaultConfig_flag;
    void threadRun();
    MISClass *mis;
    int endswithOK(const char s [], int len);
    int endswithERROR(const char s [], int len);
};

#endif	/* SERIALPORT_H */

