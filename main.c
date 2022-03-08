#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wiringPi.h>
#include <softPwm.h>
#include <pthread.h>
#include "pins.h"

#define velocity 340

#define minSpeed 8
#define maxSpeed 15

pthread_mutex_t lock;

typedef struct carInfo
{
	pthread_t threads[3];
	
	//carMode 0 - Idle
	//carMode 1 - Car Starts
	//carMode 2 - Car Stops
	int carMode;
	
	int lineSensorPause;
	
	//Instruction from Line Sensor to Motor
	int turn_left;
	int turn_right;

	//Signal for pivot
	int pivotFlag;
	
} carInfo;

int setupPin()
{
	int checkPWM_1, checkPWM_2, checkPWM_3, checkPWM_4;
	
	if(wiringPiSetup() == -1)
	{
		printf("Failed to setup wiringPi\n");
		return -1;
	}
	else
	{
		//Motor 1
		pinMode(motor1_f, OUTPUT);
		pinMode(motor1_r, OUTPUT);
		checkPWM_1 = softPwmCreate(motor1_pwm, 0, 100);
		//Motor 2
		pinMode(motor2_f, OUTPUT);
		pinMode(motor2_r, OUTPUT);
		checkPWM_2 = softPwmCreate(motor2_pwm, 0, 100);
		//Motor 3
		pinMode(motor3_f, OUTPUT);
		pinMode(motor3_r, OUTPUT);
		checkPWM_3 = softPwmCreate(motor3_pwm, 0, 100);
		//Motor 4
		pinMode(motor4_f, OUTPUT);
		pinMode(motor4_r, OUTPUT);
		checkPWM_4 = softPwmCreate(motor4_pwm, 0, 100);
		//Echo Sensor
		pinMode(l_sonar_trigger, OUTPUT);
		pinMode(l_sonar_echo, INPUT);
		//Push Button
		pinMode(startButton, INPUT);
	}
	
	if(checkPWM_1 != 0 || checkPWM_2 != 0 || checkPWM_3 != 0 || checkPWM_4 != 0)
	{
		return -1;
	}
	else
		return 0;

}

void setForward()
{
	pthread_mutex_lock(&lock);
	
	digitalWrite(motor1_f, HIGH);
	digitalWrite(motor1_r, LOW);
	digitalWrite(motor2_f, HIGH);
	digitalWrite(motor2_r, LOW);
	digitalWrite(motor3_f, HIGH);
	digitalWrite(motor3_r, LOW);
	digitalWrite(motor4_f, HIGH);
	digitalWrite(motor4_r, LOW);
	
	pthread_mutex_unlock(&lock);
}

void setLeft()
{
	pthread_mutex_lock(&lock);
	
	printf("Pivot Left\n");
	digitalWrite(motor1_f, LOW);
	digitalWrite(motor1_r, HIGH);
	//digitalWrite(motor2_f, LOW);
	//digitalWrite(motor2_r, HIGH);
	sleep(1);
	
	pthread_mutex_unlock(&lock);
}

void setRight()
{
	pthread_mutex_lock(&lock);
	
	printf("Pivot Right\n");
	//digitalWrite(motor3_f, LOW);
	//digitalWrite(motor3_r, HIGH);
	digitalWrite(motor4_f, LOW);
	digitalWrite(motor4_r, HIGH);
	sleep(1);
	
	pthread_mutex_unlock(&lock);
}

void leftAdjust()
{
	pthread_mutex_lock(&lock);

	printf("Turning Left\n");
	softPwmWrite(motor1_pwm , 8);
	softPwmWrite(motor2_pwm, 8);
	softPwmWrite(motor3_pwm, 40);
	softPwmWrite(motor4_pwm, 40);
	sleep(1);

	pthread_mutex_unlock(&lock);
}

/*************************************************************/
void rightAdjust()
{
		pthread_mutex_lock(&lock);

		printf("Turning Right\n");
		softPwmWrite(motor1_pwm, 40);
		softPwmWrite(motor2_pwm, 40);
		softPwmWrite(motor3_pwm, 8);
		softPwmWrite(motor4_pwm, 8);
		sleep(1);

		pthread_mutex_unlock(&lock);
}

void speedChange(int newSpeed)
{
	pthread_mutex_lock(&lock);
	
	softPwmWrite(motor1_pwm, newSpeed);
	softPwmWrite(motor2_pwm, newSpeed);
	softPwmWrite(motor3_pwm, newSpeed);
	softPwmWrite(motor4_pwm, newSpeed);

	pthread_mutex_unlock(&lock);
}

