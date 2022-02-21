#include <avr/io.h>
#include <avr/interrupt.h>
#include "../h/i2c_master.h"
#include "../h/liquid_crystal_i2c.h"
#include <stdio.h>
#include <util/delay.h>
#include <stdlib.h>

#define SCL PORTC5
#define SDA PORTC4

#define BUZZOFF PORTC1
#define LEDOUT PORTB5

#define ROWS 4
#define COLS 4

/*#define LIGHT_SENS_FAC 20*/

// uint8_t keyMap[ROWS][COLS] = { // key definitions
//     {1, 4, 7, 254},
//     {2, 5, 8, 0},
//     {3, 6, 9, 255},
//     {255, 255, 255, 255}
// };

uint8_t keyMap[ROWS][COLS] = { // key definitions
    {254, 7, 4, 1},
    {0, 8, 5, 2},
    {255, 9, 6, 3},
    {252, 253, 255, 255}};

uint8_t code[4] = {2, 5, 5, 6};
volatile keypadChangeState = 0; // 0 no change, 1 change
// uint8_t pin[4];

// void initADC();
// void readADC();
void initLCD();
uint8_t readKeypad(uint8_t);
unsigned char EEPROM_read(unsigned int);
void EEPROM_write(unsigned int, unsigned char);
int EEPROM_read_4digits();
void EEPROM_write_4digits(int);

// volatile uint16_t currentReading;

LiquidCrystalDevice_t device;

ISR(INT0_vect)
{
    _delay_ms(25);
    if (keypadChangeState == 0)
    {
        keypadChangeState = 1;
    }
    else
    {
        keypadChangeState = 0;
    }
}

void initADC_FRMode(uint8_t);

