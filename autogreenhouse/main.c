// Define F_CPU for ATmega328P (16 MHz, common default)
#ifndef F_CPU
#define F_CPU 16000000UL  // 16 MHz
#endif


#include <avr/io.h>
#include <util/delay.h>


// Pin definitions for ATmega328P
#define DHT_PIN PINC2      // DHT11 data pin on PC2
#define LCD_PORT PORTD     // LCD data lines on PORTD (8-bit interface)
#define LCD_RS PORTC1      // Register Select on PC1
#define LCD_EN PORTC0      // Enable on PC0
#define SOIL_SENSOR_PIN 3  // Soil Moisture Sensor on ADC3 (PC3, A3)

// Function prototypes
void init(void);
void initLCD(void);
void sendCommand(uint8_t cmd);
void sendData(uint8_t data);
void sendTemperatureWord(void);
void sendHumidityWord(void);
void sendSoilWord(void);
void convertData(void);
void readSoilMoisture(void);
void writeDHTData(void);
void writeSoilMoisture(void);
uint8_t readDHTBit(void);
void hexToAscii(uint8_t value, uint8_t *high, uint8_t *low);
uint16_t readADC(uint8_t channel);

// Global variables for temperature, humidity, and soil moisture
uint8_t humidityInt, humidityDec, temperature;
uint16_t soilMoisture;
uint8_t displayToggle = 0;  // Variable for alternating display (0 - DHT, 1 - Soil)

int main(void) {
	// Initialize stack pointer for ATmega328P (RAMEND = 0x08FF)
	SPH = (RAMEND >> 8);    // High byte (0x08 for ATmega328P)
	SPL = (RAMEND & 0xFF);  // Low byte (0xFF for ATmega328P)
	
	_delay_ms(18);  // Wait for LCD power on
	
	while (1) {
		init();
		initLCD();
		convertData();           // Read DHT data (temperature and humidity)
		readSoilMoisture();      // Read soil moisture
		
		// Clear display before new display
		sendCommand(0x01);  // Clear display
		_delay_ms(2);       // Delay after clear
		
		if (displayToggle == 0) {
			// Show DHT11 data (humidity and temperature)
			sendHumidityWord();
			writeDHTData();  // Includes both humidity and temperature
			} else {
			// Show Soil Moisture data
			sendSoilWord();
			writeSoilMoisture();
		}
		
		displayToggle = !displayToggle;  // Toggle state (0 -> 1, 1 -> 0)
		_delay_ms(3000);  // 3-second delay between displays
	}
}

void init(void) {
	DDRD = 0xFF;    // PORTD as output for LCD data (8-bit)
	DDRB &= ~(1 << 0);  // PB0 as input (not used for LCD/DHT in this config)
	DDRC |= (1 << 0) | (1 << 1) | (1 << 2);  // PC0 (EN), PC1 (RS), PC2 (DHT) as outputs
	// ADC is input by default, no need to set DDR for PC3 (A3)
}

void initLCD(void) {
	_delay_ms(50);          // Delay after power-on (>40ms)
	sendCommand(0x38);      // 8-bit, 2 lines
	_delay_ms(5);           // Delay >4.1ms
	sendCommand(0x38);      // Repeat command
	_delay_us(150);         // Delay >100us
	sendCommand(0x38);      // Repeat for reliability
	sendCommand(0x0C);      // Display on, cursor off, blink off (or 0x0F for cursor)
	sendCommand(0x06);      // Entry mode: increment, no shift
	sendCommand(0x01);      // Clear display
	_delay_ms(2);           // Delay after clear (>1.52ms)
}

void sendCommand(uint8_t cmd) {
	LCD_PORT = cmd;
	PORTC &= ~(1 << LCD_RS);  // RS = 0 (command mode)
	_delay_us(1);
	PORTC |= (1 << LCD_EN);   // EN = 1
	_delay_us(1);
	PORTC &= ~(1 << LCD_EN);  // EN = 0
	_delay_us(100);
}

void sendData(uint8_t data) {
	LCD_PORT = data;
	PORTC |= (1 << LCD_RS);   // RS = 1 (data mode)
	_delay_us(1);
	PORTC |= (1 << LCD_EN);   // EN = 1
	_delay_us(1);
	PORTC &= ~(1 << LCD_EN);  // EN = 0
	_delay_us(100);
}

