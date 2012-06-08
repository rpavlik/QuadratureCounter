#include <arduino.h>
#include <QuadratureCounter.h>

/*
  Experimenting with handling a quadrature encoder with template metaprogramming.
*/


Quadrature::QuadratureCounter<4, 5> qc;
void setup()
{
	Serial.begin(9600);
	qc.begin();
}

void loop()
{
	digitalWrite(13, HIGH);   // set the LED on
	delay(1000);              // wait for a second
	digitalWrite(13, LOW);    // set the LED off
	delay(1000);              // wait for a second
}
