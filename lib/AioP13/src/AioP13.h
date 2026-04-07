//
// AioP13.h
//
// An implementation of Plan13 in C++ by Mark VandeWettering
//
// Plan13 is an algorithm for satellite orbit prediction first formulated
// by James Miller G3RUH.  I learned about it when I saw it was the basis 
// of the PIC based antenna rotator project designed by G6LVB.
//
// http://www.g6lvb.com/Articles/LVBTracker2/index.htm
//
// I ported the algorithm to Python, and it was my primary means of orbit
// prediction for a couple of years while I operated the "Easy Sats" with 
// a dual band hand held and an Arrow antenna.
//
// I've long wanted to redo the work in C++ so that I could port the code
// to smaller processors including the Atmel AVR chips.  Bruce Robertson,
// VE9QRP started the qrpTracker project to fufill many of the same goals,
// but I thought that the code could be made more compact and more modular,
// and could serve not just the embedded targets but could be of more
// use for more general applications.  And, I like the BSD License a bit
// better too.
//
// So, here it is!
//
// =====================================================================
//
// dl9sec@gmx.net (02..11/2021):
//
// The original Plan13 BBC Basic source code can be found at:
//
// https://www.amsat.org/articles/g3ruh/111.html
//
// Published as "Donationware" in favour of AMSAT-UK, LONDON, E12 5EQ
// and the AO-13 Amateur Satellite Program.
//
// Changes:
// - Refactoring for seamless use with Arduino.
// - Renamed class DateTime to P13DateTime because of potential conflict
//   with RTClib, which uses the same class name.
// - Renamed all classes to P13... for consistency and clarification of
//   affiliation.
// - Some code beautification.
// - Changed output of method "ascii" to ISO date format
// - Added helper function for converting lat/lon coordinates to rectangular
//   map coordinates
// - Used all double
// - Used PI instead of M_PI
// - Used degrees() and radians() instead of DEGREES() and RADIANS()
// - Inserted explicit casts
// - Added a method "footprint" to class P13Satellite
// - Added a method "doppler" to calculate down- and uplink frequencies
// - Added a method "footprint" to class P13Sun
// - Renamed the whole stuff from ArduinoP13 to AioP13 to follow the
//   Arduino library specifications for naming of libraries.
// - Rework of all the variable names because of conflicts.
//   All variables got a qualifier to get more or less unique names:
//   "g_": global variables
//   "c_": class public variables
//   "cp_": class private variables
//   "p_": parameter variable
//   "l_": local variable
//   The next letter gives a hint about the data type (e.g. "d": double,
//   "i": integer, "cc": constant character, "vec": Vec3 vector, ...).
//   In some cases this is very ugly, so all the variables should be renamed
//   to speaking and useful names in one of the next releases.
//
//----------------------------------------------------------------------

#ifndef AioP13_H
#define AioP13_H

#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#define P13_FRX 0
#define P13_FTX 1


static long fnday(int p_iy, int p_im, int p_id);
static void fndate(int &p_iy, int &p_im, int &p_id, long p_idt);
static double getdouble(const char *p_ccc, int p_ii0, int p_ii1);
static long getlong(const char *p_ccc, int p_ii0, int p_ii1);
static long getuint32_tinygs(const uint8_t *p_ccc, int p_ii0);
static uint16_t getuint16_tinygs(const uint8_t *p_ccc, int p_ii0);
static long     getuint24_tinygs(const uint8_t *p_ccc, int p_ii0);

void latlon2xy(int &p_ix, int &l_iy, double p_dlat, double p_dlon, const int p_ciMapMaxX, const int p_ciMapMaxY);

//----------------------------------------------------------------------

// here are a bunch of constants that will be used throughout the 
// code, but which will probably not be helpful outside.

static const double g_scdRE   = 6378.137;                   // WGS-84 Earth ellipsoid
static const double g_scdFL   = 1.0 / 298.257224;           // -"-
static const double g_scdRP   = g_scdRE * (1.0 - g_scdFL);  // -

static const double g_scdGM   = 3.986E5;                    // Earth's Gravitational constant km^3/s^2
static const double g_scdJ2   = 1.08263E-3;                 // 2nd Zonal coeff, Earth's Gravity Field

static const double g_scdYM   = 365.25;                     // Mean Year,     days
static const double g_scdYT   = 365.2421896698;             // Tropical year, days
static const double g_scdWW   = 2.0 * PI / g_scdYT;         // Earth's rotation rate, rads/whole day
static const double g_scdWE   = 2.0 * PI + g_scdWW;         // Earth's rotation rate, radians/day 
static const double g_scdW0   = g_scdWE / 86400.0;          // Earth's rotation rate, radians/sec

