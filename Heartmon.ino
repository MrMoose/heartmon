#include "Macros.h"
#include "Adafruit_NeoPixel.h"
#include "RunningMedian.h"

/******************************
   
  		Pulse Sensor

*******************************/

const uint8_t s_pulsePin  = 10; // Pulse Sensor purple wire connected to analog pin 10
const uint8_t s_blinkPin  = 7;  // pin to blink led at each beat (7 is on-board LED)
const uint8_t s_fade_rate = 4;  // fading speed. subtracted from s_fade each 20ms after pulse 
const uint16_t s_max_pulse_age = 8192; // max cycles since last pulse before we consider us dead
uint16_t s_current_pulse_age = 0;  // set to zero every time we find a pulse
int s_last_signal = 0;

// these variables are volatile because they are used during the interrupt service routine!
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, must be seeded! 
volatile boolean s_pulse = false;   // true when pulse wave is high, false when it's low
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.
bool     s_alive = false;           // switch between running modes. Either with pulse or, if too old, with demo

// this will track values is used for watermarks
typedef RunningMedian<int, 50> Median;
Median   s_median;
uint8_t  s_sample_cnt = 0;


/******************************
   
  		Neopixels

*******************************/
const unsigned int s_num_pixels = 3;  // number of pixels in strip
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_RGB     Pixels are wired for RGB bitstream
//   NEO_GRB     Pixels are wired for GRB bitstream
//   NEO_KHZ400  400 KHz bitstream (e.g. FLORA pixels)
//   NEO_KHZ800  800 KHz bitstream (e.g. High Density LED strip)
// Test Neopixel v1
Adafruit_NeoPixel s_strip = Adafruit_NeoPixel(3, 6, NEO_GRB + NEO_KHZ400);

// Live Neopixel v2:
//Adafruit_NeoPixel s_strip = Adafruit_NeoPixel(3, 6, NEO_GRB + NEO_KHZ400);


// Funtion Declarations
void setup(void);
void loop(void);
void loop_demo(void);
void loop_pulse(void);
void setIntensity(unsigned int n_index, unsigned short n_intensity);

// fade external LEDs each beat
void ledFadeToBeat(void);

void adjust_watermarks(void);

//! sets up to read Pulse Sensor signal every 2mS 
void interruptSetup(void); 


void setup(void) {
 
	// Initialize LED strip
	s_strip.begin();
	s_strip.show();                   // Initialize all pixels to 'off'

	pinMode(s_blinkPin, OUTPUT);      // on-board LED that will blink to heartbeat

	Serial.begin((unsigned int)115200); // we agree to talk fast!
	interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 

	analogReference(EXTERNAL);        // use 3.3V power instead of 5. Important
}

void loop(void) {

	if (!s_alive && (abs(Signal - s_last_signal) > 50)) {
		s_alive = true;
	//	Serial.println("Yeah! Back from the dead!"); 
	}

	if (Signal && (s_sample_cnt++ == 5)) {
		s_median.add(Signal);
		s_sample_cnt = 0;
	}

	s_last_signal = Signal;

//	sendDataToProcessing('S', Signal);  // send Processing the raw Pulse Sensor data
	if (QS == true) {                    // Quantified Self flag is true when arduino finds a heartbeat
		s_current_pulse_age = 0;
//		sendDataToProcessing('B', BPM); // send heart rate with a 'B' prefix
//		sendDataToProcessing('Q', IBI); // send time between beats with a 'Q' prefix
		QS = false;                     // reset the Quantified Self flag for next time
	} else {
		// advance but not overflow pulse age
		if (s_current_pulse_age < (__MAX(int) - 20)) { 
			s_current_pulse_age += 20;
		}
		if (s_current_pulse_age > s_max_pulse_age) {
			s_alive = false;   // should I be sad?
		//	Serial.println("You are dead. Sorry.."); 
		}
	}

	if (s_alive) {
		loop_pulse();
	} else {
		loop_demo();
	}

	delay(20);                         //  take a break
}

void loop_demo(void) {

	digitalWrite(s_blinkPin, LOW);    // flatline is flatline	
	if (!rainbow(20)) {
		return;
	};
	if (!rainbowCycle(20)) {
		return;
	};
}


void loop_pulse(void) {

	// let it flow
//	adjust_watermarks();

	// set onboard LED to high or low pulse
	digitalWrite(s_blinkPin, s_pulse ? HIGH : LOW);

	// use external LEDs
	ledFadeToBeat();
}


void sample(void) {

}


void ledFadeToBeat(void) {

	int lowest  = 0;
	int highest = 0;
	int fade    = Signal;

	if (s_median.getMinMax(lowest, highest) == Median::OK) {

		if (highest != lowest) {
			fade = (Signal - (float)lowest) / ((float)highest - (float)lowest) * 255.0;
		}

		/*
		Serial.print("low wm: ");
		Serial.print(lowest);
		Serial.print(" high wm: ");
		Serial.print(highest);
		Serial.print(" value: ");
		Serial.println(fade);
		*/
	} 

	uint8_t constrained_fade = constrain(fade, 0, 255);

	setIntensity(0, constrained_fade);
	setIntensity(1, constrained_fade);
	setIntensity(2, constrained_fade);
	s_strip.show();
}


void setIntensity(unsigned int n_index, uint8_t n_intensity) {

	uint8_t red   = n_intensity; 
	uint8_t green = n_intensity / 10;
	uint8_t blue  = n_intensity / 4;

//	sendColorToSerial("color ", red, green, blue);

	// Test Pixels have blue and green swapped for some reason
	s_strip.setPixelColor(n_index, red, blue, green);
}

void sendDataToProcessing(char symbol, int data) {

	Serial.print(symbol);                  // symbol prefix tells Processing what type of data is coming
	Serial.println(data);                  // the data to send culminating in a carriage return
}

void sendColorToSerial(const char *n_message, const uint8_t n_red, const uint8_t n_green, const uint8_t n_blue) {

	Serial.print(n_message);
	Serial.print("r: ");
	Serial.print(n_red);
	Serial.print(" g: ");
	Serial.print(n_green);
	Serial.print(" b: ");
	Serial.println(n_blue);
}




/******************************
   
  		Neopixels

*******************************/

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {

	for (uint8_t i=0; i < s_strip.numPixels(); i++) {
		if (s_pulse) {
			return;
		}
		s_strip.setPixelColor(i, c);
		s_strip.show();	
		delay(wait);
	}
}


// return false when interrupted by pulse
bool rainbow(uint8_t wait) {

	uint16_t i, j;

	for (j=0; j<256; j++) {
		if (s_pulse) {
			return false;
		}
		for (i=0; i < s_strip.numPixels(); i++) {
			s_strip.setPixelColor(i, Wheel((i+j) & 255));
		}
		s_strip.show();
		delay(wait);
	}

	return true;
}

// Slightly different, this makes the rainbow equally distributed throughout
bool rainbowCycle(uint8_t wait) {
	
	uint16_t i, j;

	for (j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
		if (s_pulse) {
			return false;
		}
		for (i=0; i< s_strip.numPixels(); i++) {
			s_strip.setPixelColor(i, Wheel(((i * 256 / s_strip.numPixels()) + j) & 255));
		}
		s_strip.show();
		delay(wait);
	}
	return true;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  
	if (WheelPos < 85) {
		return s_strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
	} else if (WheelPos < 170) {
		WheelPos -= 85;
		return s_strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
	} else {
		WheelPos -= 170;
		return s_strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
	}
}


