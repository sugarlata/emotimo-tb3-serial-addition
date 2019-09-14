/*
 * Nunchuck functions
 *
 * ***************


 * Modified to work with Third Party Nunchucks by Grant Emsley, http://arduino.emsley.ca
 * Added several useful functions

 * ***************
 *
 * Originally by:
 * 2007 Tod E. Kurt, http://todbot.com/blog/
 * The Wii Nunchuck reading code originally from Windmeadow Labs
 *   http://www.windmeadow.com/node/42
 */

#ifndef Nunchuck_h
#define Nunchuck_h

#undef int
#include <stdio.h>
#include <stdint.h>
#include <Arduino.h>

#define JOYSTICK_THRESHOLD 60

class WiiNunchuck3 {
	public:
		WiiNunchuck3(void);
		int getData(void);
		void printData(void);
		void init(int);
		int zbutton(void);
		int cbutton(void);
		int joyx(void);
		int joyy(void);
		int accelx(void);
		int accely(void);
		int accelz(void);
		int vibration(void);
		void calibrate(void);
		int digitalx(int threshold = JOYSTICK_THRESHOLD);
		int digitaly(int threshold = JOYSTICK_THRESHOLD);
	private:
		int cnt;
		int centeredJoyX;
		int centeredJoyY;
		uint8_t nunchuck_buf[6];

		void send_zero(void);
		void clearTwiInputBuffer(void);
		void setpowerpins(void);
		void send_request(void);
		char decode_byte(char);

};

#endif
extern WiiNunchuck3 Nunchuck;