void turnSpeed(int mode)
{
	pthread_mutex_lock(&lock);
	
	//Pivot Left with:
	//Motor 1 reverse to turn on
	//Motor 2 stationary to pivot on
	//Motor 3 and Motor 4 forward to move forward
	if(mode == 1)
	{
		softPwmWrite(motor1_pwm, 100);
		softPwmWrite(motor2_pwm, 0);
		softPwmWrite(motor3_pwm, 80);
		softPwmWrite(motor4_pwm, 80);
	}
	//Pivot Right with:
	//Motor 1 and Motor 2 forward to move forward
	//Motor 3 stationary to pivot on
	//Motor 4 reverse to turn on
	else if(mode == 2)
	{
		softPwmWrite(motor1_pwm, 80);
		softPwmWrite(motor2_pwm, 80);
		softPwmWrite(motor3_pwm, 0);
		softPwmWrite(motor4_pwm, 80);
	}

	pthread_mutex_unlock(&lock);

}

void stopCar()
{
	speedChange(0);
	sleep(1);
}

void startCar()
{
	printf("startCar()\n");

	setForward();
	
	for(int i = 0; i <= maxSpeed; i+=5)
	{
		speedChange(i);
		sleep(1);
	}
}

void carSharpTurn(int pivotSignal)
{
	float tick = 0;
	int curState, lastState;
	
	lastState = digitalRead(encoder4);
	
	if(pivotSignal == 1)
	{
		setRight();
	}
	else if(pivotSignal == 2)
	{
		setLeft();
	}
	
	turnSpeed(pivotSignal);
	
	do
	{
		curState = digitalRead(encoder4);
		
		if(curState != lastState)
		{
			lastState = curState;
			tick++;
		}
	}while(tick <= 76);
	
	stopCar();

	setForward();
}

void lightSensor()
{
	pinMode(light_sensor, INPUT);
	
	//Ensure wheels are going forward
	digitalWrite(motor1_f, HIGH);
	digitalWrite(motor1_r, LOW);
	digitalWrite(motor2_f, HIGH);
	digitalWrite(motor2_r, LOW);
	digitalWrite(motor3_f, HIGH);
	digitalWrite(motor3_r, LOW);
	digitalWrite(motor4_f, HIGH);
	digitalWrite(motor4_r, LOW);
	
	
	while(digitalRead(light_sensor) == 0)
	{
		speedChange(15);
	}
	
	sleep(1);
	
	while(digitalRead(light_sensor) == 1)
	{
		speedChange(minSpeed);
	}
	stopCar();
}

void goAround(struct carInfo * carInfo)
{
	printf("goAround()\n");
	
	struct carInfo * pauseSignal;
	pauseSignal = (struct carInfo *) carInfo;
	
	printf("goARound stopCar()\n");
	stopCar();
	
	//Pivot Right
	carSharpTurn(1);
	
	lightSensor();
	
	//Pivot Left
	carSharpTurn(2);
	
	lightSensor();
	
	//Pivot Left
	carSharpTurn(2);
	
	pauseSignal->lineSensorPause = 0;
	
}

float recordTime(int echoPin)
{
	clock_t start, stop;
	
	//When echo pin receives low signal, start the clock
	while(digitalRead(echoPin) == 0)
	{
		start = clock();
	}
	//When echo pin receives high signal, stop the clock
	while(digitalRead(echoPin) == 1)
	{
		stop = clock();
	}
	
	return ((float)(stop - start));
}

void * echoSensor(void * carInfo)
{
	printf("echoSensor() Thread\n");
	
	//Range for Encho Sensor Detection 20cm
	int range = 20;
	
	//Object detection Signal
	//int checkObj = 0;
	
	//Distance of Object Variable
	float objectDist = 0;
	
	//Double Checks if object is present
	int doubleCheck = 0;
	
	struct carInfo * carSignals;
	carSignals = (struct carInfo *) carInfo;
	
	printf("echoSensor()\n");
	//printf("echoSensor %d\n", carSignals->carMode);
	
	while(carSignals->carMode == 1)
	{	
		//doubleCheck = 0;

		digitalWrite(l_sonar_trigger, LOW);
		//delay for 100ms
		delay(100);
		
		//Send pulse of 10 microseconds long
		digitalWrite(l_sonar_trigger, HIGH);
		usleep(10);
		digitalWrite(l_sonar_trigger, LOW);
		
		//printf("Getting objectDist\n");
		
		//Distance of detected objected in cm
		objectDist = recordTime(l_sonar_echo) * velocity/2/10000;
		
		printf("Object Distance: %fcm\n", objectDist);

		// if(objectDist <= range)
		// {
		// 	doubleCheck = 1;
		// 	carSignals->lineSensorPause = 1;
		// 	//carSignals->pivotFlag = 1;
		// }

		// if(carSignals->lineSensorPause == 1)
		// {
		// 	carSignals->pivotFlag = 0;
		// 	//doubleCheck=1;
		// }

		// if(doubleCheck == 0)
		// {
		// 	carSignals->lineSensorPause = 0;

		// }
		
		//Distance check of object
		//IF within 20cm of car
		if(objectDist <= range && doubleCheck == 1)
		{
			doubleCheck = 0;
			goAround(carSignals);
		}
		else if(objectDist <= range && doubleCheck == 0)
		{
			carSignals->lineSensorPause = 1;
			stopCar();
			sleep(1);
			printf("double checking\n");
			doubleCheck=1;
			
		}
		else
		{
			doubleCheck = 0;
			//speedChange(maxSpeed);
			carSignals->lineSensorPause = 0;
		}
		
		// sleep(1);
/*		
		if(checkObj == 1)
		{
			//printf("goAround()\n");
			goAround();
		}
*/		
	}
	
	printf("echoSensorThread() Exit\n");
	pthread_exit(0);
}

