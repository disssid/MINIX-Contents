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
#define TEMP_THRESHOLD 27
monitor.c: 137 lines, 4033 characters
dhcp213-11.cs.ksu.edu# cat monitor.c
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
#define TEMP_THRESHOLD 27

// message data structure (OS define type)
message m;

// bmp and gpio process endpoints (OS defined type)
endpoint_t bmp_ep,gpio_ep;

int i = 0;
int getendpoint_name(char *proc_name);

//variable to store the temperature value
int temperatureValue;

//variable to store the pressure value
float pressureValue;

/* IPC send/receive message to BMP Driver process to send the
"UPDATE" message to receive the latest temperature/pressure values*/
int sendrecBMPDriver(int status){
        int r;
        memset(&m, 0, sizeof(m));
        m.m_type = SENSOR_UPDATE;
        m.m_m1.m1i1 = status;

        //printf("\n ************************ \n");
        //printf("MONITOR : sending signal to BMP\n");
        r = ipc_sendrec(bmp_ep, &m);
        //printf("MONITOR : Value received from BMP : m..m_m1.m1i1 : %d\n",m.m_m1.m1i1);
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

        printf("MONITOR : sending signal to GPIO : m_type: %d, GPIO : %d value: %d\n", m.m_type, m.m_m1.m1i1, m.m_m1.m1i2);
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

        //indicates which cycle is currently ON
        //1 - less temperature
        //2 - more temperature
        //3 - correct temperature
        int current= 0;

        memset(&m, 0, sizeof(m));
        bmp_ep = getendpoint_name("bmp085");
        printf("MONITOR: bmp_ep: %d\n", bmp_ep);
        while(1){
                temperatureValue = sendrecBMPDriver(0);
                printf("MONITOR : Temperature Value is : %d.%2d\n",temperatureValue/10,temperatureValue%10);
                printf("\n ************************ \n");
                if((temperatureValue/10) < TEMP_THRESHOLD && (current == 0 || current != 1)){

                        //Switching ON devices
                        sendData(GPIO_BLUE,GPIO_ON);
                        sendData(GPIO_HEATER,GPIO_ON);

                        //Switching OFF devices
                        sendData(GPIO_RED,GPIO_OFF);
                        sendData(GPIO_FAN,GPIO_OFF);
                        sendData(GPIO_GREEN,GPIO_OFF);

                        current = 1;
                }
                else if((temperatureValue/10) > TEMP_THRESHOLD && (current == 0 || current != 2)){

                        //Switching ON devices
                        sendData(GPIO_RED,GPIO_ON);
                        sendData(GPIO_FAN,GPIO_ON);

                        //Switching OFF devices
                        sendData(GPIO_BLUE,GPIO_OFF);
                        sendData(GPIO_HEATER,GPIO_OFF);
                        sendData(GPIO_GREEN,GPIO_OFF);

                        current = 2;
                }
                else if((temperatureValue/10) == TEMP_THRESHOLD && (current == 0 || current != 3)){
                                //Switching ON devices
                                sendData(GPIO_GREEN,GPIO_ON);

                                //Switching OFF devices
                                sendData(GPIO_HEATER,GPIO_OFF);
                                sendData(GPIO_RED,GPIO_OFF);
                                sendData(GPIO_FAN,GPIO_OFF);
                                sendData(GPIO_BLUE,GPIO_OFF);

                                current = 3;
                }
                sleep(3);
        }
        exit(1);
}
