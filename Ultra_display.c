// Author : Team 1 : Vishwakumar Doshi && Nisarg Trivedi
//Subject : Embedded system Programming
//Course code: CSE 438

/* The below code uses two devices. An SPI LED matrix and an Ultrasonic distance measuring sensor. Here the ultrasonic 
sends a trigger pulse at contnous time intervals which generate a sonic burst of 8 pulses. As the sonic burst starts, 
the Echo pulse rises and falls when all the sonic busrts are back. The size of the Echo pulse gives the distance 
between the object and the Ultrasonic sensor. The speed of changing pattern to be diplayed  depends on the distance between the 
object and the sensor and the direction depends on the the movement direction of the object . */

//Include the libraries 

#include "Uled.h"

pthread_t sensor_id,display_id; //Two threads are there. 
                                //The sensor_id for constantly sending trigger signal an polling Echo pin for measuring distance and setting direction
                                //The display_id for displaying patterns based on distance an ddirections set in sensor thread

int main(void)  
{ 	

	//Initializing sensor. Sets pins as gpio to be used for trigger and echo
	sensor_init();
    //Initializing display. Sets pins for use as SPI communiction pins
    display_init();


	//Creating both the threads. One for distance measuring and other for pattern display
    pthread_create(&sensor_id,NULL,Echopoll,NULL);
    pthread_create(&display_id,NULL,display_function,NULL);
    

    //Waiting the two threads to terminate
	pthread_join(sensor_id,NULL);
	pthread_join(display_id,NULL);

 	return 0;
}