void * carAdjust(void * carInfo)
{
	struct carInfo * carSignals;

	carSignals = (struct carInfo *) carInfo;

	printf("carAdjust() carMode: %d\n", carSignals->carMode);

	printf("turn_left: %d\n", carSignals->turn_left);
	printf("turn_right: %d\n", carSignals->turn_right);

	
	while(carSignals->carMode == 1)
	{
		while(carSignals->lineSensorPause == 1)
		{
		// 	//Distance check of object
		// 	//IF within 20cm of car
		// 	if(carSignals->pivotFlag == 0)
		// 	{
		// 		goAround(carSignals);
		// 	}			
		// 		stopCar();
		// 		sleep(1);

		 }
		// else if(carSignals->lineSensorPause == 0)
		// {
			if(carSignals->turn_left == 1 && carSignals->turn_right == 1)
			{
				printf("Going Straight\n");
				speedChange(maxSpeed);
				sleep(1);
				//break;
			}
			else if(carSignals->turn_left == -1 && carSignals->turn_right == 1)
			{
				leftAdjust();
				//break;
			}
			else if(carSignals->turn_left == 1 && carSignals->turn_right == -1)
			{
				rightAdjust();
				//break;
			}
		// }
	}

	pthread_exit(0);
}

void * lineSensor(void * carInfo)
{
	
	printf("lineSenor() Thread\n");
	
	//Detection Vraiables
	int l_detect, r_detect;
	
	struct carInfo * carControl;
	carControl = (struct carInfo *) carInfo;
	
	//Line Sensor initial Readout
	int l_init = 0;
	int r_init = 0;

	printf("l_init: %d\n", l_init);
	printf("r_init: %d\n", r_init);

	printf("l_init sensor: %d\n", digitalRead(l_line_sensor));
	printf("r_init sensor: %d\n", digitalRead(r_line_sensor));

	
	do
	{
		while(carControl->lineSensorPause == 1)
		{
		}
		
		l_detect = digitalRead(l_line_sensor);
		//printf("l_detect: %d\n", l_detect);
		r_detect = digitalRead(r_line_sensor);
		//printf("r_detect: %d\n", r_detect);
		
		if((l_detect == l_init) && (r_detect == r_init))
		{
			//Keep Straight
			carControl->turn_left = 1;
			carControl->turn_right = 1;

			//Testing Right
			//carControl->turn_right = -1;
			//carControl->turn_left = 1;
		}
		
		if((l_detect != l_init) && (r_detect == r_init))
		{
			//Turn or Adjust Left
			carControl->turn_left = -1;
			carControl->turn_right = 1;

			//Testing Right
			//carControl->turn_right = -1;
			//carControl->turn_left = 1;
		}
		
		if((l_detect == l_init) && (r_detect != r_init))
		{
			//Turn or Adjust Right
			carControl->turn_right = -1;
			carControl->turn_left = 1;
		}
		
		//carAdjust(carControl);
		//printf("carAdjust()\n");

	}while(carControl->carMode == 1);

	printf("lineSensor() Thread Exit\n");
	pthread_exit(0);
}

void runCarThread(struct carInfo * car)
{
	printf("runCarThread()\n");
	
	//Push Button Thread
	//pthread_create(&car->threads[0], NULL, readyCar, (void *) car);
	//Echo Sensor Thread
	pthread_create(&car->threads[0], NULL, echoSensor, (void *) car);
	//Line Sensor Thread
	pthread_create(&car->threads[1], NULL, lineSensor, (void *) car);
	//Car Adjustment Thread
	pthread_create(&car->threads[2], NULL, carAdjust, (void *) car);
}

int main()
{
	if(setupPin() == -1)
	{
		printf("Failed to initPin\n");
		exit(0);
	}
	
	pthread_mutex_init(&lock, NULL);
	
	carInfo car;
	car.carMode = 0;
	
	printf("Car Idling...\n");
	
	while(car.carMode == 0)
	{
		if(digitalRead(startButton) == 1)
		{
			printf("Button Pressed - startCar()\n");
			sleep(1);
			startCar();
			car.carMode = 1;
		}
	}
	
	runCarThread(&car);
	
	while(car.carMode != 2)
	{
		if(digitalRead(startButton) == 1)
		{
			printf("Button Pressed - stopCar()\n");
			car.carMode = 2;
			sleep(1);
			stopCar();
		}
	}
	
	for(int i = 0; i < 3; i++)
	{
		printf("Joining thread: %d\n", i);
		pthread_join(car.threads[i], NULL);
	}
	
	printf("carMode after pthread_join() %d\n", car.carMode);
	
	pthread_mutex_destroy(&lock);
	
	return 0;
}
