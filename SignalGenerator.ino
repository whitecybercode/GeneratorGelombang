#include "LiquidCrystal_I2C.h"
#include "AD9833.h"
#include "Rotary.h"

//Batalkan komentar pada baris di bawah ini jika Anda ingin mengubah Phase dan bukan register FREQ
//#definisikan Fase Penggunaan

AD9833 sigGen(10, 24000000000); // disini 4 Inisialisasi AD9833 kita dengan pin FSYNC = 10 dan frekuensi clock master 24MHz jadi 24GHz AD9833 sigGen(10, 24000000);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Inisialisasi LCD
Rotary encoder(3, 2);// Inisialisasi encoder pada pin 2 dan 3 (pin interupsi)

// Variabel yang digunakan untuk memasukkan data dan menelusuri menu
unsigned long encValue;        // Nilai yang digunakan oleh pembuat enkode
unsigned long lastButtonPress; // Nilai yang digunakan oleh tombol debounce
unsigned char lastButtonState;
unsigned char settingsPos[] = {0, 14, 20, 29};
unsigned char button;
volatile unsigned char cursorPos = 0;
unsigned char lastCursorPos = 0;
unsigned char menuState = 0;
const int buttonPin = 4;
int digitPos = 0;
const unsigned long maxFrequency = 14000000000; // disini 3 : menaikkan max requensi
const unsigned int maxPhase = 4095; // Hanya digunakan jika Anda mengaktifkan pengaturan PHASE dan bukan register FREQ
unsigned long newFrequency = 1000;
volatile bool updateDisplay = false;
unsigned long depressionTime;
int freqRegister = 0; // Daftar FREQ default adalah 0
// LCD constants
const String powerState[] = {" ON", "OFF"};
const String mode[] = {"SIN", "TRI"}; // Disini 1 - const String mode[] = {"SIN", "TRI", "CLK"};
// Variabel yang digunakan untuk menyimpan fase, frekuensi, mode dan daya
unsigned char currentMode = 0;
unsigned long frequency0 = 1000;
unsigned long frequency1 = 1000;
unsigned long frequency = frequency0;
unsigned long currFrequency; // Frekuensi saat ini yang digunakan, baik 0 atau 1
unsigned long phase = 0; // Hanya digunakan jika Anda mengaktifkan pengaturan PHASE dan bukan register FREQ
unsigned char currentPowerState = 0;
// Simbol PHI Yunani untuk pergeseran fasa
// Hanya digunakan jika Anda mengaktifkan pengaturan PHASE dan bukan register FREQ
uint8_t phi[8] = {0b01110, 0b00100, 0b01110, 0b10101,
                  0b10101, 0b01110, 0b00100, 0b01110
                 };

void setup() {
  // Inisialisasi LCD, nyalakan lampu latar dan cetak pesan "bootup" selama dua detik
  lcd.begin();
  lcd.backlight();
  lcd.createChar(0, phi); // Karakter PHI khusus untuk LCD
  lcd.home();
  lcd.print("AllAboutCircuits");
  lcd.setCursor(0, 1);
  lcd.print("Signal Generator");
  delay(2000);
  // Menampilkan nilai set awal untuk frekuensi, fase, mode, dan daya
  lcd.clear();
  displayFrequency();
  displayMode();
#ifdef usePhase
  displayPhase();
#endif
  displayPower();
#ifndef usePhase
  displayFreqRegister();
#endif
  // Inisialisasi AD9833 dengan keluaran sinus 1KHz, tidak ada pergeseran fasa untuk keduanya
  // mendaftar dan tetap berada di register FREQ0
  // sigGen.lcdDebugInit(&lcd);
  sigGen.reset(1);
  sigGen.setFreq(frequency0);
  sigGen.setPhase(phase);
  sigGen.setFPRegister(1);
  sigGen.setFreq(frequency1);
  sigGen.setPhase(phase);
  sigGen.setFPRegister(0);
  sigGen.mode(currentMode);
  sigGen.reset(0);

  // Tetapkan pin A dan B dari encoder sebagai interupsi
  attachInterrupt(digitalPinToInterrupt(2), encChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), encChange, CHANGE);
  // Inisialisasi pin sebagai input dengan pull-up diaktifkan dan variabel debounce untuk
  // tombol pembuat enkode
  pinMode(4, INPUT_PULLUP);
  lastButtonPress = millis();
  lastButtonState = 1;
  button = 1;
  // Atur Kursor ke posisi awal
  lcd.setCursor(0, 0);
}

