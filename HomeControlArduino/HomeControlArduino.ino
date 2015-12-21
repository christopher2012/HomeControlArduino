#include <SoftwareSerial\SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

int rx = 2;
int tx = 3;
String message = "";
int light = 0;
int brightness;
int led = 8;

int alarm = 0;
int alarmAddress = 0;
int autoSwitchOffLight = 0;
int autoSwitchOffLightAddress = 1;
boolean touchFlag = true;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

SoftwareSerial softSerial(rx, tx);

String SSID = "UPC2633536";
String PASSWORD = "niezgadnieszmnie";

#define HOST_NAME   "api.thingspeak.com"
#define HOST_PORT   80
#define ONE_WIRE_BUS 7
#define TOUCH_SENSOR 7

void setup() {
	pinMode(led, OUTPUT);
	softSerial.begin(9600);
	Serial.begin(9600);
	Serial.write("Hello from Serial");
	lcd.begin(16, 2);
	lcd.setCursor(0, 0);
	lcd.print("Initialize LCD!");
	pinMode(TOUCH_SENSOR, INPUT);
}

void loop() {

	if (digitalRead(TOUCH_SENSOR) == HIGH && touchFlag) {
		touchFlag = false;
		Serial.println(".");
		if (light == 0) {
			digitalWrite(led, HIGH);
			light = 1;
		}
		else{
			digitalWrite(led, LOW);
			light = 0;
		}
	}
	else if (digitalRead(TOUCH_SENSOR) == LOW){
		touchFlag = true;
	}

	if (softSerial.available() > 5) {
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
						digitalWrite(led, HIGH);
						light = 1;
					} else if (c == '0') {
						Serial.println("OFF");
						digitalWrite(led, LOW);
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
					if (light == 1) {
						analogWrite(led, map(brightness, 0, 100, 0, 255));
					}
					Serial.print("Jasnosc: ");
					Serial.println(brightness);
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