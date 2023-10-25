#include "LiquidCrystal_I2C.h"
#include <inttypes.h>
#include <Arduino.h>
#include <Wire.h>

// Saat tampilan menyala, dikonfigurasi sebagai berikut:
//
// 1. Tampilan jelas
// 2. Kumpulan fungsi:
// DL = 1; Data antarmuka 8-bit
// N = 0; Tampilan 1 baris
// F = 0; Font karakter 5x8 titik
// 3. Menampilkan kontrol hidup/mati:
// D = 0; Tampilan mati
// C = 0; Kursor mati
// B = 0; Berkedip
// 4. Kumpulan mode masuk:
// Saya/D = 1; Kenaikan sebesar 1
// S = 0; Tidak ada perubahan
//
// Namun perlu diperhatikan bahwa menyetel ulang Arduino tidak menyetel ulang LCD, jadi kami
// tidak bisa berasumsi bahwa keadaannya seperti itu ketika sketsa dimulai (dan
// Konstruktor LiquidCrystal dipanggil).

LiquidCrystal_I2C::LiquidCrystal_I2C(uint8_t lcd_addr, uint8_t lcd_cols, uint8_t lcd_rows, uint8_t charsize)
{
	_addr = lcd_addr;
	_cols = lcd_cols;
	_rows = lcd_rows;
	_charsize = charsize;
	_backlightval = LCD_BACKLIGHT;
}

void LiquidCrystal_I2C::begin() {
	Wire.begin();
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

	if (_rows > 1) {
		_displayfunction |= LCD_2LINE;
	}

// untuk beberapa tampilan 1 baris Anda dapat memilih font setinggi 10 piksel
	if ((_charsize != 0) && (_rows == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

// LIHAT HALAMAN 45/46 UNTUK SPESIFIKASI INISIALISASI!
// menurut lembar data, kita memerlukan setidaknya 40 ms setelah daya naik di atas 2,7V
// sebelum mengirimkan perintah. Arduino bisa menyala sebelum 4.5V jadi kita tunggu 50
	delay(50);

	// Sekarang kita tarik RS dan R/W rendah untuk memulai perintah
	expanderWrite(_backlightval);	// setel ulang expander dan matikan lampu latar (Bit 8 =1)
	delay(1000);

//masukkan LCD ke mode 4 bit
// ini menurut lembar data hitachi HD44780
// gambar 24, hal 46

	// kita mulai dalam mode 8bit, coba atur mode 4 bit
	write4bits(0x03 << 4);
	delayMicroseconds(4500); // wait min 4.1ms

	// second try
	write4bits(0x03 << 4);
	delayMicroseconds(4500); // wait min 4.1ms

	// third go!
	write4bits(0x03 << 4);
	delayMicroseconds(150);

	// terakhir, setel ke antarmuka 4-bit
	write4bits(0x02 << 4);

	// atur # baris, ukuran font, dll.
	command(LCD_FUNCTIONSET | _displayfunction);

	// menyalakan layar tanpa kursor atau berkedip secara default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	display();

	// bersihkan
	clear();

	// Inisialisasi ke arah teks default (untuk bahasa Romawi)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

	// atur mode entri
	command(LCD_ENTRYMODESET | _displaymode);

	home();
}

/********** perintah tingkat tinggi, untuk pengguna! */
void LiquidCrystal_I2C::clear(){
	command(LCD_CLEARDISPLAY);// hapus tampilan, setel posisi kursor ke nol
	delayMicroseconds(2000);  // perintah ini memerlukan waktu lama!
}

void LiquidCrystal_I2C::home(){
	command(LCD_RETURNHOME);  // atur posisi kursor ke nol
	delayMicroseconds(2000);  // perintah ini memerlukan waktu lama!
}

void LiquidCrystal_I2C::setCursor(uint8_t col, uint8_t row){
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if (row > _rows) {
		row = _rows-1;    // kita menghitung baris dimulai dengan w/0
	}
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Menghidupkan/mematikan tampilan (dengan cepat)
void LiquidCrystal_I2C::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Mengaktifkan / menonaktifkan kursor yang bergaris bawah
void LiquidCrystal_I2C::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Menghidupkan dan mematikan kursor yang berkedip
void LiquidCrystal_I2C::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LiquidCrystal_I2C::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Perintah ini menggulir tampilan tanpa mengubah RAM
void LiquidCrystal_I2C::scrollDisplayLeft(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LiquidCrystal_I2C::scrollDisplayRight(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// Ini untuk teks yang mengalir dari Kiri ke Kanan
void LiquidCrystal_I2C::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Ini untuk teks yang mengalir dari Kanan ke Kiri
void LiquidCrystal_I2C::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Ini akan 'membenarkan' teks dari kursor
void LiquidCrystal_I2C::autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Ini akan 'rata kiri' teks dari kursor
void LiquidCrystal_I2C::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Memungkinkan kita mengisi 8 lokasi CGRAM pertama
// dengan karakter khusus
void LiquidCrystal_I2C::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7; // kami hanya memiliki 8 lokasi 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
	}
}

// Mematikan/menghidupkan lampu latar (opsional).
void LiquidCrystal_I2C::noBacklight(void) {
	_backlightval=LCD_NOBACKLIGHT;
	expanderWrite(0);
}

void LiquidCrystal_I2C::backlight(void) {
	_backlightval=LCD_BACKLIGHT;
	expanderWrite(0);
}
bool LiquidCrystal_I2C::getBacklight() {
  return _backlightval == LCD_BACKLIGHT;
}


/*********** perintah tingkat menengah, untuk mengirim data/cmds */

inline void LiquidCrystal_I2C::command(uint8_t value) {
	send(value, 0);
}

inline size_t LiquidCrystal_I2C::write(uint8_t value) {
	send(value, Rs);
	return 1;
}


/************perintah dorongan data tingkat rendah **********/

// menulis perintah atau data
void LiquidCrystal_I2C::send(uint8_t value, uint8_t mode) {
	uint8_t highnib=value&0xf0;
	uint8_t lownib=(value<<4)&0xf0;
	write4bits((highnib)|mode);
	write4bits((lownib)|mode);
}

void LiquidCrystal_I2C::write4bits(uint8_t value) {
	expanderWrite(value);
	pulseEnable(value);
}

void LiquidCrystal_I2C::expanderWrite(uint8_t _data){
	Wire.beginTransmission(_addr);
	Wire.write((int)(_data) | _backlightval);
	Wire.endTransmission();
}

void LiquidCrystal_I2C::pulseEnable(uint8_t _data){
	expanderWrite(_data | En);	// En high
	delayMicroseconds(1);		// aktifkan pulsa harus >450ns

	expanderWrite(_data & ~En);	// En low
	delayMicroseconds(50);		// perintah memerlukan > 37us untuk diselesaikan
}

void LiquidCrystal_I2C::load_custom_character(uint8_t char_num, uint8_t *rows){
	createChar(char_num, rows);
}

void LiquidCrystal_I2C::setBacklight(uint8_t new_val){
	if (new_val) {
		backlight();		// turn backlight on
	} else {
		noBacklight();		// turn backlight off
	}
}

void LiquidCrystal_I2C::printstr(const char c[]){
  //Fungsi ini tidak sama dengan fungsi yang digunakan untuk tampilan I2C "nyata".
  //ada di sini sehingga sketsa pengguna tidak perlu diubah
	print(c);
}