int main()
{

    DDRC |= (1 << BUZZOFF); // set buzzer to output mode
    DDRB |= (1 << PORTB4);  // set laser to output mode
    DDRB |= 0xF8;           // set keypad rows to input and laser to output mode
    DDRD |= 0x00;           // set keypad cols to input mode

    initLCD();
    initADC_FRMode(0);
    initINT0();

    if (EEPROM_read(4) == 0)
    {
        for (int i = 0; i < 4; i++)
        {
            code[i] = EEPROM_read(i);
        }
    }
    else
    {
        // write to EEPROM if the current EEPROM has no initial write at ADDRESS 0.
        for (int i = 0; i < 4; i++)
        {
            EEPROM_write(i, code[i]);
        }
        EEPROM_write(4, 1); // write 4th address if there is already a PIN ready. Else, it's already written
    }

    uint8_t keyPressed = 255; // initially no keys were pressed yet
    sei();                    // enable interrupts
    uint8_t alarmState = 0;   // 0 no alarm, 1 alarm
    // EEPROM_write_4digits(pin);

    while (1)
    {
        PORTB |= (1 << PORTB4); // enables or turns on laser
        if (alarmState == 0)
        { // if alarm is off
            if (ADC > 50)
            {                           // if laser is blocked
                PORTC |= (1 << PORTC1); // enables or turns on buzzer
                lq_clear(&device);
                lq_setCursor(&device, 0, 0);
                lq_print(&device, "ALARM ON");
                lq_setCursor(&device, 1, 0);
                lq_print(&device, "PIN:");
                alarmState = 1;
            }
            else
            { // if laser is not blocked
                lq_setCursor(&device, 0, 0);
                lq_print(&device, "MONITORING");
                // ALLOW FOR KEYPAD CHANGE
                if (keypadChangeState == 1)
                {
                    lq_clear(&device);
                    lq_setCursor(&device, 0, 0);
                    lq_print(&device, "KEYPAD CHANGE...");
                    uint8_t buffer[4];
                    uint8_t digitC = 0;
                    while (digitC != 10)
                    {
                        char keypad[4];
                        keyPressed = 255;
                        keyPressed = readKeypad(keyPressed);
                        if (keyPressed == 255)
                        {
                        }
                        else if (keyPressed == 254)
                        {
                            uint8_t *check = malloc(sizeof(uint8_t) * 4);
                            for (int i = 0; i < 4; i++)
                            {
                                *(check + i) = 0;
                            }
                            for (uint8_t i = 0; i < 4; i++)
                            {
                                if (buffer[i] != code[i])
                                {
                                    lq_clear(&device);
                                    keyPressed = 255;
                                    digitC = 0;
                                    lq_setCursor(&device, 0, 0);
                                    lq_print(&device, "PIN INVALID");
                                    lq_setCursor(&device, 1, 0);
                                    lq_print(&device, "PIN:");
                                    digitC = 0;
                                }
                                else if (buffer[i] == code[i])
                                {
                                    *(check + i) = 1;
                                }
                            }
                            if ((check[0] == 1) && (check[1] == 1) && (check[2] == 1) && (check[3] == 1))
                            {
                                free(check);
                                lq_clear(&device);
                                lq_setCursor(&device, 0, 0);
                                lq_print(&device, "Input new PIN");
                                // clear buffer
                                for(int i = 0; i < 4; i++){
                                    buffer[i] = 0; 
                                }
                                lq_setCursor(&device, 1, 0);
                                uint8_t newDigitC = 0;
                                keyPressed = 255;
                                keyPressed = readKeypad(keyPressed);
                                if (keyPressed == 255)
                                {
                                }
                                else if (keyPressed == 254)
                                {
                                    for (int i = 0; i < 4; i++)
                                    {
                                        EEPROM_write(i, buffer[i]);
                                    }
                                }
                                else if (keyPressed == 253)
                                {
                                    if (digitC == 0)
                                    {
                                        _delay_ms(100);
                                    }
                                    else
                                    {
                                        digitC--;
                                        lq_setCursor(&device, 1, digitC + 4);
                                        lq_print(&device, " ");
                                        _delay_ms(250);
                                    }
                                }
                                else {
                                    buffer[newDigitC] = keyPressed; // overwrite old buffer
                                    sprintf(keypad, "%u", keyPressed);
                                    lq_setCursor(&device, 1, newDigitC + 4);
                                    lq_print(&device, keypad);
                                    digitC++;
                                    _delay_ms(250);
                                }
                            }
                            else
                            {
                                _delay_ms(25);
                            }
                        }
                        else if (keyPressed == 253)
                        {
                            if (digitC == 0)
                            {
                                _delay_ms(100);
                            }
                            else
                            {
                                digitC--;
                                lq_setCursor(&device, 1, digitC + 4);
                                lq_print(&device, " ");
                                _delay_ms(250);
                            }
                        }
                        else
                        {
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
        }
        else
        { // if alarm is on
            uint8_t buffer[4];
            uint8_t digitC = 0;
            while (digitC != 10)
            {
                char keypad[4];
                keyPressed = 255;
                keyPressed = readKeypad(keyPressed);
                if (keyPressed == 255)
                {
                }
                else if (keyPressed == 254)
                {
                    uint8_t *check = malloc(sizeof(uint8_t) * 4);
                    for (int i = 0; i < 4; i++)
                    {
                        *(check + i) = 0;
                    }
                    for (uint8_t i = 0; i < 4; i++)
                    {
                        if (buffer[i] != code[i])
                        {
                            lq_clear(&device);
                            keyPressed = 255;
                            digitC = 0;
                            lq_setCursor(&device, 0, 0);
                            lq_print(&device, "PIN INVALID");
                            lq_setCursor(&device, 1, 0);
                            lq_print(&device, "PIN:");
                            digitC = 0;
                        }
                        else if (buffer[i] == code[i])
                        {
                            *(check + i) = 1;
                        }
                    }
                    if ((check[0] == 1) && (check[1] == 1) && (check[2] == 1) && (check[3] == 1))
                    {
                        free(check);
                        lq_clear(&device);
                        PORTC &= ~(1 << BUZZOFF);
                        alarmState = 0;
                        break;
                    }
                    else
                    {
                        _delay_ms(25);
                    }
                }
                else if (keyPressed == 253)
                {
                    if (digitC == 0)
                    {
                        _delay_ms(100);
                    }
                    else
                    {
                        digitC--;
                        lq_setCursor(&device, 1, digitC + 4);
                        lq_print(&device, " ");
                        _delay_ms(250);
                    }
                }
                else
                {
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
}

uint8_t readKeypad(uint8_t keyPressed)
{
    PORTB |= 0x0F; // Initialize rows pins as input with pull-up enabled
    for (uint8_t c = 0; c < COLS; ++c)
    {                                      // go through all column pins
        DDRD |= (1 << c << 4);             // we pulse each columns
        PORTD &= ~(1 << c << 4);           // to LOW level
        for (uint8_t r = 0; r < ROWS; ++r) // go through all row pins
            if (!(PINB & (1 << r)))        // and check at which row was pulled LOW
                keyPressed = keyMap[r][c]; // assign the pressed key if confirmed
        PORTD |= (1 << c << 4);            // end of column pulse
        DDRD &= ~(1 << c << 4);            // and set it back to input mode
    }
    return keyPressed; // output the pressed key
}

// SDA -> PC4
// SCL -> PC5
void initLCD()
{
    device = lq_init(0x27, 20, 2, LCD_5x8DOTS); // initialize 4-lines display

    lq_turnOnBacklight(&device); // simply turning on the backlight
}

// This function initializes ADC module to free-response mode (auto-trigger)
// and read from the specified analog channel.
void initADC_FRMode(uint8_t channel)
{
    ADMUX |= (1 << REFS0);     // selects internal AVCC as voltage reference
    ADMUX |= (channel & 0x0F); // select the input channel
    ADCSRA |= (1 << ADATE);    // enable ADC auto trigger (free-running mode)
    ADCSRB = ~((1 << ADTS2) | (1 << ADTS1) | (1 << ADTS0));
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // set prescaler to 128
    ADCSRA |= (1 << ADEN);                                // enable ADC
    ADCSRA |= (1 << ADSC);                                // start conversion
}

void initINT0()
{
    DDRD &= ~(1 << PORTD2);
    PORTD |= (1 << PORTD2);
    EICRA |= (0b10 << ISC00); // enable interrupt on falling edge
    EIMSK |= (1 << INT0);     // enable INT0
}

void EEPROM_write_4digits(int data)
{
    // Each address location in EEPROM have 8 bits or 2 hex digits,
    // 9999 is 0x270F, so we need 2 locations for 4 hex digits.
    // LSB first then MSB
    // Ex. pin = 1234; Address0 = 12, Address1 = 34
    for (uint8_t i = 0; i < 2; ++i)
    {
        EEPROM_write(i, (data >> i * 8) & 0xFF);
        // Ex. i=0, right shift data 0 times then
        // AND with 0xFF to ensure only last byte is activated
    }
}

int EEPROM_read_4digits()
{
    return (EEPROM_read(0) & 0xFF) + ((EEPROM_read(1) << 8) & 0xFFFF);
    // Ex. i=0, left shift readValue 0 times then
    // AND with 0xFF to ensure only last byte is activated
}

unsigned char EEPROM_read(unsigned int uiAddress)
{
    cli(); // disable global interrupts to ensure no data corruption
    /* Wait for completion of previous write */
    while (EECR & (1 << EEPE))
        ;

    /* Set up address register */
    EEAR = uiAddress;

    /* Start eeprom read by writing EERE */
    EECR |= (1 << EERE);

    sei(); // renable global interrupts
    /* Return data from Data Register */
    return EEDR;
}

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
    cli(); // disable global interrupts to ensure no data corruption
    /* Wait for completion of previous write */
    while (EECR & (1 << EEPE))
        ;

    /* Set up address and Data Registers */
    EEAR = uiAddress;
    EEDR = ucData;

    /* Write logical one to EEMPE */
    EECR |= (1 << EEMPE);

    /* Start eeprom write by setting EEPE */
    EECR |= (1 << EEPE);
    sei(); // renable global interrupts
}