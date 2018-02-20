#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <time.h>
#include <poll.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>
 
#define DEVICE "/dev/spidev1.0"      // SPI device file to be opened and used for display functions
#define WAIT_DURATIONus 60000ul  // delay duration in microseconds
#define T 2                       // Trigger IO pin number
#define E 3                       // Echo    IO pin number
 
uint8_t array1[2];                // Transmit buffer
int fd5,fd15,fd7,fd24,fd42,fd30,fd25,fd43,fd31,fd44,fd46;

typedef struct                    // Pattern structure
{
	uint8_t led[8];
} PATTERN;

// 4 patterna defined.
PATTERN patternF1 = 
		{{
				0x00, // Pattern bytes
				0x3C,	
				0x08,
				0x3C,
				0x10,
				0x08,
				0x04,
				0x00,
		}};

PATTERN	patternF2 = 
		{{
				0xFF, // Pattern bytes
				0x18,	
				0x08,
				0x7E,
				0x10,
				0x08,
				0x04,
				0x02,
		}};

PATTERN	patternR1 = 
		{{
				0x04, // Pattern bytes
				0x1F,	
				0x14,
				0x0C,
				0x06,
				0x05,
				0x1E,
				0x04,
		}};

PATTERN	patternR2 = 
		{{
				0x20, // Pattern bytes
				0x78,	
				0xA0,
				0x60,
				0x30,
				0x28,
				0xF0,
				0x20,
		}}; 

// Configuration data array for settings of LED driver chip 
uint8_t config [] = 
{
		0x0F, 0x00,           // No display test
        0x0C, 0x01,           // Setting shutdown as Normal Operation
        0x09, 0x00,           //No decode mode required
        0x0A, 0x0F,           // Full intensity
        0x0B, 0x07,           // Scans for all rows
};


struct spi_ioc_transfer tr = {0};  // initialising struct with 0 values

int T_desc[4], E_desc[4]; // Holds the file descriptors for gpio pins to select directions and mux select for IO pins on galileo board where Echo and Trigger are connected 

int j,dist,err,fd_spi;      // j is for loop variable and fd_spi is file descriptor for /dev/spidev1.0, dist is sensor measured distance

uint8_t dir,flag0 = 0,flag1 = 0; // direction for pattern, flags for smooth transition logic

char path[34]; // string to hold path of a particular file to be opened; eg /sys/class/gpio/gpio7/direction

struct pollfd PollEch = {0};

char gparray[14][4][4]=  {	                                      // Lookup table to get gpio pin numbers to control corresponding IO pins on board
							{"11","32","IGN","IGN"},			  // Each row corresponds to a particular IO pin number
							{"12","28","45","IGN"},               // ROW 0 corresponds to IO0 .... ROW 13 corresponds to IO13 
							{"13","34","77","IGN"},               // Here "IGN" means IGNORE meaning their values don't affect behaviour of IO pin
 							{"14","16","76","64"},
							{"6","36","IGN","IGN"},
							{"0","18","66","IGN"},
							{"1","20","68","IGN"},
							{"38","IGN","IGN","IGN"},
							{"40","IGN","IGN","IGN"},
							{"4","22","70","IGN"},
							{"10","26","74","IGN"},
							{"5","24","44","72"},
							{"15","42","IGN","IGN"},
							{"7","30","46","IGN"}
						};


int check_gpio64to79(char str[])    // Checks whether a gpio pin belongs to the range gpio64-gpio79 because these pins don't require direction file to be set
{
	int ret = atoi(str);            // Converts string to integer to make comparison possible

	if((ret>=64) && (ret<=79))
		return 0;
	else
		return 1;

}


static __inline__ unsigned long long rdtsc(void)    // Function to read TSC in 32 bit processor
{
  unsigned long long int x;
     __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
     return x;
}

// #####################################################
// Initialises sensor pins for trigger and echo. Trigger is output while echo is set as input.