// Sidereal and Solar data. Rarely needs changing. Valid to year ~2030
static const double g_scdYG   = 2014.0;                     // GHAA, Year YG, Jan 0.0
static const double g_scdG0   = 99.5828;                    // -"-
static const double g_scdMAS0 = 356.4105;                   // MA Sun and rate, deg, deg/day
static const double g_scdMASD = 0.98560028;                 // -"-
static const double g_scdINS  = radians(23.4375);           // Sun's inclination
static const double g_scdCNS  = cos(g_scdINS);              // -"-
static const double g_scdSNS  = sin(g_scdINS);              // -"-
static const double g_scdEQC1 = 0.03340;                    // Sun's Equation of centre terms
static const double g_scdEQC2 = 0.00035;                    // -"-

static const double g_scdAU   = 149.597870700E6;            // 1 AU, mean range in km to the sun

//----------------------------------------------------------------------

// The original BASIC code used three variables (e.g. Ox, Oy, Oz) to
// represent a vector quantity.  I think that makes for slightly more
// obtuse code, so I going to collapse them into a single variable 
// which is an array of three elements.

typedef double Vec3[3];

//----------------------------------------------------------------------

class P13DateTime {

public:
    long   c_lDN;
    double c_dTN;
    
    P13DateTime();
    P13DateTime(const P13DateTime &p_dt);
    P13DateTime(int p_iyear, int p_imonth, int p_iday, int p_ih, int p_im, int p_is);
    ~P13DateTime();
    
    void add(double p_ddays);
    void settime(int p_iyear, int p_imonth, int p_iday, int p_ih, int p_im, int p_is);
    void gettime(int &p_iyear, int &p_imon, int &p_iday, int &p_ih, int &p_im, int &p_is);
    void ascii(char *p_cbuf);
    void roundup(double p_dtime);
};


//----------------------------------------------------------------------

class P13Observer {

public:
    const char *c_ccObsName;
    double c_dLA;
    double c_dLO;
    double c_dHT;
    
    Vec3 c_vecU, c_vecE, c_vecN, c_vecO, c_vecV;
    
    P13Observer(const char *p_ccnm, double p_dlat, double p_dlon, double p_dasl);
    ~P13Observer();
};


//----------------------------------------------------------------------

class P13Satellite_tGS { 

public:
    const char *c_ccSatName;
    
    Vec3 c_vecSAT, c_vecVEL;      // Celestial coordinates
    Vec3 c_vecS, c_vecV;          // Geocentric coordinates
 
    P13Satellite_tGS(const uint8_t *p_tiny_tle);
    ~P13Satellite_tGS();
    
    void   tle(const uint8_t *p_tiny_tle );
    void   predict(const P13DateTime &p_dt);
    void   latlon(double &p_dlat, double &p_dlon);
    void   elaz(const P13Observer &p_obs, double &p_del, double &p_daz);
    void   footprint(int p_aipoints[][2], int p_inumberofpoints, const int p_ciMapMaxX, const int p_ciMapMaxY, double &p_dsatlat, double &p_dsatlon);
    double doppler(double p_dfreqMHz, bool p_bodir);

private:
    long   cp_lN;       // Satellite calaog number
    long   cp_lYE;      // Epoch Year               year
    double cp_dTE;      // Epoch time               days
    double cp_dIN;      // Inclination              deg
    double cp_dRA;      // R.A.A.N.                 deg
    double cp_dEC;      // Eccentricity              -
    double cp_dWP;      // Arg perigee              deg
    double cp_dMA;      // Mean anomaly             deg
    double cp_dMM;      // Mean motion              rev/d
    double cp_dM2;      // Decay Rate               rev/d/d
    double cp_dRV;      // Orbit number              -
//    double cp_dALON;    // Sat attitude             deg
//    double cp_dALAT;    // Sat attitude             deg
    long   cp_lDE;      // Epoch Fraction of day
    
    // These values are stored, but could be calculated on the fly during calls to predict() 
    // Classic space/time tradeoff

    double cp_dN0, cp_dA_0, cp_dB_0;
    double cp_dPC;
    double cp_dQD, cp_dWD, cp_dDC;

    double cp_dRS;      // Radius of satellite orbit
    double cp_dRR;      // Range rate for doppler calculation

};


//----------------------------------------------------------------------

class P13Sun {

public:
    Vec3 c_vecSUN, c_vecH;
    
    P13Sun();
    ~P13Sun();
    
    void predict(const P13DateTime &p_dt);
    void latlon(double &p_dlat, double &p_dlon);
    void elaz(const P13Observer &p_obs, double &p_del, double &p_daz);
    void footprint(int p_aipoints[][2], int p_inumberofpoints, const int p_ciMapMaxX, const int p_ciMapMaxY, double &p_dsunlat, double &p_dsunlon);
};

#endif  // AioP13_H
