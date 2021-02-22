#define BAR0 2
#define BAR1 3
#define BAR2 5
#define BAR3 6

#define MARK 7
#define PRESSURE A0
#define RECORD A1
#define SD_CS 4

#include <OneButton.h>
#include <SPI.h>
#include <SD.h>

uint16_t LEDS = 0;

OneButton RecordButton(RECORD, HIGH, false);

int log_file_index = 0;
File f;

void setup() {
  Serial.begin(115200);
  
  if(SD.begin(SD_CS)) {
    Serial.println("SD Ready");
  } else {
    Serial.println("ERR_NO_SD");
  }
  
  pinMode(BAR0, INPUT);
  pinMode(BAR1, INPUT);
  pinMode(BAR2, INPUT);
  pinMode(BAR3, INPUT);

  pinMode(PRESSURE, INPUT);
  pinMode(RECORD, INPUT);
  pinMode(MARK, INPUT);

  RecordButton.attachClick(onRecordClick);
}

byte freeze = ~(bit(BAR0) | bit(BAR1) | bit(BAR2) | bit(BAR3));
byte mask, vals;

void openLogFile() {
  if (f) {
    f.close();
  }

  char filename[20] = "";
  do {
    strlcpy(filename, (String("log-") + String(log_file_index) + ".csv").c_str(), 20);
    log_file_index++;
  } while(log_file_index < 100 && SD.exists(filename));

  if (log_file_index >= 100) {
    Serial.println("Could not find a filename :|");
  }

  f = SD.open(filename, FILE_WRITE);

  if (!f) {
    Serial.print("Error creating file: ");
    Serial.println(filename);
  } else {
    Serial.print("Logging to file: ");
    Serial.println(filename);
  }
}

void setBar(byte val) {
  mask = bitRead(val, 0 + 4) << BAR0
            | bitRead(val, 1 + 4) << BAR1
            | bitRead(val, 2 + 4) << BAR2
            | bitRead(val, 3 + 4) << BAR3;
            
  vals = bitRead(val, 0) << BAR0
            | bitRead(val, 1) << BAR1
            | bitRead(val, 2) << BAR2
            | bitRead(val, 3) << BAR3;  

  DDRD = (DDRD & freeze) | mask;
  PORTD = (PORTD & freeze) | vals;
}

const byte bars[] = {
  0b11000100,
  0b10100010,
  0b10010001,
  0b11001000,
  0b01100010,
  0b01010001,
  0b10101000,
  0b01100100,
  0b00110001,
  0b10011000,
  0b01010100,
  0b00110010,
};

byte val = 0;
long last = 0;
byte count = 0;
bool recording = false;
long recordingStarted = 0;
byte led_index = 0;
long led_change = 0;

void loop() {
  if (digitalRead(MARK) == LOW) {
    LEDS |= 0x400;
  } else {
    LEDS &= ~0x400;
  }

  if (LEDS & _BV(led_index)) {
    setBar(bars[led_index]);
  } else {
//      setBar(0);
  }
  led_index = (led_index + 1) % 12;
  led_change = millis();

  RecordButton.tick();

  if (millis() - last > 1000/60) {
    int av = analogRead(PRESSURE);
    float volt = ((float)av / 1023.0);
    val = volt * 10.0;
    LEDS &= 0xC00;
    LEDS |= 1 << 9-val;
    String data;
    if (recording) {
      data += String(millis() - recordingStarted);
    } else {
      data += String(0);
    }
    data += String(",");
    data += String(av);
    data += String(",");
    data += String(!digitalRead(MARK));
    data += String("\n");
//    Serial.print(data);
    if (f) {
      f.print(data);
    }
    last = millis();
  }
}

void onRecordClick() {
  recording = !recording;
  recordingStarted = millis();
  if (recording) {
    LEDS |= 0x800;
    openLogFile();
  } else {
    LEDS &= ~0x800;
    if (f) {
      f.close();
    }
  }
}
