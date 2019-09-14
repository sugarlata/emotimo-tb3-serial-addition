/*
* WiiNunchuck Library
*
* This library (should) work with all official and third party nunchucks, including wireless ones.
*
* Written by Grant Emsley - grant@emsley.ca - http://arduino.emsley.ca
*
* Adapted from code at http://www.windmeadow.com/node/42
*/

#include <Arduino.h>
#include "WiiNunchuck3.h"
#include "../Wire/Wire.h"
#include "../Wire/utility/twi.h"
#undef int
#include <stdio.h>

#define USE_NEW_WAY_INIT 1 // see http://wiibrew.org/wiki/Wiimote#The_New_Way.  If set to 0, this library won't work for third party nunchucks
#define WII_IDENT_LEN ((byte)6)
#define WII_TELEGRAM_LEN ((byte)6)
#define WII_NUNCHUCK_TWI_ADR ((byte)0x52)




//#define DEBUG // Use serial port debugging if defined.


uint8_t nunchuck_buf[WII_TELEGRAM_LEN]; // array to store arduino output

int cnt = 0;
int centeredJoyX = 0;
int centeredJoyY = 0;

WiiNunchuck3::WiiNunchuck3() {

}


void WiiNunchuck3::send_zero () {
// I don't know why, but it only works correct when doing this exactly 3 times
// otherwise only each 3rd call reads data from the controller (cnt will be 0 the other times)
	for(byte i = 0; i < 3; i++) {
		Wire.beginTransmission (WII_NUNCHUCK_TWI_ADR); // transmit to device 0x52
		//Wire.write (0x00); // sends one byte
		Wire.write ((uint8_t)0); // sends one byte
		Wire.endTransmission (); // stop transmitting
	}
}
void WiiNunchuck3::clearTwiInputBuffer(void) {
// clear the receive buffer from any partial data
while( Wire.available ())
Wire.read ();
}


void WiiNunchuck3::setpowerpins() {
// Uses port C (analog in) pins as power & ground for Nunchuck
#define pwrpin PORTC3
#define gndpin PORTC2
    DDRC |= _BV(pwrpin) | _BV(gndpin);
    PORTC &=~ _BV(gndpin);
    PORTC |=  _BV(pwrpin);
    delay(100);  // wait for things to stabilize
}
void WiiNunchuck3::send_request() {
	// Send a request for data to the nunchuck
    Wire.beginTransmission(0x52);// transmit to device 0x52
    //Wire.write(0x00);// sends one byte
    Wire.write ((uint8_t)0);
    Wire.endTransmission();// stop transmitting
}


char WiiNunchuck3::decode_byte (char x) {
	// Decode data format that original Nunchuck uses with old init sequence. This never worked with
	// other controllers (e.g. wireless Nunchuck from other vendors)
	#ifndef USE_NEW_WAY_INIT
	x = (x ^ 0x17) + 0x17;
	#endif
	return x;
}

int WiiNunchuck3::getData() {
	// Receive data back from the nunchuck,
	// returns 1 on successful read. returns 0 on failure
	Wire.requestFrom (WII_NUNCHUCK_TWI_ADR, WII_TELEGRAM_LEN); // request data from nunchuck

	for (cnt = 0; (cnt < WII_TELEGRAM_LEN) && Wire.available (); cnt++)
	{
		nunchuck_buf[cnt] = decode_byte (Wire.read ()); // receive byte as an integer
	}

	clearTwiInputBuffer();
	if (cnt >= WII_TELEGRAM_LEN) {
		send_zero();
        return 1;   // success
    }
    return 0; //failure


}