void sendTemperatureWord(void) {
	const char tempWord[] = "Temp    :";
	for (uint8_t i = 0; tempWord[i] != ':'; i++) {
		sendData(tempWord[i]);
	}
	sendData(':');
}

void sendHumidityWord(void) {
	const char humWord[] = "Hum     :";
	for (uint8_t i = 0; humWord[i] != ':'; i++) {
		sendData(humWord[i]);
	}
	sendData(':');
}

void sendSoilWord(void) {
	const char soilWord[] = "Soil    :";
	for (uint8_t i = 0; soilWord[i] != ':'; i++) {
		sendData(soilWord[i]);
	}
	sendData(':');
}

void convertData(void) {
	// DHT sensor communication
	PORTC &= ~(1 << DHT_PIN);  // Pull low
	_delay_ms(18);
	PORTC |= (1 << DHT_PIN);   // Pull high
	DDRC &= ~(1 << DHT_PIN);   // Set as input
	
	_delay_us(30);
	_delay_us(80);
	_delay_us(80);
	_delay_us(50);
	
	// Wait for sensor response
	while (!(PINC & (1 << DHT_PIN)));
	
	// Read humidity integer part (8 bits)
	humidityInt = 0;
	for (uint8_t i = 0; i < 8; i++) {
		while (!(PINC & (1 << DHT_PIN)));
		_delay_us(30);
		humidityInt <<= 1;
		if (PINC & (1 << DHT_PIN)) {
			humidityInt |= 1;
		}
		while (PINC & (1 << DHT_PIN));
	}
	
	// Read humidity decimal part (8 bits)
	humidityDec = 0;
	for (uint8_t i = 0; i < 8; i++) {
		while (!(PINC & (1 << DHT_PIN)));
		_delay_us(30);
		humidityDec <<= 1;
		if (PINC & (1 << DHT_PIN)) {
			humidityDec |= 1;
		}
		while (PINC & (1 << DHT_PIN));
	}
	
	// Read temperature (8 bits)
	temperature = 0;
	for (uint8_t i = 0; i < 8; i++) {
		while (!(PINC & (1 << DHT_PIN)));
		_delay_us(30);
		temperature <<= 1;
		if (PINC & (1 << DHT_PIN)) {
			temperature |= 1;
		}
		while (PINC & (1 << DHT_PIN));
	}
}

void readSoilMoisture(void) {
	// Initialize ADC for Soil Moisture Sensor on ADC3 (A3)
	ADMUX = (1 << REFS0);  // Use AVcc as reference
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Enable ADC, Prescaler 128 (16 MHz / 128 = 125 kHz)
	
	// Read ADC value from Soil Moisture Sensor
	soilMoisture = readADC(SOIL_SENSOR_PIN);
}

uint16_t readADC(uint8_t channel) {
	// Ensure channel is within valid range (0-7 for ATmega328P)
	channel &= 0x07;
	
	// Set the ADC channel
	ADMUX = (ADMUX & 0xF0) | channel;
	
	// Start conversion
	ADCSRA |= (1 << ADSC);
	
	// Wait for conversion to complete
	while (ADCSRA & (1 << ADSC));
	
	// Return ADC result
	return ADC;
}

void hexToAscii(uint8_t value, uint8_t *high, uint8_t *low) {
	uint8_t tens = 0;
	while (value >= 10) {
		value -= 10;
		tens++;
	}
	*high = tens + '0';
	*low = value + '0';
}

void writeDHTData(void) {
	// Write humidity on first line
	uint8_t high, low;
	hexToAscii(humidityInt, &high, &low);
	sendData(high);
	sendData(low);
	sendData(' ');
	sendData('%');
	
	// Move to second line and write temperature
	sendCommand(0xC0);  // Second line
	sendTemperatureWord();
	hexToAscii(temperature, &high, &low);
	sendData(high);
	sendData(low);
	sendData(' ');
	sendData('C');
}

void writeSoilMoisture(void) {
	// Write soil moisture on first line
	uint8_t high, low;
	uint8_t displayValue = soilMoisture / 10;  // Scale down for readability (e.g., 0-102)
	hexToAscii(displayValue, &high, &low);
	sendData(high);
	sendData(low);
	sendData(' ');
	sendData('%');
	
	// Clear second line or leave it empty (optional)
	sendCommand(0xC0);  // Second line
	sendData(' ');      // Clear or leave blank (you can add more spaces or text)
}