//#define DEBUG 
//#define TEST_SD
//#define TEST_SCREEN 

///////////////////////////// encoder library ///////////////////////////////////
#include "Adafruit_seesaw.h" 
#include <seesaw_neopixel.h>

#define SS_ENC0_SWITCH   12//12
#define SS_ENC1_SWITCH   14//3
#define SS_ENC2_SWITCH   17//24
#define SS_ENC3_SWITCH   9//10

#define SS_NEO_PIN       18

#define SEESAW_ADDR_1      0x49 //I2C0 address. Board 1.
#define SEESAW_ADDR_2      0x4A //I2C0 address. Board 2. For the 1 encoder board : 0x36 

#define NB_ENCODER 8

// create 4 encoders!
Adafruit_seesaw encoders_1 = Adafruit_seesaw(&Wire);
Adafruit_seesaw encoders_2 = Adafruit_seesaw(&Wire);

seesaw_NeoPixel encoder_pixels = seesaw_NeoPixel(NB_ENCODER, SS_NEO_PIN, NEO_GRB + NEO_KHZ800);

int32_t encoder_positions[NB_ENCODER] = {0, 0, 0, 0, 0, 0, 0, 0};
bool found_encoders[NB_ENCODER] = {false, false, false, false, false, false, false, false};
/////////////////////////////////////////////////////////////////////////////////

///////////////////////////// screen library ////////////////////////////////////
#include <Adafruit_GFX.h> //screen librairy
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C //I2C address

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////// SD library //////////////////////////////////////
#include <SD.h>     // SD library
#include <SPI.h>     // SPI is used by the SD card

#define PIN_SD_SS         17 //PIN_SPI0_SS

File midi_table;
File encoder_table;
/////////////////////////////////////////////////////////////////////////////////

/////////////////////////////// timer library ///////////////////////////////////
#include "hardware/timer.h"
#include "hardware/irq.h"

#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0
#define DELAY_US 32

volatile uint32_t *reg_GPIO_OUT  = (volatile uint32_t *)0xd0000010; // the register of all the GPIO OUT
static uint32_t buffer_to_send;
/////////////////////////////////////////////////////////////////////////////////

//////////////////////////////// buttons definition /////////////////////////////
#define B1_READ   11 
#define B2_READ   12
/////////////////////////////////////////////////////////////////////////////////

///////////////////////////////// some variables ////////////////////////////////
uint8_t midi_channel = 0;
uint8_t CC_number = 0;
uint8_t CC_value = 0;
int8_t movement = 0; //-1 or 1
bool refresh = false;
uint32_t buffer;
/////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);
  
  //encoder setup
  {
    // root the I2C 0
    Wire.setSDA(20); 
    Wire.setSCL(21); 
    Wire.begin(); // Wire communication begin
    Wire.setClock(100000);

    delay(100);

    
    // See if we can find encoders on this address
    if (! encoders_1.begin(SEESAW_ADDR_1) || !encoder_pixels.begin(SEESAW_ADDR_1)) {
      Serial.print("Couldn't find encoder 1");
      while(1) delay(10);
    } 
    Serial.println("seesaw 1 started");

      if (! encoders_2.begin(SEESAW_ADDR_2) || !encoder_pixels.begin(SEESAW_ADDR_2)) {
      Serial.print("Couldn't find encoder 2");
      while(1) delay(10);
    } 
    Serial.println("seesaw 2 started");


    uint32_t version = ((encoders_1.getVersion() >> 16) & 0xFFFF);
    if (version  != 5752){
      Serial.print("Wrong firmware loaded for seesaw 1 ? ");
      Serial.println(version);
      while(1) delay(10);
    }
    Serial.println("Found Product 5752");

    version = ((encoders_2.getVersion() >> 16) & 0xFFFF);
    if (version  != 5752){
      Serial.print("Wrong firmware loaded for seesaw 2 ? ");
      Serial.println(version);
      while(1) delay(10);
    }
    Serial.println("Found Product 5752");

      // use a pin for the built in encoder switch
      //encoders.pinMode(SS_SWITCH, INPUT_PULLUP);

      encoders_1.pinMode(SS_ENC0_SWITCH, INPUT_PULLUP);
      encoders_1.pinMode(SS_ENC1_SWITCH, INPUT_PULLUP);
      encoders_1.pinMode(SS_ENC2_SWITCH, INPUT_PULLUP);
      encoders_1.pinMode(SS_ENC3_SWITCH, INPUT_PULLUP);
      encoders_2.pinMode(SS_ENC0_SWITCH, INPUT_PULLUP);
      encoders_2.pinMode(SS_ENC1_SWITCH, INPUT_PULLUP);
      encoders_2.pinMode(SS_ENC2_SWITCH, INPUT_PULLUP);
      encoders_2.pinMode(SS_ENC3_SWITCH, INPUT_PULLUP);

      Serial.println("Turning on interrupts");
      delay(10);

      encoders_1.setGPIOInterrupts(1UL << SS_ENC0_SWITCH | 1UL << SS_ENC1_SWITCH | 
                      1UL << SS_ENC2_SWITCH | 1UL << SS_ENC3_SWITCH, 1);

      encoders_2.setGPIOInterrupts(1UL << SS_ENC0_SWITCH | 1UL << SS_ENC1_SWITCH | 
                      1UL << SS_ENC2_SWITCH | 1UL << SS_ENC3_SWITCH, 1);

      for (int e=0; e<4; e++) {
        encoder_positions[e] = encoders_1.getEncoderPosition(e);
        encoders_1.enableEncoderInterrupt(e);
        found_encoders[e] = true;
      }
      for (int e=0; e<4; e++) {
        encoder_positions[e+4] = encoders_2.getEncoderPosition(e);
        encoders_2.enableEncoderInterrupt(e);
        found_encoders[e+4] = true;
      } 

      //init uart
      {
        Serial1.setRX(1);
        Serial1.setTX(0);
        Serial1.setFIFOSize(128);
        Serial1.begin(31250);
      }
  }

