#include <avr/io.h>
#include <avr/interrupt.h>

#define KR0 PORTD0
#define KR1 PORTD1
#define KR2 PORTD2
#define KR3 PORTD3
#define KC0 PORTD4
#define KC1 PORTD5
#define KC2 PORTD6 
#define KC3 PORTD7

#define SCL PORTC5
#define SDA PORTC4

#define BUZZOFF PORTB0
#define PHRIN PORTC0
#define LEDOUT PORTB5

char keyPad[4][4] = {
    {'1', '2', '3', 'm'},
    {'4', '5', '6', 'i'},
    {'7', '8', '9', 'c'},
    {'p', 'r', '0', 's'}
};

int main(){

    while(1){

    }
    return 0;
}

void initADC(){

}

void readKeypad(){

}