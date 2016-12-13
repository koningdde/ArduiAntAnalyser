/***************************************************************************\
*  Name    : DDS_Sweeper.BAS                                                *
*  
*  Date    : 9/26/2013                                                      *
*  Version : 1.0                                                            *
*  Notes   : Written using for the Arduino Micro                            *
*          :   Pins:                                                        *
*          :    A0 - Reverse Detector Analog in                             *
*          :    A1 - Forward Detector Analog in                             *
\***************************************************************************/


//LCD
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

// Define Pins used to control AD9850 DDS
const int FQ_UD=10;
const int SDAT=11;
const int SCLK=9;
const int RESET=12;
const int START=8;

// Define variable
double Fin_MHz = 1.8;
double Fstart_MHz = 1000000;  // Start Frequency for sweep
double Fstop_MHz = 10;  // Stop Frequency for sweep
double current_freq_MHz; // Temp variable used during sweep
int fine = A3;
int grose = A2;
int finestore = 0;
int grosestore = 0;
int SWOPSTART;
int BAND = 1;
int start = 1;
long serial_input_number; // Used to build number from serial stream
int num_steps = 100; // Number of steps to use in the sweep
char incoming_char; // Character read from serial stream

// the setup routine runs once when you press reset:
void setup() {
  //LCD
    lcd.begin(16,40);   // initialize the lcd for 16 chars 2 lines, turn on backlight
    lcd.backlight();
    lcd.setCursor(0,0); //Start at character 4 on line 0
    lcd.print("Antenna");
    lcd.setCursor(0,1); //Start at character 4 on line 0
    lcd.print("Analyser");
    lcd.setCursor(0,2);
    lcd.print("0 - 40 Mhz");
    lcd.setCursor(0,3); //Start at character 4 on line 0
    lcd.print("PD1DDK");
    delay(2500);
    lcd.clear();
       
  // Configiure DDS control pins for digital output
  pinMode(FQ_UD,OUTPUT);
  pinMode(SCLK,OUTPUT);
  pinMode(SDAT,OUTPUT);
  pinMode(RESET,OUTPUT);
  pinMode(START,INPUT);
  
  // Configure LED pin for digital output
  pinMode(13,OUTPUT);

  // Set up analog inputs on A0 and A1, internal reference voltage
  pinMode(A0,INPUT);
  pinMode(A1,INPUT);
  pinMode(A2,INPUT);
  pinMode(A3,INPUT);
  //analogReference(INTERNAL);
  
  // initialize serial communication at 57600 baud
  Serial.begin(57600);

  // Reset the DDS
  digitalWrite(RESET,HIGH);
  digitalWrite(RESET,LOW);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Kies modus");
  lcd.setCursor(0,2);
  lcd.print("Links single");
  lcd.setCursor(0,3);
  lcd.print("Rechts sweep");

}

void loop() {

    finestore = analogRead(fine);
    if ((finestore < 400) && (start == 1)) {
      lcd.clear(); 
      lcd.setCursor(0,2);
      lcd.print("Druk toets voor");
      lcd.setCursor(0,3);
      lcd.print("wissel band");   
      start = 2;
      }
    if ((finestore > 600) && (start == 1)) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Koppel aan PC");
      lcd.setCursor(0,1);
      lcd.print("en gebruik");
      lcd.setCursor(0,2);
      lcd.print("antenne sweep");
      lcd.setCursor(0,3);
      lcd.print("programma");
      Fstart_MHz = 1;
      start = 3;
      } 
    if (start == 2) {single();}
    if (start == 3) {sweep();}

}

// the loop routine runs over and over again forever:
void single() {
      finestore = analogRead(fine);
      grosestore = analogRead(grose);
      SWOPSTART = digitalRead(START);
      if (SWOPSTART == LOW) {
          BAND = BAND + 1; 
          if (BAND > 9) {BAND = 1;}
     
      Fin_MHz = SWOP(BAND);
      }
      
      if (finestore < 400) {Fin_MHz = Fin_MHz + 0.01;}
      if (finestore > 600) {Fin_MHz = Fin_MHz - 0.01;}  
      if (grosestore < 400) {Fin_MHz = Fin_MHz + 1;}
      if (grosestore > 600) {Fin_MHz = Fin_MHz - 1;}
      if (Fin_MHz < 0) {Fin_MHz = 40;}
      if (Fin_MHz > 40) {Fin_MHz = 0;}
      //Turn frequency into FStart
      Fstart_MHz = Fin_MHz*1000000;
      SetDDSFreq(Fstart_MHz);
      delay(100);
        lcd.setCursor(0,0);
        lcd.print(Fin_MHz);
        lcd.print(" MHz     ");
        Perform_sweep();
      delay(500);
  } 