void WiiNunchuck3::init(int power) {
	// do we need to power the nunchuck?
	if(power) {
		setpowerpins();
	}

	// initialize the I2C system, join the I2C bus,
	// and tell the nunchuck we're talking to it
	unsigned short timeout = 0; // never timeout
    Wire.begin();                // join i2c bus as master
	// we need to switch the TWI speed, because the nunchuck uses Fast-TWI
	// normally set in hardware\libraries\Wire\utility\twi.c twi_init()
	// this is the way of doing it without modifying the original files
	#define TWI_FREQ_NUNCHUCK 400000L
	TWBR = ((16000000 / TWI_FREQ_NUNCHUCK) - 16) / 2;


	byte rc = 1;
	#ifndef USE_NEW_WAY_INIT
		// look at <http://wiibrew.org/wiki/Wiimote#The_Old_Way> at "The Old Way"
		Wire.beginTransmission (WII_NUNCHUCK_TWI_ADR); // transmit to device 0x52
		//Wire.write (0x40); // sends memory address
		//Wire.write (0x00); // sends sent a zero.
	    Wire.write ((uint8_t)64); //this is x40
		Wire.write ((uint8_t)0); // sends sent a zero.
		Wire.endTransmission (); // stop transmitting
	#else
		// disable encryption
		// look at <http://wiibrew.org/wiki/Wiimote#The_New_Way> at "The New Way"

		unsigned long time = millis();
		do {
			Wire.beginTransmission (WII_NUNCHUCK_TWI_ADR); // transmit to device 0x52
			Wire.write ((uint8_t)0xF0); // sends memory address
			Wire.write ((uint8_t)0x55); // sends data.
			if(Wire.endTransmission() == 0) // stop transmitting
			{
				Wire.beginTransmission (WII_NUNCHUCK_TWI_ADR); // transmit to device 0x52
				Wire.write ((uint8_t)0xFB); // sends memory address
				Wire.write ((uint8_t)0x00); // sends sent a zero.
				if(Wire.endTransmission () == 0) // stop transmitting
				{
					rc = 0;
				}
			}
		}
		while (rc != 0 && (!timeout || ((millis() - time) < timeout)));
	#endif
	// Sometimes the first request seems to get garbage data.
	// Get some data now so when the main program calls getData(), it will get good data.
	getData();
}






void WiiNunchuck3::printData() {
	// Print the input data we have recieved
	// accel data is 10 bits long
	// so we read 8 bits, then we have to add
	// on the last 2 bits.  That is why I
	// multiply them by 2 * 2
    static int i=0;
    int joy_x_axis = nunchuck_buf[0];
    int joy_y_axis = nunchuck_buf[1];
    int accel_x_axis = nunchuck_buf[2] * 2 * 2;
    int accel_y_axis = nunchuck_buf[3] * 2 * 2;
    int accel_z_axis = nunchuck_buf[4] * 2 * 2;

    int z_button = 0;
    int c_button = 0;

    // byte nunchuck_buf[5] contains bits for z and c buttons
    // it also contains the least significant bits for the accelerometer data
    // so we have to check each bit of byte outbuf[5]
    if ((nunchuck_buf[5] >> 0) & 1)
        z_button = 1;
    if ((nunchuck_buf[5] >> 1) & 1)
        c_button = 1;

    if ((nunchuck_buf[5] >> 2) & 1)
        accel_x_axis += 2;
    if ((nunchuck_buf[5] >> 3) & 1)
        accel_x_axis += 1;

    if ((nunchuck_buf[5] >> 4) & 1)
        accel_y_axis += 2;
    if ((nunchuck_buf[5] >> 5) & 1)
        accel_y_axis += 1;

    if ((nunchuck_buf[5] >> 6) & 1)
        accel_z_axis += 2;
    if ((nunchuck_buf[5] >> 7) & 1)
        accel_z_axis += 1;

    Serial.print(i,DEC);
    Serial.print("\t");

    Serial.print("joy:");
    Serial.print(joy_x_axis,DEC);
    Serial.print(",");
    Serial.print(joy_y_axis, DEC);
    Serial.print("  \t");

    Serial.print("acc:");
    Serial.print(accel_x_axis, DEC);
    Serial.print(",");
    Serial.print(accel_y_axis, DEC);
    Serial.print(",");
    Serial.print(accel_z_axis, DEC);
    Serial.print("\t");

    Serial.print("but:");
    Serial.print(z_button, DEC);
    Serial.print(",");
    Serial.print(c_button, DEC);

    Serial.print("\r\n");  // newline
    i++;
}


int WiiNunchuck3::zbutton() {
	// returns zbutton state: 1=pressed, 0=notpressed
    return ((nunchuck_buf[5] >> 0) & 1) ? 0 : 1;  // voodoo
}


int WiiNunchuck3::cbutton(){
	// returns cbutton state: 1=pressed, 0=notpressed
    return ((nunchuck_buf[5] >> 1) & 1) ? 0 : 1;  // voodoo
}


int WiiNunchuck3::joyx() {
    //returns value of x-axis joystick
    //return nunchuck_buf[0]-centeredJoyX;
	return nunchuck_buf[0];
}


int WiiNunchuck3::joyy() {
	// returns value of y-axis joystick
    //return nunchuck_buf[1]-centeredJoyX;
	return nunchuck_buf[1];
}


int WiiNunchuck3::accelx() {
	// returns value of x-axis accelerometer
	int accel_x_axis = nunchuck_buf[2] * 2 * 2;
	if ((nunchuck_buf[5] >> 2) & 1) {
		accel_x_axis += 2;
	}
	if ((nunchuck_buf[5] >> 3) & 1) {
		accel_x_axis += 1;
	}

    return accel_x_axis;


}



