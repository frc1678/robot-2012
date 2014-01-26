/* 
 * Toggler class; switches a boolean between true and false
 * Can be used to toggle a button
 */

class Toggler
{
public:
	bool returnMe; // this is the boolean that indicates the output of the toggle
	bool listenForToggler; // this is another boolean that is needed to toggle (Is it waiting to toggle?)
	
	// Constructor - initializes booleans
	Toggler()
	{
		returnMe = false; // set them to false to start with.
		listenForToggler = false;
	}
	
	void toggle(bool toggleMe) // toggleMe is the boolean/button that, if true, toggles the other boolean
	{
		// If it's not true (if the button's not pressed) then prepare to toggle
		if(!toggleMe)
		{
			listenForToggler = true;
		}
		
		// If it's waiting to be toggled and the boolean/button is true/pressed
		// switch the boolean that is returned and tell it that it is no longer waiting to be toggled
		if(toggleMe && listenForToggler)
		{
			returnMe = !returnMe; // toggle.
			listenForToggler = false; // and make it not the first time.
		}
	}
	
	// Returns a boolean that indicates the status of the toggle
	bool status()
	{
		return returnMe;
	}
	
	// Resets the toggler, used in inits
	void disable()
	{
		returnMe = false;
		listenForToggler = true;
	}
	
	// Used to manually change the return value
	// Could be used in a "semi-toggle"
	void Set(bool a) 
	{
		returnMe = a;
	}
	
};