void Perform_sweep(){
  double FWD=0;
  double REV=0;
  double VSWR;
   
    // Read the forawrd and reverse voltages
    REV = analogRead(A0);
    FWD = analogRead(A1);
    if(REV>=FWD){
      // To avoid a divide by zero or negative VSWR then set to max 999
      VSWR = 999;
    }else{
      // Calculate VSWR
      VSWR = (FWD+REV)/(FWD-REV);
    }
    lcd.setCursor(0,1); //Start at character 4 on line 0
    lcd.print("SWR 1:");
    lcd.print(VSWR);
    lcd.print("     ");
       }
   
void SetDDSFreq(double Freq_Hz){
  // Calculate the DDS word - from AD9850 Datasheet
  int32_t f = Freq_Hz * 4294967295/125000000;
  // Send one byte at a time
  for (int b=0;b<4;b++,f>>=8){
    send_byte(f & 0xFF);
  }
  // 5th byte needs to be zeros
  send_byte(0);
  // Strobe the Update pin to tell DDS to use values
  digitalWrite(FQ_UD,HIGH);
  digitalWrite(FQ_UD,LOW);
}

void send_byte(byte data_to_send){
  // Bit bang the byte over the SPI bus
  for (int i=0; i<8; i++,data_to_send>>=1){
    // Set Data bit on output pin
    digitalWrite(SDAT,data_to_send & 0x01);
    // Strobe the clock pin
    digitalWrite(SCLK,HIGH);
    digitalWrite(SCLK,LOW);
  }
}

