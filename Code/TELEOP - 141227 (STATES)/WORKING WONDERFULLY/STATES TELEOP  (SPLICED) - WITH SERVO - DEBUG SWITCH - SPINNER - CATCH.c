#pragma config(Hubs,  S1, HTMotor,  HTMotor,  HTMotor,  HTServo)
#pragma config(Sensor, S2,     ,               sensorI2CHiTechnicCompass)
#pragma config(Sensor, S3,     touch,          sensorTouch)
#pragma config(Motor,  motorA,          unused1,       tmotorNXT, openLoop)
#pragma config(Motor,  motorB,          unused2,       tmotorNXT, openLoop)
#pragma config(Motor,  motorC,          unused3,       tmotorNXT, openLoop)
#pragma config(Motor,  mtr_S1_C1_1,     unused4,       tmotorTetrix, openLoop)
#pragma config(Motor,  mtr_S1_C1_2,     arm,           tmotorTetrix, PIDControl, reversed, encoder)
#pragma config(Motor,  mtr_S1_C2_1,     FLeft,         tmotorTetrix, PIDControl, driveLeft, encoder)
#pragma config(Motor,  mtr_S1_C2_2,     BLeft,         tmotorTetrix, PIDControl, driveLeft, encoder)
#pragma config(Motor,  mtr_S1_C3_1,     FRight,        tmotorTetrix, PIDControl, reversed, driveRight, encoder)
#pragma config(Motor,  mtr_S1_C3_2,     BRight,        tmotorTetrix, PIDControl, reversed, driveRight, encoder)
#pragma config(Servo,  srvo_S1_C4_1,    door,                 tServoStandard)
#pragma config(Servo,  srvo_S1_C4_2,    spin1,                tServoContinuousRotation)
#pragma config(Servo,  srvo_S1_C4_3,    spin2,                tServoContinuousRotation)
#pragma config(Servo,  srvo_S1_C4_4,    catchServo,           tServoStandard)
#pragma config(Servo,  srvo_S1_C4_5,    servo5,               tServoNone)
#pragma config(Servo,  srvo_S1_C4_6,    servo6,               tServoNone)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

//141227 - TELEOP WITH SERVO (DEBUG PRINT CAN BE ENABLED BY THE VARIBLE "debug")
//Timers used: T1 door - auto spinner,
//NOTES FOR NEXT TIME: MAKE A DEAD BAND FOR BOTTOM VALUE <-- also add a push stop (button)

#include "JoystickDriver.c"
#include "Transfer2.0.c"
//#include "nMotorEncoderTargetAbs.c"


int nMotorEncoder_last[11];	// make this available for all motors - to start state when last checked


int armSpeedSpecial(int armSpeed, int roundup, int rounddown){//different speeds for different levels (arm) -- add them HERE
	if (roundup==4 && rounddown==3){
		return armSpeed/2;
	}
	else{
		return armSpeed;
	}
}

void init(){
	//ENCODERS//
	nMotorEncoder[arm]=0;

	//SERVOS//
	servoChangeRate[door] = 2;
	servoChangeRate[catchServo]= 5;

	for (int ii=0; ii<sizeof(nMotorEncoder_last)/sizeof(nMotorEncoder_last[0]);ii++){//debug uses
		nMotorEncoder_last[ii]=32767;
	}
}