int WiiNunchuck3::accely() {
	// returns value of y-axis accelerometer
	int accel_y_axis = nunchuck_buf[3] * 2 * 2;
	if ((nunchuck_buf[5] >> 4) & 1) {
		accel_y_axis += 2;
	}
	if ((nunchuck_buf[5] >> 5) & 1) {
		accel_y_axis += 1;
	}
    return accel_y_axis;
}


int WiiNunchuck3::accelz() {
	// returns value of z-axis accelerometer
	int accel_z_axis = nunchuck_buf[4] * 2 * 2;

	if ((nunchuck_buf[5] >> 6) & 1) {
		accel_z_axis += 2;
	}
	if ((nunchuck_buf[5] >> 7) & 1) {
		accel_z_axis += 1;
	}

    return accel_z_axis;
}



int WiiNunchuck3::vibration() {
	// calculates the "total vibration"
	// does NOT require a getdata() call first
	// takes 10 samples, finds the min and max, figures out the range for each accelerometer and adds them
	int accelerometers[3][2] = {{1024,0},{1024,0},{1024,0}}; // [x,y,z][min,max] - min's initialized to 1024, max initialized to 0

	// gather 10 samples of accelerometer data, and record the highest and lowest values
	for (int loop = 0; loop < 10; loop++) {
		getData();

		int x = accelx();
		int y = accely();
		int z = accelz();

		#ifdef DEBUG
		// :DEBUGING:
		Serial.print("X/Y/Z values: ");
		Serial.print(x);
		Serial.print("\t");
		Serial.print(y);
		Serial.print("\t");
		Serial.println(z);
		#endif


		if (x < accelerometers[0][0]) {
			accelerometers[0][0] = x;
		}
		if (x > accelerometers[0][1]) {
			accelerometers[0][1] = x;
		}




		if (y < accelerometers[1][0]) {
			accelerometers[1][0] = y;
		}
		if (y > accelerometers[1][1]) {
			accelerometers[1][1] = y;
		}


		if (z < accelerometers[2][0]) {
			accelerometers[2][0] = z;
		}
		if (z > accelerometers[2][1]) {
			accelerometers[2][1] = z;
		}
	#ifdef DEBUG
	// :DEBUGING: print accelerometer min/max
	Serial.print("Accelerometer X min/max ");
	Serial.print(accelerometers[0][0]);
	Serial.print("/");
	Serial.println(accelerometers[0][1]);

	Serial.print("Accelerometer y min/max ");
	Serial.print(accelerometers[1][0]);
	Serial.print("/");
	Serial.println(accelerometers[1][1]);

	Serial.print("Accelerometer z min/max ");
	Serial.print(accelerometers[2][0]);
	Serial.print("/");
	Serial.println(accelerometers[2][1]);
	#endif
}




	int xdiff = accelerometers[0][1] - accelerometers[0][0];
	int ydiff = accelerometers[1][1] - accelerometers[1][0];
	int zdiff = accelerometers[2][1] - accelerometers[2][0];

	#ifdef DEBUG
	// Debugging
	Serial.print ("Ranges: ");
	Serial.print(xdiff);
	Serial.print("\t");
	Serial.print(ydiff);
	Serial.print("\t");
	Serial.println(zdiff);

	Serial.print ("Final value: ");
	Serial.println(xdiff+ydiff+zdiff);
	#endif
	return xdiff+ydiff+zdiff;


}




void WiiNunchuck3::calibrate() {
	// this function returns the average values over 10 samples, to be used for calibrating the nunchuck joystick
	// the joystick must be at rest when this is done
	getData();
	centeredJoyX = joyx();
	centeredJoyY = joyy();

	Serial.print("Calibrated: ");
	Serial.print(centeredJoyX);
	Serial.print("/");
	Serial.println(centeredJoyY);

}


int WiiNunchuck3::digitalx(int threshold) {
	// returns 0 for centered, 1 for joystick left, 2 for joystick right
	// threshold is how far from center it has to be
	int calibratedjoyx = joyx() - centeredJoyX;
	if (calibratedjoyx < (threshold * -1) ) {
		return -1;
	} else if (calibratedjoyx > threshold) {
		return 1;
	} else {
		return 0;
	}
}


int WiiNunchuck3::digitaly(int threshold) {
	// returns 0 for centered, 1 for joystick up, 2 for joystick down
	// threshold is how far from center it has to be
	int calibratedjoyy = joyy() - centeredJoyY;
	if (calibratedjoyy < (threshold * -1) ) {
		return 1;
	} else if (calibratedjoyy > threshold) {
		return -1;
	} else 	{
		return 0;
	}
}









// create the object
WiiNunchuck3 Nunchuck;
