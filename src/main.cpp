#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include "EasyNextionLibrary.h"
#include <string.h>
#include <math.h>
#include <PushButton.h>
#include <vector.h>

//Definitions

#define BUTTON_PIN_RIGHT 4
#define BUTTON_PIN_LEFT 6

//Objects

Pushbutton R_Button(BUTTON_PIN_RIGHT);
Pushbutton L_Button(BUTTON_PIN_LEFT);
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);
EasyNex myNex(Serial);

//Variables

long clrVal = 0;
int chooseBand = 0;
const char* selectBand[6] = {"va0.val", "va1.val", "va2.val", "va3.val", "va4.val", "va5.val"};
const char* picBand[3] = {"picValB3.val", "picValB4.val", "picValB5.val"};
long Colors[10] = {0, 35200, 63488, 64736, 61408, 2016, 31, 49183, 29614, 65535};
int BandAmount = 6;
long CurrRes = 0;
int picVal = 0;
String picBandCurr;
boolean GSflag = false;

// Function prototypes

int getColorIndex(long color, long ColorArray[]);
uint32_t getCurrentRes();
String getTolerance();
String get_tolerance(int code);
String get_temperature_coefficient(int code);
String getTempCoeff();
long MultiplyPower10(int code);
void nexCommand(String s);

void trigger2();
void trigger3();

void setup(void) {
    Serial.begin(9600);
    myNex.begin(9600);
    nexCommand("page Startup");
    for(int i = 0; i < 100; i++){
        myNex.writeNum("j0.val", i);
        _delay_ms(20);
    }
    nexCommand("page title");
}

void loop(void) {
    myNex.NextionListen();
    if(R_Button.getSingleDebouncedPress()){
        trigger2();
    }
    if(L_Button.getSingleDebouncedPress()){
        trigger3();
    }
}

void trigger0(){  // Color Shift Up
    BandAmount = myNex.readNumber("bandNumber.val");
    clrVal = myNex.readNumber(selectBand[chooseBand]);
    if((((BandAmount == 6) || (BandAmount == 5)) && ((chooseBand == 3) || (chooseBand == 4))) || ((BandAmount == 4) && ((chooseBand == 2) || (chooseBand == 3)))){
        picBandCurr = picBand[chooseBand - 2];
        picVal = myNex.readNumber(picBandCurr);
        if((getColorIndex(clrVal, Colors) == 9) || GSflag){
            if(picVal !=2){
                myNex.writeNum(picBandCurr, picVal+1);
                GSflag = true;
            }else{
                myNex.writeNum(picBandCurr, 0);
                clrVal = Colors[0];
                GSflag = false; 
            }
        }else{
            clrVal = Colors[(getColorIndex(clrVal, Colors) + 1)%10];
        }
    }else{
        clrVal = Colors[(getColorIndex(clrVal, Colors) + 1)%10];
    }
    myNex.writeNum(selectBand[chooseBand], clrVal);
    myNex.writeNum("resCalc.val", getCurrentRes());
    myNex.writeStr("t1.txt", getTolerance());
    myNex.writeStr("t4.txt", getTempCoeff());
}

void trigger1(){  // Color Shift Down
    BandAmount = myNex.readNumber("bandNumber.val");
    clrVal = myNex.readNumber(selectBand[chooseBand]);
    if((((BandAmount == 6) || (BandAmount == 5)) && ((chooseBand == 3) || (chooseBand == 4))) || ((BandAmount == 4) && ((chooseBand == 2) || (chooseBand == 3)))){
        picBandCurr = picBand[chooseBand - 2];
        picVal = myNex.readNumber(picBandCurr);
        if((getColorIndex(clrVal, Colors) == 0) || GSflag){
            if(picVal !=1){
                myNex.writeNum(picBandCurr, (picVal+2)%3);
                GSflag = true;
            }else{
                myNex.writeNum(picBandCurr, 0);
                clrVal = Colors[9]; 
                GSflag = false;
            }
        }else{
            clrVal = Colors[(getColorIndex(clrVal, Colors) + 9)%10];
        }
    }else{
        clrVal = Colors[(getColorIndex(clrVal, Colors) + 9)%10];
    }
    myNex.writeNum(selectBand[chooseBand], clrVal);
    myNex.writeNum("resCalc.val", getCurrentRes());
    myNex.writeStr("t1.txt", getTolerance());
    myNex.writeStr("t4.txt", getTempCoeff());    
}

