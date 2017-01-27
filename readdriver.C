/* Program to start the BMP driver service on Beaglebone Black
   and parse the data from the character device file to run a
   CPU fan using the GPIO driver 
   The service can be manually brought up using
   /bin/service up /service/bmp085 -label bmp085.3.77 -dev /dev/bmp085b3s77 -args 'bus=3 address=0x77'
*/

#include <stdio.h>

/************** Variables related to BMP Sensor - START ******************/
//defines the status of the file open/close operation
int fileio;
int filestat;
//the name of the character specific device file
char filename[] = "/dev/bmp085b3s77";

//the operation mode of the above file
char filemode[] = "r";

//character buffer to read the data from the file
char readbuf[255];

//File pointer
FILE *fp;
FILE *fp1;

//variable to store the temperature value
int temperatureValue;

//variable to store the status of fan(0 - Off, 1 - On)
int fanStatus = 0;

//variable to store the pressure value
float pressureValue;
int i = 0;
int serv_stat;

/************** Variables related to BMP Sensor - END ********************/


int main(){
	
	filestat = start();
        if(filestat != 0)
                exit(1);
	
	bmpdata();
	
	fileio = closeFile();
	if(fileio != 0){
		printf("File close failed\n");
		return 0;
	}
	printf("\nFile closed successfully\n");
	return 0;
}

int start()
{
        //open the character specific device file
        fileio = openFile();

        //check if the file is opened successfully
        if(fileio != 0){
                if(fileio == 1){
                        printf("\n FILE opening failed. Trying to bring the service up");
                        serv_stat = serviceup();
                        if(serv_stat == 0){
                                //return 0;
                                fileio = openFile();
                                if(fileio != 0)
                                        return fileio;
                                else
                                        return fileio;
                        }
                        else
                                return serv_stat;
                }
                else{
                        printf("\nProgram terminated with an error");
                        return 0;
                }
        }
        fileio = closeFile();
        if(fileio != 0){
                printf("File close failed\n");
                return 0;
        }
        printf("\nFile closed successfully\n");
        return 0;
}

/********this function brings the bmp085 service up (this is a ine time process) ********/
int serviceup(){
        serv_stat = system("/bin/service up /service/bmp085 -label bmp085.3.77 dev /dev/bmp085b3s77 -args 'bus=3 address=0x77'");
        return serv_stat;
}

/******** this function opens the character device file ********/
int openFile(){
        fp = fopen(filename,filemode);
        if(fp == NULL){
                printf("\nError : couldn't open the file %s",filename);
                return 1;
        }
        else{
                printf("\nFile opened successfully");
                return 0;
        }
}

/******** this function reads the data from the buffer and parses temperature into an integer value and returns to the calling function initialise() ********/
void bmpdata(){
        fscanf(fp, "%s", readbuf);
        fgets(readbuf, 255, (FILE*)fp);
        temperatureValue = atoi(readbuf);
        printf("\n*******************\nTemperature Value : %d\n",temperatureValue);
        fscanf(fp, "%s", readbuf);
        fgets(readbuf, 255, (FILE*)fp);
        pressureValue = atof(readbuf);
        printf("Pressure Value : %.2f\n",pressureValue);
}


//this function closes the character device file
int closeFile(){
	return fclose(fp);
}


