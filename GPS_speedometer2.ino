/* 
 * Arduino GPS clock with local time using NEO-6M module and 20x4 LCD.
 * This is a free software with NO WARRANTY.
 * https://simple-circuit.com/
 * https://simple-circuit.com/arduino-gps-clock-local-time-neo-6m/
 * http://www.instructables.com/id/Custom-Large-Font-For-16x2-LCDs/
 * small changes by Nicu FLORICA (niq_ro) from https://nicuflorica.blogspot.com/
 * ver.2.0 - added local zone adjust for clock
*/
 
 #include <EEPROM.h>
#include <TinyGPS++.h>        // include TinyGPS++ library
#include <TimeLib.h>          // include Arduino time library
#include <SoftwareSerial.h>   // include software serial library
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// Set the LCD address to 0x27 for a 20 chars and 4 line display
LiquidCrystal_I2C lcd(0x27, 20, 4);
 
TinyGPSPlus gps;
 
#define S_RX    4   // define software serial RX pin
#define S_TX    3   // define software serial TX pin
 
SoftwareSerial SoftSerial(S_RX, S_TX);   // configure SoftSerial library
 
 
#//define time_offset   7200  // define a clock offset of 3600 seconds (1 hour) ==> UTC + 1
int timezone = 12; 
long time_offset = 0;
byte format12 = 0;

float diferenta[38] = {-12., -11.,-10.,-9.5,-9.,-8.,-7.,-6.,-5.,-4.,-3.5,-3.,-2.,-1.,0,
                      1.,2.,3.,3.5,4.,4.5,5.,5.5,5.75,6.,6.5,7.,8.,8.75,9.,9.5,10.,10.5,
                      11.,12.,12.75,13.,14};   // added manualy by niq_ro
                      

#define sw1 12  // pin for increase the time zone
#define sw2 11 // pin for decrease the time zone
#define adresa  200  // adress for store the
byte zero = 0;  // variable for control the initial read/write the eeprom memory


// variable definitions
//char Taim[]  = "TIME: 00:00:00";
//char Date[]  = "DATE: 00-00-2000";
char Taim[]  = "00:00:00";
char Date[]  = " 00/00/00";

byte last_second, Second, Minute, Hour, Day, Month;
int Year;

int spid;
int spid0 = 123;
double latit;
double longit;
byte sat;
byte x = 0;
byte y = 0;

float diferenta0, diferenta1;
int diferenta2;

byte starebuton1,starebuton2;             // the current reading from the input pin
byte citirestarebuton1, citirestarebuton2;   // 
byte ultimastarebuton1 = HIGH;   // the previous reading from the input pin
byte ultimastarebuton2 = HIGH;
unsigned long ultimtpdebounce1 = 0;  // the last time the output pin was toggled
unsigned long ultimtpdebounce2 = 0;
unsigned long tpdebounce = 50;    // the debounce time; increase if the output flickers
unsigned long tpscriere = 10000;   // put data in EEPROM after last push with some delay
byte memorare1 = 1;
byte memorare2 = 1;

