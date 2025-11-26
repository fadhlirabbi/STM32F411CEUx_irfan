#include <Arduino.h>

// ==========================================
// 1. DEFINISI PIN (MAPPING)
// ==========================================

// LED (Urutan dari Kiri: PB9, PB8, PB7, PB6, PB5)
#define PIN_LED1 PB9  // Task 1: Timer 2 (Lambat)
#define PIN_LED2 PB8  // Task 1: Timer 4 (Sedang)
#define PIN_LED3 PB7  // Task 1: Timer 3 (Cepat)
#define PIN_LED4 PB6  // Task 2: Indikator Debounce
#define PIN_LED5 PB5  // Task 3: Push to Blink

// TOMBOL (Hanya yang dipakai: PB12 & PB13)
#define BTN_DEBOUNCE     PB12 // Tombol 1 (Kanan) -> Soal No 2
#define BTN_PUSH_BLINK   PB13 // Tombol 2 -> Soal No 3

// ==========================================
// 2. OBJEK & VARIABEL GLOBAL
// ==========================================

// Objek Hardware Timer (Untuk Soal No 1)
HardwareTimer *MyTim2 = new HardwareTimer(TIM2);
HardwareTimer *MyTim3 = new HardwareTimer(TIM3);
HardwareTimer *MyTim4 = new HardwareTimer(TIM4);

// Variabel untuk Debounce & Semaphore (Soal No 2)
volatile bool semaphoreSignal = false; 
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 500; // 50ms waktu stabil

// Variabel untuk Push to Blink (Soal No 3)
unsigned long previousBlinkMillis = 0;
const long blinkInterval = 80; // Kecepatan kedip saat ditahan

// ==========================================
// 3. FUNGSI INTERRUPT (CALLBACK)
// ==========================================

// Callback Timer 2: Toggle LED 1
void Update_LED1_T2() {
    digitalWrite(PIN_LED1, !digitalRead(PIN_LED1));
}

// Callback Timer 4: Toggle LED 2
void Update_LED2_T4() {
    digitalWrite(PIN_LED2, !digitalRead(PIN_LED2));
}

// Callback Timer 3: Toggle LED 3
void Update_LED3_T3() {
    digitalWrite(PIN_LED3, !digitalRead(PIN_LED3));
}

// ==========================================
// 4. SETUP
// ==========================================
void setup() {
    // A. Konfigurasi Pin LED
    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);
    pinMode(PIN_LED3, OUTPUT);
    pinMode(PIN_LED4, OUTPUT);
    pinMode(PIN_LED5, OUTPUT);

    // B. Konfigurasi Pin Tombol (Pull-Up Internal)
    // PB14, PB3, PB4 sudah dihapus
    pinMode(BTN_DEBOUNCE, INPUT_PULLUP);
    pinMode(BTN_PUSH_BLINK, INPUT_PULLUP);

    // C. Konfigurasi TIMER (Soal No 1)
    
    // --- TIMER 2 (LED 1 - Lambat: 1 Hz / 1 Detik) ---
    MyTim2->setOverflow(1, HERTZ_FORMAT); 
    MyTim2->attachInterrupt(Update_LED1_T2);
    MyTim2->resume();

    // --- TIMER 4 (LED 2 - Sedang: 2 Hz / 0.5 Detik) ---
    MyTim4->setOverflow(2, HERTZ_FORMAT);
    MyTim4->attachInterrupt(Update_LED2_T4);
    MyTim4->resume();

    // --- TIMER 3 (LED 3 - Cepat: 5 Hz / 0.2 Detik) ---
    MyTim3->setOverflow(5, HERTZ_FORMAT);
    MyTim3->attachInterrupt(Update_LED3_T3);
    MyTim3->resume();
}

// ==========================================
// 5. MAIN LOOP
// ==========================================
void loop() {
    unsigned long currentMillis = millis();

    // ------------------------------------------
    // TUGAS 2: DEBOUNCING BUTTON (PB12 -> LED4)
    // ------------------------------------------
    int reading = digitalRead(BTN_DEBOUNCE);

    // Jika status tombol berubah
    if (reading != lastButtonState) {
        lastDebounceTime = currentMillis; 
    }

    // Jika sudah stabil selama 50ms
    if ((currentMillis - lastDebounceTime) > debounceDelay) {
        static int stableState = HIGH;
        
        if (reading != stableState) {
            stableState = reading;
            
            // Deteksi penekanan (Active LOW)
            if (stableState == LOW) {
                semaphoreSignal = true; // Kirim sinyal
            }
        }
    }
    lastButtonState = reading;

    // Cek Semaphore
    if (semaphoreSignal == true) {
        // Toggle LED 4
        digitalWrite(PIN_LED4, !digitalRead(PIN_LED4));
        semaphoreSignal = false; // Reset sinyal
    }

    // ------------------------------------------
    // TUGAS 3: PUSH TO BLINK (PB13 -> LED5)
    // ------------------------------------------
    if (digitalRead(BTN_PUSH_BLINK) == LOW) {
        if (currentMillis - previousBlinkMillis >= blinkInterval) {
            previousBlinkMillis = currentMillis;
            digitalWrite(PIN_LED5, !digitalRead(PIN_LED5));
        }
    } else {
        // Matikan jika dilepas
        digitalWrite(PIN_LED5, LOW);
    }
}