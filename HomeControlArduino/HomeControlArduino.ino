#include <SoftwareSerial\SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Timer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoThread\Thread.h>
#include <Commands.h>
#include <EEPROM\EEPROM.h>
#include <EEPROMAddressess.h>
Timer timer;

String message = "";


int brightness;
float tempInside = -127;
float tempOutside = -127;
boolean alarmMovement= false;
boolean autoSwitchOn= false;
boolean smokeAlarm = false;
boolean monoxideAlarm = false;


int alarm = 0;
int alarmAddress = 0;
//int autoSwitchOffLight = 0;
//int autoSwitchOffLightAddress = 1;
int smokeLevel = 1;
int monoxideLevel = 1;

boolean touchFlag = true;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);


String SSID = "UPC2633536";
String PASSWORD = "niezgadnieszmnie";

#define HOST_NAME   "api.thingspeak.com"
#define HOST_PORT   80

#define RX 2
#define TX 3
#define SMOKE_IND 4
#define MONOXIDE_IND 5
#define ONE_WIRE_BUS 6
#define TOUCH_SENSOR 7
#define PIR_SENSOR 8
#define LIGHT 9
#define ALARM_OUTPUT 10
#define ALARM_IND 12
#define AUTO_LIGHT_IND 11
OneWire oneWire(ONE_WIRE_BUS);
SoftwareSerial softSerial(RX, TX);
DallasTemperature sensors(&oneWire);
Thread alarmThread = Thread();
int alarmDuration = 1000;
boolean isAlarmRunning = false;

void updateTemp() {
	lcd.setCursor(0, 0);
	lcd.print("Temperature ");
	sensors.requestTemperatures();
	float temp = sensors.getTempCByIndex(0);
	tempInside = temp;
	Serial.println(tempInside);
	String temp1 = String(temp);
	softSerial.print("TTT");
	softSerial.println(temp1);
	temp1.remove(temp1.length() - 1);
	lcd.print(temp1);
}

#define NOTE_F2  87
#define NOTE_E3  165

int alarmTone[] = {
	NOTE_F2, NOTE_E3, NOTE_F2, NOTE_E3
};

int noteDurations[] = {
	8,8,8,8
};

void playAlarm() {
	Serial.println("Hello from alarm player!");
	long startedTime = millis();
	while (startedTime + alarmDuration > millis()) {
		for (int thisNote = 0; thisNote < 8; thisNote++) {
			int noteDuration = 1000 / noteDurations[thisNote];
			tone(ALARM_OUTPUT, alarmTone[thisNote], noteDuration);

			int pauseBetweenNotes = noteDuration * 1.30;
			delay(pauseBetweenNotes);
			noTone(ALARM_OUTPUT);
		}
	}
	isAlarmRunning = false;
}

int isEEPROMData = 0;

void setup() {
	pinMode(SMOKE_IND, OUTPUT);
	pinMode(MONOXIDE_IND, OUTPUT);
	pinMode(ALARM_IND, OUTPUT);
	pinMode(AUTO_LIGHT_IND, OUTPUT);
	pinMode(LIGHT, OUTPUT);
	pinMode(PIR_SENSOR, INPUT);
	pinMode(ALARM_OUTPUT, OUTPUT);
	softSerial.begin(9600);
	Serial.begin(9600);
	isEEPROMData = EEPROM.read(0);
	if (isEEPROMData == 0) {
		Serial.println("Saving data");
		EEPROM.write(ADDR_BRIGHTNESS, brightness);
		EEPROM.write(ADDR_ALARM, alarmMovement);
		EEPROM.write(ADDR_AUTO_ON, autoSwitchOn);
		EEPROM.write(ADDR_SMOKE, smokeAlarm);
		EEPROM.write(ADDR_MONOXIDE, monoxideAlarm);
		EEPROM.write(ADDR_NOTFIRST, 1);
	}
	else if(isEEPROMData==1){
		Serial.println("Reading data");
		brightness=EEPROM.read(ADDR_BRIGHTNESS);
		alarmMovement=EEPROM.read(ADDR_ALARM);
		autoSwitchOn = EEPROM.read(ADDR_AUTO_ON);
		smokeAlarm = EEPROM.read(ADDR_SMOKE );
		monoxideAlarm= EEPROM.read(ADDR_MONOXIDE);
	}
	Serial.println(isEEPROMData);

	delay(10000);
	Serial.println("Hello from Serial");
	lcd.begin(16, 2);
	lcd.setCursor(0, 0);
	lcd.print("Initialize LCD!");
	displayIP();
	delay(1500);
	pinMode(TOUCH_SENSOR, INPUT);
	sensors.begin();
	updateTemp();
	delay(1500);
	timer.every(15*60000, updateTemp);
	alarmThread.onRun(playAlarm);
}

void reset_alarm() {
	digitalWrite(ALARM_OUTPUT, LOW);
}

void reset_light() {
	digitalWrite(LIGHT, LOW);
	brightness = 0;
}

