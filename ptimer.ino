#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Fonts/Picopixel.h>


#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define LEDS_PER_SIDE 8
#define LEDS_FOR_STATUS (LEDS_PER_SIDE * 4 - 4)
#define PIN_LEDSTRIP 16
#define PIN_START 21
#define PIN_PLUS 19
#define PIN_MINUS 18

#define TICK_LEN_MS 1000
#define IO_CHECK_INTERVAL 100
#define DEFAULT_COUNTDOWN_MINUTES 25 // 20
#define DEFAULT_COUNTDOWN_SECONDS (DEFAULT_COUNTDOWN_MINUTES * 60)
#define DEFAULT_PAUSE_MINUTES 5
#define DEFAULT_PAUSE_SECONDS (DEFAULT_PAUSE_MINUTES * 60)
#define INCDEC_STEP_SECONDS (1 * 60)

#define STATE_OFF 0
#define STATE_ON 1
#define STATE_RESET 2 
#define STATE_FINISHED 4
#define STATE_PAUSE 8

u_long io_counter = 0;
uint sec_configured = DEFAULT_COUNTDOWN_SECONDS;
uint sec_target = sec_configured;
uint sec_counter = sec_configured;
uint state = STATE_OFF;
uint init_done = 0;
bool blinkReset = 0;

uint lastBtnStart = LOW;
uint currBtnStart = lastBtnStart;

uint lastBtnPlus = LOW;
uint currBtnPlus = lastBtnPlus;

uint lastBtnMinus = LOW;
uint currBtnMinus = lastBtnPlus;


u_int8_t progressBar[][2] = {
  {0,0},{0,1},{0,2},{0,3},{0,4},{0,5},{0,6},
  {0,7},{1,7},{2,7},{3,7},{4,7},{5,7},{6,7},
  {7,7},{7,6},{7,5},{7,4},{7,3},{7,2},{7,1},
  {7,0},{6,0},{5,0},{4,0},{3,0},{2,0},{1,0},
};

Adafruit_NeoMatrix  matrix = Adafruit_NeoMatrix(LEDS_PER_SIDE, LEDS_PER_SIDE, PIN_LEDSTRIP,
  NEO_MATRIX_BOTTOM     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);

void setupPins() {
  pinMode(PIN_START, INPUT_PULLUP);
  pinMode(PIN_PLUS, INPUT_PULLUP);
  pinMode(PIN_MINUS, INPUT_PULLUP);
}

void setupMatrix() {
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(10);
  matrix.setTextColor(matrix.Color(128, 128, 128));
  matrix.setTextSize(1);
  matrix.setFont(&Picopixel);
  matrix.show();
}


void setup() {
  setupPins();
  Serial.begin(9600);
  setupMatrix();
}

void blankLEDs(uint16_t color = 0) {
  matrix.fillScreen(color);
  //matrix.show();
}

void blinkEND() {
  for(int i = 0; i<5; i++) {
    blankLEDs(0);
    matrix.show();
    delay(TICK_LEN_MS/8);
    blankLEDs(matrix.Color(255/2,0,0));
    matrix.show();
    delay(TICK_LEN_MS/8);
    blankLEDs(matrix.Color(0,255/2,0));
    matrix.show();
    delay(TICK_LEN_MS/8);
    blankLEDs(matrix.Color(0,0,255/2));
    matrix.show();
    delay(TICK_LEN_MS/8);
  }
  blankLEDs(0);
}

String padInt(uint num) {
  String numStr = String(num);
  if(numStr.length() < 2 ) {
    return "0" + numStr;
  } else {
    return numStr;
  }
}

void drawTime() {
  uint16_t color;
  short offset;
  matrix.setCursor(1, 6);
  switch(state) {
    case STATE_ON:
      color = matrix.Color(128,0,0);
      offset = 1;
      break;
    case STATE_PAUSE:
      color = matrix.Color(0,0,128);
      offset = 1;
      break;
    default:
      color = matrix.Color(128,128,128);
      offset = 0;
      break;
  }
  matrix.setTextColor(color);
  matrix.print(padInt(ceil(sec_counter / 60 ) + offset));
}

