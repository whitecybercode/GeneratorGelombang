#include <SPI.h>
#include "AD9833.h"

AD9833::AD9833(int _FSYNC, unsigned long _mclk) {
  FSYNC = _FSYNC;
  pinMode(FSYNC, OUTPUT);     // Tetapkan pin FSYNC sebagai masukan
  mclk = _mclk;               // Atur osilator yang digunakan, misalnya 24MHz
  controlRegister = 0x2000;   // Daftar kontrol default : FREQ0, PHASE0, Sine
  fqRegister = 0x4000;        // Daftar frekuensi default adalah 0
  pRegister = 0xC000;         // Register fase default adalah 0
  SPI.begin();                // Inisialisasi SPI BUS
  SPI.setDataMode(SPI_MODE2); // Atur SPI di mode2, ini harus dipindahkan
  // metode ketika SPI.transfer dipanggil jika Anda
  // memiliki beberapa perangkat yang menggunakan bus SPI
}

void AD9833::writeData(int data) {
  // Pin FSYNC harus ditarik rendah ketika data baru diterima oleh AD9833
  digitalWrite(FSYNC, LOW);
  // Kirim 8 MSBs data pertama
  SPI.transfer(highByte(data));
  // Kirim 8 LSBs data terakhir
  SPI.transfer(lowByte(data));
  // Atur pin FSYNC ke transaksi SPI tinggi lalu akhiri
  digitalWrite(FSYNC, HIGH);
  // debugging
  if (debugging) {
    lcdDebug(String(data, HEX));
  }
}

void AD9833::setFreq(unsigned long _freq) {
  // Periksa dulu apakah data yang diterima baik-baik saja
  unsigned long freqReg;
  // Frekuensi tidak boleh negatif
  if (_freq < 0) {
    freqReg = 0;
    freq = 0;
  }
  // Jika frekuensinya lebih dari frekuensi maksimum, atur saja ke maksimum
  else if (_freq > mclk) {
    freqReg = pow(2, 28) - 1;
    freq = mclk / 2;
  }
  // Jika semuanya baik-baik saja, hitung freqReg dengan mengetahui keluaran analognya
  // (mclk/2^28) * freqReg
  else {
    freq = _freq;
    freqReg = (freq * pow(2, 28)) / mclk;
  }
  // Inisialisasi dua variabel sepanjang 16bit yang kita gunakan untuk membagi
  // freqReg dalam dua kata
  // setel D15 ke 0 dan D14 ke 1 untuk memasukkan data ke register FREQ0/1
  int MSW = ((int)(freqReg >> 14)) | fqRegister; // Keluarkan 14bit pertama
  // dan atur D15 ke 0 dan D14 ke
  // 1 atau sebaliknya tergantung pada
  // frekuensi freq
  int LSW = ((int)(freqReg & 0x3FFF)) |
            fqRegister; // Ambil hanya 14bit terakhir menggunakan mask dan setel D15 ke
  // 0 dan D14 ke 1 atau sebaliknya tergantung pada FREQ reg
  // Kirimkan data, kata yang paling penting terlebih dahulu
  writeData(LSW);
  writeData(MSW);
}
unsigned long AD9833::getFreq() {
  return freq;
}
void AD9833::setPhase(int _phase) {
  // Fase tidak boleh negatif
  if (_phase < 0) {
    phase = 0;
  }
  // Fase maksimum adalah 2^12
  else if (_phase >= 4096) {
    phase = 4096 - 1;
  }
  // Jika semuanya baik-baik saja, atur nilai fase baru
  else {
    phase = _phase;
  }
  // Ekstrak 12 bit dari freqReg dan atur D15-1, D14-1, D13-0, D12-X ke
  // memasukkan data ke dalam register PHASE0/1
  int phaseData = phase | pRegister;
  int LSW = (phase & 0x3FFF) | pRegister;
  writeData(phaseData);
}
int AD9833::getPhase() {
  return phase;
}

void AD9833::setCtrlReg(unsigned long _controlRegister) {
  // Pastikan saja dua bit pertama disetel ke 0
  controlRegister = _controlRegister & 0x3FFF;
  writeCtrlReg();
}

int AD9833::getCtrlReg() {
  return controlRegister;
}

void AD9833::writeCtrlReg() {
  writeData(controlRegister);
}

void AD9833::sleep(int s) {
  switch (s) {

    case 0: {
        controlRegister &= 0xFF3F; // Tidak ada pemadaman listrik: D7-0 dan D6-0
      } break;

    case 1: {
        controlRegister &= 0xFF7F; // DAC dimatikan: D7-0 dan D6-1
        controlRegister |= 0x0040;
      } break;

    case 2: {
        controlRegister &= 0xFFBF; // Jam internal dinonaktifkan: D7-1 dan D6-0
        controlRegister |= 0x0080;
      } break;

    case 3: {
        controlRegister |=
          0x00C0; // DAC dimatikan dan jam internal dinonaktifkan
      } break;
  }
  // Perbarui register kontrol
  writeCtrlReg();
}

void AD9833::reset(int r) {
  if (r == 0) {
    controlRegister &= 0xFEFF; // Set D8 to 0
  }

  else if (r == 1) {
    controlRegister |= 0x0100; // Set D8 to 1
  }
  writeCtrlReg();
}

void AD9833::mode(int m) {
  switch (m) {
    case 0: {
        controlRegister &= 0xFFDD; // Output sine: D5-0 and D1-0
      } break;
    case 1: {
        controlRegister &= 0xFFDF; // Output triangle: D5-0 and D1-1
        controlRegister |= 0x0002;
      } break;
    case 2: {
        controlRegister &= 0xFFFD; // Output clock (rectangle): D5-1 and D1-0
        controlRegister |= 0x0020;
      } break;
  }
  writeCtrlReg();
}

void AD9833::setFPRegister(int r) {
  if (r == 0) {
    controlRegister &= 0xF3FF; // Atur D11 dan D10 di register kontrol ke 0
    fqRegister = 0x4000; // Tetapkan D15 ke 0 dan D14 ke 1 pada variabel yang diinginkan
    // nanti pilih register FREQ0
    pRegister =
      0xC000; // Atur D15 ke 1 dan D14 ke 1 dan D13 ke 0 untuk register PHASE
  } else if (r == 1) {
    controlRegister |= 0x0C00; // Tetapkan D11 dan D10 di register kontrol ke 1
    fqRegister = 0x8000; // Tetapkan D15 ke 1 dan D14 ke 0 pada variabel yang diinginkan
    // nanti pilih register FREQ1
    pRegister =
      0xD000; // Atur D15 ke 1 dan D14 ke 1 dan D13 ke 1 untuk register PHASE
  }
  writeCtrlReg();
}

void AD9833::lcdDebugInit(LiquidCrystal_I2C *_debugger) {
  debugger = _debugger;
  debugging = true;
}

void AD9833::lcdDebug(String message) {
  if (debugging) {
    debugger->setCursor(0, 1);
    debugger->print(message);
  }
}