void loop() {
  // Periksa apakah tombol telah ditekan
  checkButton();
  // Perbarui tampilan / display jika diperlukan
  if (updateDisplay == true) {
    displayFrequency();
#ifdef usePhase
    displayPhase();
#endif
    displayPower();
#ifndef usePhase
    displayFreqRegister();
#endif
    displayMode();
    updateDisplay = false;
  }
  // Kita menggunakan variabel menuState untuk mengetahui di mana kita berada dalam menu dan
  // apa yang harus dilakukan jika kita menekan tombol atau menambah/mengurangi melalui
  // pembuat enkode
  // Masuk ke mode pengaturan jika tombol telah ditekan dan tampilan berkedip
  // arahkan kursor ke opsi (menuState 0)
  // Pilih pengaturan (menuState 1)
  // Ubah pengaturan tertentu dan simpan pengaturan (menuState 2-5)

  switch (menuState) {
    // Default state
    case 0: {
        lcd.noBlink();
        if (button == 0) {
          button = 1;
          lcd.setCursor(0, 0);
          lcd.blink();
          menuState = 1;
          cursorPos = 0;
        }
      } break;
    // Settings mode
    case 1: {
        if (button == 0) {
          button = 1;
          // Jika pengaturan di Power cukup aktifkan dan nonaktifkan
          if (cursorPos == 1) {
            currentPowerState = abs(1 - currentPowerState);
            updateDisplay = true;
            menuState = 0;
            if (currentPowerState == 1)
              sigGen.sleep(3); // DAC dan jam dimatikan
            else
              sigGen.sleep(0); // DAC dan jam dihidupkan
          }
          // Jika usePhase belum disetel
#ifndef usePhase
          else if (cursorPos == 2) {
            updateDisplay = true;
            menuState = 0; // kembali ke "menu utama"
            if (freqRegister == 0) {
              freqRegister = 1;
              sigGen.setFPRegister(1);
              frequency = frequency1;
            } else {
              freqRegister = 0;
              sigGen.setFPRegister(0);
              frequency = frequency0;
            }
          }
#endif
          // Jika tidak, tetapkan status baru
          else
            menuState = cursorPos + 2;
        }
        // Pindahkan posisi kursor jika ada perubahan
        if (lastCursorPos != cursorPos) {
          unsigned char realPosR = 0;
          unsigned char realPosC;
          if (settingsPos[cursorPos] < 16)
            realPosC = settingsPos[cursorPos];
          else {
            realPosC = settingsPos[cursorPos] - 16;
            realPosR = 1;
          }
          lcd.setCursor(realPosC, realPosR);
          lastCursorPos = cursorPos;
        }
      } break;
    // Pengaturan frekuensi// Frequency setting
    case 2: {
        // Setiap penekanan tombol akan memungkinkan untuk mengubah nilai digit lainnya
        // atau jika semua digit telah diubah, untuk menerapkan pengaturan
        if (button == 0) {
          button = 1;
          if (digitPos < 7)
            digitPos++;
          else {
            digitPos = 0;
            menuState = 0;
            sigGen.setFreq(frequency);
          }
        } else if (button == 2) {
          button = 1;
          digitPos = 0;
          menuState = 0;
        }
        // Atur kursor berkedip pada digit yang saat ini dapat Anda ubah
        lcd.setCursor(9 - digitPos, 0);
      } break;

    // Pengaturan fase
    case 4: {
        if (button == 0) {
          button = 1;
          if (digitPos < 3)
            digitPos++;
          else {
            digitPos = 0;
            menuState = 0;
            sigGen.setPhase(phase);
          }
        }
        lcd.setCursor(5 - digitPos, 1);
      } break;
    // Ubah mode (sinus/segitiga/jam)
    case 5: {
        if (button == 0) {
          button = 1;
          menuState = 0;
          sigGen.mode(currentMode);
        }
        lcd.setCursor(13 ,1);
      } break;
    // Kalau-kalau kita mengacaukan sesuatu
    default: {
        menuState = 0;
      }
  }
}
// Berfungsi untuk melakukan debounce tombol
// 0 = ditekan, 1 = tertekan, 2 = ditekan lama
void checkButton() {
  if ((millis() - lastButtonPress) > 100) {
    if (digitalRead(buttonPin) != lastButtonState) {
      button = digitalRead(buttonPin);
      lastButtonState = button;
      lastButtonPress = millis();
    }
  }
}

