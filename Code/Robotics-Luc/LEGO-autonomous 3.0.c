#pragma config(Hubs,  S1, HTMotor,  HTMotor,  HTMotor,  HTServo)
#pragma config(Sensor, S2,     compass,        sensorI2CHiTechnicCompass)
#pragma config(Sensor, S3,     touch,          sensorTouch)
#pragma config(Sensor, S4,     SMUX,           sensorI2CCustomFastSkipStates)
#pragma config(Motor,  motorA,           ,             tmotorNXT, openLoop)
#pragma config(Motor,  motorB,           ,             tmotorNXT, openLoop)
#pragma config(Motor,  motorC,           ,             tmotorNXT, openLoop)
#pragma config(Motor,  mtr_S1_C1_1,     motorD,        tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C1_2,     arm,           tmotorTetrix, PIDControl, reversed, encoder)
#pragma config(Motor,  mtr_S1_C2_1,     leftSpliced,   tmotorTetrix, PIDControl, driveLeft, encoder)
#pragma config(Motor,  mtr_S1_C2_2,     left,          tmotorTetrix, PIDControl, driveLeft, encoder)
#pragma config(Motor,  mtr_S1_C3_1,     rightSpliced,  tmotorTetrix, PIDControl, reversed, driveRight, encoder)
#pragma config(Motor,  mtr_S1_C3_2,     right,         tmotorTetrix, PIDControl, reversed, driveRight, encoder)
#pragma config(Servo,  srvo_S1_C4_1,    door,                 tServoStandard)
#pragma config(Servo,  srvo_S1_C4_2,    spin1,                tServoContinuousRotation)
#pragma config(Servo,  srvo_S1_C4_3,    spin2,                tServoContinuousRotation)
#pragma config(Servo,  srvo_S1_C4_4,    catchServo,           tServoStandard)
#pragma config(Servo,  srvo_S1_C4_5,    servo5,               tServoNone)
#pragma config(Servo,  srvo_S1_C4_6,    servo6,               tServoNone)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

//using updated versions of "wallfollow2.0.c" and "compassfollow2.0.c" -- has no motor limit

#include "wallfollow2.0.c"
#include "compassfollow2.0.c"
#include "hitechnic-sensormux.h"//put this before other third party ones
#include "JoystickDriver.c"

void init(){
	//ENCODERS//

	//SERVO//
	//servoChangeRate[door]=2;

	//SOUND//
	nVolume = 2;
	bPlaySounds = true;
}

task main()
{

	waitForStart();
	//	int compOffSet=SensorValue[compass];
	//	int rotateAmount=20;//rotate amount in degrees for second part
	//	int rotateTarget=compOffSet;//for 2nd half
	int rotateTarget;
	bool debug=false;
	//	int rotateSpeed=50;

	int fieldlength=176;//cm
	int failsafedis=-25600;//600*(fieldlength/19);//600 on the motor encoder results in moving 19 cm
	int dropdis    = 9;//cm
	int walldis    = 15;//cm
	int speed      = -80;//for wall follow
	int armSpeed   = 20;
	int rotateSpeed;
	int timeSensorEnable=0;
	int stopDis;
	tMotor leftSide[2]={leftSpliced, left};
	tMotor rightSide[2]={rightSpliced, right};
	tMUXSensor lSonar=msensor_S4_2;
	tMUXSensor fSonar=msensor_S4_4;
	tMUXSensor bSonar=msensor_S4_3;

	USreadDist(fSonar);//dummy to stop random 0 from passing
	USreadDist(bSonar);//dummy to stop random 0 from passing
	USreadDist(lSonar);//dummy to stop random 0 from passing

	tMotor motorName;
	for (int ii=0; ii<2; ii++){//resetting encoders
		motorName=leftSide[ii];
		nMotorEncoder[motorName]=0;
	}

	for (int ii=0; ii<2; ii++){
		motorName=rightSide[ii];
		nMotorEncoder[motorName]=0;
	}

	servo[door]=10; //door closed -- doubling over as an attempt to fix servo bug
	init();
	servo[door]=10; //door closed
	servo[spin1]=127; // not spinning
	servo[spin2]=127; // not spinning
	servo[catchServo]=100;//catch up

	//START MOVING DOWN RAMP FOLLOWING WALL
	if (debug)writeDebugStreamLine("\n********************\nSTART WALL FOLLOWING\n");
	wallfollow(walldis,speed,dropdis,failsafedis,bSonar,lSonar,leftSide,rightSide,-12799, 500, 3,true,true);
	if (debug)writeDebugStreamLine("\n********************\nEND WALL FOLLOWING\n");

	// ADJACENT TO GOAL, PUT CATCH DOWN
	if (debug)writeDebugStreamLine("\n********************\nOVERLAPPING GOAL\n");
	servo[catchServo]=0;//catch down
	wait1Msec(500);
	if (debug)writeDebugStreamLine("\n********************\nCATCH IS DOWN\n");

/*	servo[door]=150; //old/complicated other one works better
	wait1Msec(300);
	servo[spin1]=255;
	servo[spin2]=0;
	wait1Msec(1000);
	servo[spin1]=127;
	servo[spin2]=127;
	wait1Msec(200);
*/

	if (debug)writeDebugStreamLine("\n********************\nPUT THE ARM UP\n");

	motor[arm]=0;
	nMotorEncoderTarget[arm]=4500;
	motor[arm]=armSpeed;
	servo[spin1]=104;
	servo[spin2]=150;
	while (nMotorRunState[arm]!=runStateIdle){
		if (debug)writeDebugStreamLine("ARM ENC: %d", nMotorEncoder[arm]);
		if (nMotorEncoder[arm]>500){
			if (debug)writeDebugStreamLine("\n********************\nOPEN DOOR\n");
			servo[door]=150;
			servo[spin1]=127;
			servo[spin2]=127;
		}
	}
	motor[arm]=0;
	wait1Msec(1000);

	//SECOND Lurch//
	if (debug)writeDebugStreamLine("\n********************\nSECOND LURCH\n");
	motor[arm]=0;
	nMotorEncoderTarget[arm]=80;
	motor[arm]=armSpeed;
	while (nMotorRunState[arm]!=runStateIdle){
		if (debug)writeDebugStreamLine("ARM ENC: %d", nMotorEncoder[arm]);
	}
	motor[arm]=0;
	wait1Msec(500);


	// ROTATE TO NEW BEARING ON SPOT
	rotateTarget=20;
	speed = 100;

	compassfollow(speed,rotateTarget,compass,bSonar,lSonar,leftSide,rightSide,true,-1,-1,true,true);

	//RETRACT ARM//
	motor[arm]=-armSpeed*4;
	while (SensorValue[touch]!=1){
	}
	motor[arm]=0;

	// THEN DRIVE ON SIMILAR BEARING UNTIL HITS WALL
	rotateTarget=15;
	speed = 100;
	timeSensorEnable=0;//1 ms increments
	stopDis=20;

	compassfollow(speed,rotateTarget,compass,fSonar,lSonar,leftSide,rightSide,false,stopDis,timeSensorEnable, true,true);

	// Rotate to get in the parking zong
	rotateTarget=50;
	speed = 50;

	compassfollow(speed,rotateTarget,compass,bSonar,lSonar,leftSide,rightSide,true,-1,-1,true,true);
}
