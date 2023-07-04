#define RX_PIN 3
#define CLK_PIN 4
#define TX_PIN 5
#define CS_PIN 6
#define HOOKSWITCH_PIN 7
#define LED_PIN 13

void strobeClk() {
  digitalWrite(CLK_PIN, 1);
  digitalWrite(CLK_PIN, 0);
}

void _write8(uint8_t b) {
  // MBS first! 
  for (unsigned int i = 0; i < 8; i++) {
    if (b & 0x80) {
      digitalWrite(TX_PIN, 1);
    } else {
      digitalWrite(TX_PIN, 0);        
    }
    strobeClk();
    b = b << 1;
  }
}

uint8_t _read8() {
  uint8_t result = 0;
  for (unsigned int i = 0; i < 8; i++) {
    result = result << 1;
    // Per 5.11 data is valid when serial clock is high
    digitalWrite(CLK_PIN, 1);    
    if (digitalRead(RX_PIN) == 1) {
      result |= 1;
    }    
    digitalWrite(CLK_PIN, 0);    
  }
  return result;
}

void write0(uint8_t addr) {
  digitalWrite(CS_PIN, 0);
  _write8(addr);
  digitalWrite(TX_PIN, 0);        
  digitalWrite(CS_PIN, 1);    
}

void write8(uint8_t addr, uint8_t data) {
  digitalWrite(CS_PIN, 0);
  _write8(addr);
  _write8(data);
  digitalWrite(TX_PIN, 0);        
  digitalWrite(CS_PIN, 1);    
}

void write16(uint8_t addr, uint16_t data) {
  digitalWrite(CS_PIN, 0);
  _write8(addr);
  // MSByte
  _write8((data & 0xff00) >> 8);
  // LSByte
  _write8((data & 0x00ff));
  digitalWrite(TX_PIN, 0);        
  digitalWrite(CS_PIN, 1);    
}

uint8_t read8(uint8_t addr) {
  digitalWrite(CS_PIN, 0);
  _write8(addr);
  uint8_t d = _read8();
  digitalWrite(CS_PIN, 1);    
  return d;
}

uint16_t read16(uint8_t addr) {
  digitalWrite(CS_PIN, 0);
  _write8(addr);
  // MSByte
  uint8_t msb = _read8();
  // LSByte
  uint8_t lsb = _read8();
  digitalWrite(CS_PIN, 1);    
  return (msb << 8) | lsb;
}

int rxReady() {
  uint16_t a = read16(0xe6);
  return (a & 0b0000000001000000) != 0;
}

int rxEnergy() {
  uint16_t a = read16(0xe6);
  //            5432109876543210
  return (a & 0b0000010000000000) != 0;
}

int txReady() {
  uint16_t a = read16(0xe6);
  return (a & 0b0000100000000000) != 0;
}

int programReady() {
  uint16_t a = read16(0xe6);
  return (a & 0b0010000000000000) != 0;
}

void setupModem() {
  // TX control
  // We are the answering modem (high band)
  //              5432109876543210    
  write16(0xe1, 0b0111011000010110);
  // TX OFF
  //write16(0xe1, 0b0000111000010110);

  // RX control
  // This is the calling modem setting used for echo test:
  //              5432109876543210    
  //write16(0xe2, 0b0111111000110110); 
  // We are the answering modem (low band)
  // NORMAL:
  //              5432109876543210    
  write16(0xe2, 0b0110000000110110); 
}

void sendSilence() {
  // TX control: Tone pair TA
  write16(0xe1, 0b0001000000000000);
}

void sendDialTone() {
  // TX control: Tone pair TA
  write16(0xe1, 0b0001000000001100);
}

void sendRingTone() {
  // TX control: Tone pair TB
  write16(0xe1, 0b0001000000001101);
}

void sendText(const char* s) {
  for (unsigned int i = 0; s[i] != 0; i++) {
    // Wait util we can send
    while (!txReady()) {
    }
    write8(0xe3, (uint8_t)s[i]);
  }
}

