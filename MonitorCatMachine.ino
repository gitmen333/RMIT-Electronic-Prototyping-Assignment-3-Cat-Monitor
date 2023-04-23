//Servo
  #include <ESP32Servo.h>
  Servo myservo; // create servo object to control a servo
  int pos = 0; // variable to store the servo position
  #if defined(ARDUINO_ESP32S2_DEV)
  int servoPin = 15;
  #else
  int servoPin = 15;
  #endif



//Adafruit
  #include "AdafruitIO_WiFi.h"      //adafruit libraries
  #include "Adafruit_MQTT.h"
  #include "Adafruit_MQTT_Client.h"

  #define IO_USERNAME  "S3839132"     //adafruit username
  #define IO_KEY       "aio_TDwt16WqITC4p0vTDi0tud8pF7v3"   //adafruit key
  #define WIFI_SSID "TelstraE875C0"
  #define WIFI_PASS "btmt8m4xze"

  AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

  AdafruitIO_Feed *catmovementFeed = io.feed("catmovement");
  AdafruitIO_Feed *nocatFeed = io.feed("nocat");  
  AdafruitIO_Feed *nocatdisplayFeed = io.feed("nocatdisplay"); 
int CHN = 0;              //channel for PWM 
int FRQ = 1000;           //frequency for PWM
int PWM_BIT = 8;          //PWM bit rate

bool counterStarted = false;  //bool value to tell if the timer has started yet. 
unsigned long previousToyMillis = 0;         //stores the millis value at the time when the button was pressed to restart the timer. this is for the toy timer (how long the toy will wiggle for)
unsigned long previous24Millis = 0;           //stores the millis value at the time when to restart the timer. (cat moves) this is for the 24 hour timer
unsigned long previousadafruitMillis = 0;     //stores millis value when the adafruit was uploaded. 
unsigned long currentMillis = millis();   //current millis 
long wiggleToy = 5000;        //wiggle the toy for 20 seconds (set to 5 seconds for testing purposes)
long twentyfourhours = 20000;    //24 hour timer would be 8,640,000 ms I put in as 20 seonds for testing purposes
long adafruitdelay = 10000;       //10 seconds timer to make sure adafruit will only send once in at least 10 seconds to avoid flooding the network
bool wiggletimerstarted = false;    //tells the system is the wiggle timer has started or not
bool toycanwigglenow = false;     //tells the system if the toy is wiggling or not
bool movementtobesent = 0;        //bool value to tell system that movement data needs to be sent to adafruit
bool nocat = 0;                    //bool value to tell system if there is a cat
bool nocatsent = 0;               //bool value to tell the system if the message had already been sent to stop spam messages every 10 seconds
int motionDetector = 2;      //pin for the motion detector
bool motion;                  //bool value for motion being detected
int val = 0;                  //val is a raw value returned by the motion detector (0 or 1)

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  ledcSetup(CHN, FRQ, PWM_BIT); //setup pwm channel
   // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50); // standard 50 hz servo
  myservo.attach(servoPin, 500, 2450); // attaches the servo on pin 18 to the servo object
  io.connect();
}
 //Funtions: 
 
void detectmotion(){ // function for detecting motion
    val = digitalRead(motionDetector); // read input value from motion sensor
    if (val == HIGH) { // check if the input is HIGH
      motion = true;
      Serial.println("Motion detected!");  
     
    } else {
            //Serial.println("No Motion");
            motion = false;   //if there is no motion set motion back to false
       }
}

void wiggletoy(){
  myservo.write(30);              // tell servo to go to position ‘30’
    delay(250);                       // wait 500ms
    myservo.write(150);          // tell servo to go to position ‘150’
    delay(200);                       // wait 500ms
}

void startwiggletimer(){
  if (wiggletimerstarted == false){
    previousToyMillis = currentMillis;
    wiggletimerstarted = true;
  }
}

void sendmovementtoADAFRUIT(){
  if((currentMillis-previousadafruitMillis >= adafruitdelay) && (movementtobesent == 1)){
    Serial.println ("movement sent!");
    catmovementFeed->save(1);  
    movementtobesent = 0;
    previousadafruitMillis = currentMillis;
    if(nocat ==1){
    Serial.println("No cat detected in the past 24 hours...");
    }
  }
  else{}
  if((currentMillis-previousadafruitMillis >= adafruitdelay) && (motion == false)){
    catmovementFeed->save(0); 
    previousadafruitMillis = currentMillis;
    Serial.println ("no movement sent!");
      if(nocat ==1){
         Serial.println("No cat detected in the past 24 hours...");
        if((nocat == 1)&&(nocatsent == 0)){
          nocatdisplayFeed->save(1);
          nocatFeed->save(1);       //pulse the no cat signal for 10ms
          delay(2000);
          nocatFeed->save(0);
          nocatsent = 1;
        }
      }    
  }
  else{}
}

void twentyfourhourtimer(){
  if(currentMillis-previous24Millis >= twentyfourhours)
  nocat = 1;
}



void loop() {
  // put your main code here, to run repeatedly:
  currentMillis = millis();                         //sets currentMillis to the millis since the arduino started
  Serial.println (currentMillis-previousadafruitMillis);
  //Serial.println (currentMillis-previous24Millis);
  detectmotion();       //detect motion function
  startwiggletimer();
  io.run(); // Required for Adafruit IO communication
  sendmovementtoADAFRUIT();
  

  if((currentMillis-previousToyMillis<=wiggleToy) && (toycanwigglenow == true)){
    wiggletoy();
  }

  twentyfourhourtimer();

  if(motion == true){
  previousToyMillis = currentMillis;
  previous24Millis = currentMillis;
  nocatFeed->save(0); 
  nocatdisplayFeed->save(0);   //signal to send to adafruit to say no cat
  nocat = 0;                  // signal to say that no cat present is false because the motion detector has detected the cat
  toycanwigglenow = true;     //prevents the toy from wiggling when booting up the system
  movementtobesent = 1;       //movement to be sent - sets a bool value so when all the data gets sent, it will tell the program to send a cat movement signal
  nocatsent = 0;              //change the no cat signal sent to false, as there is movement again, and it means the cat is back, and the warning can be sent to your phone after another 24 hours of no movement
  } 

}
