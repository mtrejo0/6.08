#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>
#include <WiFi.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include<string.h>
TFT_eSPI tft = TFT_eSPI();
//LIGHTS
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define PIN 19// Parameter 1 = number of pixels in strip
#define PINLIFE 21
Adafruit_NeoPixel strip = Adafruit_NeoPixel(20, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel striplife = Adafruit_NeoPixel(3, PINLIFE, NEO_GRB + NEO_KHZ800);
// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

#define INITIALIZE 0  //example definition
#define WAITING 1
#define INGAME 2
#define DEAD 3
#define WIN 4

class Button {
  public:
    uint32_t t_of_state_2;
    uint32_t t_of_button_change;
    uint32_t debounce_time;
    uint32_t long_press_time;
    uint8_t pin;
    uint8_t flag;
    bool button_pressed;
    uint8_t state; // This is public for the sake of convenience
    Button(int p) {
      flag = 0;
      state = 0;
      pin = p;
      t_of_state_2 = millis(); //init
      t_of_button_change = millis(); //init
      debounce_time = 10;
      long_press_time = 1000;
      button_pressed = 0;
    }
    void read() {
      uint8_t button_state = digitalRead(pin);
      button_pressed = !button_state;
    }
    int update() {
      read();
      flag = 0;
      if (state == 0) { // Unpressed, rest state
        if (button_pressed) {
          state = 1;
          t_of_button_change = millis();
        }
      } else if (state == 1) { //Tentative pressed
        if (!button_pressed) {
          state = 0;
          t_of_button_change = millis();
        } else if (millis() - t_of_button_change >= debounce_time) {
          state = 2;
          t_of_state_2 = millis();
        }
      } else if (state == 2) { // Short press
        if (!button_pressed) {
          state = 4;
          t_of_button_change = millis();
        } else if (millis() - t_of_state_2 >= long_press_time) {
          state = 3;
        }
      } else if (state == 3) { //Long press
        if (!button_pressed) {
          state = 4;
          t_of_button_change = millis();
        }
      } else if (state == 4) { //Tentative unpressed
        if (button_pressed && millis() - t_of_state_2 < long_press_time) {
          state = 2; // Unpress was temporary, return to short press
          t_of_button_change = millis();
        } else if (button_pressed && millis() - t_of_state_2 >= long_press_time) {
          state = 3; // Unpress was temporary, return to long press
          t_of_button_change = millis();
        } else if (millis() - t_of_button_change >= debounce_time) { // A full button push is complete
          state = 0;
          if (millis() - t_of_state_2 < long_press_time) { // It is a short press
            flag = 1;
          } else {  // It is a long press
            flag = 2;
          }
        }
      }
      return flag;
    }
};

class PWM_608
{
  public:
    int pin; //digital pin class controls
    int period; //period of PWM signal (in milliseconds)
    int on_amt; //duration of "on" state of PWM (in milliseconds)
    PWM_608(int op, float frequency); //constructor op = output pin, frequency=freq of pwm signal in Hz
    void set_duty_cycle(float duty_cycle); //sets duty cycle to be value between 0 and 1.0
    void update(); //updates state of system based on millis() and duty cycle
};

//add your PWM_608 class definitions here:
PWM_608::PWM_608(int op, float frequency) {
  pin = op;
  period = int((1 / frequency) * 1000);
  on_amt = 0;
}

void PWM_608::update() {
  if (millis() % period == 0) {
    digitalWrite(pin, HIGH);
  }
  if (millis() % period >= on_amt) {
    digitalWrite(pin, LOW);
  }
  if (millis() % period > 0 && millis() % period < on_amt) {
    digitalWrite(pin, HIGH);
  }
}

void PWM_608::set_duty_cycle(float duty_cycle) {
  if (duty_cycle < 0) {
    duty_cycle = 0;
  }
  else if (duty_cycle > 1) {
    duty_cycle = 1;
  }
  on_amt = duty_cycle * period;
}

const int BUTTON_PIN = 16; // laser
const int BUTTON_PIN2 = 5; // button for laser
const int BUTTON_PIN3 = 17; // for getting in the game
int global_time;
uint32_t counter; //used for timing
uint32_t counterlaser; //used for timing
const uint32_t pwm_channel = 0; //hardware pwm channel used in second part

#define base 0
#define first 1
#define second 2
#define third 3
#define laserbase 0
#define laserfirst 1
#define lasersecond 2
#define baselife 0
#define firstlife 1

int out_buffer_size;
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response
char host[] = "608dev.net";
char username[] = "Moises";
//game variables
int timer;
bool shot;
int lives = 3;
int state_for_game;
int statelife = 0;
uint32_t counterlife;
int state = 0;
int statebullet = 0;
int ammo = 20;
uint32_t time_counter;

//instances of classes
PWM_608 laser(BUTTON_PIN, 500); //create instance of PWM to control backlight on pin 12, operating at 50 Hz
Button button3(BUTTON_PIN3);

Button gun(BUTTON_PIN2);
HardwareSerial mySoftwareSerial(2);
DFRobotDFPlayerMini myDFPlayer;

//char network[] = "6s08";  //SSID for 6.08 Lab
//char password[] = "iesc6s08"; //Password for 6.08 Lab
char network[] = "MIT GUEST";
char password[] = "";
char action[10];
void printDetail(uint8_t type, int value);
void setup() {
  Serial.begin(115200); //for debugging if needed.
  // Wifi beginning stuff

  WiFi.begin(network, password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }             // Set up serial port
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 1);
  tft.print("Long Press to start the game");

  //mp3 player stuff
  mySoftwareSerial.begin(9600, SERIAL_8N1, 32, 33);  // speed, type, RX, TX
  Serial.begin(115200);
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  delay(1000);
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.

    Serial.println(myDFPlayer.readType(), HEX);
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    //    while (true);
  }
  //  Serial.println(F("DFPlayer Mini online."));
  //  myDFPlayer.setTimeOut(500); //Set serial communictaion time out 500ms
  //  //----Set volume----
  //  myDFPlayer.volume(25);  //Set volume value (0~30).
  //  myDFPlayer.volumeUp(); //Volume Up
  //  myDFPlayer.volumeDown(); //Volume Down
  //  //----Set different EQ----
  //  myDFPlayer.EQ(DFPLAYER_EQ_BASS);
  //  //----Set device we use SD as default----
  //  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  //  int delayms = 100;
  //  //----Read imformation----
  //  Serial.println(F("readState--------------------"));
  //  Serial.println(myDFPlayer.readState()); //read mp3 state
  //  Serial.println(F("readVolume--------------------"));
  //  Serial.println(myDFPlayer.readVolume()); //read current volume
  //  //Serial.println(F("readEQ--------------------"));
  //  //Serial.println(myDFPlayer.readEQ()); //read EQ setting
  //  Serial.println(F("readFileCounts--------------------"));
  //  Serial.println(myDFPlayer.readFileCounts()); //read all file counts in SD card
  //  Serial.println(F("readCurrentFileNumber--------------------"));
  //  Serial.println(myDFPlayer.readCurrentFileNumber()); //read current play file number
  //  Serial.println(F("readFileCountsInFolder--------------------"));
  //  Serial.println(myDFPlayer.readFileCountsInFolder(3)); //read fill counts in folder SD:/03
  //  Serial.println(F("--------------------"));
  //  Serial.println(F("myDFPlayer.play(1)"));
  //  myDFPlayer.play(1);  //Play the first mp3
  shot = false;
  timer = millis();
  ledcSetup(pwm_channel, 10, 12);//create pwm channel, @50 Hz, with 12 bits of precision
  ledcAttachPin(16, pwm_channel); //link pwm channel to IO pin 14
  ledcWrite(pwm_channel, 4095);
  pinMode(BUTTON_PIN, OUTPUT); //controlling TFT with hardware PWM (second part of lab)
  pinMode(BUTTON_PIN2, INPUT_PULLUP); //controlling TFT with hardware PWM (second part of lab)
  pinMode(BUTTON_PIN3, INPUT_PULLUP);
  counter = millis();
  state_for_game = INITIALIZE;

  //   This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  strip.begin();
  strip.setBrightness(40);
  striplife.begin();
  striplife.setBrightness(40);
  strip.show(); // Initialize all pixels to 'off'
  striplife.show();
  for (int j = 0; j < 20; j++) {
    strip.setPixelColor(j, strip.Color(255, 255, 255));
  }
  strip.show();
  for (int j = 0; j < 3; j++) {
    striplife.setPixelColor(j, strip.Color(0, 255, 0));
  }
  striplife.show();
}