void sensor_init()
{	

	int fdexport;

	fdexport = open("/sys/class/gpio/export", O_WRONLY | O_NONBLOCK);  // Opens file in which we will export all the gpio pins necessary to use IO pins on Galileo board

	if(fdexport < 0) 
	{
		printf("gpio export open failed \n");
	}

	
	for(j=0;j<4;j++)
	{
		write(fdexport,gparray[T][j],sizeof(gparray[T][j])); // Each IO pin on board requires a maximum of 4 gpio pins to control it 
		write(fdexport,gparray[E][j],sizeof(gparray[E][j])); // (1 for direction, 1 is pin which controls IO pin, 2 for selecting that gpio pin via multiplexing)
	}

   close(fdexport);



   // INITIALISING ECHO

	sprintf(path,"/sys/class/gpio/gpio%s/direction",gparray[E][0]);

		E_desc[0] = open(path, O_WRONLY);
		if(E_desc[0] < 0) 
			printf("ERROR: gpio%s direction open failed \n",gparray[E][0]);

		err = write(E_desc[0],"in",3);       //Setting direction to in
		if(err < 0)
			printf("ERROR writing to gpio%s direction\n",gparray[E][0]);
		close(E_desc[0]);


	sprintf(path,"/sys/class/gpio/gpio%s/direction",gparray[E][1]);  // Setting Level shifter to High to make Echo as input

		E_desc[1] = open(path, O_WRONLY);
		if(E_desc[1] < 0) 
			printf("ERROR: gpio%s direction open failed \n",gparray[E][1]);

		err = write(E_desc[1],"out",3);       //Setting direction to out
		if(err < 0)
				printf("ERROR writing to gpio%s direction\n",gparray[E][1]);
		close(E_desc[1]);

	sprintf(path,"/sys/class/gpio/gpio%s/value",gparray[E][1]);

		E_desc[1] = open(path, O_WRONLY);
		if(E_desc[1] < 0) 
			printf("ERROR: gpio%s value open failed \n",gparray[E][1]);
		err = write(E_desc[1],"1",1);                                  // Writing 1
		close(E_desc[1]);



	for(j=2;j<4;j++)                                                 // Other mux pins are set to Low to select GPIO
	{
		if(strcmp(gparray[E][j],"IGN") != 0)
		{
			if(check_gpio64to79(gparray[E][j]))
			{
				sprintf(path,"/sys/class/gpio/gpio%s/direction",gparray[E][j]);

				E_desc[j] = open(path, O_WRONLY);
					if(E_desc[j] < 0) 
						printf("ERROR: gpio%s direction open failed \n",gparray[E][j]);

				err = write(E_desc[j],"out",3);       //Setting direction to out
				if(err < 0)
					printf("ERROR writing to gpio%s direction\n",gparray[E][j]);

				close(E_desc[j]);
			}

			sprintf(path,"/sys/class/gpio/gpio%s/value",gparray[E][j]);

			E_desc[j] = open(path, O_WRONLY);
				if(E_desc[j] < 0) 
					printf("ERROR: gpio%s value open failed \n",gparray[E][j]);
			
			err = write(E_desc[j],"0",1);  // Sets value to 1 (or HIGH)
			if(err < 0)
					printf("ERROR writing to gpio%s\n",gparray[E][j]);

			close(E_desc[0]);
		}
	}


// INITIALISING TRIGGER

	for(j=0;j<4;j++)
	{
		if(strcmp(gparray[T][j],"IGN") != 0)  // Initialises only if it's not to be ignored
		{
			if(check_gpio64to79(gparray[T][j]))// Sets direction only if it doesn't belong to the range gpio64-gpio79
			{
				sprintf(path,"/sys/class/gpio/gpio%s/direction",gparray[T][j]);

				T_desc[j] = open(path, O_WRONLY | O_NONBLOCK);
					if(T_desc[j] < 0) 
						printf("ERROR: gpio%s direction open failed \n",gparray[T][j]);

				err = write(T_desc[j],"out",3);       //Setting direction to 'out'
				if(err < 0)
					printf("ERROR writing to gpio%s\n",gparray[T][j]);
				close(T_desc[j]);
			}

			sprintf(path,"/sys/class/gpio/gpio%s/value",gparray[T][j]);

			T_desc[j] = open(path, O_WRONLY | O_NONBLOCK);
				if(T_desc[j] < 0) 
					printf("ERROR: gpio%s value open failed \n",gparray[T][j]);

			err = write(T_desc[j],"0",1);  // Sets value to 0
			if(err < 0)
					printf("ERROR writing to gpio%s\n",gparray[T][j]);

			close(T_desc[j]);
		}
	}
}


