#include "MyRobot.h"


// Robot class- contains all code, except code for the camera (except not any more.)
class Robot : public IterativeRobot
{

	// Declare variable for the robot drive system
	RobotDrive *drivetrain; 

	// Compressor
	Compressor *compressor;

	/* 
	 * Declare variables for the controllers being used (12 buttons per controller)
	 * driverL is left joystick
	 * driverR is right joystick
	 * Button numbers...are written on the joysticks.
	 */
	Joystick *driverL;
	Joystick *driverR;
	Joystick *manipulator;
	 
	// Declare a variable to use to access the driver station
	DriverStation *m_ds;
	// For printing to Driver Station
	DriverStationLCD *m_dsLCD;


	// Declare encoders
	Encoder *right_encoder;
	Encoder *left_encoder;

	// Solenoids
	Solenoid *ramp_down;
	Solenoid *ramp_up;
	Solenoid *gear_up;
	Solenoid *gear_down;
	
	Solenoid *lock_in; //TODO New Code
	Solenoid *lock_out;


	// Roller spike (relay)
	Victor *roller_grabber;

	// flailing sweeper - AKA noodles/pasta (surgical tubing)
	Victor *noodlesA;
	Victor *noodlesB;

	// shooter trigger wheel
	Relay *trigger_wheel;
	
	// Shooter speed controllers
	Victor *shooterA;
	Victor *shooterB;

	// Limit switch by trigger wheel
	DigitalInput *triggerLimitSwitch;
	
	
	// DriveTrainValues
	float right;
	float left;
	float oldright;
	float oldleft;
	float useright;
	float useleft;
	float threshold;
	float leftchange; // the difference between oldleft and useleft
	float rightchange; // the difference between oldright and useright

	// Deadzone on joysticks
	float leftdead; // left deadzone threshold
	float rightdead; // right deadzone threshold
	float leftdeadslow; // left deadzone threshold for slow-mo
	float rightdeadslow; // right deadzone threshold for slow-mo

	// shooter wheel toggler thing.
	bool prep_toggle;
	bool listenForPrepButton;
	int currentShooterSpeed;
	
	// ramp toggler thing
	Toggler *rampToggle;

	// pasta and roller toggle
	Toggler *PastaRollerToggle;
	
	//Timer for the lock on the bridge manip
	Timer *RampTimer; //TODO New Code
	
	AnalogChannel *currentSensor;

	
public:

	// Constructor (initialize variables)
	Robot (void)
	{
		// Create a robot using standard right/left robot drive on PWMS 1, 2, 9, and 10
		drivetrain = new RobotDrive(1, 2, 9, 10);
		// Overrides the default expiration time (0.5 seconds) and sets to 15 seconds
		// the length of autonomous
		drivetrain->SetExpiration(15);

		// GetInstance gets a pointer to a driver station
		m_ds = DriverStation::GetInstance();
		m_dsLCD = DriverStationLCD::GetInstance();

		// Declare compressor
		// (switch-GPIO, relay)
		compressor=new Compressor(12,1);

		// Define joystick USB ports on the Driver Station for joysticks
		driverR = new Joystick(1);
		driverL = new Joystick(2);
		manipulator = new Joystick(3);

		// Define encoders (channel A, channel B, true, k4X)
		// True tells the encoder to invert the counting direction
		right_encoder = new Encoder(6, 5, true);
		left_encoder = new Encoder(1, 2, true);

		// Solenoids
		ramp_down=new Solenoid(3);
		ramp_up=new Solenoid(4);
		gear_down = new Solenoid(1);
		gear_up = new Solenoid(2);
		
		lock_in=new Solenoid(5); //TODO New Code
		lock_out=new Solenoid(6);

		// Spinning sweeper (pasta)
		noodlesA = new Victor(6); 
		noodlesB = new Victor(7); 
		
		// Shooter trigger wheel (victor)
		trigger_wheel = new Relay(2); //change

		// Shooter motors
		shooterA = new Victor(3); 
		shooterB = new Victor(4); 

		// Roller speed controller
		roller_grabber = new Victor(8); //change

		// Limit switch near trigger wheel
		triggerLimitSwitch = new DigitalInput(14);


		// DriveTrainValues
		threshold=0.2;
		right=0;
		left=0;
		oldright=0;
		oldleft=0;
		useright=0;
		useleft=0;
		leftchange=0;
		rightchange=0;

		// deadzone
		leftdead=0.02;
		rightdead=0.02;
		leftdeadslow=0.01;
		rightdeadslow=0.01;

		// shooter toggler (shooter wheel)
		prep_toggle = false;
		listenForPrepButton = true;
		currentShooterSpeed = NO_SPEED;
		
		// Ramp toggler
		rampToggle = new Toggler();

		// Pasta and roller toggle
		PastaRollerToggle = new Toggler();
		
		//Timer for the lock on the bridge manip
		RampTimer= new Timer(); //TODO: New Code
		
		currentSensor = new AnalogChannel(7);
	}

	
	/******************* Inits ************************/

