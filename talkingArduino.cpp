#include <Arduino.h>

const int idPin = 8;

void setup(){
	init();
	Serial.begin(9600);
	Serial3.begin(9600);
}


int main(){
	setup();

	// dont need pull up here
	pinMode(idPin, INPUT);

	while (true){
		while (Serial3.available()==0){};
		char readByte = Serial3.read();
		Serial.print(readByte);
	}

	while (true){
		while (Serial.available()==0){};
		Serial3.print(Serial.read());
	}

	Serial3.flush();
	Serial.flush();

	return 0;
}