//#########################################
//Polls echo pin to calculate distance and also decides direction based on change of distance

void* Echopoll(void* arg)       // Function to check distance using HC Sr04 sensor with trigger and echo pin numbers in arguments
{
	int edg,trig,echo,diff,prev = 0;           // File descriptors and prev stores previous distance to calculate diff 
	unsigned long long time;
	char buff;

	sprintf(path,"/sys/class/gpio/gpio%s/edge",gparray[E][0]);

	edg = open(path, O_RDWR);                                    //Opening edge file for echo pin
	if(edg < 0)
		printf("ERROR in opening gpio%s edge\n",gparray[E][0]);


	sprintf(path,"/sys/class/gpio/gpio%s/value",gparray[E][0]);

	echo = open(path, O_RDWR);                                   // Opening value file for echo pin
	if(echo < 0)
		printf("ERROR in opening gpio%s\n",gparray[E][0]);

	
	PollEch.fd = echo;                           // This is the fd of pin whose value is to be polled
	PollEch.events = POLLPRI|POLLERR;            // Specifies the event which are to be considered by poll function

	sprintf(path,"/sys/class/gpio/gpio%s/value",gparray[T][0]);
	trig = open(path, O_RDWR);                                   // Opens trigger value file
    
	while(1)                                    // Continuous polling
	{
				
		err = lseek(echo,0,SEEK_SET);
		if(err < 0)                             //A negative value is returned when lseeks fails. Thus checking for the error
			printf("ERROR in lseek\n");

		err = write(edg,"rising",sizeof("rising"));      // Setting the edge type as rising first
		if(err < 0)                                       // checking for error
			printf("ERROR writing to gpio edge\n");
				
		err = write(trig,"0",1);
		if(err<0)
			printf("Error in writing to Trigger\n");

		usleep(2);                                        // Waits before giving trigger pulse

		err = write(trig,"1",sizeof("1"));
		if(err<0)
			printf("Error in writing to Trigger\n");

		usleep(30);                                       // Holding trigger high for atleast 30us(device requirement)

		err = write(trig,"0",sizeof("1"));
		if(err<0)
			printf("Error in writing to Trigger\n");

			
		err = poll(&PollEch,1,1000);                    // Polls for rising edge. Seting timeout value as 1000ms
		if(err < 0)                            //Checking for the polling error. A negative value is returned when that happens
			printf("ERROR in poll\n");
		if(err == 0)                           //Returns 0 on timeout
			printf("POLL timeout!\n");
					
		if(err > 0)
		{
			if(PollEch.revents & POLLPRI)             // If poll event occurred
			{	
				time = rdtsc();                        // Reading TSC for echo pulse length calculation
				err = read(echo,&buff,sizeof("1"));     // Reads echo value file
				if(err < 0)
					printf("ERROR in reading echo\n");
			}
						
			err = write(edg,"falling",sizeof("falling"));  // Now setting edge to wait for as 'falling'
			if(err < 0)
				printf("ERROR writing to gpio edge\n");

			err = lseek(echo,0,SEEK_SET);         //Putting the Offset value of the echo file at '0'
			if(err < 0)                           //Checking for error which is a return of negative value
				printf("ERROR in lseek\n");

			
			err = poll(&PollEch,1,1000);        //Polls for falling edge
			if(err < 0)
				printf("ERROR in echo\n");
			if(err == 0)
				printf("POLL timeout!\n");

			if(PollEch.revents & POLLPRI)
			{	
				err = read(echo,&buff,sizeof("1"));
				if(err < 0)
					printf("ERROR in reading echo\n");
				dist = ((double)(rdtsc() - time)*34)/80000ul;  // Distance calculations in ms

				if(dist < 100)                                // Setting boundary conditions for distance values
					dist = 100;
				if(dist > 2500)
					dist = 2500; 
				
			}
		}

		// Calculating difference between previous and current distance values
		diff = dist - prev;

		// Logic for smooth transitions. changes directions only if absolute difference value is more than 10mm for continuous 3 measurements.
		if(diff > 5)
		{
			flag1 = flag1 + 1;			
			if (flag1 == 1)
			{	
				flag0 = 0;
			}
			else if (flag1 == 3)
			{
				dir = 1;
				flag1 = 0;
			}
		
		}

		else if(diff < -5)
		{
				flag0 = flag0 + 1;			
			if (flag0 == 1)
			{	
				flag1 = 0;
			}						
			else if (flag0 == 3)
			{
				dir = 0;
				flag0 = 0;
			}

		}

		prev = dist;         // Updates previous value for next iteration
		usleep(WAIT_DURATIONus);					
	}	
		
}

