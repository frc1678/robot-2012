/* 
 * This is a list of where the buttons are mapped to. This was created to easily change the button values
 * and write more readable code.
 */

// 1 is trigger, all others are labelled on the joystick.
// All button names will end with _B.

// Joysticks:

// Driver Controls
const int LOWER_RAMP_B = 1; // left and right
const int HALF_SPEED_B = 2;	// left and right
const int SHIFT_DOWN_B = 3; // left
const int SHIFT_UP_B = 3;   // right

// Manipulator
const int SHOOT_B = 1;
const int PASTA_ROLLER_B = 2;
const int PREPARE_LOW_B = 4;	// 75%
const int PREPARE_MED_B = 3; 	// 85% out of order because of the joystick layout
const int PREPARE_HIGH_B = 5;	// 100%
const int PREPARE_LOW_2_B = 7; 	// 70% slow version for too-new balls, added at Madera
const int PREPARE_LOW_3_B = 6;	// 65%
const int CLEAR_SYSTEM_B = 8;
