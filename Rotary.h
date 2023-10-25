/*
 * Perpustakaan encoder putar untuk Arduino.
 */

#ifndef rotary_h
#define rotary_h

#include "Arduino.h"

// Aktifkan ini untuk mengeluarkan kode dua kali per langkah.
//#definisikan SETENGAH_LANGKAH

// Aktifkan pull-up yang lemah
#define ENABLE_PULLUPS

// Nilai yang dikembalikan oleh 'proses'
// Belum ada langkah yang lengkap.
#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Anti-clockwise step.
#define DIR_CCW 0x20

class Rotary
{
  public:
    Rotary(char, char);
    // Process pin(s)
    unsigned char process();
  private:
    unsigned char state;
    unsigned char pin1;
    unsigned char pin2;
};

#endif
 