task main()
{
	bool debug=false;

	if (debug)writeDebugStreamLine("\n\n\n=====START=======");

	/*Motor map:

	Motor Controller 1 = arm motors(not called in)
	Motor Controller 2 = left motor 1 & 2, spliced
	left motor 3
	Motor Controller 3 = right motor 1 & 2, spliced
	right motor 3*/

	//varibles defined
	int armSpeed=30;
	int dband = 10; // Deadband for joystick
	int joylevels[5]={0,620,2350,4230,4445};
	int tophat_old=-1; // Last tophat value, initialize in Neutral position -1
	int tophat_last=-2; // Used only to decide on chnage event for debug printing
	bool manualused=false;
	int loopNum = 0;
	int button_old1=-1;
	int button_old2=-1;
	float speedGainDriveHigh=1;//faster speed
	float speedGainDriveLow=0.5;//slower speed
	float speedGainDrive=speedGainDriveHigh;//(what to multiply the standard gain by), INITIALLY SET TO LOW
	float speedGainArmHigh=1;
	float speedGainArmLow=0.5;
	float speedGainArm=speedGainArmHigh;//(what to multiply the standard gain by), INITIALLY SET TO LOW
	int lowerLevelDb=30;//deadband for tophat on low level
	int upperLevelDb=30;//deadband for tophat on high level
	int armLevelDb=30;//used line 301 for deadBanding a check
	int movement=0;//for auto spinners 0 = off, 1 = down, 2 = up

	//varibles undefined
	int roundup;
	int rounddown;
	int joy_1y1;
	int joy_1y2;
	int joy_2y1;
	int joy_2y2;
	int tophat;
	int buttons_joy2;
	int buttons_joy1;

	//BUTTON SETUP//
	bool luccomputer=false; // Set to TRUE if switch on back of controller is set to "X", FALSE if "D"

	//for driver joystick
	int speedButtonDrive=32;
	int catchEngage;
	int catchDisengage;

	//for gunner joystick
	bool spinneron=false;
	int doorbutton;
	int spinnerIn;
	int spinnerOut;
	int speedButtonArm;
//	int autoSpinnerButton=32;
	bool autoSpinner=false;
	int spinnerSpeedOut=0;//0-126 BACKWARDS (0 IS FULL BACK), 127 STILL, 128-255 FORWARD (255 IS FULL FORWARD) -- spin1 leads
	int spinnerSpeedIn=255;
	int catchDownPos=0;//change in future
	int catchUpPos=100;
	bool dooropen=true;
	int dooropenpos=150;
	int doorclosedpos=5;

	// These button's change for different controllers
	if (luccomputer){//if it is on lucs comp (set to X on controller)
		//driver's control buttons
		catchEngage=1;
		catchDisengage=2;
		//gunner's control buttons
		doorbutton=4;
		spinnerIn=1;
		spinnerOut=2;
		speedButtonArm=32;
	}
	else // "D" type controller (for competition)
	{
		//driver's control buttons
		catchEngage=2;
		catchDisengage=4;
		speedButtonDrive=32;

		//gunner's control buttons
		doorbutton=1;
		spinnerIn=2;
		spinnerOut=4;
		speedButtonArm=32;
		//autoSpinnerButton=32;

	}

	// Debug initial joystick
	//	for(int ii=0; ii<10;ii++)
	//	{		getJoystickSettings(joystick);		tophat=joystick.joy2_TopHat;		writeDebugStreamLine("tophat = %d",tophat); }


	init(); // Set up encoders, servos

	//servo sets
	servo[catchServo]=catchUpPos;
	servo[spin1]=127;
	servo[spin2]=127;
	while (true)
	{
		loopNum++;

		//----------------------------JOYSTICK-----------------------------
		// Controls the wheels and the arm
		getJoystickSettings(joystick);
		joy_1y1=transfer_J_To_M(joystick.joy1_y1, dband,(150./192.)*(float)speedGainDrive);//Driver Joy
		joy_1y2=transfer_J_To_M(joystick.joy1_y2, dband, (150./192.)*(float)speedGainDrive);
		joy_2y1=transfer_J_To_M(joystick.joy2_y1, dband, (100./320.)*(float)speedGainArm);//Gunner Joy
		joy_2y2=transfer_J_To_M(joystick.joy2_y2, dband, (100./320.)*(float)speedGainArm);
		tophat=joystick.joy2_TopHat;
		buttons_joy1=joystick.joy1_Buttons;
		buttons_joy2=joystick.joy2_Buttons;

		// Wheel speed
		motor[FLeft]=joy_1y1;
		motor[BLeft]=joy_1y1;
		motor[FRight]=joy_1y2;
		motor[BRight]=joy_1y2;
		if (debug && true){
			writeDebugStreamLine("left: %d, right: %d",nMotorEncoder[FLeft],nMotorEncoder[FRight]);
		}

		if (debug){
			//writeDebugStreamLine("%d",ServoValue[door]);//STATUS PRINTS
			//nxtDisplayCenteredBigTextLine(3,"%d",motor[FRight]);
			//nxtDisplayCenteredBigTextLine(6,"%d",joystick.joy1_y1);
			//writeDebugStreamLine("Mid: %d, Back:%d",nMotorEncoder[FRight],nMotorEncoder[BRight]);

			//STATUS EVERY LOOP
			if(0) writeDebugStreamLine("STATUS: (arm) Enc,Target,Speed = %d,%d,%d"
				,nMotorEncoder[arm],nMotorEncoderTarget[arm],motor[arm]);

			//Arm Position
			int me = nMotorEncoder[arm];
			//if( me != nMotorEncoder_last[arm] ) // Report any change
			if( abs(me - nMotorEncoder_last[arm])>=20 ) // Report when position changes by >= 20
			{
				writeDebugStreamLine("(%4d)UPDATED VALUE: (arm) Encoder[%d] = %d",loopNum,arm,me);
				nMotorEncoder_last[arm] = me;
			}

			//Arm Motor//
			if(tophat != tophat_last){
				writeDebugStreamLine("(%4d)UPDATED VALUE:  Tophat = %d",loopNum,tophat);
				tophat_last = tophat;
			}
		}//END DEBUG


		//----------------------------BUTTONS-----------------------------

		//Driver Buttons
		if (buttons_joy1==speedButtonDrive){//speed up button
			//button is held down
			speedGainDrive=speedGainDriveLow;
			//if (debug)writeDebugStreamLine("SPEED");
		}
		else{
			//button is released
			speedGainDrive=speedGainDriveHigh;
			//if (debug)writeDebugStreamLine("no speed");
		}

		if (buttons_joy1!=button_old1){
			if (buttons_joy1==catchEngage){//catch block
				servo[catchServo]=catchDownPos;
			}
			else if (buttons_joy1==catchDisengage){
				servo[catchServo]=catchUpPos;
			}
		}
		button_old1=buttons_joy1;

		//Gunner Buttons
		if (buttons_joy2==speedButtonArm){//arm slow down button
			//button is held down
			speedGainArm=speedGainArmLow;
			if (debug)writeDebugStreamLine("arm slow");
		}
		else{
			//button is released
			speedGainArm=speedGainArmHigh;
			if (debug)writeDebugStreamLine("arm normal");
		}

		if (buttons_joy2!=button_old2){
			if (buttons_joy2==doorbutton && dooropen==false){//open door
				if (debug) writeDebugStreamLine("    OPEN door");
				dooropen=true;
				servo[door]=dooropenpos;
			}
			else if (buttons_joy2==doorbutton && dooropen==true){
				if (debug) writeDebugStreamLine("    CLOSE door");
				dooropen=false;
				servo[door]=doorclosedpos;
			}

			if (!autoSpinner){//spinner block
				if (buttons_joy2==spinnerIn && spinneron==false){
					if (debug) writeDebugStreamLine("    INHALE spinner");
					spinneron=true;//forward
					servo[spin1]=spinnerSpeedOut;
					servo[spin2]=spinnerSpeedIn;
				}
				else if ( (buttons_joy2==spinnerIn || buttons_joy2==spinnerOut) && spinneron==true){
					if (debug) writeDebugStreamLine("    STOP spinner");
					spinneron=false;//stop
					servo[spin1]=127;
					servo[spin2]=127;
				}
				else if (buttons_joy2==spinnerOut && spinneron== false){
					spinneron=true;//back
					servo[spin1]=spinnerSpeedIn;
					servo[spin2]=spinnerSpeedOut;
				}
			}
/*
			if (buttons_joy2==autoSpinnerButton){//auto spinner (not working)
				autoSpinner=!autoSpinner;
			}*/
		}
		button_old2=buttons_joy2;


		if (nMotorRunState[arm]==runStateIdle){
			movement=0;
		}


		//Spinner Arm Level Check

		if (autoSpinner){//auto-spinner block
			if (nMotorEncoder[arm]<joylevels[1] /*&& time1(T1)>800 */&& !dooropen){
				//shut-off spinners when door is closed (waits a bit)
				if (debug)writeDebugStreamLine("door closed, stop spinners");
				servo[spin1]=127;
				servo[spin2]=127;
			}
			else if (nMotorEncoder[arm]<joylevels[1] && (movement==1 || movement==0)){//down or stopped and below joylevels[1]
				servo[spin1]=spinnerSpeedIn;//in
				servo[spin2]=spinnerSpeedOut;
			}
			else if (movement==2 || movement==0){//up or stopped (stopped but is above joylevels[1])
				servo[spin1]=spinnerSpeedOut;//out because spin1 leads
				servo[spin2]=spinnerSpeedIn;
			}

			if (dooropen){//if the door is open
				if (debug)writeDebugStreamLine("door closed resetting (%d)",ServoValue[door]);
				clearTimer(T1);
			}
		}



		//Arm Stop Checking ON LOW LEVEL// -- TO STOP POSITION HOLD WHEN ON LOWEST LEVEL
		// We implement a deadband do the motor doesn't keep running when arm is very close to lowest level
		// *** Could consider having this kick in only after a few seconds at lowest position
		if (nMotorRunState[arm]==runStateIdle && (nMotorEncoder[arm]<joylevels[0]+lowerLevelDb)){
			motor[arm]=0;
		}

		if (SensorValue[touch]==1 && movement==1){//down and is pushing button
			motor[arm]=0;
		}

		if (joy_2y1!=0){
			// If the gunner's joystick is being used then THIS MANUAL MODE OVERRIDES semi-automatic modes below

			if (joy_2y1>0){
				movement=2;
			}
			else{//must be down
				movement=1;
			}


			manualused=true;
			nMotorEncoderTarget[arm]=0; // Turn off target value before setting speed

			if (movement==1 && SensorValue[touch]){
				nMotorEncoder[arm]=0;
			}
			else {
				motor[arm]=joy_2y1;
			}
		}
		else if ((tophat==0 || tophat==4)&&tophat!=tophat_old){//Tophat
			// SEMI-AUTOMATIC MODE
			if (debug)writeDebugStreamLine("ENTERING TOPHAT");
			manualused=false;


			if (tophat==0){//tophat up (auto spinners)
				movement=2;
			}
			else{//tophat down
				movement=1;
			}


			//Checking levels if top or bottom, else it goes into for loop
			if (nMotorEncoder[arm]<=joylevels[0]+lowerLevelDb){
				if (debug){
					writeDebugStreamLine("on/below bottom");
					writeDebugStreamLine("%d",nMotorEncoder[arm]);
				}

				rounddown=-1;
				roundup=1;
			}
			else if(nMotorEncoder[arm]>=joylevels[(sizeof(joylevels)/sizeof(joylevels[0]))-1]-upperLevelDb){
				if (debug){
					writeDebugStreamLine("above top");
					writeDebugStreamLine("%d",nMotorEncoder[arm]);}

				rounddown=(sizeof(joylevels)/sizeof(joylevels[0]))-2;
				//-2 because it is the second highest level

				roundup=-1;
			}
			else {
				for (int ii=0; ii<(sizeof(joylevels)/sizeof(joylevels[0]))-1;ii++){
					//added -1 to middle condition /\ so the ii+1 wouldn't go over joylevels

					if (joylevels[ii]+armLevelDb<nMotorEncoder[arm] && joylevels[ii]-armLevelDb>nMotorEncoder[arm]){
						//deadBanded check to see if already on a level

						if (debug)writeDebugStreamLine("%d",nMotorEncoder[arm]);

						rounddown=ii-1;//already know it isnt on bottom or top, cuz the first two if's
						roundup=ii+1;
						break; // leave the for loop through joylevels
					}
					else if (nMotorEncoder[arm]>joylevels[ii]&&nMotorEncoder[arm]<joylevels[ii+1]){
						//I know ii+1 wont go above the number of indexes in joylevels because of previous if statements
						if (debug){
							writeDebugStreamLine("perfect: d: %d, U: %d",rounddown,roundup);
							writeDebugStreamLine("%d",nMotorEncoder[arm]);
						}

						rounddown=ii;
						roundup=ii+1;
						break;
					}
				}
			}//end of roundup/rounddown block

			if (debug){
				writeDebugStreamLine("\nrounddown (idx=%d) , roundup (idx=%d)",rounddown,roundup);
				for (int ii=0; ii<sizeof(joylevels)/sizeof(joylevels[0]);ii++) writeDebugStream("[%d]=%d ,",ii,joylevels[ii]);
				writeDebugStreamLine("\n");
				writeDebugStreamLine("SET MOTOR SPEED to %d from %d in advance of setting target", 0 , motor[arm] );
			}
			motor[arm]=0;//setting to 0 is crutial otherwise the encoderTarget wont work

			if (tophat==0 && roundup != -1){ //up
				if (debug)writeDebugStreamLine("DRIVING UP to %d from %d",joylevels[roundup] , nMotorEncoder[arm]);
				nMotorEncoderTarget[arm] = joylevels[roundup] - nMotorEncoder[arm];//setting motorEncoder to "-nMotorEncoderTarget[arm]"
				//because it is relative
				if (debug){writeDebugStreamLine("doing %d = %d - %d", nMotorEncoderTarget[arm], joylevels[roundup], nMotorEncoder[arm]);
					writeDebugStreamLine("SET MOTOR SPEED to %d from %d", armSpeed , motor[arm] );
				}
				motor[arm]=armSpeed;
				//motor[arm]=armSpeedSpecial(armSpeed, roundup, rounddown);
			}
			else if (tophat==4 && rounddown != -1 && SensorValue[touch]!=1){ //down and arm button isn't pushed
				if (debug)writeDebugStreamLine("DRIVING DOWN to %d from %d",joylevels[rounddown], nMotorEncoder[arm]);
				nMotorEncoderTarget[arm] = (joylevels[rounddown] - nMotorEncoder[arm]);
				if (debug){writeDebugStreamLine("doing %d = %d - %d", nMotorEncoderTarget[arm], joylevels[rounddown], nMotorEncoder[arm]);
					writeDebugStreamLine("SET MOTOR SPEED to %d from %d", -armSpeed , motor[arm] );}
				motor[arm]=-armSpeed;
				//motor[arm]=armSpeedSpecial(-armSpeed, roundup, rounddown);
			}
			else{
				if (debug)writeDebugStreamLine("SET MOTOR SPEED to %d from %d", 0 , motor[arm] );
				motor[arm]=0;//just in case something weird happens
			}
		}
		else if (manualused==true){//manualused
			//150124 /\ removed "joy_2y1==0 &&" because this will only execute if the joy and tophat arn't being used
			manualused=false;
			motor[arm]=0;
		}
		tophat_old=tophat;
	}
}

// End
