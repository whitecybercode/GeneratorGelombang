/* Pengendali encoder putar untuk Arduino. v1.1
 *
 * Hak Cipta 2011 Ben Buxton. Berlisensi di bawah GNU GPL Versi 3.
 * Hubungi: bb@cactii.net
 *
 * Encoder putar mekanis tipikal mengeluarkan kode abu-abu dua bit
 * pada 3 pin keluaran. Setiap langkah dalam keluaran (sering disertai
 * dengan 'klik' fisik) menghasilkan urutan keluaran tertentu
 * kode pada pin.
 *
 * Ada 3 pin yang digunakan untuk pengkodean putar - satu pin umum dan
 * dua pin 'bit'.
 *
 * Berikut ini adalah urutan kode tipikal pada output kapan
 * berpindah dari satu langkah ke langkah berikutnya:
 *
 * Posisi Bit1 Bit2
 * ----------------------
 * Langkah  1 0 0
 *        1/4 1 0
 *        1/2 1 1
 *        3/4 ​​0 1
 * Langkah 2 0 0
 *
 * Dari tabel ini, kita dapat melihatnya ketika berpindah dari satu 'klik' ke
 * selanjutnya ada 4 perubahan pada kode keluaran.
 *
 * - Dari awal 0 - 0, Bit1 menjadi tinggi, Bit0 tetap rendah.
 * - Kemudian kedua bit menjadi tinggi, di tengah langkah.
 * - Kemudian Bit1 menjadi rendah, tetapi Bit2 tetap tinggi.
 * - Akhirnya di akhir langkah, kedua bit kembali ke 0.
 *
 * Mendeteksi arah itu mudah - tabelnya masuk ke arah yang lain
 * arah (baca ke atas, bukan ke bawah).
 *
 * Untuk memecahkan kode ini, kami menggunakan mesin negara sederhana. Setiap kali keluarannya
 * kode berubah, mengikuti status, hingga akhirnya bernilai langkah penuh
 * kode diterima (dalam urutan yang benar). Pada akhir 0-0, itu kembali
 * nilai yang menunjukkan langkah dalam satu arah atau lainnya.
 *
 * Dimungkinkan juga untuk menggunakan mode 'setengah langkah'. Ini hanya memunculkan sebuah peristiwa
 * pada posisi 0-0 dan 1-1. Ini mungkin berguna bagi sebagian orang
 * pembuat enkode tempat Anda ingin mendeteksi semua posisi.
 *
 * Jika terjadi keadaan tidak valid (misalnya kita langsung dari '0-1'
 * ke '1-0'), mesin negara direset ke awal hingga 0-0 dan
 * kode valid berikutnya muncul.
 *
 * Keuntungan terbesar menggunakan mesin negara dibandingkan algoritma lainnya
 * adalah bahwa ini memiliki debounce bawaan. Algoritme lain memancarkan palsu
 * keluaran dengan sakelar memantul, tetapi yang ini hanya akan beralih di antara keduanya
 * sub-negara bagian hingga pantulan teratasi, lalu lanjutkan di sepanjang negara bagian tersebut
 * mesin.
 * Efek samping dari debounce adalah rotasi yang cepat dapat menyebabkan langkah-langkah
 * dilewati. Dengan tidak memerlukan debounce, putaran cepat bisa akurat
 * diukur.
 * Keuntungan lainnya adalah kemampuan menangani kondisi buruk dengan baik
 * karena EMI, dll.
 * Ini juga jauh lebih sederhana daripada yang lain - tabel keadaan statis dan lebih sedikit
 * dari 10 baris logika.
 */

#include "Arduino.h"
#include "Rotary.h"

/*
 * Tabel negara bagian di bawah ini memiliki, untuk setiap negara bagian (baris), negara bagian baru
 * untuk mengatur berdasarkan keluaran encoder berikutnya. Dari kiri ke kanan,
 * tabel, output encoder adalah 00, 01, 10, 11, dan nilainya
 * pada posisi tersebut adalah keadaan baru yang akan ditetapkan.
 */

#define R_START 0x0

#ifdef HALF_STEP
// Gunakan tabel keadaan setengah langkah (memancarkan kode pada 00 dan 11)
#define R_CCW_BEGIN 0x1
#define R_CW_BEGIN 0x2
#define R_START_M 0x3
#define R_CW_BEGIN_M 0x4
#define R_CCW_BEGIN_M 0x5
const unsigned char ttable[6][4] = {
  // R_START (00)
  {R_START_M,            R_CW_BEGIN,     R_CCW_BEGIN,  R_START},
  // R_CCW_BEGIN
  {R_START_M | DIR_CCW, R_START,        R_CCW_BEGIN,  R_START},
  // R_CW_BEGIN
  {R_START_M | DIR_CW,  R_CW_BEGIN,     R_START,      R_START},
  // R_START_M (11)
  {R_START_M,            R_CCW_BEGIN_M,  R_CW_BEGIN_M, R_START},
  // R_CW_BEGIN_M
  {R_START_M,            R_START_M,      R_CW_BEGIN_M, R_START | DIR_CW},
  // R_CCW_BEGIN_M
  {R_START_M,            R_CCW_BEGIN_M,  R_START_M,    R_START | DIR_CCW},
};
#else
// Gunakan tabel status langkah penuh (hanya mengeluarkan kode pada 00)
#define R_CW_FINAL 0x1
#define R_CW_BEGIN 0x2
#define R_CW_NEXT 0x3
#define R_CCW_BEGIN 0x4
#define R_CCW_FINAL 0x5
#define R_CCW_NEXT 0x6

const unsigned char ttable[7][4] = {
  // R_START
  {R_START,    R_CW_BEGIN,  R_CCW_BEGIN, R_START},
  // R_CW_FINAL
  {R_CW_NEXT,  R_START,     R_CW_FINAL,  R_START | DIR_CW},
  // R_CW_BEGIN
  {R_CW_NEXT,  R_CW_BEGIN,  R_START,     R_START},
  // R_CW_NEXT
  {R_CW_NEXT,  R_CW_BEGIN,  R_CW_FINAL,  R_START},
  // R_CCW_BEGIN
  {R_CCW_NEXT, R_START,     R_CCW_BEGIN, R_START},
  // R_CCW_FINAL
  {R_CCW_NEXT, R_CCW_FINAL, R_START,     R_START | DIR_CCW},
  // R_CCW_NEXT
  {R_CCW_NEXT, R_CCW_FINAL, R_CCW_BEGIN, R_START},
};
#endif

/*
 * Konstruktor. Setiap arg adalah nomor pin untuk setiap kontak encoder.
 */
Rotary::Rotary(char _pin1, char _pin2) {
  // Assign variables.
  pin1 = _pin1;
  pin2 = _pin2;
  // Set pins to input.
  pinMode(pin1, INPUT);
  pinMode(pin2, INPUT);
#ifdef ENABLE_PULLUPS
  digitalWrite(pin1, HIGH);
  digitalWrite(pin2, HIGH);
#endif
  // Initialise state.
  state = R_START;
}

unsigned char Rotary::process() {
  // Grab state of input pins.
  unsigned char pinstate = (digitalRead(pin2) << 1) | digitalRead(pin1);
  // Determine new state from the pins and state table.
  state = ttable[state & 0xf][pinstate];
  // Return emit bits, ie the generated event.
  return state & 0x30;
}
