#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include "EasyNextionLibrary.h"
#include <string.h>
#include <math.h>
#include <PushButton.h>

//Definitions (For button Pins)

#define BUTTON_PIN_RIGHT 4
#define BUTTON_PIN_OK 6
#define BUTTON_PIN_LEFT 2

//Objects from other libraries

Pushbutton R_Button(BUTTON_PIN_RIGHT);
Pushbutton OK_button(BUTTON_PIN_OK);
Pushbutton L_Button(BUTTON_PIN_LEFT);
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
int current_button_id = 0;

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

void right_btn_press();
void left_btn_press();
void set_first_button();
void OK_btn_press();

//--------------------------------------------------------------------------

void setup(void) {
    Serial.begin(9600);
    myNex.begin(9600);
    while(!Serial){}  // Awaiting Serial (Screen, fake monitor) Connection
    nexCommand("page Startup");
    for(int i = 0; i < 100; i++){
        myNex.writeNum("j0.val", i);
        _delay_ms(20);
    }
    nexCommand("page title");
    set_first_button();
}

void loop(void) {
    myNex.NextionListen();
    if(R_Button.getSingleDebouncedPress()){
        right_btn_press();
    }
    if(OK_button.getSingleDebouncedPress()){
        OK_btn_press();
    }
    if(L_Button.getSingleDebouncedPress()){
        left_btn_press();
    }
}

void trigger0(){  // On Color->Resistor page, shift color of selected band up
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

void trigger1(){  // On Color->Resistor page, shift color of selected band down
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

void trigger2(){  // On Color->Resistor page, select next band
  BandAmount = myNex.readNumber("bandNumber.val");
  chooseBand = (chooseBand + 1)%BandAmount;
  myNex.writeNum("CurrBand.val", chooseBand+1);
}

void trigger3(){  // On Color->Resistor page, select previous band
  BandAmount = myNex.readNumber("bandNumber.val");
  chooseBand = (chooseBand + BandAmount - 1)%BandAmount;
  myNex.writeNum("CurrBand.val", chooseBand+1);
}

void trigger4(){  // Triggered when switching pages - set current band to the first one, sets first button as selected
    chooseBand = 0;
    set_first_button();
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

String get_temperature_coefficient(int code) {  // Helper functon for temperature coefficients
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

long MultiplyPower10(int code){  // Helper function for multiplications
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

String get_tolerance(int code) {  // Helper function for tolerances
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

void right_btn_press(){  // Press Right arrow button -> Move to button that is to the right/next row
    int buttons_on_page = myNex.readNumber("buttonsAmount.val");
    
    if(buttons_on_page){
        myNex.writeNum(String("bt" + String(current_button_id) + ".bco"), 54970);
        current_button_id = (current_button_id + 1)%buttons_on_page;
        myNex.writeNum(String("bt" + String(current_button_id) + ".bco"), 35953);
    }
}

void left_btn_press(){  // Press Left arrow button -> Move to button that is to the left/previous row
    int buttons_on_page = myNex.readNumber("buttonsAmount.val");
    if(buttons_on_page){
        myNex.writeNum(String("bt" + String(current_button_id) + ".bco"), 54970);
        current_button_id = (current_button_id + (buttons_on_page) - 1)%buttons_on_page;
        myNex.writeNum(String("bt" + String(current_button_id) + ".bco"), 35953);
    }
}

void set_first_button(){  // Set the first button as active/selected when switching pages
    int buttons_on_page = myNex.readNumber("buttonsAmount.val");
    if(buttons_on_page){
        current_button_id = 0;
        myNex.writeNum("bt0.bco",35953);
    }
}

void OK_btn_press(){  // Press OK Button -> Trigger Press/Release events of current buttons
    int buttons_on_page = myNex.readNumber("buttonsAmount.val");
    if(buttons_on_page){
        nexCommand(String("click bt"+String(current_button_id)+",1"));
        nexCommand(String("click bt"+String(current_button_id)+",0"));
    }
}
