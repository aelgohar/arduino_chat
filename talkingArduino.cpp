#include <Arduino.h>

//initialize variables
const int analogPin = 1;
const int digitalPin = 13;
// initialize the two keys that will be used to encrypt and decrypt messages
uint32_t key1, key2;


void setup(){
//setup Arduino functions andd start Serial and Serial3

  init();

// set both pins as input
  pinMode(analogPin, INPUT);
	pinMode(digitalPin, INPUT);

  Serial.begin(9600);
  Serial3.begin(9600);
}

/*******CODE TAKEN FROM CLASS**CREDIT: ZACH FRIGSTAD******/
/** Waits for a certain number of bytes on Serial3 or timeout
* @param nbytes: the number of bytes we want
* @param timeout: timeout period (ms); specifying a negative number
*                turns off timeouts (the function waits indefinitely
*                if timeouts are turned off).
* @return True if the required number of bytes have arrived.
*/

bool wait_on_serial3( uint8_t nbytes, long timeout ) {
  unsigned long deadline = millis() + timeout;//wraparound not a problem
  while (Serial3.available()<nbytes && (timeout<0 || millis()<deadline))
  {
    delay(1); // be nice, no busy loop
  }
  return Serial3.available()>=nbytes;
}

/** Writes an uint32_t to Serial3, starting from the least-significant
* and finishing with the most significant byte.
*/
void uint32_to_serial3(uint32_t num) {
  Serial3.write((char) (num >> 0));
  Serial3.write((char) (num >> 8));
  Serial3.write((char) (num >> 16));
  Serial3.write((char) (num >> 24));
}

/** Reads an uint32_t from Serial3, starting from the least-significant
* and finishing with the most significant byte.
*/
uint32_t uint32_from_serial3() {
  uint32_t num = 0;
  num = num | ((uint32_t) Serial3.read()) << 0;
  num = num | ((uint32_t) Serial3.read()) << 8;
  num = num | ((uint32_t) Serial3.read()) << 16;
  num = num | ((uint32_t) Serial3.read()) << 24;
  return num;
}

/** Implements the Park-Miller algorithm with 32 bit integer arithmetic
* @return ((current_key * 48271)) mod (2^31 - 1);
* This is linear congruential generator, based on the multiplicative
* group of integers modulo m = 2^31 - 1.
* The generator has a long period and it is relatively efficient.
* Most importantly, the generator's modulus is not a power of two
* (as is for the built-in rng),
* hence the keys mod 2^{s} cannot be obtained
* by using a key with s bits.
* Based on:
* http://www.firstpr.com.au/dsp/rand31/rand31-park-miller-carta.cc.txt
*/
uint32_t next_key(uint32_t current_key) {

  const uint32_t modulus = 0x7FFFFFFF; // 2^31-1
  const uint32_t consta = 48271;  // we use that consta<2^16
  uint32_t lo = consta*(current_key & 0xFFFF);
  uint32_t hi = consta*(current_key >> 16);
  lo += (hi & 0x7FFF)<<16;
  lo += hi>>15;
  if (lo > modulus) lo -= modulus;
  return lo;
}
/******CODE TAKEN FROM CLASS END ********/

char encryptChar(char inputChar, uint32_t sharedKey){
  //generates encrypted character by using XOR with the input character  and shared secret key
  uint8_t encryptedChar = inputChar ^ uint8_t(sharedKey);
  return char(encryptedChar);
}

uint32_t mulMod(uint32_t a, uint32_t b, uint32_t m) {
// function that is able to multiply 2 32-bit numbers a and b without
// overflowing, given a certain modulo "m"
  a = a%m;
  b = b%m;

  uint32_t result = 0;
  for (int i = 0; i < 32; i++){
    //shifts the required bit to become the least significant bit as a form of
    // binary expansion, then multiplies b by each term, adding it to the result
    // every time to find the final result = a * b % m
    if ((a>>i)&1){
      result += b;
      result = result % m;
    }
    // shifts b one bit to the left and then mods it
    b = (b*2) % m;
  }
  return result;
}

/*******CODE TAKEN FROM CLASS**CREDIT: ZACH FRIGSTAD******/
uint32_t powModFast(uint32_t a, uint32_t b, uint32_t m) {
  // initialise variables
  uint32_t result = 1%m;
  uint32_t sqrVal = a%m; //stores a^{2^i} values, initially 2^{2^0}
  uint32_t newB = b;
  // multiplies a by itself b times, then takes the mod
  while (newB > 0) {
    if (newB&1) { // evalutates to true iff i'th bit of b is 1
    result = mulMod(result,sqrVal,m);
    }
  sqrVal = mulMod(sqrVal,sqrVal,m);
  newB = (newB>>1);
  }

return result;
}
/******CODE TAKEN FROM CLASS END ********/