void loop() {
  //uint8_t button_state = digitalRead(6);
  state_machine();


  //  Serial.print("gamestate: ");
  //  Serial.print(state_for_game);
  //  Serial.print("    ");
  //  Serial.print("ammo: ");
  //  Serial.print(ammo);
  //  Serial.print("    ");
  //  Serial.print("lives: ");
  //  Serial.println(lives);
}

void state_machine() {
  uint8_t flag = button3.update();
  switch (state_for_game)
  {
    case INITIALIZE:
      if (flag == 2) {
        post_for_starting_game();
        state_for_game = WAITING;
        lives = 3;
        tft.fillScreen(TFT_BLACK);
      }
      break;
    case WAITING:
      get_("waitingString");

      if (strcmp(response, "0 seconds remaining!") == 0)
      {
        tft.fillScreen(TFT_BLACK);
        get_("waitingString");
        get_("getTime");
        get_("gameStatus");
        global_time = millis();
        myDFPlayer.pause();
        state_for_game = INGAME;
      }

      break;
    case INGAME: {
        float reading1 = analogRead(A3) * 3.3 / 4096;
        float reading2 = analogRead(A0) * 3.3 / 4096;
        float reading3 = analogRead(A7) * 3.3 / 4096;

        uint8_t gunState = gun.update();

        laserbutton(gunState);


        Serial.println("--------------");
        Serial.println(reading1);
        Serial.println(reading2);
        Serial.println(reading3);





        if (reading1 < 1 or reading2 < 1 or reading3 < 1)
        {


          lives --;


          if (lives == 2) {
            striplife.setPixelColor(0 , strip.Color(0, 255, 0));
            striplife.setPixelColor(1 , strip.Color(0, 255, 0));
            striplife.setPixelColor(2, strip.Color(255, 0, 0));

          }
          else if (lives == 1) {
            striplife.setPixelColor(0 , strip.Color(0, 255, 0));
            striplife.setPixelColor(1, strip.Color(255, 0, 0));
            striplife.setPixelColor(2, strip.Color(255, 0, 0));

          }
          else if (lives == 0) {
            striplife.setPixelColor(0, strip.Color(255, 0, 0));
            striplife.setPixelColor(1, strip.Color(255, 0, 0));
            striplife.setPixelColor(2, strip.Color(255, 0, 0));

          }


          striplife.show();
          shot = true;
          timer = millis();
          post_for_getting_shot(0);

          tft.fillScreen(TFT_RED);

          if (lives == 0) {
            tft.fillScreen(TFT_BLACK);
            state_for_game = DEAD;
          }
          else {

            delay(5000);
          }



        }
        if (lives > 0) {
          tft.setCursor(10, 10, 1);
          tft.println("Health:");
          tft.setCursor(10, 20, 1);
          tft.println(lives);
          tft.setCursor(10, 40, 1);
          tft.println("Bullets:");
          tft.setCursor(10, 50, 1);
          tft.println(ammo / 2);
        }
        if ( millis() % 3000 == 0) {
          get_("gameStatus");
        }
        if (strstr(response, username))
        {
          Serial.println("WINNER");

          delay(5000);
          clearServer();
          tft.fillScreen(TFT_BLACK);
          state_for_game = WIN;


        }





      }
      break;

    case DEAD:
      ledcWrite(pwm_channel, 0);
      striplife.setPixelColor(0, strip.Color(255, 0, 0));
      striplife.setPixelColor(1, strip.Color(255, 0, 0));
      striplife.setPixelColor(2, strip.Color(255, 0, 0));
      striplife.show();

      get_("gameStatus");
      tft.setCursor(10, 50, 1);
      tft.println("You're are Dead :(");
      for (int j = 0; j < 20; j++) {
        strip.setPixelColor(j, strip.Color(255, 0, 0));
      }
      strip.show();
      tft.println("Long Press to go back to lobby");


      if (flag == 2) {
        setup();
      }
      break;
    case WIN:
      ledcWrite(pwm_channel, 0);

      tft.setCursor(10, 50, 1);
      tft.println("You WON!!");
      tft.println("Long Press to go back to lobby");
      for (int j = 0; j < 20; j++) {
        strip.setPixelColor(j, strip.Color(0, 255, 0));
      }
      strip.show();


      if (flag == 2) {
        setup();
      }
      break;
  }
}