//Initialisation of the pins used in SPI transfer

void display_init()      
{
    int fdexport;
    fdexport = open("/sys/class/gpio/export", O_WRONLY);
        if (fdexport < 0)
        {
            printf("\n gpio export open failed");
        }

        // #######################################
        //Exporting pins
 
        write(fdexport,"15",2);
        write(fdexport,"24",2);
        write(fdexport,"42",2);
        write(fdexport,"30",2);
        write(fdexport,"44",2);
        write(fdexport,"46",2);
                     
        close(fdexport);

        //#######################################
        // Setting direction

        fd15 = open("/sys/class/gpio/gpio15/direction", O_WRONLY);    // Open the file descriptors thaa are needed
        if (fd15 < 0)
        {
            printf("\n gpio15 direction open failed");
        }
        fd24 = open("/sys/class/gpio/gpio24/direction", O_WRONLY);
        if (fd24 < 0)
        {
            printf("\n gpio24 direction open failed");
        }
        fd42 = open("/sys/class/gpio/gpio42/direction", O_WRONLY);
        if (fd42 < 0)
        {
            printf("\n gpio42 direction open failed");
        }
        fd30 = open("/sys/class/gpio/gpio30/direction", O_WRONLY);
        if (fd30 < 0)
        {
            printf("\n gpio24 direction open failed");
        }
        fd44 = open("/sys/class/gpio/gpio44/direction", O_WRONLY);
        if (fd44 < 0)
        {
            printf("\n gpio44 direction open failed");
        }
        fd46 = open("/sys/class/gpio/gpio46/direction", O_WRONLY);
        if (fd46 < 0)
        {
            printf("\n gpio46 direction open failed");
        }
      

        if(0> write(fd15,"out",3))
            printf("\nerror Fd15");
 
        if(0> write(fd24,"out",3))
            printf("\nerror Fd24");
 
        if(0> write(fd42,"out",3))
            printf("\nerror Fd42");
 
        if(0> write(fd30,"out",3))
            printf("\nerror Fd30");
 
        if(0> write(fd44,"out",3))
            printf("\nerror Fd44");
 
        if(0> write(fd46,"out",3))
            printf("\nerror Fd46");
 
        //##########################################

        //Close the file descriptors that were used
        
        close(fd15);
        close(fd24);
        close(fd42);
        close(fd30);
        close(fd44);
        close(fd46); 



        // ######################################################
        // Setting values of pins for level shifter and mux select
      

        fd15 = open("/sys/class/gpio/gpio15/value", O_WRONLY);
                if (fd15 < 0)
                {
                    printf("\n gpio15 value open failed");
                }
        fd24 = open("/sys/class/gpio/gpio24/value", O_WRONLY);
                if (fd24 < 0)
                {
                    printf("\n gpio24 value open failed");
                }
        fd42 = open("/sys/class/gpio/gpio42/value", O_WRONLY);
            if (fd42<0)
                {
                    printf("\n gpio42 value open failed");
                }         
        fd30 = open("/sys/class/gpio/gpio30/value", O_WRONLY);           
        if (fd30< 0)
                {
                    printf("\n gpio30 value open failed");       
                }   
        fd44 = open("/sys/class/gpio/gpio44/value", O_WRONLY);
                if (fd44 < 0)
                {
                    printf("\n gpio44 value open failed");
                }
        fd46 = open("/sys/class/gpio/gpio46/value", O_WRONLY);
            if (fd46<0)
                {
                    printf("\n gpio46 value open failed");
                }       
         
     
        if(0> write(fd15,"0",1))
                    printf("\nerror Fd15 value");
       
        if(0> write(fd24,"0",1))
                    printf("\nerror Fd24 value");
 
        if(0> write(fd42,"0",1))
                    printf("\nerror Fd42 value");
 
        if(0> write(fd30,"0",1))
                    printf("\nerror Fd30 value");
 
        if(0> write(fd44,"1",1))
                    printf("\nerror Fd44 value");
 
        if(0> write(fd46,"1",1))
                    printf("\nerror Fd46 value");
 

}

