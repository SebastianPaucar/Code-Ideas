#include <SPFD5408_Adafruit_GFX.h>    
#include <SPFD5408_Adafruit_TFTLCD.h> 
#include <SPI.h>
#include <SD.h>
#include <SPFD5408_TouchScreen.h>

#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

#define YP A3  
#define XM A2  
#define YM 9   
#define XP 8   

#define TS_MINX 125
#define TS_MINY 85
#define TS_MAXX 965
#define TS_MAXY 905

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define LCD_RESET A4

#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define BOXSIZE 40
#define PENRADIUS 3

#define MINPRESSURE 10
#define MAXPRESSURE 1000

  #define MENX0  0
  #define MENY0  0
  #define dXMEN  1.5*BOXSIZE
  #define dYMEN  1.1*BOXSIZE

  #define TEMPX0  0.5*BOXSIZE
  #define TEMPY0  2*BOXSIZE
  #define dXTEMP  2.5*BOXSIZE
  #define dYTEMP  2*BOXSIZE

  #define PHX0  3*BOXSIZE
  #define PHY0  2*BOXSIZE
  #define dXPH  2.5*BOXSIZE
  #define dYPH  2*BOXSIZE

  #define NTUX0  0.5*BOXSIZE
  #define NTUY0  4.75*BOXSIZE
  #define dXNTU  2.5*BOXSIZE
  #define dYNTU  2*BOXSIZE

  #define TDSX0  3*BOXSIZE
  #define TDSY0  4.75*BOXSIZE
  #define dXTDS  2.5*BOXSIZE
  #define dYTDS  2*BOXSIZE

  #define MULX0  0
  #define MULY0  0
  #define dXMUL  6*BOXSIZE
  #define dYMUL  1.75*BOXSIZE

  int band = 0;

#define LCD_CS A3 
#define LCD_CD A2 
#define LCD_WR A1 
#define LCD_RD A0 

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

#define SD_CS 53  

#else


#define SD_CS 10     

#endif

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

void setup()
{
  Serial.begin(9600);
   digitalWrite(35, HIGH);         
  tft.reset();

  tft.begin(0x9325);

  progmemPrint(PSTR("Initializing SD card..."));
  if (!SD.begin(SD_CS)) {
    progmemPrintln(PSTR("failed!"));
    return;
  }
  progmemPrintln(PSTR("OK!"));

  bmpDraw("MENUbm.bmp", 0, 0);

  delay(1000);
}

void loop()
{
 
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
    p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
    
  }
  if (p.x > TEMPX0 && p.x < TEMPX0 + dXTEMP && p.y > TEMPY0 && p.y < TEMPY0 + dYTEMP ) { 
        band = 1;
        bmpDraw("TEbm.bmp", 0, 0);
        do{
          band = GoToMenu(band);
        }while(band == 1);
       } else if (p.x > NTUX0 && p.x < NTUX0 + dXNTU && p.y > NTUY0 && p.y < NTUY0 + dYNTU) {
        band = 1;
        bmpDraw("TUbm.bmp", 0, 0);
        do{
          band = GoToMenu(band);
        }while(band == 1);
       }else if (p.x > PHX0 && p.x < PHX0 + dXPH && p.y > PHY0 && p.y < PHY0 + dYPH) {
        band = 1;
        bmpDraw("PHbm.bmp", 0, 0);
        do{
          band = GoToMenu(band);
        }while(band == 1);
       }else if (p.x > TDSX0 && p.x < TDSX0 + dXTDS && p.y > TDSY0 && p.y < TDSY0 + dYTDS) {
        band = 1;
        bmpDraw("TDSbm.bmp", 0, 0);
        do{
          band = GoToMenu(band);
        }while(band == 1);
       }else if(p.x > MULX0 && p.x < MULX0 + dXMUL && p.y > MULY0 && p.y < MULY0 + dYMUL) {
        band = 1;
        bmpDraw("MULTbm.bmp", 0, 0);
        do{
          band = GoToMenu(band);
        }while(band == 1);
       }
       
}


#define BUFFPIXEL 20

void bmpDraw(char *filename, int x, int y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel in buffer (R+G+B per pixel)
  uint16_t lcdbuffer[BUFFPIXEL];  // pixel out buffer (16-bit per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();
  uint8_t  lcdidx = 0;
  boolean  first = true;

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  progmemPrint(PSTR("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');
  if ((bmpFile = SD.open(filename)) == NULL) {
    progmemPrintln(PSTR("File not found"));
    return;
  }


  if(read16(bmpFile) == 0x4D42) { 
    progmemPrint(PSTR("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile);
    bmpImageoffset = read32(bmpFile); 
    progmemPrint(PSTR("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    progmemPrint(PSTR("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { 
      bmpDepth = read16(bmpFile); // bits per pixel
      progmemPrint(PSTR("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { 

        goodBmp = true; 
        progmemPrint(PSTR("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        rowSize = (bmpWidth * 3 + 3) & ~3;

        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { 
          if(flip) 
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else    
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { 
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); 
          }

          for (col=0; col<w; col++) { 
            if (buffidx >= sizeof(sdbuffer)) { 
              if(lcdidx > 0) {
                tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first  = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0;
            }
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            lcdbuffer[lcdidx++] = tft.color565(r,g,b);
          } 
        } 
        if(lcdidx > 0) {
          tft.pushColors(lcdbuffer, lcdidx, first);
        } 
        progmemPrint(PSTR("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } 
    }
  }
  bmpFile.close();
  if(!goodBmp) progmemPrintln(PSTR("BMP format not recognized."));
}

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); 
  ((uint8_t *)&result)[1] = f.read(); 
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); 
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); 
  return result;
}

void progmemPrint(const char *str) {
  char c;
  while(c = pgm_read_byte(str++)) Serial.print(c);
}

void progmemPrintln(const char *str) {
  progmemPrint(str);
  Serial.println();
}

int GoToMenu(int nband){
    digitalWrite(13, HIGH);
    TSPoint p = ts.getPoint();
    digitalWrite(13, LOW);
    pinMode(XM, OUTPUT);
    pinMode(YP, OUTPUT);
    if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
      p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
      p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());  
    }
    if (p.x > MENX0 && p.x < MENX0 + dXMEN && p.y > MENY0 && p.y < MENY0 + dYMEN){
      nband = 0;
      bmpDraw("MENUbm.bmp", 0, 0);
    }
  return nband;
}

