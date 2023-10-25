#ifndef AD9833_H
#define AD9833_H

#include <SPI.h>
#include "LiquidCrystal_I2C.h"

class AD9833
{
  public:
    //Inisialisasi AD9833
    //_FYNC adalah pin pada UC tempat FYNC terhubung
    //_mclk adalah frekuensi generator kristal 
    AD9833(int _FSYNC, unsigned long _mclk);
    //Atur dan dapatkan frekuensinya
    void setFreq(unsigned long _freq);
    unsigned long getFreq();
    //Atur dan dapatkan fase
    void setPhase(int _phase);
    int getPhase();
    //Atur dan dapatkan untuk register kendali
    void setCtrlReg(unsigned long _controlRegister);
    int getCtrlReg();
    //Kirim data ke AD9833
    void writeData(int data);
    //Kirim register kontrol
    void writeCtrlReg();
    //Perbarui register kontrol dengan nilai yang sesuai untuk mode tidur/reset/mode
    //s = 0 Tidak ada listrik mati
    //s = 1 DAC dimatikan
    //s = 2 Jam internal dimatikan
    //s = 3 DAC dan jam internal dimatikan
    void sleep(int s);
    //r = 0 Disable Reset
    //r = 1 Enable Reset
    void reset(int r); // Disini 2 -> bisa diganti
    //m = 0 Sine
    //m = 1 Triangle
    //m = 2 Clock (rectangle) 
    void mode(int m);
    //r = 0 FREQ0 and PHASE0 are used
    //r = 1 FREQ1 and PHASE1 are used
    void setFPRegister(int r);
    void lcdDebugInit(LiquidCrystal_I2C *);
    void lcdDebug(String);

  private:
    int FSYNC;          //FSYNC Pin AD9833 harus dihubungkan ke pin GPIO
    unsigned long freq; //Output frequency
    int phase;          //Signal phase shift
    unsigned long mclk; //External oscillator frequency
    int fqRegister;    //Switch between Frequency and Phase register 1/0
    int pRegister;
    int controlRegister;
    LiquidCrystal_I2C *debugger; //Men-debug sesuatu melalui I2C
    boolean debugging;
};

#endif
