
int out = 13;

void setup()
{

	pinMode(out, INPUT);

}

void loop()
{

	digitalWrite(out, !digitalRead(out));
	delay(1000);

}