	// Actions which will be performed once (and only once) upon initialization of the robot are here
	void RobotInit(void) 
	{
		// Start compressor
		compressor->Start();
	}

	// First time the disabled mode is called
	void DisabledInit(void) 
	{
		// stop everything
		right_encoder->Stop();
		left_encoder->Stop();
		roller_grabber->Set(0.0);
		noodlesA->Set(0.0);
		noodlesB->Set(0.0);
		shooterA->Set(0.0);
		shooterB->Set(0.0);
		trigger_wheel->Set(Relay::kOff);
		// reset shooter toggler
		prep_toggle = false;
		listenForPrepButton = true;
	}


	// First time autonomous mode is called
	void AutonomousInit(void) 
	{
		// This applies to all autonomous/hybrid modes...
		
		// start compressors
		compressor->Start();
		// start encoders
		right_encoder->Start();
		left_encoder->Start();
		left_encoder->Reset();
		right_encoder->Reset();

		lock_in->Set(false); 
		lock_out->Set(true);
					
		// Find out what digital I/Os are on in the driver station to determine what autonomous mode to use
		bool auto1 = m_ds->GetDigitalIn(1);	// shoot + turn
		bool auto2 = m_ds->GetDigitalIn(2);	// bridge + feed (untested)
		bool auto3 = m_ds->GetDigitalIn(3);	// shoot
		bool auto4 = m_ds->GetDigitalIn(4);	// delayed shoot (untested)
		bool auto5 = m_ds->GetDigitalIn(5);	// clear system
		bool auto6 = m_ds->GetDigitalIn(6);	// shoot slower + turn
		bool auto7 = m_ds->GetDigitalIn(7);	// shoot slower
		bool auto8 = m_ds->GetDigitalIn(8);	// shoot fed balls (super untested)


		
		double autonomousDelayTime = m_ds->GetAnalogIn(1) + m_ds->GetAnalogIn(2); // Could be 0 and 1, but it shouldn't be according to WPILib Reference
		double autonomousDelayTime2 = m_ds->GetAnalogIn(3) + m_ds->GetAnalogIn(4); // Delay after the first ball is spit out
		
		// An idea that may be used at the competition
		// bool speed_low = m_ds->GetDigitalIn(5);
		// bool speed_medium = m_ds->GetDigitalIn(6);
		// bool speed_high = m_ds->GetDigitalIn(7);

		/************ Autonomous Number 1 - shoot two balls turn around and drive to ramp***********/
		if(auto1)
		{	
			
			// Set robot to low gear
			gear_down->Set(true);
			gear_up->Set(false);
			
			// Send the drivetrain 0's
			drivetrain->TankDrive(0.0, 0.0);

			// Lower the ramp lowerer
			ramp_down->Set(true); 
			ramp_up->Set(false);
			
			// Start shooter wheels and wait for wheels to get up to speed
			shooterA->Set(1.0);
			shooterB->Set(-1.0);
			Wait(3.7); 
			
			//Now lock the bridge manip
			//TODO New Code
			//After the 3.7 second wait, the bridge manipulator will definitely be out
			lock_in->Set(true); 
			lock_out->Set(false);
			
			// Run trigger for x seconds
			trigger_wheel->Set(Relay::kForward);
			Wait(1.0); 
			
			// wait for ball to fire then turn off shooter wheel
			trigger_wheel->Set(Relay::kOff);
			Wait(0.5);
			
			// Start roller grabber and pasta (to bring second ball around)
			roller_grabber->Set(1.0);
			noodlesA->Set(-0.75);
			noodlesB->Set(-0.75);
			Wait(0.5);	// wait for ball to go to trigger wheel
			
			// Run trigger wheel and wait for ball to fire
			trigger_wheel->Set(Relay::kForward);
			Wait(2.5);

			// Turn shooting stuff off
			shooterA->Set(0.0);
			shooterB->Set(0.0);
			trigger_wheel->Set(Relay::kOff);
			roller_grabber->Set(0.0);
			noodlesA->Set(0.0);
			noodlesB->Set(0.0);


			// Use encoder values to determine how much to turn robot (180 degrees)
			while((right_encoder->Get() < 1262) && (left_encoder->Get() > -1262))
			{
				drivetrain->TankDrive(-1.0, 1.0);	// Turns right
			}
			drivetrain->TankDrive(0.0, 0.0); // Stop turning when robot has turned 180 degrees

			// A brief pause
			Wait(0.5);
			
			//keep the ramp lowered in the beginning of tele-op
			rampToggle->Set(true);
			
			// Drive the robot forward in a straight line
			// Drivetrain values correct for mechanical/electrical issues
			drivetrain->TankDrive(0.68, 0.65);
			Wait(1.0);
			
			// Slow down the drivetrain
			drivetrain->TankDrive(0.42, 0.39);
			Wait(0.8);
			
			// Stop the robot
			drivetrain->TankDrive(0.0, 0.0);
			
		}
		/****************** End of Autonomous 1 ******************/

		/********** Autonomous 2 - drive to ramp, knock down ramp, turn around and shoot balls*********/
		if(auto2)
		{
			// Warning: This code has not been properly tested. 

			// Deploy pistons to lower the ramp lowererer
			ramp_down->Set(true); 
			ramp_up->Set(false);

			//TODO New Code
			//Wait for the bridge manipulator to come out
			Wait(1.0);
			//Now lock the bridge manip
			lock_in->Set(true); 
			lock_out->Set(false);
						
			// Start the rolly grabber to keep the two balls in the robot from falling out	
			roller_grabber->Set(1.0);
			
			// Start pasta to keep the balls in the robot
			noodlesA->Set(-0.5);		
			noodlesB->Set(-0.5);	
			
			// Reset encoders
			left_encoder->Reset();
			right_encoder->Reset();
			
			while ((right_encoder->Get() < 1262) && (left_encoder->Get() < 1262)) // Probably will be bigger
			{
				// Drive forward 
				drivetrain->TankDrive(0.68,0.65); 
			}
			// The robot should now have hit the bridge
			
			// Wait a bit before turning
			drivetrain->TankDrive(0.0,0.0);
			Wait(0.5);
			
			// Back away from the bridge
			drivetrain->TankDrive(-0.68, -0.65);
			Wait(0.7);
			
			// Reset encoders
			left_encoder->Reset();
			right_encoder->Reset();
			
			// Turn around 180 degrees
			while((right_encoder->Get() < 1262) && (left_encoder->Get() > -1262))
			{
				drivetrain->TankDrive(-1.0, 1.0); // Turns right
			}
			drivetrain->TankDrive(0.0, 0.0); // Stops turning

			Wait(0.2);
			
			//Drive back
			while ((right_encoder->Get() < 1262) && (left_encoder->Get() < 1262)) // Probably will be bigger
			{ 
				drivetrain->TankDrive(0.68,0.65);
			}
			
			// Run roller grabber and noodles backwards to spit out balls
			roller_grabber->Set(-1.0);
			noodlesA->Set(0.5);
			noodlesB->Set(0.5);
			
			Wait(autonomousDelayTime2);
			trigger_wheel->Set(Relay::kReverse);
		
			
			// Turn stuff off
			trigger_wheel->Set(Relay::kOff);
			noodlesA->Set(0.0);
			noodlesB->Set(0.0);
			roller_grabber->Set(0.0);
		}
		/************End of Autonomous 2***************/

		/*************** Beginning of Autonomous 3 - just shooting *****************/
		if(auto3)
		{

			// Set robot to low gear
			gear_down->Set(true);
			gear_up->Set(false);
			
			// Send the drivetrain 0's
			drivetrain->TankDrive(0.0, 0.0);

			// Lower the ramp lowerer
			ramp_down->Set(true); 
			ramp_up->Set(false);
			
			//keep the ramp lowered in the beginning of tele-op
			rampToggle->Set(true);
			
			// Start shooter wheels and wait for wheels to get up to speed
			shooterA->Set(1.0);
			shooterB->Set(-1.0);
			Wait(3.7); 
			
			//Now lock the bridge manip
			//TODO New Code
			//After the 3.7 second wait, the bridge manipulator will definitely be out
			lock_in->Set(true); 
			lock_out->Set(false);
			
			// Run trigger for x seconds
			trigger_wheel->Set(Relay::kForward);
			Wait(1.0); 
			
			// wait for ball to fire then turn off shooter wheel
			trigger_wheel->Set(Relay::kOff);
			Wait(0.5);
			
			// Start roller grabber and pasta (to bring second ball around)
			roller_grabber->Set(1.0);
			noodlesA->Set(-0.75);
			noodlesB->Set(-0.75);
			Wait(0.5);	// wait for ball to go to trigger wheel
			
			// Run trigger wheel and wait for ball to fire
			trigger_wheel->Set(Relay::kForward);
			Wait(2.5);

			// Turn shooting stuff off
			shooterA->Set(0.0);
			shooterB->Set(0.0);
			trigger_wheel->Set(Relay::kOff);
			roller_grabber->Set(0.0);
			noodlesA->Set(0.0);
			noodlesB->Set(0.0);

		}
		/****************** End of Autonomous 3 *****************/
		
		/****************** Autonomous 4 -delayed shoot********************/
		if(auto4)
		{

			// Set robot to low gear
			gear_down->Set(true);
			gear_up->Set(false);
			
			// Send the drivetrain 0's
			drivetrain->TankDrive(0.0, 0.0);

			// Lower the ramp lowerer
			//ramp_down->Set(true); 
			//ramp_up->Set(false);
			
			//keep the ramp lowered in the beginning of tele-op
			//rampToggle->Set(true);
			
			Wait(autonomousDelayTime); //delay according to analog inputs

			// Start shooter wheels and wait for wheels to get up to speed
			shooterA->Set(1.0);
			shooterB->Set(-1.0);
			Wait(3.7); //let them get up to speed
			
			
			// Run trigger for x seconds
			trigger_wheel->Set(Relay::kForward);
			Wait(1.0); 
			
			// wait for ball to fire then turn off shooter wheel
			trigger_wheel->Set(Relay::kOff);
			Wait(0.5);
			
			// Start roller grabber and pasta (to bring second ball around)
			roller_grabber->Set(1.0);
			noodlesA->Set(-0.75);
			noodlesB->Set(-0.75);
			Wait(0.5);	// wait for ball to go to trigger wheel
			
			// Run trigger wheel and wait for ball to fire
			trigger_wheel->Set(Relay::kForward);
			Wait(2.5);

			// Turn shooting stuff off
			shooterA->Set(0.0);
			shooterB->Set(0.0);
			trigger_wheel->Set(Relay::kOff);
			roller_grabber->Set(0.0);
			noodlesA->Set(0.0);
			noodlesB->Set(0.0);

		}
		
		/*************** Beginning of Autonomous 5 - clear systems to feed balls to another robot *****************/
		if(auto5)
		{
			// Send the drivetrain 0's
			drivetrain->TankDrive(0.0, 0.0);
			
			// Wait for however many seconds is set in the Driver Station
			Wait(autonomousDelayTime);
						
			// Run roller grabber and noodles backwards
			roller_grabber->Set(-1.0);
			noodlesA->Set(0.5);
			noodlesB->Set(0.5);
			
			Wait(autonomousDelayTime2);
			trigger_wheel->Set(Relay::kReverse);
		}
		/****************** End of Autonomous 5 *****************/
		
		/*************** Beginning of Autonomous 6 - Slower shooter with bridge *****************/
		if(auto6)
		{
			
			// Set robot to low gear
			gear_down->Set(true);
			gear_up->Set(false);
			
			// Send the drivetrain 0's
			drivetrain->TankDrive(0.0, 0.0);

			// Lower the ramp lowerer
			ramp_down->Set(true); 
			ramp_up->Set(false);
			
			// Start shooter wheels and wait for wheels to get up to speed
			shooterA->Set(0.88);
			shooterB->Set(-0.88);
			Wait(3.7); 

			//Now lock the bridge manip
			//TODO New Code
			//After the 3.7 second wait, the bridge manipulator will definitely be out
			lock_in->Set(true); 
			lock_out->Set(false);
			
			// Run trigger for x seconds
			trigger_wheel->Set(Relay::kForward);
			Wait(1.0); 
			
			// wait for ball to fire then turn off shooter wheel
			trigger_wheel->Set(Relay::kOff);
			Wait(0.5);
			
			// Start roller grabber and pasta (to bring second ball around)
			roller_grabber->Set(1.0);
			noodlesA->Set(-0.75);
			noodlesB->Set(-0.75);
			Wait(0.5);	// wait for ball to go to trigger wheel
			
			// Run trigger wheel and wait for ball to fire
			trigger_wheel->Set(Relay::kForward);
			Wait(2.5);

			// Turn shooting stuff off
			shooterA->Set(0.0);
			shooterB->Set(0.0);
			trigger_wheel->Set(Relay::kOff);
			roller_grabber->Set(0.0);
			noodlesA->Set(0.0);
			noodlesB->Set(0.0);


			// Use encoder values to determine how much to turn robot (180 degrees)
			while((right_encoder->Get() < 1262) && (left_encoder->Get() > -1262))
			{
				drivetrain->TankDrive(-1.0, 1.0);	// Turns right
			}
			drivetrain->TankDrive(0.0, 0.0); // Stop turning when robot has turned 180 degrees

			// A brief pause
			Wait(0.5);
			
			//keep the ramp lowered in the beginning of tele-op
			rampToggle->Set(true);
			
			// Drive the robot forward in a straight line
			// Drivetrain values correct for mechanical/electrical issues
			drivetrain->TankDrive(0.68, 0.65);
			Wait(1.0);
			
			// Slow down the drivetrain
			drivetrain->TankDrive(0.42, 0.39);
			Wait(0.8);
			
			// Stop the robot
			drivetrain->TankDrive(0.0, 0.0);
		}
		/******************* End of auto 6*****************/
		
		/**************** Auto 7 - slower speed without bridge **********************/
		if(auto7)
		{
			// Set robot to low gear
			gear_down->Set(true);
			gear_up->Set(false);
			
			// Send the drivetrain 0's
			drivetrain->TankDrive(0.0, 0.0);

			// Lower the ramp lowerer
			ramp_down->Set(true); 
			ramp_up->Set(false);
			
			//keep the ramp lowered in the beginning of tele-op
			rampToggle->Set(true);
			
			// Start shooter wheels and wait for wheels to get up to speed
			shooterA->Set(0.88);
			shooterB->Set(-0.88);
			Wait(3.7); 

			//Now lock the bridge manip
			//TODO New Code
			//After the 3.7 second wait, the bridge manipulator will definitely be out
			lock_in->Set(true); 
			lock_out->Set(false);
			
			// Run trigger for x seconds
			trigger_wheel->Set(Relay::kForward);
			Wait(1.0); 
			
			// wait for ball to fire then turn off shooter wheel
			trigger_wheel->Set(Relay::kOff);
			Wait(0.5);
			
			// Start roller grabber and pasta (to bring second ball around)
			roller_grabber->Set(1.0);
			noodlesA->Set(-0.75);
			noodlesB->Set(-0.75);
			Wait(0.5);	// wait for ball to go to trigger wheel
			
			// Run trigger wheel and wait for ball to fire
			trigger_wheel->Set(Relay::kForward);
			Wait(2.5);

			// Turn shooting stuff off
			shooterA->Set(0.0);
			shooterB->Set(0.0);
			trigger_wheel->Set(Relay::kOff);
			roller_grabber->Set(0.0);
			noodlesA->Set(0.0);
			noodlesB->Set(0.0);
		} 
		/****************** End of auto 7 *****************/
	
		/*************** Beginning of Autonomous 8 - shoot fed balls *****************/
		if(auto8)
		{

			// Set robot to low gear
			gear_down->Set(true);
			gear_up->Set(false);
			
			// Send the drivetrain 0's
			drivetrain->TankDrive(0.0, 0.0);
			
			//to deal with incoming balls
			roller_grabber->Set(1.0);

			// Lower the ramp lowerer
			//ramp_down->Set(true); 
			//ramp_up->Set(false);
			
			//keep the ramp lowered in the beginning of tele-op
			//rampToggle->Set(true);
			
			// Start shooter wheels and wait for wheels to get up to speed
			shooterA->Set(1.0);
			shooterB->Set(-1.0);
			Wait(3.7); 

			// Run trigger for x seconds
			trigger_wheel->Set(Relay::kForward);
			Wait(1.0); 
			
			// wait for ball to fire then turn off trigger wheel
			trigger_wheel->Set(Relay::kOff);
			Wait(0.5);
			
			// Start roller grabber and pasta (to bring second ball around)
			noodlesA->Set(-0.75);
			noodlesB->Set(-0.75);
			Wait(0.5);	// wait for ball to go to trigger wheel
			
			// Run trigger wheel and wait for ball to fire
			trigger_wheel->Set(Relay::kForward);
			Wait(1.0);
			

			while (true)
			{
				if(ballSensorValue())
				{
					trigger_wheel->Set(Relay::kOff);
					Wait(1.0);
					trigger_wheel->Set(Relay::kForward);
					Wait(0.5);
				}
				else
				{
					trigger_wheel->Set(Relay::kForward);
				}
			}
			
			
			// Turn shooting stuff off
			shooterA->Set(0.0);
			shooterB->Set(0.0);
			trigger_wheel->Set(Relay::kOff);
			roller_grabber->Set(0.0);
			noodlesA->Set(0.0);
			noodlesB->Set(0.0);

		}
		/***************** end of auto 8***/
	
	
	} // End of Autonomous Init