uint32_t calculateSharedSecretKey(uint32_t myPrivateKey, uint32_t otherPublicKey){
  const uint32_t p = 2147483647;
//use the other public key and the private key as well as the prime
  return(powModFast(otherPublicKey, myPrivateKey, p));
}

uint32_t generateRandomPrivateKey(){
  // generates random private key by reading the least significant bit from Arduino Analog Port 1 sixteen times
  uint32_t random;
  for (int i = 0; i<16; i++){
    uint32_t readInt = analogRead(analogPin) & 1;
    random = (random << 1) + readInt;
    delay(50);
  }
  return random;
}

void readOtherUserChat(uint32_t sharedKey){
  //if the other user types into their serial monitor, each character is read, decrypted, and printed on Serial
  if (Serial3.available() > 0){
  	char tempReadChar = Serial3.read();
    char decryptedTempChar = encryptChar(tempReadChar, sharedKey);
    Serial.print(decryptedTempChar);

    if (digitalRead(digitalPin) == HIGH) {
      key1 = next_key(key1);
    }
    else {
      key2 = next_key(key2);
    }
  }
}

void writeToExternalSerial(uint32_t sharedKey){
  // if text is entered in the serial monitor, each character is read, encrypted, and printing on Serial3
  if (Serial.available() > 0){
    char tempReadChar = Serial.read();
    // echo typed in text
    Serial.print(tempReadChar);
    char encryptCharacter = encryptChar(tempReadChar, sharedKey);
    //prints the encrypted character
    Serial3.print(encryptCharacter);
    if (digitalRead(digitalPin) == HIGH) {
      key2 = next_key(key2);
      sharedKey = key2;
    }
    else {
      key1 = next_key(key1);
      sharedKey = key1;
    }
    if (tempReadChar == '\r'){
      // if the arduino reads the carriage return character '\r' from the serial monitor, it sends
      // the return character followed by line feed character
      char encryptCharacterNewline = encryptChar('\n', sharedKey);
      Serial3.print(encryptCharacterNewline);
      Serial.print('\n');
      //while (Serial.available()>0){Serial.read();}
      if (digitalRead(digitalPin) == HIGH) {
        key2 = next_key(key2);
      }
      else {
        key1 = next_key(key1);
      }
    }
  }
}

void onePowModFastTest(uint32_t a, uint32_t b, uint32_t m,
  // tests PowModFast
	uint32_t expected) {
  uint32_t calculated = powModFast(a, b, m);
  if (calculated != expected) {
    Serial.println("error in powMod test");
    Serial.print("expected: ");
    Serial.println(expected);
    Serial.print("got: ");
    Serial.println(calculated);
  }
}

void testPowModFast() {
	Serial.println("starting tests");
	// run powMod through a series of tests
  onePowModFastTest(1, 1, 20, 1); //1^1 mod 20 == 1
  onePowModFastTest(5, 7, 37, 18);
  onePowModFastTest(5, 27, 37, 31);
  onePowModFastTest(3, 0, 18, 1);
  onePowModFastTest(2, 5, 13, 6);
  onePowModFastTest(1, 0, 1, 0);
  onePowModFastTest(123456, 123, 17, 8);

  Serial.println("Now starting big test");
  uint32_t start = micros();
  onePowModFastTest(123, 2000000010ul, 17, 16);
  uint32_t end = micros();
  Serial.print("Microseconds for big test: ");
  Serial.println(end-start);

  Serial.println("Another big test");
  onePowModFastTest(123456789, 123456789, 61234, 51817);

  Serial.println("With a big modulus (< 2^31)");
  onePowModFastTest(123456789, 123456789, 2147483647, 1720786551);

  Serial.println("tests done!");
}

