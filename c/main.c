#include <avr/io.h>
#include <avr/interrupt.h>
#include "../h/i2c_master.h"
#include "../h/liquid_crystal_i2c.h"
#include <stdio.h>
#include <util/delay.h>
#include <stdlib.h>

#define SCL PORTC5
#define SDA PORTC4

#define BUZZOFF PORTC0
#define PHRIN PORTC0
#define LEDOUT PORTB5

#define ROWS 4
#define COLS 4

#define LIGHT_SENS_FAC 20

uint8_t keyMap[ROWS][COLS] = { // key definitions
    {1, 4, 7, 254},
    {2, 5, 8, 0},
    {3, 6, 9, 255},
    {255, 255, 255, 255}
};

uint8_t code[4] = {2, 5, 5, 6}; 


void initADC();
void readADC();
void initLCD();
uint8_t readKeypad(uint8_t);

volatile uint16_t currentReading; 

LiquidCrystalDevice_t device;

ISR(ADC_vect){

}

int main(){

    initADC();
    initLCD();
    DDRB &= ~(1 << PORTB0) & ~(1 << PORTB1) & ~(1 << PORTB2) & ~(1 << PORTB3);
    DDRC |= (1 << PORTC0);
    DDRD |= 0x1F; // set PORTC pins as output
    uint8_t keyPressed = 255; // initially no keys were pressed yet
    sei(); // enable interrupts 
    uint16_t curr = 0;
    uint8_t alarmState = 0; // 0 no alarm, 1 alarm
    while(1){
        readADC();
        uint16_t _currentReading = currentReading;
        if(alarmState == 0){
            if(_currentReading < LIGHT_SENS_FAC){
                lq_clear(&device);
                PORTC |= (1 << BUZZOFF);
                lq_setCursor(&device, 0, 0);
                lq_print(&device, "ALARM ON");
                lq_setCursor(&device, 1, 0);
                lq_print(&device, "PIN:");
                alarmState = 1;
            } else {
                lq_setCursor(&device, 0, 0);
                lq_print(&device, "MONITORING");
            }
        } else {
            uint8_t buffer[4];
            uint8_t digitC = 0;
            while(digitC != 10){
                char keypad[4];
                keyPressed = 255;
                keyPressed = readKeypad(keyPressed);
                if(keyPressed == 255){

                } else if(keyPressed == 254){
                    uint8_t* check = malloc(sizeof(uint8_t) * 4);
                    for(int i = 0; i < 4; i++){
                        *(check + i) = 0; 
                    }
                    for(uint8_t i = 0; i < 4; i++){
                        if(buffer[i] != code[i]){
                            lq_clear(&device);
                            keyPressed = 255;
                            digitC = 0; 
                            lq_setCursor(&device, 0, 0);
                            lq_print(&device, "PIN INVALID");
                            lq_setCursor(&device, 1, 0);
                            lq_print(&device, "PIN:");
                            digitC = 0; 
                        } else if(buffer[i] == code[i]){
                            *(check + i) = 1;
                        }
                    }
                    if((check[0] == 1) && (check[1] == 1) && (check[2] == 1) && (check[3] == 1)){
                        free(check);
                        lq_clear(&device);
                        PORTC &= ~(1 << BUZZOFF);
                        alarmState = 0;
                        break;
                    } else {
                        _delay_ms(25);
                    }
                } else {
                    buffer[digitC] = keyPressed;
                    sprintf(keypad, "%u", keyPressed);
                    lq_setCursor(&device, 1, digitC + 4);
                    lq_print(&device, keypad);
                    digitC++;
                    _delay_ms(250);
                }
            }

        }
    }
    return 0;
}

/*

    High light = Low resistance

*/
void initADC(){
    PRR &= ~(1 << PRADC); // disable power saving for ADC
    ADMUX |= (0b11 << REFS0) | (0b0001 << MUX0);
    ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADIE) | (0b001 << ADPS0);
    while(ADCSRA & (1 << ADSC)); // wait for initial conversion to finish
    // ADCSRB |= (0b000 << ADTS0); // Let ADC be in free running mode
}

uint8_t readKeypad(uint8_t keyPressed){
    PORTB |= 0x0F; // Initialize rows pins as input with pull-up enabled
    for (uint8_t c = 0; c < COLS; ++c) { // go through all column pins
        DDRD |= (1<<c<<4); // we pulse each columns 
        PORTD &= ~(1<<c<<4); // to LOW level
        for (uint8_t r = 0; r < ROWS; ++r) // go through all row pins
            if (!(PINB & (1<<r)))  // and check at which row was pulled LOW
                keyPressed = keyMap[r][c]; // assign the pressed key if confirmed
        PORTD |= (1<<c<<4); // end of column pulse
        DDRD &= ~(1<<c<<4); // and set it back to input mode
    }
    return keyPressed; // output the pressed key
}

// SDA -> PC4
// SCL -> PC5
void initLCD(){
    device = lq_init(0x27, 20, 4, LCD_5x8DOTS); // intialize 4-lines display

    lq_turnOnBacklight(&device); // simply turning on the backlight

}

void readADC(){
    ADCSRA |= (1 << ADSC);
    while(ADCSRA & (1 << ADSC));
    uint8_t regL = ADCL;
    uint8_t regH = ADCH;
    currentReading = (regH << 8) | regL;
}