float SWOP(int BAND) {
  float MEM;
  switch (BAND) {
    case 1:    // your hand is on the sensor
      MEM = 1.88;
        lcd.clear();
        lcd.setCursor(0,0); //Start at character 4 on line 0
        lcd.print("160 meter");
        lcd.setCursor(0,1); //Start at character 4 on line 0
        lcd.print("band");
        lcd.setCursor(0,2);
        lcd.print("Druk toets voor");
        lcd.setCursor(0,3);
        lcd.print("wissel band");  
      break;
    case 2:    // your hand is on the sensor
      MEM = 3.6;
        lcd.clear();
        lcd.setCursor(0,0); //Start at character 4 on line 0
        lcd.print("80 meter");
        lcd.setCursor(0,1); //Start at character 4 on line 0
        lcd.print("band");
        lcd.setCursor(0,2);
        lcd.print("Druk toets voor");
        lcd.setCursor(0,3);
        lcd.print("wissel band");       
        break;
    case 3:    // your hand is on the sensor
      MEM = 7.1;
        lcd.clear();
        lcd.setCursor(0,0); //Start at character 4 on line 0
        lcd.print("40 meter");
        lcd.setCursor(0,1); //Start at character 4 on line 0
        lcd.print("band");
        lcd.setCursor(0,2);
        lcd.print("Druk toets voor");
        lcd.setCursor(0,3);
        lcd.print("wissel band");      
        break;
    case 4:    // your hand is on the sensor
      MEM = 10.125;
        lcd.clear();
        lcd.setCursor(0,0); //Start at character 4 on line 0
        lcd.print("30 meter");
        lcd.setCursor(0,1); //Start at character 4 on line 0
        lcd.print("band");   
        lcd.setCursor(0,2);
        lcd.print("Druk toets voor");
        lcd.setCursor(0,3);
        lcd.print("wissel band");  
        break;
    case 5:    // your hand is on the sensor
      MEM = 14.125;
        lcd.clear();
        lcd.setCursor(0,0); //Start at character 4 on line 0
        lcd.print("20 meter");
        lcd.setCursor(0,1); //Start at character 4 on line 0
        lcd.print("band");      
        lcd.setCursor(0,2);
        lcd.print("Druk toets voor");
        lcd.setCursor(0,3);
        lcd.print("wissel band"); 
        break;
    case 6:    // your hand is on the sensor
      MEM = 18.1;
        lcd.clear();
        lcd.setCursor(0,0); //Start at character 4 on line 0
        lcd.print("17 meter");
        lcd.setCursor(0,1); //Start at character 4 on line 0
        lcd.print("band");      
        lcd.setCursor(0,2);
        lcd.print("Druk toets voor");
        lcd.setCursor(0,3);
        lcd.print("wissel band");
        break;
    case 7:    // your hand is on the sensor
      MEM = 21.20;
        lcd.clear();
        lcd.setCursor(0,0); //Start at character 4 on line 0
        lcd.print("15 meter");
        lcd.setCursor(0,1); //Start at character 4 on line 0
        lcd.print("band");      
        lcd.setCursor(0,2);
        lcd.print("Druk toets voor");
        lcd.setCursor(0,3);
        lcd.print("wissel band");
        break;
    case 8:    // your hand is on the sensor
      MEM = 24.95;
        lcd.clear();
        lcd.setCursor(0,0); //Start at character 4 on line 0
        lcd.print("12 meter");
        lcd.setCursor(0,1); //Start at character 4 on line 0
        lcd.print("band");     
        lcd.setCursor(0,2);
        lcd.print("Druk toets voor");
        lcd.setCursor(0,3);
        lcd.print("wissel band");
        break;
    case 9:    // your hand is on the sensor
      MEM = 28.5;
        lcd.clear();
        lcd.setCursor(0,0); //Start at character 4 on line 0
        lcd.print("10 meter");
        lcd.setCursor(0,1); //Start at character 4 on line 0
        lcd.print("band");      
        lcd.setCursor(0,2);
        lcd.print("Druk toets voor");
        lcd.setCursor(0,3);
        lcd.print("wissel band");
        break;
  }
  delay(1000);
  return MEM;
  }

 void sweep() {
  //Check for character
  if(Serial.available()>0){
    incoming_char = Serial.read();
    switch(incoming_char){
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      serial_input_number=serial_input_number*10+(incoming_char-'0');
      break;
    case 'A':
      //Turn frequency into FStart
      Fstart_MHz = ((double)serial_input_number)/1000000;
      serial_input_number=0;
      break;
    case 'B':
      //Turn frequency into FStart
      Fstop_MHz = ((double)serial_input_number)/1000000;
      serial_input_number=0;
      break;
    case 'C':
      //Turn frequency into FStart and set DDS output to single frequency
      Fstart_MHz = ((double)serial_input_number)/1000000;
      SetDDSFreq(Fstart_MHz);
      serial_input_number=0;    
      break;
    case 'N':
      // Set number of steps in the sweep
      num_steps = serial_input_number;
      serial_input_number=0;
      break;
    case 'S':    
    case 's':    
      Perform_sweep2();
      break;
    case '?':
      // Report current configuration to PC    
      Serial.print("Start Freq:");
      Serial.println(Fstart_MHz*1000000);
      Serial.print("Stop Freq:");
      Serial.println(Fstart_MHz*1000000);
      Serial.print("Num Steps:");
      Serial.println(num_steps);
      break;
    }
    Serial.flush();     
  } 
}

void Perform_sweep2(){
  double FWD=0;
  double REV=0;
  double VSWR;
  double Fstep_MHz = (Fstop_MHz-Fstart_MHz)/num_steps;
 
  // Start loop 
  for(int i=0;i<=num_steps;i++){
    // Calculate current frequency
    current_freq_MHz = Fstart_MHz + i*Fstep_MHz;
    // Set DDS to current frequency
    SetDDSFreq(current_freq_MHz*1000000);
    // Wait a little for settling
    delay(10);
    // Read the forawrd and reverse voltages
    REV = analogRead(A0);
    FWD = analogRead(A1);
    if(REV>=FWD){
      // To avoid a divide by zero or negative VSWR then set to max 999
      VSWR = 999;
    }else{
      // Calculate VSWR
      VSWR = (FWD+REV)/(FWD-REV);
    }
    
    // Send current line back to PC over serial bus
    Serial.print(current_freq_MHz*1000000);
    Serial.print(",0,");
    Serial.print(int(VSWR*1000));
    Serial.print(",");
    Serial.print(FWD);
    Serial.print(",");
    Serial.println(REV);
  }
  // Send "End" to PC to indicate end of sweep
  Serial.println("End");
  Serial.flush();    
}