	// Run the first time Teleop is called
	void TeleopInit(void) 
	{
			
		// Start compressor
		compressor->Start();
		

		
		/*if(m_ds->GetDigitalIn(1) || m_ds->GetDigitalIn(2))
		{
			rampToggle->Set(true);
			ramp_down->Set(true);
			ramp_up->Set(false);
		}*/
		
		// Start the encoders
		right_encoder->Start();
		left_encoder->Start();
		
		// Indicate that there is no set shooting speed
		currentShooterSpeed = NO_SPEED;
		
		// Turn stuff off again
		roller_grabber->Set(0.0);
		noodlesA->Set(0.0);
		noodlesB->Set(0.0);
		trigger_wheel->Set(Relay::kOff);
	}

	
	/******************* Periodics - run continuously ********************/
	void DisabledPeriodic(void) 
	{

	}


	void AutonomousPeriodic(void) 
	{

	}


	void TeleopPeriodic(void) 
	{
		/* 
		 * Code placed in here will be called only when a new packet of information
		 * has been received by the Driver Station. Any code which needs new information
		 * from the DS should go in here.
		 */
		
		// Start compressor
		// Has to be the first line of code in void
		compressor->Start();
		
		// Miscellaneous Routines - each routine controls a different aspect of the robot
		// Routines run in order continuously
		
		// Drivetrain voids
		driveTrainValues();
		deadzone();
		gear();
		
		// Ramp void
		ramp();
		
		// Shooting voids
		pasta_roller();
		trigger();
		shooter_wheel(manipulator->GetRawButton(PREPARE_LOW_B), manipulator->GetRawButton(PREPARE_MED_B), manipulator->GetRawButton(PREPARE_HIGH_B), manipulator->GetRawButton(PREPARE_LOW_2_B), manipulator->GetRawButton(PREPARE_LOW_3_B));


		// This is where values are sent to the drivetrain
		// The values are calculated in the driveTrainValues void
		
		// When button three is pressed robot drives at 50% speed
		if ((driverL->GetRawButton(HALF_SPEED_B) || driverR->GetRawButton(HALF_SPEED_B)))
		{
			drivetrain->TankDrive((-useleft * -useleft * -useleft) * 0.4, (-useright * -useright * -useright) * 0.4);
			// Negative for reversed polarity (joystick values are probably backwards)
		}
		// Otherwise, drive normally
		else
		{
			drivetrain->TankDrive(-useleft * -useleft * -useleft, -useright * -useright * -useright);
			// Negative for reversed polarity
		}
		
		float voltage = currentSensor -> GetAverageVoltage();
		printf("Current sensor value: %f\n", voltage);
	}	