// the 8 arrays that form each segment of the custom numbers
byte LT[8] = 
{
  B00111,
  B01111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
byte UB[8] =
{
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};
byte RT[8] =
{
  B11100,
  B11110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
byte LL[8] =
{
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B01111,
  B00111
};
byte LB[8] =
{
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};
byte LR[8] =
{
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11110,
  B11100
};
byte UMB[8] =
{
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111
};
byte LMB[8] =
{
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};

 
void setup(void)
{
  SoftSerial.begin(9600);  // initialize software serial at 9600 baud

  // set up the LCD's number of columns and rows
  lcd.begin(); // initialize the LCD
  // assignes each segment a write number
  lcd.createChar(8,LT);
  lcd.createChar(1,UB);
  lcd.createChar(2,RT);
  lcd.createChar(3,LL);
  lcd.createChar(4,LB);
  lcd.createChar(5,LR);
  lcd.createChar(6,UMB);
  lcd.createChar(7,LMB);
  lcd.clear(); // clear the screen
  lcd.backlight(); // Turn on the blacklight and print a message.

  lcd.setCursor(0, 0); // move LCD cursor to column 1, row 0 [upper left position (0, 0)]
  lcd.print("  GPS speedometer   ");
  lcd.setCursor(6, 1);
  lcd.print("ver. 2.0");
  lcd.setCursor(0, 2);
  lcd.print("  sketch by niq_ro  ");
  lcd.setCursor(0, 3);
  lcd.print("   (Nicu FLORICA)   ");  

/*
pinMode(sw1, INPUT);
pinMode(sw2, INPUT);
digitalWrite(sw1, HIGH);
digitalWrite(sw2, HIGH);  
*/
pinMode(sw1,INPUT_PULLUP);
pinMode(sw2,INPUT_PULLUP);

zero = EEPROM.read(adresa - 1); // variable for write initial values in EEPROM
if (zero != 16)
{
EEPROM.update(adresa - 1, 16);  // zero
EEPROM.update(adresa, 16); // time zone (0...37) // https://en.wikipedia.org/wiki/Coordinated_Universal_Time
} 
// read EEPROM memory;
timezone = EEPROM.read(adresa);  // timezone +12
time_offset = (float)diferenta[timezone]*3600;  // convert in seconds
  delay(5000);
  lcd.clear();
} // end setup

void custom0O()
{ // uses segments to build the number 0
  lcd.setCursor(x,y); 
  lcd.write(8);  
  lcd.write(1); 
  lcd.write(2);
  lcd.setCursor(x,y+1); 
  lcd.write(3);  
  lcd.write(4);  
  lcd.write(5);
}

void custom1()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(2);
  lcd.setCursor(x+1,y+1);
  lcd.write(255);
}

void custom2()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(8);
  lcd.write(7);
  lcd.write(4);
}

void custom3()
{
  lcd.setCursor(x,y);
  lcd.write(6);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5); 
}

void custom4()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(4);
  lcd.write(255);
  lcd.setCursor(x,y+1);
  lcd.write(1);
  lcd.write(1);
  lcd.write(255);
}

void custom5()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(1);
  lcd.setCursor(x,y+1);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5);
}

void custom6()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x,y+1);
  lcd.write(3);
  lcd.write(7);
  lcd.write(5);
}

void custom7()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(1);
  lcd.write(2);
  lcd.setCursor(x+1,y+1);
  lcd.write(8);
}

void custom8()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(3);
  lcd.write(7);
  lcd.write(5);
}

void custom9()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(4);
  lcd.write(7);
  lcd.write(5);
}

void customA()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(255);
}

void customB()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(5);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(7);
  lcd.write(2);
}

void customC()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(1);
  lcd.write(1);
  lcd.setCursor(x,y+1);
  lcd.write(3);
  lcd.write(4);
  lcd.write(4);
}

void customD()
{
  lcd.setCursor(x,y); 
  lcd.write(255);  
  lcd.write(1); 
  lcd.write(2);
  lcd.setCursor(x,y+1); 
  lcd.write(255);  
  lcd.write(4);  
  lcd.write(5);
}

void customE()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(7);
  lcd.write(7); 
}

void customF()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x,y);
  lcd.write(255);
}

void customG()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(1);
  lcd.write(1);
  lcd.setCursor(x,y+1);
  lcd.write(3);
  lcd.write(4);
  lcd.write(2);
}

void customH()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(4);
  lcd.write(255);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(255); 
}

void customI()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(255);
  lcd.write(1);
  lcd.setCursor(x,y+1);
  lcd.write(4);
  lcd.write(255);
  lcd.write(4);
}

void customJ()
{
  lcd.setCursor(x+2,y);
  lcd.write(255);
  lcd.setCursor(x,y+1);
  lcd.write(4);
  lcd.write(4);
  lcd.write(5);
}

void customK()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(4);
  lcd.write(5);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(2); 
}

void customL()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(4);
  lcd.write(4);
}

void customM()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(3);
  lcd.write(5);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(254);
  lcd.write(255);
}

void customN()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(2);
  lcd.write(254);
  lcd.write(255);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(3);
  lcd.write(5);
}

void customP()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(255);
}

void customQ()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(1);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(3);
  lcd.write(4);
  lcd.write(255);
  lcd.write(4);
}

void customR()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(255);
  lcd.write(254);
  lcd.write(2); 
}

void customS()
{
  lcd.setCursor(x,y);
  lcd.write(8);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x,y+1);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5);
}

void customT()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(255);
  lcd.write(1);
  lcd.setCursor(x,y+1);
  lcd.write(254);
  lcd.write(255);
}