void setup() {

  Serial.begin(9600);
 
  pinMode(RX_PIN, INPUT);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(TX_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  pinMode(HOOKSWITCH_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(CLK_PIN, 0);
  digitalWrite(TX_PIN, 0);
  digitalWrite(CS_PIN, 1);

  Serial.println("Starting");

  // General reset
  write0(0x01);

  delay(20);

  // Write general control to power on
  //write16(0xe0, 0x0180); 
  //              5432109876543210
  write16(0xe0, 0b0000000110000000);
  
  // Per datasheet, start clock
  delay(20);

  // Configure analog loopback, power on
  //write16(0xe0, 0x0900);
  //              5432109876543210
  write16(0xe0, 0b0000000100000000);
    
  // Program the tone pairs
  while (!programReady()) { }
  //Serial.println("Programming ...");

  write16(0xe8, 0x8000);

  // TA is used for US dial tone
  // TA1 frequency is 440, 440 * 3.414 = 1502, hex = 05DE
  while (!programReady()) { }
  write16(0xe8, 0x05DE);
  // TA1 amplitude is 0.5Vrms * 93780 / 3.3 = 14,209, hex = 3781
  while (!programReady()) { }
  write16(0xe8, 0x3981);
  // TA2 frequency is 350, 350 * 3.414 = 1194, hex = 04AA
  while (!programReady()) { }
  write16(0xe8, 0x04AA);
  // TA2 amplitude is 0.5Vrms * 93780 / 3.3 = 14,209, hex = 3781
  while (!programReady()) { }
  write16(0xe8, 0x3981);

  // TB is used for US ring tone
  // TB1 frequency is 440, 440 * 3.414 = 1502, hex = 05DE
  while (!programReady()) { }
  write16(0xe8, 0x05DE);
  // TB1 amplitude is 0.5Vrms * 93780 / 3.3 = 14,209, hex = 3781
  while (!programReady()) { }
  write16(0xe8, 0x3981);
  // TB2 frequency is 480, 480 * 3.414 = 1638 , hex = 0666 
  while (!programReady()) { }
  write16(0xe8, 0x0666);
  // TB2 amplitude is 0.5Vrms * 93780 / 3.3 = 14,209, hex = 3781
  while (!programReady()) { }
  write16(0xe8, 0x3981);

  //sendSilence();
  //sendDialTone();
  //setupModem();
  
  Serial.write(26);
  Serial.write(26);
  Serial.print("AT");
  Serial.write(10);
}

#define ON_HOOK 1
#define OFF_HOOK 0

int lastHs = 1;
long lastHsTransition = 0;
int hsState = ON_HOOK;
// TEST
//int hsState = OFF_HOOK;

int state = 0;
// TEST: Jump to TX/RX
//int state = 11;
long stateChangeStamp = 0;
int clickCount = 0;
int digitCount = 0;
int dialDigits[10];

void loop() {

  if (state == 11) {
    
    // Check for inbound on the modem
    if (rxReady()) {
      uint8_t d = read8(0xe5);
      Serial.print((char)d);
      // Echo typed characters
      while (!txReady()) { }
      write8(0xe3, (uint8_t)d);
    }
  
    if (Serial.available()) {
      int c = Serial.read();
      // Wait util we can send
      while (!txReady()) {
      }
      write8(0xe3, (uint8_t)c);
    }

    // Show RX energy
    if (rxEnergy()) {
      digitalWrite(LED_PIN, 1);
    } else {
      digitalWrite(LED_PIN, 0);      
    }
  }
   
  // Look for a hookswitch transition  
  int hs = digitalRead(HOOKSWITCH_PIN);
  if (hs != lastHs) {
    lastHs = hs;
    lastHsTransition = millis();
  }  

  // Look for debounced hookswitch transition
  // (was 20)
  if (millis() - lastHsTransition > 50 && hsState != lastHs) {
    hsState = lastHs;
    // Use LED to show hook state
    if (hsState == OFF_HOOK) {
      //digitalWrite(LED_PIN, 1);
    } else {
      //digitalWrite(LED_PIN, 0);
    }
  }

  // State machine

  // Hung up
  if (state == 0) {
    if (hsState == OFF_HOOK) {
      //Serial.println("Off Hook");

      // Short delay before the tone starts
      delay(200);      
      sendDialTone();
      //setupModem();
      
      clickCount = 0;
      digitCount = 0;
      state = 1;
      stateChangeStamp = millis();
    }
  }
  // Sending dial tone
  else if (state == 1) {
    // Look for timeout
    if (millis() - stateChangeStamp > 15000) {
      //Serial.println("No dial");
      sendSilence();
      state = 99;
      stateChangeStamp = millis();
    }
    // Look for break.  Could be hang up or dialing.
    if (hsState == ON_HOOK) {
      // Turn off dial tone
      sendSilence();
      state = 2;
      stateChangeStamp = millis();
    }
  }
  // A first break was detected, look for dialing
  else if (state == 2) {
    if (hsState == ON_HOOK) {
      // Look for hang up
      if (millis() - stateChangeStamp > 500) {
        state = 0;
        stateChangeStamp = millis();
        //Serial.println("On hook");
      }
    }
    // Look for transition back off hook.  This indicates
    // dialing
    else if (hsState == OFF_HOOK) {
      state = 4;
      stateChangeStamp = millis();
      clickCount++;
    }
  }
  // In this state a click (rising edge) was just recorded
  else if (state == 4) {
    if (hsState == ON_HOOK) {
      state = 2;
      stateChangeStamp = millis();
    } else {
      // Look for digit timeout
      if (millis() - stateChangeStamp > 250) {
        //Serial.print("Got digit ");
        //Serial.println(clickCount);
        dialDigits[digitCount] = clickCount;
        clickCount = 0;        
        digitCount++;
        // Check to see if we have a full number
        if (digitCount == 7) {
          state = 10;
        } else {
          state = 5;   
        }
      }
    }
  }
  // In this state we are waiting for the next digit to be dialed,
  // or timeout
  else if (state == 5) {
    // Look for break.  Could be hang up or more dialing.
    if (hsState == ON_HOOK) {
      state = 2;
      stateChangeStamp = millis();
    } else {
      // Look for inter-digit timeout
      if (millis() - stateChangeStamp > 10000) {
        //Serial.println("Gave up waiting for dialing");
        // Send error
        state = 99;
        stateChangeStamp = millis();
      }
    }
  }
  else if (state == 10) {

    //Serial.print("Connecting to ");
    //for (int i = 0; i < digitCount; i++) 
    //  Serial.print(dialDigits[i]);
    //Serial.println();

    delay(1000);
    sendRingTone();
    delay(2000);
    sendSilence();
    delay(4000);
    sendRingTone();
    delay(2000);
    sendSilence();
    delay(4000);    

    setupModem();
    state = 11;
    stateChangeStamp = millis();

    // Notify that we have a connection
    Serial.write(10);   
    Serial.print("AT+CONN=1.1.1.1");   
    Serial.write(10);   

    // Send a message to the remote station
    delay(1000);
    sendText("You are now connected.\r\nWelcome to the 1980's\r\n\r\n");
  }
  else if (state == 11) {
    
    // Wait for hangup
    if (hsState == ON_HOOK) {

      Serial.write(26);
      Serial.write(26);
      Serial.print("AT+DISC");
      Serial.write(10);
      
      sendSilence();
      
      state = 0;
      stateChangeStamp = millis();
    }
  }
  else if (state == 99) {
    if (hsState == ON_HOOK) {
      state = 0;
      stateChangeStamp = millis();
      //Serial.println("On hook");
    }
  }
}