//CLIENT SIDE CODE //
uint32_t intializeHandshakeWithServer(uint32_t key){
  // initialise the states, start, waiting for acknowledgment and data exchange
	enum clientState {start, waitingForAck, dataExchange};
  clientState currentClientState = start;
  uint32_t sKey, currentMillis;
  while (true){
    if (currentClientState == start){
    	Serial3.print('C');
      uint32_to_serial3(key);
      currentClientState = waitingForAck;
      currentMillis = millis();
    }

    else if (currentClientState == waitingForAck && Serial3.read()=='A' && ((millis()-currentMillis) < 1000)){
    	bool areFourBytesAvailable = wait_on_serial3(4,1000);
      if (areFourBytesAvailable){
      	//read the key from serial 3
        sKey = uint32_from_serial3();
        // send acknowledgment
        Serial3.print('A');
        // go to data exchange state
        currentClientState = dataExchange;
      }
      else{
      	currentClientState = start;
      }
    }

    else if(currentClientState == dataExchange){
      // break out the while loop and return the other user's public key
      break;
    }

    else if (((millis()-currentMillis)) > 1000){
      // go back to start if time out
      currentClientState = start;
    }
  }
  return sKey;
  }

//SERVER SIDE CODE //
uint32_t intializeHandshakeWithClient(uint32_t key){
  enum serverStates { listen, waitingForKey, waitingForAck, waitingForKeyAgain, waitingForAckAgain, dataExchange, ERR};
  // initialise the states listen, waiting for key twice, waiting for
  // acknowledgment twice and data exchange
  serverStates currentServerState = listen;
  // initialise timer and received key
  uint32_t currentMillis;
  uint32_t rKey;
  while (true){
    if (currentServerState==listen && Serial3.read()=='C'){
    	currentServerState = waitingForKey;
    }

    else if (currentServerState == waitingForKey){
      bool areFourBytesAvailable = wait_on_serial3(4,1000);
      if (areFourBytesAvailable){
      	//read received key from serial3
        rKey = uint32_from_serial3();
        Serial3.print('A');
        uint32_to_serial3(key);
        currentServerState = waitingForAck;
        currentMillis = millis();
      }
      else{
        currentServerState = listen;
      }
    }
    else if (currentServerState==waitingForAck && ((millis()-currentMillis) < 1000)){
      if (Serial3.read()=='A'){
      	currentServerState = dataExchange;
      }
    	else if (Serial3.read()=='C'){
      	currentServerState = waitingForKeyAgain;
        currentMillis = millis();
      }
    }

    else if (currentServerState==waitingForKeyAgain && (Serial3.available()>=4) && ((millis()-currentMillis) < 1000)){
    	bool areFourBytesAvailable = wait_on_serial3(4,1000);
      if (areFourBytesAvailable){
      	rKey = uint32_from_serial3();
        currentServerState = waitingForAckAgain;
      }
      else{
      	currentServerState = listen;
      }
    }

    else if (currentServerState==waitingForAckAgain && ((millis()-currentMillis) < 1000)){
    	if (Serial3.read()=='A'){
      	currentServerState = dataExchange;
      }
      else if (Serial3.read()=='C'){
        currentServerState = waitingForKeyAgain;
      }
    }
    else if (currentServerState==dataExchange){
      // break out while loop
      break;
    }
    else if (((millis()-currentMillis)) > 1000){
      //go back to listen state if timeout
      currentServerState = listen;
    }
  }
  return rKey;
}
uint32_t publicKeyGenerator(){
  // defines the prime and the generator
  const uint32_t p = 2147483647, g = 16807;

  // calculates the public key using the generator, privateKeyGlobal, and p
  uint32_t privateKey = generateRandomPrivateKey();
  uint32_t myPublicKey = powModFast(g, privateKey, p);
  Serial.print("This arduino's public key is ");
  Serial.println(myPublicKey);
  uint32_t otherUserPublicKey;
  Serial.println("Looking for other arduino...");
  if (digitalRead(digitalPin) == HIGH) {
    otherUserPublicKey = intializeHandshakeWithClient(myPublicKey);
  }
  else {
    otherUserPublicKey = intializeHandshakeWithServer(myPublicKey);
  }
  Serial.print("Other arduino's public key is ");
  Serial.println(otherUserPublicKey);
  //calculates shared secret key using arduino's private key and other arduino's
  //public key
  uint32_t sharedKey = calculateSharedSecretKey(privateKey, otherUserPublicKey);
  Serial.println("Communication initiated!");
  //Serial.print(sharedKey);
  return sharedKey;
}

int main(){
  setup();

  uint32_t sharedKey = publicKeyGenerator();
  key1 = sharedKey;
  key2 = sharedKey;
  while (true){
    if (digitalRead(digitalPin) == HIGH) {
      readOtherUserChat(key1);
      writeToExternalSerial(key2);
    }
    else {
      readOtherUserChat(key2);
      writeToExternalSerial(key1);
    }

  }
  testPowModFast();

  Serial.flush();
  Serial3.flush();
  return 0;
  }
