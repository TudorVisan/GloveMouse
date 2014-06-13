uint8_t Xv, Yv, Xv0, Yv0, button0;
int8_t ceva = 0, da;

void startup() {
  Serial.println("\nHere we go...");
  Serial.print("3..."); delay(1000);
  Serial.print("2..."); delay(1000);
  Serial.println("1...\n"); delay(1000);
}

uint8_t read(uint8_t channel) {
  if(channel > 5) 
    return -1;
	
  ADMUX &= 0xE0;
  ADMUX |= channel;
  ADCSRA |= (1<<ADSC);
  while(! (ADCSRA & (1<<ADIF)));
  
  return (uint8_t)map((uint8_t)(ADC>>2), 0, 171, 0, 255);
}

uint8_t mediate(uint8_t pin) {
  uint16_t sum = 0, i;
  for(i=0; i<16; ++i) {
    sum += read(pin);
  }  
  
  return (uint8_t)(sum>>4);
}

void setup() {
  Serial.begin(9600);
  ADMUX = (1<<REFS0);
  Xv0 = mediate(0); Serial.print("Xv0 = "); Serial.println(Xv0);
  Xv = Xv0;
  Yv0 = mediate(1); Serial.print("Yv0 = "); Serial.println(Yv0);
  Yv = Yv0;
  button0 = mediate(4); Serial.print("button0 = "); Serial.println(button0);
 
  pinMode(3, INPUT);
  pinMode(2, INPUT);  
  PORTD |= (3<<PORTD2);
 
  startup();
}


//test accelerometru
/*void loop() {
  Xacc = (Xacc*3 + read(0))/4;
  Yacc = (Yacc*3 + read(1))/4;
  Xv = read(2);
  Yv = read(3);
  Serial.print("Xacc = "); Serial.print(Xacc); 
  if(!movementX) {
	ceva = Xacc - Xacc0;
	if(ceva) movementX = 1;
  } else {
      if((ceva + Xacc - Xacc0)*ceva <= 0) ceva = 0;
      else ceva += Xacc - Xacc0;
  }
		
  if(Xacc == Xacc0) ++countX;
  else countX = 0;
  if(countX == 7) {
    movementX = 0;					
    countX = 0;
    ceva = 0;
  }	
  Serial.print(" movement = "); if(movementX) Serial.print(Xacc - Xacc0); else Serial.print("none");		
  Serial.print(" ceva = "); Serial.println(ceva); 
  // Xacc0 = Xacc;
  //Serial.print("  |  Xv = "); Serial.print(Xv);
  //Serial.print("  |  Yv = "); Serial.println(Yv);
}*/


//test giroscop
void loop() {
  Xv = (Xv*5 + read(2)*3)/8;
  da = ((Xv < Xv0 - 2) || (Xv > Xv0 + 2))? (Xv - Xv0) * (3*abs(Xv - Xv0)/2 + 37) / 80 : 0;
  Serial.print("Xv_diff = "); Serial.print(da);
		
  Yv = (Yv*5 + read(3)*3)/8;
  da = ((Yv < Yv0 - 2) || (Yv > Yv0 + 2))? (Yv0 - Yv) * (3*abs(Yv0 - Yv)/2 + 37) / 80: 0;
  Serial.print("  |  Yv_diff = "); Serial.println(da);
}


//test butoane pe analogic
/*void loop() {
  //Serial.println(read(4));
  button0 = (button0*3 + read(4)) / 4; 
  Serial.println(button0);
  if(button0 < 5)
    Serial.println("Left is pressed");
  //Serial.println(read(5));
  //if(read(5) > 127)
    //Serial.println("Right is pressed");
 }*/
 
 
 //test butoane pe digital
 /*void loop() {
   if(!digitalRead(3))
     Serial.println("Right is pressed");
   if(!digitalRead(2))
     Serial.println("Left is pressed");
 }*/