void customU()
{
  lcd.setCursor(x,y); 
  lcd.write(255);  
  lcd.write(254); 
  lcd.write(255);
  lcd.setCursor(x,y+1); 
  lcd.write(3);  
  lcd.write(4);  
  lcd.write(5);
}

void customV()
{
  lcd.setCursor(x,y); 
  lcd.write(3);  
  lcd.write(254);
  lcd.write(254); 
  lcd.write(5);
  lcd.setCursor(x+1,y+1); 
  lcd.write(2);  
  lcd.write(8);
}

void customW()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.write(254);
  lcd.write(254);
  lcd.write(255);
  lcd.setCursor(x,y+1);
  lcd.write(3);
  lcd.write(8);
  lcd.write(2);
  lcd.write(5);
}

void customX()
{
  lcd.setCursor(x,y);
  lcd.write(3);
  lcd.write(4);
  lcd.write(5);
  lcd.setCursor(x,y+1);
  lcd.write(8);
  lcd.write(254);
  lcd.write(2); 
}

void customY()
{
  lcd.setCursor(x,y);
  lcd.write(3);
  lcd.write(4);
  lcd.write(5);
  lcd.setCursor(x+1,y+1);
  lcd.write(255);
}

void customZ()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(6);
  lcd.write(5);
  lcd.setCursor(x,y+1);
  lcd.write(8);
  lcd.write(7);
  lcd.write(4);
}

void customqm()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x,y+1);
  lcd.write(254);
  lcd.write(7);
}

void customsm()
{
  lcd.setCursor(x,y);
  lcd.write(255);
  lcd.setCursor(x,y+1);
  lcd.write(7);
}

void customplus()  // made by niq_ro
{
  lcd.setCursor(x,y);
  lcd.write(4);
  lcd.write(255);
  lcd.write(4);
  lcd.setCursor(x,y+1);
  lcd.write(1);
  lcd.write(255);
  lcd.write(1);
}

void customminus() // made by niq_ro
{
  lcd.setCursor(x,y);
  lcd.write(4);
  lcd.write(4);
  lcd.write(4);
  lcd.setCursor(x,y+1);
  lcd.write(1);
  lcd.write(1);
  lcd.write(1);
}

void customgrad()
{
  lcd.setCursor(x,y);
  lcd.write(1);
  lcd.write(8);
  lcd.write(1);
  lcd.write(1);
  lcd.setCursor(x,y+1);
  lcd.write(254);
  lcd.write(3);
  lcd.write(4);
  lcd.write(4);
}

void spatiu()
{
  lcd.setCursor(x,y);
  lcd.print("    ");
  lcd.setCursor(x,y+1);
  lcd.print("    ");
}

// subrutina de afisare a numerelor
void afisare(int numar)
{
  switch (numar)
  {
    case 0:
    custom0O();
    break;

    case 1:
    custom1();
    break;

    case 2:
    custom2();
    break;

    case 3:
    custom3();
    break;

    case 4:
    custom4();
    break;

    case 5:
    custom5();
    break;

    case 6:
    custom6();
    break;

    case 7:
    custom7();
    break;

    case 8:
    custom8();
    break;

    case 9:
    custom9();
    break;
  }
}