void loop() {

	timer.update();

		analogWrite(LIGHT, map(brightness, 0, 100, 0, 255));

		digitalWrite(SMOKE_IND, smokeAlarm);

		digitalWrite(MONOXIDE_IND, monoxideAlarm);

		digitalWrite(ALARM_IND, alarmMovement);

		digitalWrite(AUTO_LIGHT_IND, autoSwitchOn);
		

	if (digitalRead(TOUCH_SENSOR) == HIGH && touchFlag) {
		touchFlag = false;
		Serial.println(".");
		if (brightness == 0) {
			updateLight(255);
		}
		else{
			updateLight(0);
		}
	}
	else if (digitalRead(TOUCH_SENSOR) == LOW){
		touchFlag = true;
	}

	if (digitalRead(PIR_SENSOR) == HIGH) {
		if (autoSwitchOn) {
				brightness = 255;
				timer.after(15000, reset_light);
		}
		if (alarmMovement && (!isAlarmRunning)) {
			isAlarmRunning = true;
			alarmThread.run();
		}
		
	}

	if (softSerial.available() > 4) {
		message = "";
		char c;
		for (int i = 0; i < 3; i++) {
			c = softSerial.read();
			message.concat(c);
		}

		if (message.equals("MSG")) {
			Serial.print("Wiadomosc...");
			Serial.println(message);

			c = softSerial.read();
			if (c == '=') {
				c = softSerial.read();
				switch (c) {
				case CMD_SWITCH_LIGHT:
					Serial.println("");
					Serial.print("Swiatlo ");
					c = softSerial.read();
					if (c == '1') {
						Serial.println("ON");
						updateLight(255);
					}
					else if (c == '0') {
						Serial.println("OFF");
						updateLight(0);
					}
					break;
				case CMD_CHANGE_BRIGHTNESS:
					message = "";
					for (int i = 0; i < 3; i++) {
						c = softSerial.read();
						if (!(c == 0 && message.equals("")))
							message += c;
					}

					updateLight(message.toInt());

					Serial.print("Jasnosc: ");
					Serial.println(brightness);
					break;
				case CMD_GET_DATA: {
					softSerial.print("{\"STATUS\":\"OK\", \"BRIGHTNESS\":");
					softSerial.print(brightness);
					softSerial.print(", \"TEMP_IN\":");
					softSerial.print(tempInside);
					softSerial.print(", \"TEMP_OUT\":");
					softSerial.print(tempOutside);
					softSerial.print(", \"ALARM\":");
					softSerial.print(alarmMovement);
					softSerial.print(", \"AUTO_ON\":");
					softSerial.print(autoSwitchOn);
					softSerial.print(", \"SMOKE\":");
					softSerial.print(smokeLevel);
					softSerial.print(", \"MONOXIDE\":");
					softSerial.print(monoxideLevel);
					softSerial.print(", \"SMOKE_ALARM\":");
					softSerial.print(smokeAlarm);
					softSerial.print(", \"MONOXIDE_ALARM\":");
					softSerial.print(monoxideAlarm);
					softSerial.println("}");
					break;
				}
				case CMD_ALARM: {
					Serial.println("Switching alarm");
					char c = softSerial.read();
					if (c == '1') 
						alarmMovement = true;
					else
						alarmMovement = false;

					EEPROM.update(ADDR_ALARM, alarmMovement);
					break;
				}
				case CMD_AUTO_LIGHT_ON: {
					Serial.println("Switching auto light");
					char c = softSerial.read();
					if (c == '1')
						autoSwitchOn = true;
					else
						autoSwitchOn = false;
					EEPROM.write(ADDR_AUTO_ON, autoSwitchOn);
					break;
				}
				case CMD_SMOKE_ALARM: {
					Serial.println("Switching smoke alarm");
					char c = softSerial.read();
					if (c == '1')
						smokeAlarm = true;
					else
						smokeAlarm = false;
					EEPROM.write(ADDR_SMOKE, smokeAlarm);
					break;
				}

				case CMD_MONOXIDE_ALARM:{
					Serial.println("Switching monoxide alarm");
					char c = softSerial.read();
					if (c == '1') 
						monoxideAlarm = true;
					else
						monoxideAlarm = false;
					EEPROM.write(ADDR_MONOXIDE, monoxideAlarm);
					break;
			    }

				case 'I':

					break;
				}
			}
			while (softSerial.available())
				softSerial.read();
		}
	}
	else {
	}
}

void updateLight(int i) {
	if (i == 1) {
		brightness = i;
		EEPROM.update(ADDR_BRIGHTNESS, brightness);
	}
	else {
		brightness = i;
		EEPROM.update(ADDR_BRIGHTNESS, brightness);
	}
}

void displayIP() {

	Serial.println("Getting IP");
	while (softSerial.available())
	{
		softSerial.read();
	}
	softSerial.println("III");
	

	String ip = "";
	unsigned long start = millis();

	while (millis() - start < 3000) {
		if (softSerial.available()>5) {
			ip = softSerial.readString();
			break;
		}
	}
	ip.remove(ip.length()-2);
	Serial.println(ip);
	lcd.setCursor(0, 1);
	lcd.print(ip);
}