void encChange() {
  // Tergantung pada status menu Anda
  // encoder akan mengubah nilai pengaturan:
  //-+ frekuensi, perubahan antara register FREQ0 dan FREQ1 (atau -+ fase), On/Off, mode
  // atau akan mengubah posisi kursor
  unsigned char state = encoder.process();
  // Arahnya searah jarum jam
  if (state == DIR_CW) {
    switch (menuState) {
      case 1: {
          if (cursorPos == 3)
            cursorPos = 0;
          else
            cursorPos++;
        } break;

      case 2: {
          // Di sini kita menginisialisasi dua variabel.
          // newFrequency adalah nilai frekuensi setelah kita menaikkannya
          // dispDigit adalah digit yang sedang kita modifikasi dan kita peroleh
          // dengan sedikit trik yang rapi, menggunakan operator % bersama dengan pembagian
          // Kita kemudian membandingkan variabel-variabel ini dengan nilai maksimum untuk variabel kita
          // frekuensi, jika semuanya baik, lakukan perubahan
          unsigned long newFrequency = frequency + power(10, digitPos);
          unsigned char dispDigit =
            frequency % power(10, digitPos + 1) / power(10, digitPos);
          if (newFrequency <= maxFrequency && dispDigit < 9) {
            frequency += power(10, digitPos);
            updateDisplay = true;
          }

          if (freqRegister == 0) {
            frequency0 = frequency;
          } else if (freqRegister == 1) {
            frequency1 = frequency;
          }

        } break;

      case 4: {
          // jika usePhase telah ditentukan, perubahan pada encoder akan memvariasikan fasenya
          // nilai (hingga 4096)
          // Implementasi yang lebih baik adalah dengan menggunakan kenaikan pi/4 atau subkelipatan dari
          // pi dimana 2pi = 4096
#ifdef usePhase
          unsigned long newPhase = phase + power(10, digitPos);
          unsigned char dispDigit =
            phase % power(10, digitPos + 1) / power(10, digitPos);
          if (newPhase < maxPhase && dispDigit < 9) {
            phase += power(10, digitPos);
            updateDisplay = true;
          }
#endif
        } break;

      case 5: {
          if (currentMode == 2)
            currentMode = 0;
          else
            currentMode++;
          updateDisplay = true;
        } break;
    }
  }
  // Arahnya berlawanan arah jarum jam
  else if (state == DIR_CCW) {
    switch (menuState) {
      case 1: {
          if (cursorPos == 0)
            cursorPos = 3;
          else
            cursorPos--;
        } break;

      case 2: {
          unsigned long newFrequency = frequency + power(10, digitPos);
          unsigned char dispDigit =
            frequency % power(10, digitPos + 1) / power(10, digitPos);
          if (newFrequency > 0 && dispDigit > 0) {
            frequency -= power(10, digitPos);
            updateDisplay = true;
          }

          if (freqRegister == 0) {
            frequency0 = frequency;
          } else if (freqRegister == 1) {
            frequency1 = frequency;
          }
        } break;

      case 4: {
          // jika usePhase telah ditentukan, perubahan pada encoder akan memvariasikan fasenya
          // nilai (hingga 4096)
          // Implementasi yang lebih baik adalah dengan menggunakan kenaikan pi/4 atau subkelipatan dari
          // pi dimana 2pi = 4096
#ifdef usePhase
          unsigned long newPhase = phase + power(10, digitPos);
          unsigned char dispDigit =
            phase % power(10, digitPos + 1) / power(10, digitPos);
          if (newPhase > 0 && dispDigit > 0) {
            phase -= power(10, digitPos);
            updateDisplay = true;
          }
#endif
        } break;

      case 5: {
          if (currentMode == 0)
            currentMode = 2;
          else
            currentMode--;
          updateDisplay = true;
        } break;
    }
  }
}
// Berfungsi untuk menampilkan frekuensi saat ini di pojok kiri atas
void displayFrequency() {
  unsigned long frequencyToDisplay = frequency;
  lcd.setCursor(0, 0);
  lcd.print("f=");
  for (int i = 7; i >= 0; i--) {
    unsigned char dispDigit = frequencyToDisplay / power(10, i);
    lcd.print(dispDigit);
    frequencyToDisplay -= dispDigit * power(10, i);
  }
  lcd.print("Hz");
}
// Berfungsi untuk menampilkan status daya (ON/OFF) di pojok kanan atas
void displayPower() {
  lcd.setCursor(13, 0);
  lcd.print(powerState[currentPowerState]);
}
// Berfungsi untuk menampilkan mode di pojok kanan bawah
void displayMode() {
  lcd.setCursor(13, 1);
  lcd.print(mode[currentMode]);
}
// Berfungsi untuk menampilkan mode di pojok kiri bawah
// Hanya digunakan jika Anda mengaktifkan pengaturan PHASE dan bukan register FREQ
void displayPhase() {
  unsigned int phaseToDisplay = phase;
  lcd.setCursor(0, 1);
  lcd.write(0);
  lcd.print("=");
  for (int i = 3; i >= 0; i--) {
    unsigned int dispDigit = phaseToDisplay / power(10, i);
    lcd.print(dispDigit);
    phaseToDisplay -= dispDigit * power(10, i);
  }
}
// Berfungsi untuk menampilkan register FREQ (0 atau 1) di kiri bawah
// sudut
void displayFreqRegister() {
  lcd.setCursor(0, 1);
  lcd.print("FREQ");
  lcd.print(freqRegister);
}

unsigned long power(int a, int b) {
  unsigned long res;
  if (b == 0) {
    res = 1;
  } else {
    res = a;
    for (int i = 0; i < (b - 1); i++) {
      res *= a;
    }
  }
  return res;
}
