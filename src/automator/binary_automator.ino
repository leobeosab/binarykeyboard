/*  PS2Keyboard library example
  
  PS2Keyboard now requries both pins specified for begin()

  keyboard.begin(data_pin, irq_pin);
  
  Valid irq pins:
     Arduino Uno:  2, 3
     Arduino Due:  All pins, except 13 (LED)
     Arduino Mega: 2, 3, 18, 19, 20, 21
     Teensy 3.0:   All pins, except 13 (LED)
     Teensy 2.0:   5, 6, 7, 8
     Teensy 1.0:   0, 1, 2, 3, 4, 6, 7, 16
     Teensy++ 2.0: 0, 1, 2, 3, 18, 19, 36, 37
     Teensy++ 1.0: 0, 1, 2, 3, 18, 19, 36, 37
     Sanguino:     2, 10, 11
  
  for more information you can read the original wiki in arduino.cc
  at http://www.arduino.cc/playground/Main/PS2Keyboard
  or http://www.pjrc.com/teensy/td_libs_PS2Keyboard.html
  
  Like the Original library and example this is under LGPL license.
  
  Modified by Cuninganreset@gmail.com on 2010-03-22
  Modified by Paul Stoffregen <paul@pjrc.com> June 2010
*/
   
#include <PS2Keyboard.h>

const int DataPin = 6;
const int IRQpin =  3;
const int zeroSolPin = 11;
const int oneSolPin = 12;

const int solStartDelay = 50;
const int solEndDelay = 40;

bool queue[1000];
bool ready = true;
bool pushing = false;
bool pulling = false;
int activeSol = 0;
unsigned long timeLastPushed;
unsigned long timeLastPulled;

bool queueDraining = false;
enum MODE {
  ONE_KEY, BATCH
};

MODE currentMode = BATCH;
int queueIndex = 0;
int drainIndex = 0;

PS2Keyboard keyboard;

void setup() {
  pinMode(zeroSolPin, OUTPUT);
  pinMode(oneSolPin, OUTPUT);
  keyboard.begin(DataPin, IRQpin);
  Serial.begin(9600);
  Serial.println("Keyboard Test:");
}

void send_letter_one_key(char letter) {
  int numericalLetter = (int)letter;

  noInterrupts();
  for (int i = 1; i != (1 << 8); i<<=1) {
    if ((numericalLetter & i) != 0) {
      queue[queueIndex] = true;
    } else {
      queue[queueIndex] = false;
    }
    queueIndex++;
  }
  interrupts();
}

void send_letter_batch(char letter) {
  int numericalLetter = (int)letter;

  noInterrupts();
  for (int i = 128; i != 0; i>>=1) {
    if ((numericalLetter & i) != 0) {
      queue[queueIndex] = true;
    } else {
      queue[queueIndex] = false;
    }
    queueIndex++;
  }
  interrupts();
}

void handle_solenoids() {
  if (pushing) {
    if (millis() - timeLastPushed >= solStartDelay) {
      pushing = false;
      digitalWrite(activeSol, LOW);
      pulling = true;
      timeLastPulled = millis();
    }

    return;
  }
  if (pulling) {
    if (millis() - timeLastPulled >= solEndDelay) {
      pulling = false;
    }

    return;
  }
   
}

void process_queue() {
  if (queueIndex-1 == -1 || pushing || pulling) {
    return;
  }
  
  const bool isOne = queue[queueIndex-1];
  Serial.println(isOne);
  if (isOne) {
    activeSol = oneSolPin;
    digitalWrite(oneSolPin, HIGH);
   } else {
    activeSol = zeroSolPin;
    digitalWrite(zeroSolPin, HIGH);
  }
  pushing = true;
  timeLastPushed = millis();   

  queueIndex--;
}

void drain_queue() {
  if (pushing || pulling) {
    return;
  }

  if (drainIndex >= queueIndex) {
    drainIndex = 0;
    queueIndex = 0;
    return;
  }
  
  const bool isOne = queue[drainIndex];
  Serial.println(isOne);
  if (isOne) {
    activeSol = oneSolPin;
    digitalWrite(oneSolPin, HIGH);
   } else {
    activeSol = zeroSolPin;
    digitalWrite(zeroSolPin, HIGH);
  }
  pushing = true;
  timeLastPushed = millis();

  drainIndex++;
}

void reset() {
  queueIndex = 0;
  digitalWrite(oneSolPin, LOW);
  digitalWrite(zeroSolPin, LOW);
}

void loop() {
  if (keyboard.available() && !queueDraining) {
    // read the next key
    char c = keyboard.read();
    
    // check for some of the special keys
    if (c == PS2_DELETE) {
      Serial.print("[Del]");
      switch (currentMode) {
        case ONE_KEY:
          send_letter_one_key((char)8);
          break;
        case BATCH:
          send_letter_batch((char)8);
          break;
      }
    } else if (c == PS2_PAGEUP) {
      Serial.println("ONE_KEY MODE");
      currentMode = ONE_KEY;
      reset();
    } else if (c == PS2_PAGEDOWN) {
      Serial.println("BATCH MODE");
      currentMode = BATCH;
      reset();
    } else if (c == PS2_ENTER) {
      queueDraining = true;
    } else {
      switch (currentMode) {
        case ONE_KEY:
          send_letter_one_key(c);
          break;
        case BATCH:
          send_letter_batch(c);
          break;
      }

      Serial.print(c);
    }
  }

  switch (currentMode) {
    case ONE_KEY:
      process_queue();
      break;
    case BATCH:
      if (queueDraining) {
         drain_queue();
      }
      if (queueIndex == 0 && !pushing && !pulling) {
        queueDraining = false;
      }
      break;
  }

  handle_solenoids();
}
