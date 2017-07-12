/****************************************************************
Monitor program to read BMP180 Temperature/Pressure Sensor values 
and control the GPIO pins accordingly for MINIX using IPC
****************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/cdefs.h>
#include <lib.h>
#include <stdlib.h>
#include <strings.h>
#include "msg.h"

#define GPIO_UPDATE 1
#define GPIO_RED 4
#define GPIO_GREEN 3
#define GPIO_BLUE 2
#define GPIO_FAN 5
#define GPIO_HEATER 6
#define GPIO_ON 1
#define GPIO_OFF 0

// message data structure (OS define type)
message m;

// tempControl process endpoint (OS defined type)
endpoint_t bmp_ep,gpio_ep;

// gpio process endpoint (OS defined type)
endpoint_t 

int i = 0;
int getendpoint_name(char *proc_name);

//variable to store the temperature value
int temperatureValue;

//variable to store the status of fan(0 - Off, 1 - On)
int fanStatus = 0;

//variable to store the pressure value
float pressureValue;
int serv_stat;

/* IPC send/receive message to BMP Driver process to send the
"UPDATE" message to receive the latest temperature/pressure values*/
int sendrecBMPDriver(int status){
        int r;
        memset(&m, 0, sizeof(m));
        m.m_source = getendpoint_name("tempSensor");
        m.m_type = SENSOR_UPDATE;
        m.m_m1.m1i1 = status;

        printf("\n ************************ \n");
        printf("MONITOR: sendBMPDriver: m_source: %d\nm_type: %d, value: %d\n", m.m_source,m.m_type, m.m_m1.m1i1);

        r = ipc_sendrec(bmp_ep, &m);
        printf("MONITOR :: Value received from BMP driver : m..m_m1.m1i1 : %d\n",m.m_m1.m1i1);
        return m.m_m1.m1i1;
        
        if (r != 0) {
                        printf("sendnrec()%d\n",r);
                }
}

// send GPIO commands to GPIO FS via IPC
void sendData(int gpionum, int gpiovalue){
        int r;
	gpio_ep = getendpoint_name("gpio");
        printf("MONITOR : endpoint of GPIO : %d\n",gpio_ep);
        memset(&m, 0, sizeof(m));
        m.m_type = GPIO_UPDATE;
        m.m_m1.m1i1 = gpionum;
        m.m_m1.m1i2 = gpiovalue;

        printf("MONITOR: sending signal to GPIO : m_type: %d, GPIO : %d value: %d\n", m.m_type, m.m_m1.m1i1, m.m_m1.m1i2);
        r = ipc_sendrec(gpio_ep, &m);
        if (r != 0) {
             printf("sendnrec()%d\n",r);
        }
}

/**************************************************************
                       main function                                                                                     *
***************************************************************/
void main(int argc, char **argv){
        char ch;
        int i = 0;
        memset(&m, 0, sizeof(m));
        bmp_ep = getendpoint_name("bmp085");
	printf("MONITOR: bmp_ep: %d\n", bmp_ep);
        while(1){
                temperatureValue = -1 * sendrecBMPDriver(0);
                printf("SENSOR : Temperature Value is : %d.%2d\n",temperatureValue/100,temperatureValue%100);
                printf("\n ************************ \n");
		if((temperatureValue/100) < 26){

			//Switching ON devices
		        sendData(GPIO_BLUE,GPIO_ON);
		        sendData(GPIO_HEATER,GPIO_ON);

			//Switching OFF devices
			sendData(GPIO_RED,GPIO_OFF);
		        sendData(GPIO_FAN,GPIO_OFF);
			sendData(GPIO_GREEN,GPIO_OFF);
		}
		else if((temperatureValue/100) > 28){

			//Switching ON devices
		        sendData(GPIO_RED,GPIO_ON);
		        sendData(GPIO_FAN,GPIO_ON);

			//Switching OFF devices
			sendData(GPIO_BLUE,GPIO_OFF);
		        sendData(GPIO_HEATER,GPIO_OFF);
			sendData(GPIO_GREEN,GPIO_OFF);
		}
		else{

			//Switching ON devices
		        sendData(GPIO_GREEN,GPIO_ON);

			//Switching OFF devices
			sendData(GPIO_HEATER,GPIO_OFF);
			sendData(GPIO_RED,GPIO_OFF);
		        sendData(GPIO_FAN,GPIO_OFF);
			sendData(GPIO_BLUE,GPIO_OFF);
		}
                sleep(3);
        }
        exit(1);
}