	/********************************** Miscellaneous Routines ****************************/

	void driveTrainValues(void) {

		/*
		 * This function prevents sudden acceleration of the robot, by placing a cap on the amount that the 
		 * robot's speed can increase or decrease each time a new joystick value is received.
		 * This prevents really large and fast movements of the joystick from translating immediately into
		 * really large acceleration.
		 */
		
		// Assign right joystick value to the variable 'right'
		if(m_ds->GetDigitalIn(8))
		{
			right=(driverL->GetTwist());
		}
		else if(m_ds->GetDigitalIn(7))
		{
			right=manipulator->GetY();
		}
		else
		{
			right=(driverR->GetY());
		}
		// Find the difference in the last two joystick values received
		rightchange=fabs(oldright-right);

		// If the change in joystick values is less than 0.2 then use the new right value
		// The change in the joystick value wasn't too sudden...
		// so the driver probably intended for the robot to move at this new speed
		if(rightchange<=threshold) {
			useright=right;
		}
		
		// If the change in the joystick values is dramatic (AKA the joystick was moved really fast)
		// Then, make the change in the speed of the robot less sudden by only allowing the robot to speed up
		// or slow down by a threshold value of 0.2 
		else {			
			
			// If the new right value is greater than the old one use the old value plus the threshold 
			// (because the driver intended the robot to speed up) 
			// Assign this speed as the speed that will be sent to the drivetrain
			if(oldright<right) {
				useright=oldright+threshold;
			}
			
			// If the new right is less than the old right use the old value minus the threshold 
			// (because the driver intended the robot to slow down)
			// Assign this speed as the speed that will be sent to the drivetrain
			if(oldright>right) {
				useright=oldright-threshold;
			}
		}

		// Do this same process for the left drivetrain values
		left=(driverL->GetY());
		leftchange=fabs(oldleft-left);

		if(leftchange<=threshold) {
			useleft=left;
		}
		else {
			if(oldleft<left) {
				useleft=oldleft+threshold;
			}
			if(oldleft>left) {
				useleft=oldleft-threshold;
			}
		}


		// Make useright and useleft (the values sent to the robot) the old values for the next loop
		oldright=useright;
		oldleft=useleft;
		
		// useleft and useright are sent to the drivetrain in the TeleopPeriodic void

	}

	
	void deadzone(void) {
		
		/*
		 *  If the joystick values, useleft or useright, are less 
		 *  than the small number "leftdead" or "rightdead" then do not move robot
		 *  The joysticks are in their neutral position, but are still sending out small values
		 */

		// The threshold values are different if the robot is driving in half speed mode
		if(driverR->GetRawButton(HALF_SPEED_B) || driverL->GetRawButton(HALF_SPEED_B))
		{
			if((useleft <= leftdeadslow) && (useleft >= -leftdeadslow))
			{
				useleft = 0;
			}
			if((useright <= rightdeadslow) && (useright >= -rightdeadslow))
			{
				useright = 0;
			}
		}
		// If the robot is not in half speed mode
		else
		{
			if((useleft <= leftdead) && (useleft >= -leftdead))
			{
				useleft = 0;
			}
			if((useright <= rightdead) && (useright >= -rightdead))
			{
				useright = 0;
			}
		}
	}

	
	void gear(void)
	{
		// This void allows the driver to switch between high and low gear
		
		// Gear down
		if (driverL->GetRawButton(SHIFT_DOWN_B))
		{
			gear_down->Set(true);
			gear_up->Set(false);
			
			// Display a message on the Driver Station to tell the driver the robot is in low gear
			//m_dsLCD->Clear();
			//m_dsLCD->PrintfLine((DriverStationLCD::Line) 0, "low gear");
			//m_dsLCD->UpdateLCD();
		}
		
		// Gear up
		else if (driverR->GetRawButton(SHIFT_UP_B))
		{
			gear_down->Set(false);
			gear_up->Set(true);
			
			// Display a message on the Driver Station to tell the driver the robot is in high gear
			//m_dsLCD->Clear();
			//m_dsLCD->PrintfLine((DriverStationLCD::Line) 0, "high gear");
			//m_dsLCD->UpdateLCD();
		}
	}


	
	void ramp(void)
	{
		// This void lowers and raises the ramp manipulator by deploying pistons
		
		// Create a toggle using the toggler class, which uses either of two buttons on the driver joysticks
		rampToggle->toggle(driverL->GetRawButton(LOWER_RAMP_B) || driverR->GetRawButton(LOWER_RAMP_B));

		// Set the ramp lowering pistons to true or false, based on the output of the toggle
		// If the ramp toggler returns true then bring the ramp down, if it returns false bring the ramp up
		
		//TODO Old code
		//ramp_down->Set(rampToggle->status());
		//ramp_up->Set(!rampToggle->status());

		
		/************* Testing ****************/
		
		//TODO New Code
		
		//TODO
		//Change solenoid numbers
		//Add lock to Autonomous
		
		//If the bridge manipulator is out and a certain # of seconds have past...
		//then lock the bridge manipulator and keep the bridge manipulator out
		
		if ((rampToggle->status() == true) && (RampTimer->Get() < 1.0) && (RampTimer->Get() > 0.0))
		{
			//The timer has started, but the bridge manipulator is not all the way out
			//bring the bridge manip down
			ramp_down->Set(true);
			ramp_up->Set(false);

		}
		else if ((rampToggle->status() == true) && (RampTimer->Get() > 1.0))
		{
			//The bridge manipulator is now all the way out
			//lock the manipulator in place
			//lock_in is piston out...
			lock_out->Set(false);
			lock_in->Set(true);
			
			//leave the bridge manipulator down
			ramp_down->Set(true);
			ramp_up->Set(false);
			
		}
		else if (rampToggle->status() == true)
		{
			//The timer has not started, this is right after the ramp down button is pressed
			//So, start the timer
			RampTimer->Start();
		}
		
		
		//The Ramp manipulator up button has been pressed...
		if(!rampToggle->status())
		{
			//reset the timer
			RampTimer->Reset();
			
			//Take out the lock
			lock_out->Set(true);
			lock_in->Set(false);
			
			//bring the ramp up
			ramp_down->Set(false);
			ramp_up->Set(true);
		}
		
		/*********************/
		
	}
	

