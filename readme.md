# Automation Greenhouse

## Overview
This project is a **Automation Greenhouse** using **ATmega328P**, **DHT11** (temperature & humidity sensor), and a **Soil Moisture Sensor**. The system is developed in **C** using **Microchip Studio** and simulated in **Proteus**.

## Features
- Measures **temperature** and **humidity** using the **DHT11** sensor.
- Detects **soil moisture levels** using a **Soil Moisture Sensor**.
- Automatically controls the **water pump** based on soil moisture levels.
- Uses **ATmega328P** as the microcontroller.
- Simulated in **Proteus** for validation.

## Components
- **Microcontroller:** ATmega328P
- **Temperature & Humidity Sensor:** DHT11
- **Soil Moisture Sensor**
- **Water Pump & Relay Module**
- **Power Supply (5V/12V as required)**
- **LCD Display (optional, for displaying sensor readings)**

## Circuit Diagram
![Circuit Diagram](https://github.com/user-attachments/assets/be8fb89e-7363-473b-9899-7b49087cd26f)

## Software & Tools
- **Microchip Studio** (for programming in C)
- **Proteus** (for circuit simulation)

## Code
The system continuously reads data from the **DHT11** and **Soil Moisture Sensor** and controls the water pump accordingly.

```c
#include <avr/io.h>
#include <util/delay.h>

#define SOIL_SENSOR_PIN  PC0
#define PUMP_PIN         PB0

void init_ADC() {
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t read_ADC(uint8_t channel) {
    ADMUX = (ADMUX & 0xF8) | (channel & 0x07);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

void setup() {
    DDRB |= (1 << PUMP_PIN);
    init_ADC();
}

int main() {
    setup();
    while (1) {
        uint16_t soil_moisture = read_ADC(SOIL_SENSOR_PIN);
        if (soil_moisture < 500) {
            PORTB |= (1 << PUMP_PIN);
        } else {
            PORTB &= ~(1 << PUMP_PIN);
        }
        _delay_ms(1000);
    }
}
```

## Simulation in Proteus
1. **Connect ATmega328P** with the sensors and relay module.
2. **Load the compiled HEX file** from Microchip Studio into Proteus.
3. **Run the simulation** to test sensor readings and water pump activation.

## Future Enhancements
- Add an **LCD** or **OLED** display for real-time monitoring.
- Implement **remote monitoring** using an **ESP8266 WiFi module**.
- Optimize power consumption with **low-power modes**.

## Author
**Kabdylkak Arnur Nurlanuly**

