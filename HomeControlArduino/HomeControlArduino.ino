#include <SoftwareSerial\SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Timer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoThread\Thread.h>

Timer timer;

String message = "";


int light = 0;
int brightness;
int tempInside = -127;
int tempOutside = -127;
boolean alarmMovement=true;
//boolean autoSwitchOff=true;
boolean autoSwitchOn=true;
boolean smokeAlarm = false;
boolean monoxideAlarm = false;


int alarm = 0;
int alarmAddress = 0;
int autoSwitchOffLight = 0;
int autoSwitchOffLightAddress = 1;
int smokeLevel = 1;
int monoxideLevel=1;

boolean touchFlag = true;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);


String SSID = "UPC2633536";
String PASSWORD = "niezgadnieszmnie";

#define HOST_NAME   "api.thingspeak.com"
#define HOST_PORT   80

#define RX 2
#define TX 3
#define ONE_WIRE_BUS 6
#define TOUCH_SENSOR 7
#define PIR_SENSOR 8
#define LIGHT 9
#define ALARM_OUTPUT 10
OneWire oneWire(ONE_WIRE_BUS);
SoftwareSerial softSerial(RX, TX);
DallasTemperature sensors(&oneWire);
Thread alarmThread = Thread();
int alarmDuration = 1000;

void updateTemp() {
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Temperature");
	sensors.requestTemperatures();
	float temp = sensors.getTempCByIndex(0);
	tempInside = temp;
	Serial.println(sensors.getTempCByIndex(0));
	String temp1 = String(temp);
	softSerial.print(temp1);
	lcd.print(temp1);
	lcd.setCursor(0, 1);
	lcd.print(millis());
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
}

void setup() {
	pinMode(LIGHT, OUTPUT);
	pinMode(PIR_SENSOR, INPUT);
	pinMode(ALARM_OUTPUT, OUTPUT);
	softSerial.begin(9600);
	Serial.begin(9600);
	Serial.println("Hello from Serial");
	lcd.begin(16, 2);
	lcd.setCursor(0, 0);
	lcd.print("Initialize LCD!");
	pinMode(TOUCH_SENSOR, INPUT);
	sensors.begin();
	timer.every(15*60000, updateTemp);
	displayIP();
	alarmThread.onRun(playAlarm);
}

void reset_alarm() {
	digitalWrite(ALARM_OUTPUT, LOW);
}

void reset_light() {
	digitalWrite(LIGHT, LOW);
}

void loop() {

	timer.update();

	if (digitalRead(TOUCH_SENSOR) == HIGH && touchFlag) {
		touchFlag = false;
		Serial.println(".");
		if (light == 0) {
			digitalWrite(LIGHT, HIGH);
			light = 1;
		}
		else{
			digitalWrite(LIGHT, LOW);
			light = 0;
		}
	}
	else if (digitalRead(TOUCH_SENSOR) == LOW){
		touchFlag = true;
	}

	if (digitalRead(PIR_SENSOR) == HIGH) {
		if (alarmMovement) {
			alarmThread.run();
			//digitalWrite(ALARM_OUTPUT, HIGH);
			//timer.after(15000, reset_alarm);
		}
		if (autoSwitchOn) {
			if (light == 0) {
				digitalWrite(LIGHT, HIGH);
				light = 1;
				timer.after(15000, reset_light);
			}
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
				case 'L':
					Serial.println("");
					Serial.print("Swiatlo ");
					c = softSerial.read();
					if (c == '1') {
						Serial.println("ON");
						digitalWrite(LIGHT, HIGH);
						brightness = 100;
						light = 1;
					} else if (c == '0') {
						Serial.println("OFF");
						digitalWrite(LIGHT, LOW);
						brightness = 0;
						light = 0;
					}
					break;
				case 'B':
					message = "";
					for (int i = 0; i < 3; i++) {
						c = softSerial.read();
						if (!(c == 0 && message.equals("")))
							message += c;
					}

					brightness = message.toInt();

					if (brightness == 0) {
						digitalWrite(LIGHT, LOW);
						light = 0;
					}
					else {
						analogWrite(LIGHT, map(brightness, 0, 100, 0, 255));
						light = 1;
					}

					Serial.print("Jasnosc: ");
					Serial.println(brightness);
					break;
				case 'D': {
					softSerial.print("{\"STATUS\":\"OK\", \"LIGHT\":");
					softSerial.print(light);
					softSerial.print(", \"BRIGHTNESS\":");
					softSerial.print(brightness);
					softSerial.print(", \"TEMP_IN\":");
					softSerial.print(tempInside);
					softSerial.print(", \"TEMP_OUT\":");
					softSerial.print(tempOutside);
					softSerial.print(", \"SMOKE\":");
					softSerial.print(smokeLevel);
					softSerial.print(", \"MONOXIDE\":");
					softSerial.print(monoxideLevel);
					softSerial.print(", \"SMOKE_ALARM\":");
					softSerial.print(smokeAlarm);
					softSerial.print(", \"MONOXIDE_ALARM\":");
					softSerial.print(monoxideAlarm);
					softSerial.println("}");
					//softSerial.println(dataJson);
					break;
				}
				case 'A':

					break;
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

void displayIP() {
	softSerial.print("I");
	
	String ip = "";
	unsigned long start = millis();
	while (millis() - start < 1000) {
		while (softSerial.available() > 0) {
			ip += softSerial.readString();
		}
	}
	ip.remove(ip.length()-2);
	Serial.println(ip);
	lcd.setCursor(0, 1);
	lcd.print(ip);
}