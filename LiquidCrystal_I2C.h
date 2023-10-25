#ifndef FDB_LIQUID_CRYSTAL_I2C_H
#define FDB_LIQUID_CRYSTAL_I2C_H

#include <inttypes.h>
#include <Print.h>

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En B00000100  // Enable bit
#define Rw B00000010  // Read/Write bit
#define Rs B00000001  // Register select bit

/**
 * Ini adalah driver untuk layar Liquid Crystal LCD yang menggunakan bus I2C.
 *
 * Setelah membuat instance kelas ini, panggil dulu Begin() sebelum yang lainnya.
 * Lampu latar menyala secara default, karena kemungkinan besar itulah mode pengoperasiannya
 * Kebanyakan kasus.
 */
class LiquidCrystal_I2C : public Print {
public:
	/**
	 * Constructor
	 *
	 * @param lcd_addr	Alamat budak I2C dari layar LCD. Kemungkinan besar dicetak di
   * Papan sirkuit LCD, atau lihat dokumentasi LCD yang tersedia.
	 * @param lcd_cols	Jumlah kolom yang dimiliki layar LCD Anda
	 * @param lcd_rows	Jumlah baris yang dimiliki layar LCD Anda.
	 * @param charsize	Ukuran dalam titik yang dimiliki layar, gunakan LCD_5x10DOTS atau LCD_5x8DOTS.
	 */
	LiquidCrystal_I2C(uint8_t lcd_addr, uint8_t lcd_cols, uint8_t lcd_rows, uint8_t charsize = LCD_5x8DOTS);

/**
* Atur tampilan LCD dalam keadaan awal yang benar, harus dipanggil sebelum melakukan hal lain.
*/
	void begin();

    /**
    * Hapus semua karakter yang sedang ditampilkan. Operasi cetak/tulis berikutnya akan dimulai
    * dari posisi pertama pada layar LCD.
    */
	void clear();

  /**
    * Operasi cetak/tulis selanjutnya akan dimulai dari posisi pertama pada layar LCD.
    */
	void home();

  /**
    * Jangan tampilkan karakter apa pun pada layar LCD. Status lampu latar tidak akan berubah.
    * Juga semua karakter yang tertulis di layar akan kembali, ketika tampilan diaktifkan kembali.
    */
	void noDisplay();

  /**
    * Menampilkan karakter pada layar LCD, ini adalah perilaku normal. Metode ini seharusnya
    * hanya dapat digunakan setelah noDisplay() digunakan.
    */
	void display();

  /**
    * Jangan mengedipkan indikator kursor.
    */
	void noBlink();

	/**
	 * Start blinking the cursor indicator.
	 */
	void blink();

	/**
	 * Do not show a cursor indicator.
	 */
	void noCursor();

  /**
    * Tampilkan indikator kursor, kursor dapat berkedip tidak berkedip. Menggunakan
    * Metode kedip() dan noBlink() untuk mengubah kedipan kursor.
    */
	void cursor();

	void scrollDisplayLeft();
	void scrollDisplayRight();
	void printLeft();
	void printRight();
	void leftToRight();
	void rightToLeft();
	void shiftIncrement();
	void shiftDecrement();
	void noBacklight();
	void backlight();
	bool getBacklight();
	void autoscroll();
	void noAutoscroll();
	void createChar(uint8_t, uint8_t[]);
	void setCursor(uint8_t, uint8_t);
	virtual size_t write(uint8_t);
	void command(uint8_t);

	inline void blink_on() { blink(); }
	inline void blink_off() { noBlink(); }
	inline void cursor_on() { cursor(); }
	inline void cursor_off() { noCursor(); }

// Alias ​​fungsi API Kompatibilitas
	void setBacklight(uint8_t new_val);				// alias for backlight() and nobacklight()
	void load_custom_character(uint8_t char_num, uint8_t *rows);	// alias for createChar()
	void printstr(const char[]);

private:
	void send(uint8_t, uint8_t);
	void write4bits(uint8_t);
	void expanderWrite(uint8_t);
	void pulseEnable(uint8_t);
	uint8_t _addr;
	uint8_t _displayfunction;
	uint8_t _displaycontrol;
	uint8_t _displaymode;
	uint8_t _cols;
	uint8_t _rows;
	uint8_t _charsize;
	uint8_t _backlightval;
};

#endif // FDB_LIQUID_CRYSTAL_I2C_H