void laserbutton(int gunButton) {
  switch (state) {
    case base:

      if (gunButton != 0) {
        Serial.println("LOSING BULLETS");

        state = 1;
        ammo -= 2;
        post_for_setting_bullets();
        for (int j = ammo; j < 20; j++) {
          strip.setPixelColor(j , strip.Color(0, 0, 0));
        }

        strip.show();
        counter = millis();
      }

      if (ammo == 0) {
        ledcWrite(pwm_channel, 0);
        state = 3;
        counter = millis();
      }
      break;

    case first:
      if (millis() - counter > 1000) {
        ledcWrite(pwm_channel, 0);
        state = 2;
      }
      else if (gunButton == 0) {
        ledcWrite(pwm_channel, 4095);
        state = 0;
      }

    case second :
      if (millis() - counter > 3000) {
        ledcWrite(pwm_channel, 4095);
        state = 0;
        ammo = 20;
        post_for_setting_bullets();

      }
      break;

    case third:
      if (millis() - counter > 3000) {
        ledcWrite(pwm_channel, 4095);
        ammo = 20;
        for (int j = 0; j < 20; j++) {
          strip.setPixelColor(j , strip.Color(255, 255, 255));
          state = 0;
        }
        strip.show();
      }

      break;
  }
}




/*----------------------------------
   char_append Function:
   Arguments:
      char* buff: pointer to character array which we will append a
      char c:
      uint16_t buff_size: size of buffer buff

   Return value:
      boolean: True if character appended, False if not appended (indicating buffer full)
*/
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