	void pasta_roller(void)
	{
		// This void runs a toggle which turns the rolly grabber and pasta/sweeper on and off
		
		// Create a toggle using the toggler class, which uses a button on the manipulator joystick
		PastaRollerToggle->toggle(manipulator->GetRawButton(PASTA_ROLLER_B));

		
		// If the clear system button is pressed...
		// Get all of the balls out of the system, by running the rolly grabber backwards
		if(manipulator->GetRawButton(CLEAR_SYSTEM_B))
		{
			roller_grabber->Set(-1.0);
		}

		// If the toggle returns true...
		else if (PastaRollerToggle->status())
		{
			// Run the rolly grabber to suck in balls
			roller_grabber->Set(1.0); // negative because wires are backwards
			
			// Now run the pasta maker to cook the noodles...
			// If there already is a ball lock in the trigger wheel of the robot
			// then run pasta/noodles/sweeper at 75 percent speed
			if(ballSensorValue())
			{
				noodlesA->Set(-0.75);
				noodlesB->Set(-0.75);
			}
			// If there is no ball in the trigger wheel then run pasta/noodles/sweeper at full speed
			else
			{
				noodlesA->Set(-1.0);
				noodlesB->Set(-1.0);
			}
		}
		
		// If the toggler returns false
		else
		{
			// Turn off pasta and the rolly grabber
			noodlesA->Set(0.0);
			noodlesB->Set(0.0);
			roller_grabber->Set(0.0);
		}
	}


