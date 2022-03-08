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
		softPwmWrite(motor3_pwm, 100);
		softPwmWrite(motor4_pwm, 100);
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
	pthread_mutex_lock(&lock);
	
	softPwmWrite(motor1_pwm, 0);
	softPwmWrite(motor2_pwm, 0);
	softPwmWrite(motor3_pwm, 0);
	softPwmWrite(motor4_pwm, 0);

	pthread_mutex_unlock(&lock);
	sleep(1);
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
	digitalWrite(motor2_f, LOW);
	digitalWrite(motor2_r, HIGH);
	sleep(1);
	
	pthread_mutex_unlock(&lock);
}

void setRight()
{
	pthread_mutex_lock(&lock);
	
	printf("Pivot Right\n");
	digitalWrite(motor3_f, LOW);
	digitalWrite(motor3_r, HIGH);
	digitalWrite(motor4_f, LOW);
	digitalWrite(motor4_r, HIGH);
	sleep(1);
	
	pthread_mutex_unlock(&lock);
}

void carSharpTurn(int pivotSignal)
{
	float tick = 0;
	int curState, lastState;
	
	lastState = digitalRead(encoder4);
	
	if(pivotSignal == 2)
	{
		setRight();
	}
	else if(pivotSignal == 1)
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

void goAround()
{
	printf("goAround()\n");
	
	//struct carInfo * pauseSignal;
	//pauseSignal = (struct carInfo *) carInfo;
	
	//stopCar();
	
	//Pivot Right
	carSharpTurn(2);
	
	//lightSensor();
	
	//Pivot Left
	carSharpTurn(1);
	
	//lightSensor();
	
	//Pivot Left
	carSharpTurn(1);
	
	//pauseSignal->lineSensorPause = 0;
	
}

int main(int argc, char const *argv[])
{
		if(setupPin() == -1)
	{
		printf("Failed to initPin\n");
		exit(0);
	}

	pthread_mutex_init(&lock, NULL);	

	while(1)
	{
		goAround();
	}

	stopCar();

	return 0;
}