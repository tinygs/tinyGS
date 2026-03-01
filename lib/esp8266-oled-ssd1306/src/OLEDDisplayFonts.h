#ifndef OLEDDISPLAYFONTS_h
#define OLEDDISPLAYFONTS_h

#ifdef __MBED__
#define PROGMEM
#endif

// Font data is defined once in OLEDDisplayFonts.cpp to avoid
// duplication across multiple translation units.
extern const uint8_t ArialMT_Plain_10[] PROGMEM;
extern const uint8_t ArialMT_Plain_16[] PROGMEM;
// extern const uint8_t ArialMT_Plain_24[] PROGMEM;

#endif // OLEDDISPLAYFONTS_h
