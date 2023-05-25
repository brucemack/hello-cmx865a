#define RX_PIN 3
#define CLK_PIN 4
#define TX_PIN 5
#define CS_PIN 6

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

void setup() {

  Serial.begin(9600);
 
  pinMode(RX_PIN, INPUT);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(TX_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);

  digitalWrite(CLK_PIN, 0);
  digitalWrite(TX_PIN, 0);
  digitalWrite(CS_PIN, 1);

  delay(200);
  
  // General reset
  write0(0x01);
  delay(20);
  // Write general control to power on
  write16(0xe0, 0x0180);
  // Per datasheet, start clock
  delay(20);
  // Configure analog loopback, power on
  write16(0xe0, 0x0900);
  // TX control
  write16(0xe1, 0b0111111000010110);
  // RX control
  write16(0xe2, 0b0111111000110110);
  // Clear any crap
  read8(0xe5);
  
  delay(100);
  // Send some data?
  //write8(0xe3, 0xaa);
  write8(0xe3, 0x88);
  
  Serial.println("Started");
}

void loop() {
  uint16_t a = read16(0xe6);
  Serial.println(a, HEX);
  if (a & 0x0040) {
    uint8_t d = read8(0xe5);
    Serial.print("GOT ");
    Serial.println(d, HEX);
  }
  delay(1000);
}