void drawProgressBar() {
  if(state != STATE_ON && state != STATE_PAUSE) {
    return;
  }

  float pct_progress = ((float) sec_counter) / ((float) sec_target);
  u_int16_t led_progress = (u_int16_t) ((1-pct_progress) * LEDS_FOR_STATUS);
  for(u_int16_t i = 0; i < led_progress; i++) {
    matrix.setCursor(0, 0);
    matrix.drawPixel(progressBar[LEDS_FOR_STATUS-i][0], progressBar[LEDS_FOR_STATUS-i][1], matrix.Color(0,128,0));
  } 
}


void loop() {
  if((io_counter % (1000/IO_CHECK_INTERVAL) == 0)) {
    blankLEDs(0);
    switch(state) {
      case STATE_OFF:
        state = STATE_RESET;
        break;
      case STATE_ON:
        if(sec_counter > 0) {
          sec_counter -= 1;
          drawProgressBar();
          drawTime();
        } else {
          blinkEND();
          sec_counter = DEFAULT_PAUSE_SECONDS;
          sec_target = DEFAULT_PAUSE_SECONDS;
          state = STATE_PAUSE;
        }
        break;
      case STATE_PAUSE:
        if(sec_counter > 0) {
          sec_counter -= 1;
          drawProgressBar();
          drawTime();
        } else {
          state = STATE_FINISHED;
        }
        break;
      case STATE_FINISHED:
        blinkEND();
        state = STATE_RESET;
        break;
      case STATE_RESET:
        sec_counter = sec_configured;
        blinkReset = !blinkReset;
        if(blinkReset) {
          drawTime();
        } 
        break;
    }
    matrix.show();
  }

  // START BTN
  currBtnStart = digitalRead(PIN_START);
  switch(currBtnStart) {
    case LOW:
      if(lastBtnStart == HIGH) {
        switch(state) {
          case STATE_RESET:
            sec_target = sec_configured;
            state = STATE_ON;
            break;
          case STATE_ON:
            state = STATE_RESET;
            break;
        }
        lastBtnStart = LOW;
      }
      break;
    case HIGH:
      lastBtnStart = HIGH;
  }

  // PLUS BTN
  currBtnPlus = digitalRead(PIN_PLUS);
  switch(currBtnPlus) {
    case LOW:
      if(lastBtnPlus == HIGH) {
        switch(state) {
          case STATE_RESET:
            sec_configured += INCDEC_STEP_SECONDS;
            break;
          case STATE_ON:
          case STATE_PAUSE:
            sec_counter += INCDEC_STEP_SECONDS;
            if(sec_counter > sec_target) {
              sec_target += INCDEC_STEP_SECONDS;
            }            
            break;
        }
        lastBtnPlus = LOW;
      }
      break;
    case HIGH:
      lastBtnPlus = HIGH;
  }

  // MINUS BTN
  currBtnMinus = digitalRead(PIN_MINUS);
  switch(currBtnMinus) {
    case LOW:
      if(lastBtnMinus == HIGH) {
        switch(state) {
          case STATE_RESET:
            if(sec_configured < INCDEC_STEP_SECONDS) {
              sec_configured = DEFAULT_COUNTDOWN_SECONDS;
            } else {
              sec_configured -= INCDEC_STEP_SECONDS;
            }
            break;
          case STATE_ON:
          case STATE_PAUSE:
            if(sec_counter < INCDEC_STEP_SECONDS) {
              sec_counter = 0;
            } else {
              sec_counter -= INCDEC_STEP_SECONDS;
            }
            break;
        }
        lastBtnMinus = LOW;
      }
      break;
    case HIGH:
      lastBtnMinus = HIGH;
  }
  
  if(io_counter == LONG_MAX) {
    io_counter = 0;
  } else {
    io_counter += 1;
  }

  delay(IO_CHECK_INTERVAL);
}
