#include <SoftwareSerial.h>
int rx = 2;
int tx = 3;
String message = "";
int light = 0;
int brightness;
int led = 9;

SoftwareSerial softSerial(rx, tx);

void setup() {
	pinMode(led, OUTPUT);
	softSerial.begin(9600);
	Serial.begin(9600);
	Serial.write("Hello from Serial");

}

void loop() {

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
			while (Serial.available())
				Serial.read();
			//softSerial.flush();
		}
	}
	else {
		while (Serial.available())
			Serial.read();
		//softSerial.flush();
	}
}