/*
 * GloveMouse.c
 * v2.2
 * 
 * Designed for use with java userspace driver
 * 
 * Created: 21.04.2013 23:30:49
 *  Author: Tudor
 */

//CPU frequency 
#define F_CPU 16000000

//ADC channels
#define CHANNEL_X				7
#define CHANNEL_Y				6
#define CHANNEL_BUTTON_LEFT		10
#define CHANNEL_BUTTON_RIGHT	9
#define CHANNEL_BUTTON_PAUSE	8

//sensitivity select button
#define BUTTON_SENS_PORT		PORTB
#define BUTTONS_SENS_PIN		PORTB4

//leds
#define LED_PORT				PORTF
#define LED_1_PIN				PORTF1
#define LED_2_PIN				PORTF0

//buttons
#define CLICK_LEFT			0
#define CLICK_RIGHT			1
#define CLICK_SCROLL		2


#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>


//mouse report structure
typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t wheel;	
} mouseReport;


//initialize USART
void initUSART() {
    UBRR1 = 8;								//baud rate 115.2k
	UCSR1A = 0;
	UCSR1B = (1<<TXEN1);					//enable transmitter
	UCSR1C = (1<<UCSZ10) | (1<<UCSZ11);		//8-bit character size
}

//initialize pins
void initPins() {
	BUTTON_SENS_PORT &= ~(1<<BUTTONS_SENS_PIN);	//set sensitivity select button as input
	LED_PORT |= (1<<LED_1_PIN) | (1<<LED_2_PIN);	//set led pins as output
}

//initialize ADC
void initADC() {
	ADMUX = (1<<REFS0) | (1<<ADLAR);							//set AVCC as reference; left-adjust the result
	ADCSRA = (1<<ADEN) | (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2);	//enable ADC; set prescaler to 128
	DIDR0 = (1<<ADC6D) | (1<<ADC7D);							//disable digital input buffers to reduce noise
	DIDR2 = (1<<ADC8D) | (1<<ADC9D) | (1<<ADC10D);
}

//make ADC conversion on channel channel
uint8_t read(uint8_t channel) {
	//erase previous channel
	ADMUX &= ~((1<<MUX4) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (1<<MUX0));
	ADCSRB &= ~(1<<MUX5);	
	
	//set conversion channel
	if(channel <= 7) {
		ADMUX |= channel;					
	} else {
		ADMUX |= channel - 8;
		ADCSRB |= (1<<MUX5);
	}
			
	ADCSRA |= (1<<ADSC);				//start conversion
	while(!(ADCSRA & (1<<ADIF)));		//wait for conversion to finish
	return ADCH;						//return the result
}

//mediate 16 ADC conversions on channel channel
uint8_t mediate(uint8_t channel) {
	uint16_t sum = 0, i;				
	for(i=0; i<16; ++i) {	
		sum += read(channel);			
	}
	return (uint8_t)(sum>>4);			
}

//transmits a mouseReport to the computer
void write(mouseReport report) {
	//send start sequence
	while (!(UCSR1A & (1<<UDRE1)));		
	UDR1 = 'T';							
	while (!(UCSR1A & (1<<UDRE1)));
	UDR1 = 'V';
	
	//send the actual report
	while (!(UCSR1A & (1<<UDRE1)));		
	UDR1 = report.buttons;
	while (!(UCSR1A & (1<<UDRE1)));		
	UDR1 = report.x;
	while (!(UCSR1A & (1<<UDRE1)));
	UDR1 = report.y;
	while (!(UCSR1A & (1<<UDRE1)));
	UDR1 = report.wheel;
	
	//delay because driver is shit
	_delay_ms(5);
}

//test function; moves the cursor in a square pattern
void test() {
	int ind;
	mouseReport nullReport = { 0, 0, 0, 0 };		//an empty report
	mouseReport report = { 0, 0, 0, 0};								
	
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

//main function for GloveMouse application
int main() {
	mouseReport nullReport = { 0, 0, 0, 0 };		//an empty report
	mouseReport report = { 0, 0, 0, 0 };			//useful report
	uint8_t Xv0, Yv0;								//speed references for both axes
	uint8_t Xv, Yv;									//current speeds
	uint8_t buttonLeft, buttonRight, buttonPause;	//the three touch buttons
	uint8_t pauseFlag = 0, buttonFlag = 0;			//status indicators
	uint8_t threshold = 100;						//button press threshold
	
	//initialization sequence
	initUSART();
	initADC();
	initPins();
	
	//delay to keep thing steady
	_delay_ms(1000);
	
	//calculate speed references
	Xv0 = mediate(CHANNEL_X);							
	Yv0 = mediate(CHANNEL_Y);
	Xv = Xv0; Yv = Yv0; 
	
	//do a test just to be sure
	test();	
				
	while(1) {
		//set all actions to 0
		report.buttons = 0;
		report.x = 0;
		report.y = 0;
		report.wheel = 0;
		
		//noise threshold is +/- 2
		//acceleration function is f(x) = (3/2 * abs(x) + 37) / 80 ... yeah, you heard me.  
		
		//gyro X axis
		Xv = read(CHANNEL_X);
		report.x = ((Xv < Xv0 - 2) || (Xv > Xv0 + 2))? (Xv0 - Xv) * (3*abs(Xv - Xv0)/2 + 37) / 80 : 0;	
		
		//gyro Y axis
		Yv = read(CHANNEL_Y);
		report.y = ((Yv < Yv0 - 2) || (Yv > Yv0 + 2))? (Yv0 - Yv) * (3*abs(Yv0 - Yv)/2 + 37) / 80 : 0;
		
		//TODO: sensitivity select button
		
		//left click
		buttonLeft = read(CHANNEL_BUTTON_LEFT);
		if(buttonLeft < threshold)
			report.buttons |= (1<<CLICK_LEFT);
		
		//right click
		buttonRight = read(CHANNEL_BUTTON_RIGHT);
		if(buttonRight < threshold)
			report.buttons |= (1<<CLICK_RIGHT);
		
		//pause button -> enter/exit pause mode
// 		buttonPause = read(CHANNEL_BUTTON_PAUSE);
// 		if(buttonPause < threshold && buttonFlag) {
// 			buttonFlag = 0;
// 			pauseFlag ^= 1;
// 			LED_PORT &= ~(1<<LED_1_PIN);
// 			LED_PORT &= ~(1<<LED_2_PIN);
// 		} else {
// 			buttonFlag = 1;
// 		}				
							
		//if in pause mode don't send anything
		if(pauseFlag) {
			nullReport.buttons = 0;
			write(nullReport);
		}
		//if in active mode send the new report
		else {
			write(report);
			nullReport.buttons = report.buttons;
			write(nullReport);
		}	
    } //while
} //main