/*----------------------------------
   do_http_request Function:
   Arguments:
      char* host: null-terminated char-array containing host to connect to
      char* request: null-terminated char-arry containing properly formatted HTTP request
      char* response: char-array used as output for function to contain response
      uint16_t response_size: size of response buffer (in bytes)
      uint16_t response_timeout: duration we'll wait (in ms) for a response from server
      uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
   Return value:
      void (none)
*/
void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) ;//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      //      if (serial) Serial.println(response);
      if (strcmp(response, "\r") == 0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout) break;
    }
    memset(response, 0, response_size);
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    //    if (serial) Serial.println(response);
    client.stop();
    if (serial) ;
  } else {
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

void printDetail(uint8_t type, int value) {
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void music_player()
{
  // static unsigned long timer = millis();

  if (Serial.available()) {
    String inData = "";
    //Serial.write("*");
    inData = Serial.readStringUntil('\n');
    if (inData.startsWith("n")) {
      Serial.println(F("next--------------------"));
      myDFPlayer.next();
      Serial.println(myDFPlayer.readCurrentFileNumber()); //read current play file number
    } else if (inData.startsWith("p")) {
      Serial.println(F("previous--------------------"));
      myDFPlayer.previous();
      Serial.println(myDFPlayer.readCurrentFileNumber()); //read current play file number
    } else if (inData.startsWith("+")) {
      Serial.println(F("up--------------------"));
      myDFPlayer.volumeUp();
      Serial.println(myDFPlayer.readVolume()); //read current volume
    } else if (inData.startsWith("-")) {
      Serial.println(F("down--------------------"));
      myDFPlayer.volumeDown();
      Serial.println(myDFPlayer.readVolume()); //read current volume
    } else if (inData.startsWith("*")) {
      Serial.println(F("pause--------------------"));
      myDFPlayer.pause();
    } else if (inData.startsWith(">")) {
      Serial.println(F("start--------------------"));
      myDFPlayer.start();
    }
  }

  if (myDFPlayer.available()) {
    if (myDFPlayer.readType() == DFPlayerPlayFinished) {
      Serial.println(myDFPlayer.read());
      Serial.println(F("next--------------------"));
      myDFPlayer.next();  //Play next mp3 every 3 second.
      Serial.println(F("readCurrentFileNumber--------------------"));
      Serial.println(myDFPlayer.readCurrentFileNumber()); //read current play file number
      delay(500);
    }
  }

}

void get_(char* action)
{
  char request[500];
  sprintf(request, "GET /sandbox/sc/moisest/finalProject/request.py?user=Derek&action=%s HTTP/1.1\r\n", action);
  sprintf(request + strlen(request), "Host: %s\r\n", host);
  strcat(request, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request + strlen(request), "Content-Length: %d\r\n\r\n", 0);
  do_http_request(host, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
  Serial.println(response);
  tft.setCursor(5, 60, 1);
  tft.println(response);
}

void post_for_starting_game()
{ tft.fillScreen(TFT_BLACK);
  char body[200]; //for body;
  sprintf(body, "user=%s&action=initial", username); //generate body, posting to User, 1 step
  sprintf(request_buffer, "POST http://608dev.net/sandbox/sc/moisest/finalProject/request.py HTTP/1.1\r\n");
  strcat(request_buffer, "Host: 608dev.net\r\n");
  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", strlen(body)); //append string formatted to end of request buffer
  strcat(request_buffer, "\r\n"); //new line from header to body
  strcat(request_buffer, body); //body
  strcat(request_buffer, "\r\n"); //header
  Serial.println(request_buffer);
  do_http_request("608dev.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
}
void post_for_setting_bullets()
{ tft.fillScreen(TFT_BLACK);

  char body[200]; //for body;
  sprintf(body, "user=%s&action=setBullets&bullets=%d", username, ammo / 2); //generate body, posting to User, 1 step

  sprintf(request_buffer, "POST http://608dev.net/sandbox/sc/moisest/finalProject/request.py HTTP/1.1\r\n");
  strcat(request_buffer, "Host: 608dev.net\r\n");
  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", strlen(body)); //append string formatted to end of request buffer
  strcat(request_buffer, "\r\n"); //new line from header to body
  strcat(request_buffer, body); //body
  strcat(request_buffer, "\r\n"); //header
  do_http_request("608dev.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
}

void post_for_getting_shot(int time_of_shot)
{
  char body[200]; //for body;
  sprintf(body, "user=%s&action=shot&timeShot=%d", username, time_of_shot); //generate body, posting to User, 1 step
  sprintf(request_buffer, "POST http://608dev.net/sandbox/sc/moisest/finalProject/request.py HTTP/1.1\r\n");
  strcat(request_buffer, "Host: 608dev.net\r\n");
  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", strlen(body)); //append string formatted to end of request buffer
  strcat(request_buffer, "\r\n"); //new line from header to body
  strcat(request_buffer, body); //body
  strcat(request_buffer, "\r\n"); //header
  Serial.println(request_buffer);
  do_http_request("608dev.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);

}
void clearServer()
{
  char body[200]; //for body;
  sprintf(body, "user=%s", username); //generate body, posting to User, 1 step
  sprintf(request_buffer, "POST http://608dev.net/sandbox/sc/moisest/finalProject/clear.py HTTP/1.1\r\n");
  strcat(request_buffer, "Host: 608dev.net\r\n");
  strcat(request_buffer, "Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer + strlen(request_buffer), "Content-Length: %d\r\n", strlen(body)); //append string formatted to end of request buffer
  strcat(request_buffer, "\r\n"); //new line from header to body
  strcat(request_buffer, body); //body
  strcat(request_buffer, "\r\n"); //header
  Serial.println(request_buffer);
  do_http_request("608dev.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
}
