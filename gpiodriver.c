/* -----------------------------------------------*\
Program to trigger GPIO functionality using IPC
-------------- Created : 02/18/2017 --------------
-------------- Created by : Sid ------------------
--------------------------------------------------*/
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

//declarations related to IPC - START//
// message data structure (OS define type)
message m;
// gpio process endpoint (OS defined type)
endpoint_t *gpio_ep;
int i = 0;
int getendpoint_name(char *proc_name);
//declarations related to IPC - END//

// send GPIO commands to GPIO FS
void sendData(int gpionum, int gpiovalue){
        int r;
        memset(&m, 0, sizeof(m));
        m.m_type = GPIO_UPDATE;
        m.m_m1.m1i1 = gpionum;
        m.m_m1.m1i2 = gpiovalue;

        printf("GPIODriver: sending signal to GPIO : m_type: %d, GPIO : %d value: %d\n", m.m_type, m.m_m1.m1i1, m.m_m1.m1i2);
        r = ipc_sendrec(gpio_ep, &m);
        if (r != 0) {
             printf("sendnrec()%d\n",r);
        }
}

/* receive command from tempController to trigger the GPIO*/
int receiveData(){
        int r, status;
        r = ipc_receive(gpio_ep, &m, &status);
        printf("SENSOR: receiveTEMPSENSOR: status: %d, m_type: %d, value: %d\n", status, m.m_type, m.m_m1.m1i1);
        return r;
}

void main (){
        int r,epval,gpionum,gpiovalue;
        gpio_ep = getendpoint_name("gpio");
        //epval = minix_rs_lookup("i2c",gpio_ep);
        printf("gpioDriver : endpoint of GPIO : %d\n",epval);
        printf("\nEnter the GPIO number : ");
        scanf("%d",&gpionum);
        printf("\nEnter the GPIO value : ");
        scanf("%d",&gpiovalue);
        sendData(gpionum,gpiovalue);
}
