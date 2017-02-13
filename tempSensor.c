/**************************************************************
*       Temperature Sensor Program                                                                *
*       by Daniel Wang                                                                                    *
***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include "msg.h"

// message data structure (OS define type)
message m;
// tempControl process endpoint (OS defined type)
endpoint_t tempCnt_ep,bmp_ep;
int i = 0;

//variable to store the temperature value
int temperatureValue;

//variable to store the status of fan(0 - Off, 1 - On)
int fanStatus = 0;

//variable to store the pressure value
float pressureValue;
int serv_stat;

// function for initialization
void initialize(){
        memset(&m, 0, sizeof(m));
        // setup tempControl endpoint
        tempCnt_ep = getendpoint_name("tempControl");
}

// simulate the process of retrieving data from sensor
void retrieveSensorData(){
        // fake sensor data
const int sensordata[] = {70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, 80, -1};

        if(sensordata[i] == -1)
                exit(1);

        m.m_type = SENSOR_UPDATE;

        /*send BMP sensor data by calling the bmpdata function*/
}

int sendDataToTempControl(){
        printf("SENSOR: NBsend currentTemp: m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);
        // uses non-blocking send
        ipc_sendnb(tempCnt_ep, &m);
}

// message BMP Driver process;
int sendrecBMPDriver(int status){
        int r;
        memset(&m, 0, sizeof(m));
        m.m_source = getendpoint_name("tempSensor");
        m.m_type = SENSOR_UPDATE;
        m.m_m1.m1i1 = status;

        printf("TEMPCONTROL: sendBMPDriver: m_type: %d, value: %d\n", m.m_type, m.m_m1.m1i1);

        r = ipc_sendrec(bmp_ep, &m);
        printf("tempSensor :: Value received from driver : m..m_m1.m1i1 : %d\n",m.m_m1.m1i1);
        return m.m_m1.m1i1;
        //if(m.m_type == SENSOR_UPDATE)
        //      temperatureValue = m.m_m1.m1i1;
        if (r != 0) {
                        printf("sendnrec()%d\n",r);
                }
}

int serviceup(){
        serv_stat = system("/bin/service up /service/bmp085 -label bmp085.3.77 -dev /dev/bmp085b3s77 -args 'bus=3 address=0x77'");
        return serv_stat;
}
/**************************************************************
*       main function                                                                                     *
***************************************************************/
void main(int argc, char **argv){
        char ch;
        int i = 0;
        //i = serviceup();
        //sleep(7);
        initialize();
        printf("SENSOR: tempCnt_ep: %d\n", tempCnt_ep);
        memset(&m, 0, sizeof(m));
        bmp_ep = getendpoint(347);
        printf("SENSOR: bmp_ep: %d\n", bmp_ep);
        while(1){
                temperatureValue = sendrecBMPDriver(0);
                printf("SENSOR : Temperature Value is : %d\n",temperatureValue);
                sleep(3);
        }
        exit(1);
}