void viteza(int spid2)
{
lcd.setCursor(x,y);  // move cursor to column 0 row 0
lcd.print("           "); 
lcd.setCursor(x,y+1);  // move cursor to column 0 row 1
lcd.print("           ");
 
if (spid2 >= 100)
{ 
  x = 0;
  afisare(int(spid2/100));
  
  x = x + 4;
  afisare(int(int(spid2%100)/10));
  
  x = x + 4;
  afisare(int(int(spid2%100)%10));
}

if (spid2 > 10 and spid2 < 100)
{ 
  x = 0;
  spatiu();
   
  x = x + 4;
  afisare(int(int(spid2%100)/10));
  
  x = x + 4;
  afisare(int(int(spid2%100)%10));
}

if (spid2 < 10)
{ 
/*
  x = 0;
  spatiu();
  
  x = x + 4;
  spatiu();
*/
         lcd.setCursor(x,y);  // move cursor to column 0 row 0
         lcd.print("(km/h)  "); 
  
  x = x + 8;
  afisare(int(int(spid2%100)%10));
}
}


 
void loop()
{

citirestarebuton1 = digitalRead(sw1);  // read the state of the switch into a local variable: 
  if (citirestarebuton1 != ultimastarebuton1) // If the switch changed, due to noise or pressing:
  {
    ultimtpdebounce1 = millis();  // reset the debouncing timer
  }
  if ((millis() - ultimtpdebounce1) > tpdebounce) 
  {
    if (citirestarebuton1 != starebuton1) // if the button state has changed
    {
      starebuton1 = citirestarebuton1;         
      if (starebuton1 == LOW) // only toggle the LED if the new button state is LOW
      {
     //    Serial.println("sw1");
        timezone = timezone + 1;
        if (timezone > 37) timezone = 0;
        time_offset = (float)diferenta[timezone]*3600.;
        memorare1 = 0;
        memorare2 = 1;
        //lcd.clear();  
diferenta0 = diferenta[timezone];
if (diferenta0 < 0)
   diferenta1 = - diferenta0;
 else
   diferenta1 = diferenta0;
diferenta2 = 100 * diferenta1;
lcd.setCursor(11,3); // move cursor to column 11 row 3
if (diferenta1 < 10)  lcd.print(" ");
  lcd.print("UTC");     
if (diferenta0 > 0) lcd.print("+");
if (diferenta0 < 0) lcd.print("-");
if (diferenta0 != 0)
{
lcd.print(diferenta2 / 100);  
if (diferenta2 % 100 == 0) lcd.print("   ");
if (diferenta2 % 100 == 50) lcd.print(":30");
if (diferenta2 % 100 == 75) lcd.print(":45");
if (diferenta2 % 100 == 25) lcd.print(":15");
}
else lcd.print("     ");
      }
    }
  }

citirestarebuton2 = digitalRead(sw2);  // read the state of the switch into a local variable: 
  if (citirestarebuton2 != ultimastarebuton2) // If the switch changed, due to noise or pressing:
  {
    ultimtpdebounce2 = millis();  // reset the debouncing timer
  }
  if ((millis() - ultimtpdebounce2) > tpdebounce) 
  {
    if (citirestarebuton2 != starebuton2) // if the button state has changed
    {
      starebuton2 = citirestarebuton2;         
      if (starebuton2 == LOW) // only toggle the LED if the new button state is LOW
      {
     // Serial.println("sw2");
        timezone = timezone - 1;
        if (timezone < 0) timezone = 37;
        time_offset = (float)diferenta[timezone]*3600.;
        memorare2 = 0;
        memorare1 = 1;
        //lcd.clear(); 
        diferenta0 = diferenta[timezone];
if (diferenta0 < 0)
   diferenta1 = - diferenta0;
 else
   diferenta1 = diferenta0;
diferenta2 = 100 * diferenta1;
lcd.setCursor(11,3); // move cursor to column 11 row 3
if (diferenta1 < 10)  lcd.print(" ");
  lcd.print("UTC");     
if (diferenta0 > 0) lcd.print("+");
if (diferenta0 < 0) lcd.print("-");
if (diferenta0 != 0)
{
lcd.print(diferenta2 / 100);  
if (diferenta2 % 100 == 0) lcd.print("   ");
if (diferenta2 % 100 == 50) lcd.print(":30");
if (diferenta2 % 100 == 75) lcd.print(":45");
if (diferenta2 % 100 == 25) lcd.print(":15");
}
else lcd.print("     "); 
      }
    }
  }

ultimastarebuton1 = citirestarebuton1;
ultimastarebuton2 = citirestarebuton2;

  if (((millis() - ultimtpdebounce1) > tpscriere) and (memorare1 == 0))
  {
   EEPROM.update(adresa, timezone); // store in eeprom
   memorare1 = 1;
   lcd.setCursor(12,2);
   lcd.print("*");
   delay(500);
   lcd.setCursor(12,2);
   lcd.print(" ");
   lcd.setCursor(11,3); // move cursor to column 11 row 3
   lcd.print("         ");
  }

  if (((millis() - ultimtpdebounce2) > tpscriere) and (memorare2 == 0))
  {
   EEPROM.update(adresa, timezone); // store in eeprom
   memorare2 = 1;
   lcd.setCursor(12,2);
   lcd.print("*");
   delay(500);
   lcd.setCursor(12,2);
   lcd.print(" ");
   lcd.setCursor(11,3); // move cursor to column 11 row 3
   lcd.print("         ");
  }
 
  while (SoftSerial.available() > 0)
  {
    if (gps.encode(SoftSerial.read()))
    {
      // get time from GPS module
      if (gps.time.isValid())
      {
        Minute = gps.time.minute();
        Second = gps.time.second();
        Hour   = gps.time.hour();
      }
 
      // get date from GPS module
      if (gps.date.isValid())
      {
        Day   = gps.date.day();
        Month = gps.date.month();
        Year  = gps.date.year();
      }

      // get speed from GPS module
      if (gps.speed.isValid())
      {
        spid  = gps.speed.kmph();
      }

      // get location from GPS module
      if (gps.location.isValid())
      {
       latit =  gps.location.lat();
       longit = gps.location.lng();
      }

      // get received sattelites from GPS module
      if (gps.satellites.isValid())
      {
       sat =  gps.satellites.value();
      }

      if(last_second != gps.time.second())  // if time has changed
      {
        last_second = gps.time.second();
 
        // set current UTC time
        setTime(Hour, Minute, Second, Day, Month, Year);
        // add the offset to get local time
        adjustTime(time_offset);
 
        // update time array
        Taim[6] = second() / 10 + '0';
        Taim[7] = second() % 10 + '0';
        Taim[3]  = minute() / 10 + '0';
        Taim[4] = minute() % 10 + '0';
        Taim[0]  = hour()   / 10 + '0';
        Taim[1]  = hour()   % 10 + '0';
 
        // update date array
        Date[7] = (year()  / 10) % 10 + '0';
        Date[8] =  year()  % 10 + '0';
        Date[4]  =  month() / 10 + '0';
        Date[5] =  month() % 10 + '0';
        Date[1]  =  day()   / 10 + '0';
        Date[2]  =  day()   % 10 + '0';
 
        // print time & date
        //lcd.setCursor(11, 3);  // move cursor to column 11, row 3
        lcd.setCursor(12, 0);     // move cursor to column 12 row 0
        lcd.print(Taim);         // print time (HH:MM:SS)
        lcd.setCursor(11, 1);     // move cursor to column 11 row 1
        if ((millis()/1000)%12 < 3)
        print_wday1(weekday());   // print day of the week
        else
        if (((millis()/1000)%9 >= 6) and ((millis()/1000)%12 <= 8))
        print_wday2(weekday());
        else
        lcd.print(Date);         // print date (DD-MM-YY)

        x = 0;
        y = 0;

    //spid = random(120);
    if (spid != spid0) viteza(spid);

    spid0 = spid;
 if (latit != 0.)
 {
 lcd.setCursor(0,2); // move cursor to column 0 row 2
 lcd.print("LAT:");
 lcd.print(latit,6);
 lcd.write(0b11011111);
 lcd.setCursor(0,3); // move cursor to column 0 row 3
 lcd.print("LON:");
 lcd.print(longit,6);
 lcd.write(0b11011111);
 }
 if (sat != 0)
 {
 lcd.setCursor(15,3); // move cursor to column 10 row 3
 if (sat < 10)  lcd.print(" ");
 lcd.print(sat);
 lcd.print("sat");
 }
      }
    } 
  }
 
}  // end main loop
 
// function for displaying day of the week
void print_wday1(byte wday)
{
  switch(wday)
  {
    case 1:  lcd.print(" DUMINICA");   break;
    case 2:  lcd.print(" LUNI    ");   break;
    case 3:  lcd.print(" MARTI   ");   break;
    case 4:  lcd.print(" MIERCURI");   break;
    case 5:  lcd.print(" JOI     ");   break;
    case 6:  lcd.print(" VINERI  ");   break;
    default: lcd.print(" SAMBATA ");
  }
}

void print_wday2(byte wday)
{
  switch(wday)
  {
    case 1:  lcd.print(" SUNDAY   ");   break;
    case 2:  lcd.print(" MONDAY   ");   break;
    case 3:  lcd.print(" TUESDAY  ");   break;
    case 4:  lcd.print("WEDNESDAY ");   break;
    case 5:  lcd.print(" THURSDAY ");   break;
    case 6:  lcd.print(" FRIDAY   ");   break;
    default: lcd.print(" SATURDAY ");
  }
}
// end of code.