	void trigger(void)
	{
		/* This void controls when the trigger wheel is running.
		 * If the pasta and roller are on, then have the trigger wheel run constantly until a ball reaches 
		 * a sensor by the trigger wheel. When this happens, stop the trigger wheel to trap the ball near 
		 * the sensor. When, the manipulator presses the shoot button, run the trigger wheel again to allow the 
		 * trapped ball to reach the shooter wheel.
		 */
		
		// If clear the system button is pressed
		if(manipulator->GetRawButton(CLEAR_SYSTEM_B))
		{
			// Do not do anything
		}
		
		// If the shoot button is pressed and the shooter wheel is running (prep_toggle is true)...
		else if (manipulator->GetRawButton(SHOOT_B) && prep_toggle)
		{
			// Run the trigger wheel x seconds to shoot
			trigger_wheel->Set(Relay::kForward);
			Wait(0.5);
		}
		
		// If the shoot button is not pressed and the pasta and roller are running 
		// and the sensor by the trigger wheel is not pressed
		else if (PastaRollerToggle->status() && !ballSensorValue())
		{
			// Run the trigger wheel forwards until a ball reaches the trigger sensor
			trigger_wheel->Set(Relay::kForward);
		}

		// If the pasta and roller are running and the sensor is pressed
		else if (PastaRollerToggle->status() && ballSensorValue())
		{
			// Then stop the trigger wheel to trap the ball in the trigger wheel
			trigger_wheel->Set(Relay::kOff);
		}

		// If the PastaRollerToggle is off, then don't turn the trigger wheel
		else
		{
			trigger_wheel->Set(Relay::kOff);
		}
	}

	
	void shooter_wheel(bool a, bool b, bool c, bool d, bool e)
	{
		/* 
		 * Gets the shooter wheel up to speed. It is independent of the other systems.
		 * The shooter wheel is toggled with three buttons that change its speed. Pressing
		 * the button of the speed the wheel is currently at turns the wheel off. Pressing a 
		 * button for a speed that the wheel is not currently running at changes the speed.
		 */
		if(!a && !b && !c && !d && !e)
		{
			listenForPrepButton = true;
		}

		if(listenForPrepButton && (a || b || c || d || e)) // If the shooter should be toggled and a button is pressed.
		{
			// If you pressed the previous button again...
			if(currentShooterSpeed == NO_SPEED || (a && (currentShooterSpeed == LOW_SPEED)) || (b && (currentShooterSpeed == MED_SPEED)) || (c && (currentShooterSpeed == HIGH_SPEED)) || (d && (currentShooterSpeed == LOW_SPEED_2)) || (e && (currentShooterSpeed == LOW_SPEED_3)))
			{
				// toggle the shooter boolean, but only if you just pressed the same button.
				prep_toggle = !prep_toggle;
			}

			// Set what the currently selected shooter speed is
			if(a)
			{
				currentShooterSpeed = LOW_SPEED;
			} 
			else if(b)
			{
				currentShooterSpeed = MED_SPEED;
			} 
			else if(c)
			{
				currentShooterSpeed = HIGH_SPEED;
			}
			else if(d)
			{
				currentShooterSpeed = LOW_SPEED_2;
			}
			else if(e)
			{
				currentShooterSpeed = LOW_SPEED_3;
			}
			else
			{
				currentShooterSpeed = NO_SPEED;
			}

			// make this not the first time the button is pressed.
			listenForPrepButton = false;
		}

		if (prep_toggle) // If the shooter is toggled on...
		{
			// ... Set the shooter speed to the selected speed.
			if(currentShooterSpeed == LOW_SPEED)
			{
				shooterA->Set(0.75); // Set it to "low" speed (75%)
				shooterB->Set(-0.75);
				
				m_dsLCD->Clear();
				m_dsLCD->PrintfLine((DriverStationLCD::Line) 1, "Low Speed");
				m_dsLCD->UpdateLCD();
				
			} 
			else if(currentShooterSpeed == MED_SPEED)
			{
				shooterA->Set(0.85); // Set it to "medium" speed (85%)
				shooterB->Set(-0.85);
				
				m_dsLCD->Clear();
				m_dsLCD->PrintfLine((DriverStationLCD::Line) 1, "Med Speed");
				m_dsLCD->UpdateLCD();
				
			} 
			else if(currentShooterSpeed == HIGH_SPEED)
			{
				shooterA->Set(1.0); // Set it to "high" speed (100%)
				shooterB->Set(-1.0);
				
				m_dsLCD->Clear();
				m_dsLCD->PrintfLine((DriverStationLCD::Line) 1, "High Speed");
				m_dsLCD->UpdateLCD();
			}
			else if(currentShooterSpeed == LOW_SPEED_2)
			{
				shooterA->Set(0.70); 
				shooterB->Set(-0.70);
				
				m_dsLCD->Clear();
				m_dsLCD->PrintfLine((DriverStationLCD::Line) 1, "70 Speed");
				m_dsLCD->UpdateLCD();
			}
			else if(currentShooterSpeed == LOW_SPEED_3)
			{
				shooterA->Set(0.65); 
				shooterB->Set(-0.65);
				
				m_dsLCD->Clear();
				m_dsLCD->PrintfLine((DriverStationLCD::Line) 1, "65 Speed");
				m_dsLCD->UpdateLCD();
			}
		}
		else // If the shooter is toggled off....
		{
			
			shooterA->Set(0.0); // Turn it off!
			shooterB->Set(0.0);
			
			m_dsLCD->Clear();
			m_dsLCD->UpdateLCD();
		}
	}
	
	
	// Get the current state of the trigger limit switch
	bool ballSensorValue()
	{
		return !triggerLimitSwitch->Get(); // Inverted because the limit switch is "on" when there is no ball.
	}

}; // End of class Robot

// Run the robot class
START_ROBOT_CLASS(Robot);
