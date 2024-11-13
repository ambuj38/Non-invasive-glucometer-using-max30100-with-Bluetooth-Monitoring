#include <Wire.h>
#include <EEPROM.h>
#include "MAX30100_PulseOximeter.h"
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h> // Library for Bluetooth communication

#define ENABLE_MAX30100 1

#define REPORTING_PERIOD_MS 5000

PulseOximeter pox;

uint32_t tsLastReport = 0;
uint32_t tsLastSave = 0;

float glucose_records[3] = {0.0, 0.0, 0.0}; // Array to store the last three records

// Initialize the LCD (assuming a 16x2 LCD with I2C address 0x27)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// HC-05 Bluetooth module connections: RX to Arduino pin 10, TX to pin 11
SoftwareSerial bluetooth(10, 11);

// Callback (registered below) fired when a pulse is detected
void onBeatDetected() {
    Serial.println("Beat!");
}

void setup() {
    Serial.begin(115200);
    bluetooth.begin(9600); // Initialize Bluetooth with baud rate of 9600

    Serial.println("Initializing LCD");
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Pulse Oximeter");
    delay(2000);
    lcd.clear();

    Serial.print("Initializing pulse oximeter..");
    if (!pox.begin()) {
        Serial.println("FAILED");
        while (1);
    } else {
        Serial.println("SUCCESS");
    }

    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);

    load_glucose_records();
    print_glucose_records(); // Print loaded records to Serial
}

void loop() {
    pox.update();
    int bpm = 0;
    int spo2 = 0;
    float glucose_level = 0.0;

    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        bpm = pox.getHeartRate();
        spo2 = pox.getSpO2();

        if (bpm > 0 && spo2 > 0) {
            glucose_level = (-0.6605686 * bpm) + (-1.3400007 * spo2) + 260.3409673409161;

        }

        Serial.print("Heart rate: ");
        Serial.println(bpm);
        Serial.print("SpO2: ");
        Serial.println(spo2);
        Serial.print("Glucose Level: ");
        Serial.println(glucose_level);

        // Send data to Bluetooth
        bluetooth.print("Heart rate: ");
        bluetooth.println(bpm);
        bluetooth.print("SpO2: ");
        bluetooth.println(spo2);
        bluetooth.print("Glucose Level: ");
        bluetooth.println(glucose_level);

        tsLastReport = millis();
        display_data(bpm, spo2, glucose_level);
    }

    // Save the glucose level if it is smaller than 500 every 10 seconds
    if (glucose_level < 500.0 && glucose_level > 0 && millis() - tsLastSave > 10000) {
        save_glucose_level(glucose_level);
        tsLastSave = millis();
    }
}

void display_data(int bpm, int spo2, float glucose_level) {
    lcd.clear();
    
    // Display BPM and SpO2 on the first line
    lcd.setCursor(0, 0);
    lcd.print("BPM:");
    lcd.print(bpm);
    lcd.print(" SpO2:");
    lcd.print(spo2);

    // Display glucose level on the second line
    lcd.setCursor(0, 1);
    if (glucose_level > 500.0 || glucose_level < 0) {
        lcd.print("Glucose: MEAS");
    } else {
        lcd.print("Glucose:");
        lcd.print(glucose_level);
    }
}

void save_glucose_level(float glucose_level) {
    // Shift the old records
    glucose_records[2] = glucose_records[1];
    glucose_records[1] = glucose_records[0];
    glucose_records[0] = glucose_level;

    // Save the records to EEPROM
    for (int i = 0; i < 3; i++) {
        EEPROM.put(i * sizeof(float), glucose_records[i]);
    }

    print_glucose_records(); // Print saved records to Serial for debugging
}

void load_glucose_records() {
    // Load the records from EEPROM
    for (int i = 0; i < 3; i++) {
        EEPROM.get(i * sizeof(float), glucose_records[i]);
    }
}

void print_glucose_records() {
    // Print glucose records to Serial for debugging
    Serial.println("Glucose Records:");
    for (int i = 0; i < 3; i++) {
        Serial.print(i);
        Serial.print(": ");
        Serial.println(glucose_records[i]);
    }
}