//#################################################################

// Function to transfer a specific pattern in array using SPI
 
void transfer(uint8_t array[],int len) 
{
	int i;
        for(i = 1; i <= len ; i++)    
        { 
            array1[0] = i;    //array1[0] stores address of a register 
            array1[1] = array [i-1]; //array1[1] stores data to be stored in that address
            if(0>write(fd15,"0",1)) //Holding chip select line to '0' during trnasfer
                    printf("\nerror Fd15 value");
 
            err = ioctl(fd_spi, SPI_IOC_MESSAGE (1), &tr);  // Sends 2 byte data at a time to the LED driver
			if(err<0)                                        // Checks for error that is a return of a negative value
				printf("\n error in ioctl message transfer");
			usleep(1000);                                    //Waits for 1000us before next transfer
            if(0> write(fd15,"1",1))                //Holding chip select line to '1' after transfer
                    printf("\nerror Fd15 value");
 
        }
}


//###############################################
//Thread function to continously display the pattern according to the distance calculated in Echopoll


void* display_function(void* arg)
{    
    int i;
    fd_spi= open(DEVICE,O_WRONLY);
     
    if(fd_spi==-1)    //Chekcing for error while opeming of the File descriptor
    {
        printf("file %s either does not exit or is currently used by an another user\n", DEVICE);
        exit(-1);
    }

        //initialising the fields of spi_ioc_transfer

        tr.tx_buf = (unsigned long)array1;  //Setting transmit buffer
        tr.rx_buf = 0;                      //Receive buffer null as no data is to be received
        tr.len = 2;                         //Number of bytes of data in one transfer
        tr.delay_usecs = 1;                 //Delay in us
        tr.speed_hz = 10000000;             //Bus speed in Hz 
        tr.bits_per_word = 8;               //Number of bits in each word
        tr.cs_change = 1;                   //Changing chip select line value between transfers
    

        //Sending configuration data to LED driver chip
        for(i = 0; i < 10 ; i+=2)          
        { 
            array1[0] = config[i];
            array1[1] = config [i+1];
            if(0>write(fd15,"0",1))
                    printf("\nerror Fd15 value");
 
            err = ioctl(fd_spi, SPI_IOC_MESSAGE (1), &tr);
			if(err<0)
				printf("\n error in ioctl message transfer");
			usleep(1000);
            if(0> write(fd15,"1",1))
                    printf("\nerror Fd15 value");
 
        }
    
    // Switching between two display patterns according to direction set and switching between 2 patterns for particular direction  with speed according to distance in Echopoll thread
	while(1)
	{
		while(dir == 0)   
		{
			transfer(patternF1.led,sizeof(patternF1.led)); //Switching between two forward direction patterns
			usleep(dist*350);                              //Keeping Delay proportional to calculated distance
			transfer(patternF2.led,sizeof(patternF2.led)); 
			usleep(dist*350);
		}

		while(dir == 1)
		{
			transfer(patternR1.led,sizeof(patternR1.led));  //Switching between two reverse direction patterns
			usleep(dist*350);                               //Keeping Delay proportional to calculated distance
			transfer(patternR2.led,sizeof(patternR2.led));
			usleep(dist*350);
		}
	}
              
    close (fd_spi);
 
return 0;
}

//########################################
//Unexporting the pins that were set for the SPI

void unexport()
{
    int fdunexport;
    fdunexport = open("/sys/class/gpio/unexport", O_WRONLY);
        if (fdunexport < 0)
        {
            printf("\n gpio unexport open failed");
        }
 
        if(0> write(fdunexport,"15",2))
            printf("error fdunexport 15 \n");
        if(0> write(fdunexport,"24",2))
            printf("error fdunexport 24 \n");
        if(0> write(fdunexport,"42",2))
            printf("error fdunexport 42 \n");
        if(0> write(fdunexport,"30",2))
            printf("error fdunexport 30 \n");
        if(0> write(fdunexport,"44",2))
            printf("error fdunexport 44 \n");
        if(0> write(fdunexport,"46",2))
            printf("error fdunexport 46 \n");
         
        close(fdunexport);
 
}