void trigger2(){  // Next Band
  BandAmount = myNex.readNumber("bandNumber.val");
  chooseBand = (chooseBand + 1)%BandAmount;
  myNex.writeNum("CurrBand.val", chooseBand+1);
}

void trigger3(){  // Previous Band
  BandAmount = myNex.readNumber("bandNumber.val");
  chooseBand = (chooseBand + BandAmount - 1)%BandAmount;
  myNex.writeNum("CurrBand.val", chooseBand+1);
}

void trigger4(){  // Reset Bands while decreasing Band amount
  chooseBand = 0;
}

void trigger5(){  // ...I'm not sure?
    myNex.writeStr("page choice");
}

void trigger6(){    // ...I'm not sure?
    myNex.writeStr("page SixBands");
    trigger4();
}

void trigger7(){    // ...I'm not sure?
    myNex.writeStr("page measurement");
}

int getColorIndex(long color, long ColorArray[10]){  // Easy scan of Color array to get number from color
    for(int i = 0; i < 10; i++){
        if(ColorArray[i] == color){
            return i;
        }
    }
    return 0;
}

uint32_t getCurrentRes(){  // Resistance calculations, doesn't account for gold/silver in multiplication
    BandAmount = myNex.readNumber("bandNumber.val");
    if(BandAmount == 4){
        return (getColorIndex(myNex.readNumber("va0.val"), Colors)*10 + getColorIndex(myNex.readNumber("va1.val"), Colors)) * MultiplyPower10(getColorIndex(myNex.readNumber("va2.val"), Colors));
    }
    else{
        return (getColorIndex(myNex.readNumber("va0.val"), Colors)*100 + getColorIndex(myNex.readNumber("va1.val"), Colors)*10 + getColorIndex(myNex.readNumber("va2.val"), Colors)) * MultiplyPower10(getColorIndex(myNex.readNumber("va3.val"), Colors));
    }
}

String get_temperature_coefficient(int code) {
    switch(code) {
        case 0: return "250 ppm/K";
        case 1: return "100 ppm/K";
        case 2: return "50 ppm/K";
        case 3: return "15 ppm/K";
        case 4: return "25 ppm/K";
        case 5: return "20 ppm/K";
        case 6: return "10 ppm/K";
        case 7: return "5 ppm/K";
        case 8: return "1 ppm/K";
        default: return "unknown";
    }
}

long MultiplyPower10(int code){
    switch(code) {
        case 0: return 1;
        case 1: return 10;
        case 2: return 100;
        case 3: return 1000;
        case 4: return 10000;
        case 5: return 100000;
        case 6: return 1000000;
        case 7: return 10000000;
        case 8: return 100000000;
        case 9: return 1000000000;
        default: return 0;
    }
}

String get_tolerance(int code) {
    switch(code) {
        case 1: return "+-1%";
        case 2: return "+-2%";
        case 3: return "+-0.05%";
        case 4: return "+-0.02%";
        case 5: return "+-0.5%";
        case 6: return "+-0.25%";
        case 7: return "+-0.1%";
        case 8: return "+-0.01%";
        default: return "unknown";
    }
}

String getTolerance(){  // Final function for tolerance calculation
    int gscheck;
    BandAmount = myNex.readNumber("bandNumber.val");
    if(BandAmount == 4){
        gscheck = myNex.readNumber("picValB4.val");
        if(gscheck == 1){
            return "+-5%";
        }else if (gscheck == 2){
           return "+-10%";
        }
        return get_tolerance(getColorIndex(myNex.readNumber("va3.val"), Colors));
    }
    else{
        gscheck = myNex.readNumber("picValB5.val");
        if(gscheck == 1){
            return "+-5%";
        }else if (gscheck == 2){
           return "+-10%";
        }
        return get_tolerance(getColorIndex(myNex.readNumber("va4.val"), Colors));
    }
}

String getTempCoeff(){  // Final function for TempCoeff calculation
    BandAmount = myNex.readNumber("bandNumber.val");
    if(BandAmount == 6){
        return get_temperature_coefficient(getColorIndex(myNex.readNumber("va5.val"), Colors));
    }
    else{
        return "unknown";
    }
}

void nexCommand(String s){  // Sending commands to nextion, without having to bulk program
    Serial.print(s);
    Serial.print(char(255));
    Serial.print(char(255));
    Serial.print(char(255));
}