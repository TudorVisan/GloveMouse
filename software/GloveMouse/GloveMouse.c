/*
 * GloveMouse.c
 * v1.0
 *
 * Created: 6/18/2012 4:24:27 PM
 *  Author: Tudor
 */ 

#define F_CPU 16000000

#define XvPin		0
#define YvPin		1
#define Button0Pin	4
#define Button1Pin	5
#define BUTTON0		0
#define BUTTON1		1
#define BUTTON2		2
//#define XaccPin	2
//#define YaccPin	3

#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>


typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;	
} mouseReport;


void initUSART() {
    UBRR0 = 103;
	UCSR0A = 0;
	UCSR0B = (1<<TXEN0);
	UCSR0C = (3<<UCSZ00);
}

void initPins() {
	PORTD |= (7<<PORTD2);
}

void initADC() {
	ADCSRA = (1<<ADEN) | (3<<ADPS0);
	ADMUX = (1<<REFS0) | (1<<ADLAR);
}

inline long map(uint8_t val, float minVal, float maxVal, float min, float max) {
	return (long)(((max - min) / (maxVal - minVal)) * val + ((min * maxVal - max * minVal) / (maxVal - minVal)));
}

uint8_t read(uint8_t channel) {
	if(channel > 5) 
		return -1;
	
	ADMUX &= 0xE0;
	ADMUX |= channel;
	ADCSRA |= (1<<ADSC);
	while(! (ADCSRA & (1<<ADIF)));
	return (uint8_t)map(ADCH, 0, 171, 0, 255);
}

uint8_t mediate(uint8_t channel) {
	uint16_t sum = 0, i;
	for(i=0; i<16; ++i) {
		sum += read(channel);
	}
	return (uint8_t)(sum>>4);
}

void write(mouseReport report) {
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = report.buttons;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = report.x;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = report.y;
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = report.wheel;
}

void test() {
	int ind;
	mouseReport nullReport = { 0, 0, 0, 0 };
	mouseReport report;	
	_delay_ms(1000);

	report.buttons = 0;
	report.x = 0;
	report.y = 0;
	report.wheel = 0;

	report.x = -2;
	for (ind=0; ind<20; ind++) {
		write(report);
		write(nullReport);
	}

	report.x = 0;
	report.y = -2;
	for (ind=0; ind<20; ind++) {
		write(report);
		write(nullReport);
	}

	report.x = 2;
	report.y = 0;
	for (ind=0; ind<20; ind++) {
		write(report);
		write(nullReport);
	}

	report.x = 0;
	report.y = 2;
	for (ind=0; ind<20; ind++) {
		write(report);
		write(nullReport);
	}
}

int main(void)
{
	mouseReport nullReport = { 0, 0, 0, 0 };
	mouseReport report = { 0, 0, 0, 0 };
	uint8_t Xv0, Yv0;
	uint8_t Xv, Yv;
	uint8_t B0, B1;
	//uint8_t Xacc0, Yacc0;
	//uint8_t Xacc = 0, Yacc = 0;
	//int movementX = 0, movementY = 0, countX = 0, countY = 0;
	
	initUSART();
	initADC();
	initPins();
	_delay_ms(1000);
	
	//Xacc0 = mediate(XaccPin);
	//Yacc0 = mediate(YaccPin);
	//Xacc = Xacc0; Yacc = Yacc0;
	Xv0 = mediate(XvPin);
	Yv0 = mediate(YvPin);
	Xv = Xv0; Yv = Yv0; 
	B0 = mediate(Button0Pin);
	B1 = mediate(Button1Pin);
	
	test();
					
    while(1)
    {
		report.buttons = 0;
		report.x = 0;
		report.y = 0;
		report.wheel = 0;
		
		//gyro X axis
		//Xv = (Xv*5 + read(XvPin)*3) / 8;
		Xv = read(XvPin);
		//report.x = (Xv - Xv0);
		report.x = ((Xv < Xv0 - 2) || (Xv > Xv0 + 2))? (Xv - Xv0) * (3*abs(Xv - Xv0)/2 + 37) / 80 : 0;
		
		//gyro Y axis	
		//Yv = (Yv*5 + read(YvPin)*3) /8;	
		Yv = read(YvPin);
		//report.y = (Yv0 - Yv);
		report.y = ((Yv < Yv0 - 2) || (Yv > Yv0 + 2))? (Yv0 - Yv) * (3*abs(Yv0 - Yv)/2 + 37) / 80: 0;
		
		//accelerometer X axis
		//TODO: get this shit working
		
		//accelerometer Y axis
		//TODO: get this shit working also
			
		//buttons
		if(!(PIND & (1<<PORTD2)))
			report.buttons |= (1<<BUTTON0);
		if(!(PIND & (1<<PORTD3)))
			report.buttons |= (1<<BUTTON1);
					
		write(report);
		nullReport.buttons = report.buttons;
		write(nullReport);
    }
}