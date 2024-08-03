/*
Arduino Nano firmware using Alnitak protocol

Code Adapter from https://github.com/jwellman80/ArduinoLightbox/blob/master/LEDLightBoxAlnitak.ino


Send     : >S000\n      //request state
Recieve  : *S19000\n    //returned state

Send     : >B128\n      //set brightness 128
Recieve  : *B19128\n    //confirming brightness set to 128

Send     : >J000\n      //get brightness
Recieve  : *B19128\n    //brightness value of 128 (assuming as set from above)

Send     : >L000\n      //turn light on (uses set brightness value)
Recieve  : *L19000\n    //confirms light turned on

Send     : >D000\n      //turn light off (brightness value should not be changed)
Recieve  : *D19000\n    //confirms light turned off.
*/

#include <Servo.h>
#include <EEPROM.h>

#define SHUTTER_OPEN = 30		//degrees for servo motor
#define SHUTTER_CLOSED = 300

#define SHUTTER_ADDRESS = 0;
#define BRIGHTNESS_ADDRESS = 1;

int ledPin = 5;
int brightness = 0;
Servo servo;

enum devices {
	FLAT_MAN_L = 10,
	FLAT_MAN_XL = 15,
	FLAT_MAN = 19,
	FLIP_FLAT = 99
};

enum motorStatuses {
	STOPPED = 0,
	RUNNING
};

enum lightStatuses {
	OFF = 0,
	ON
};

enum shutterStatuses {
	UNKNOWN = 0,
	CLOSED,
	OPEN
};

int deviceId = FLIP_FLAT;
int motorStatus = STOPPED;
int lightStatus = OFF;
int coverStatus = UNKNOWN;

void setup() {
	servo.attach(9);
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);
    analogWrite(ledPin, 0);
}

void loop() {
    handleSerial();
}

void handleSerial() {
    if(Serial.available() >= 6) {
        char* cmd;
        char* data;
        char temp[10];
        int len = 0;
        char str[20];
        memset(str, 0, 20);
        Serial.readBytesUntil('\n', str, 20);
    	cmd = str + 1;
    	data = str + 2;

        switch(*cmd) {
    	    /*
    	    Ping device
    	    Request: >P000\n
    	    Return : *Pii000\n
    	    id = deviceId
    	    */
            case 'P': {
    	        sprintf(temp, "*P%d000", deviceId);
    	        Serial.println(temp);
    	        break;
            }
            /*
        	Open shutter
        	Request: >O000\n
        	Return : *Oii000\n
        	id = deviceId
        	This command is only supported on the Flip-Flat!
    	    */
            case 'O': {
				EEPROM.write(SHUTTER_ADDRESS, OPEN);
        	    sprintf(temp, "*O%d000", deviceId);
        	    setShutter(OPEN);
        	    Serial.println(temp);
        	    break;
            }
            /*
        	Close shutter
        	Request: >C000\n
        	Return : *Cii000\n
        	id = deviceId
        	This command is only supported on the Flip-Flat!
    	    */
            case 'C': {
				EEPROM.write(SHUTTER_ADDRESS, CLOSED);
        	    sprintf(temp, "*C%d000", deviceId);
        	    setShutter(CLOSED);
        	    Serial.println(temp);
        	    break;
            }
    	    /*
        	Turn light on
        	Request: >L000\n
        	Return : *Lii000\n
        	id = deviceId
    	    */
            case 'L': {
        	    sprintf(temp, "*L%d000", deviceId);
        	    Serial.println(temp);
        	    lightStatus = ON;
        	    analogWrite(ledPin, brightness);
        	    break;
            }
    	    /*
        	Turn light off
        	Request: >D000\n
        	Return : *Dii000\n
        	id = deviceId
    	    */
            case 'D': {
        	    sprintf(temp, "*D%d000", deviceId);
        	    Serial.println(temp);
        	    lightStatus = OFF;
        	    analogWrite(ledPin, 0);
        	    break;
            }
    	    /*
        	Set brightness
        	Request: >Bxxx\n
        	xxx = brightness value from 000-255
        	Return : *Biiyyy\n
        	id = deviceId
        	yyy = value that brightness was set from 000-255
    	    */
            case 'B': {
        	    brightness = atoi(data);
				EEPROM.write(BRIGHTNESS_ADDRESS, brightness);
        	    if(lightStatus == ON) {
        	    	analogWrite(ledPin, brightness);
                }
        	    sprintf(temp, "*B%d%03d", deviceId, brightness);
        	    Serial.println(temp);
                break;
            }
    	    /*
        	Get brightness
        	Request: >J000\n
        	Return : *Jiiyyy\n
        	id = deviceId
        	yyy = current brightness value from 000-255
    	    */
            case 'J': {
				if(EEPROM.read(BRIGHTNESS_ADDRESS) != brightness) {
					EEPROM.write(BRIGHTNESS_ADDRESS, brightness);
				}
                sprintf(temp, "*J%d%03d", deviceId, brightness);
                Serial.println(temp);
                break;
            }
    	    /*
        	Get device status:
        	Request: >S000\n
        	Return : *SidMLC\n
        	id = deviceId
        	M  = motor status (0 stopped, 1 running)
        	L  = light status (0 off, 1 on)
        	C  = cover status (0 moving, 1 closed, 2 open)
    	    */
            case 'S': {
                sprintf(temp, "*S%d%d%d%d", deviceId, motorStatus, lightStatus, coverStatus);
                Serial.println(temp);
                break;
            }
    	    /*
        	Get firmware version
        	Request: >V000\n
        	Return : *Vii001\n
        	id = deviceId
    	    */
            case 'V': {
                sprintf(temp, "*V%d001", deviceId);
                Serial.println(temp);
                break;
            }
        }
    	while(Serial.available() > 0) {
    		Serial.read();  
        }
    }
}

void setShutter(int shutter) {
	if(shutter != OPEN || shutter != CLOSED) {
		return;
	}
	EEPROM.write(SHUTTER_ADDRESS, shutter);
	if(shutter == OPEN && coverStatus != OPEN) {
		coverStatus = OPEN;
		servo.write(SHUTTER_OPEN);
	} else if(shutter == CLOSED && coverStatus != CLOSED) {
		coverStatus = CLOSED;
		servo.write(SHUTTER_CLOSED);
	} else {
		coverStatus = shutter;
		servo.write(shutter == OPEN ? SHUTTER_OPEN : SHUTTER_CLOSED);
	}
}