// SD setup
  {
    // initialize SD card
    #ifdef DEBUG  
      Serial.print("Initializing SD card...");
      if (!SD.begin(PIN_SD_SS)) {
        Serial.println("initialization failed!");
        return;
      }
      Serial.println("initialization done.");
    #else
      if (!SD.begin(PIN_SD_SS)) {
        return;
      }
    #endif

    midi_table = SD.open("midi_table.bin", FILE_WRITE);
    encoder_table = SD.open("encoder_table.bin", FILE_WRITE);

    #ifdef DEBUG  
      if (!midi_table) {
        Serial.println("midi_table.bin failed to open");
        return;
      }
      Serial.println("midi_table.bin opened");
    
      if (!encoder_table) {
        Serial.println("encoder_table.bin failed to open");
        return;
      }
      Serial.println("encoder_table.bin opened");
    #endif

    #ifdef TEST_SD
      //test
      clear_table_data();
      clear_table_encoder();

      //on change la valeur midi du slider 4 :
      set_midi_value_encoder(4,13);

      //on change le CC_number :
      set_CC_number_value_encoder(4,122);

      //et enfin on change la CC_value :
      set_value_encoder(4, 103);

      //et on la relie :
      Serial.println(get_value_encoder(4));

      midi_table.close();
      encoder_table.close();

      Serial.println("files closed");
    #endif
  }

  // screen setup
  {
    // root the I2C1 : Wire 1 for screen
    Wire1.setSDA(14);
    Wire1.setSCL(15);
    Wire1.begin(); // Wire communication begin
    Wire1.setClock(1000000);


    // Initialize the display
    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    display.clearDisplay();
    display.setRotation(2);
    //display.setFont(&FreeSansOblique12pt7b);
    display.setTextColor(WHITE);
    

    #ifdef TEST_SCREEN
      Serial.println("debug screen start");
      display.setCursor(20, 30);
      display.setTextSize(3);
      display.println("test");
      Serial.println("debug screen start display");
      display.display();
      Serial.println("debug screen over");
    #endif
  }


  // buttons setup
  {
    pinMode(B1_READ, INPUT_PULLUP);
    pinMode(B2_READ, INPUT_PULLUP);
  }

  //init display
  {
    uint8_t midi = read_midi_value_encoder(0);
    uint8_t CC_number = read_CC_number_value_encoder(0);
    refresh_screen(midi, CC_number, read_value_data(midi, CC_number));
  }

  Serial.println("setup ok");
}

void loop() {

  if (!reset_table()) //if no reset
  {
    encoder_running();
  }
  
  // don't overwhelm serial port
  delay(50);
}