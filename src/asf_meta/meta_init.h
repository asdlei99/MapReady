/****************************************************************
FUNCTION NAME:  meta_get_internal

DESCRIPTION:
   Internal routine descriptions, prototypes, structures, etc.
****************************************************************/

#define speedOfLight 299792458.0 /*Speed of light in vacuum, m/s */

#ifndef SQR
#define SQR(x) ((x)*(x))
#endif

/*Internal Definitions:*/
#include "ceos.h"
#include "asf_meta.h"
#include "asf.h"


/***************************************
 * ceos_init:
 * Reads SAR structure parameters from CEOS into existing meta_parameters
 * structure.  Calls a facility-specific decoder. */
void ceos_init(const char *fName, meta_parameters *sar, report_level_t level);

/***************************************
 * ardop_init:
 * Reads additional SAR structure parameters from ARDOP input file into
 * meta_parameters structure. */
void ardop_init(const char *fName,meta_parameters *sar);

/***************************************
 * get_units: in meta_get_util.c
 * Used to resolve the difference between millimeters, meters, and kilometers in
 * state vectors and slant ranges. Returns power of 1000.0 which must be applied
 * to given data to make its value near expected value. Works between Hz and
 * MHz, sec and msec, m and km, etc. Note the Expected_* values only have to be
 * within 30 times the real value. */
#define EXPECTED_SR 800000.0 /*Expected slant range, in meters.*/
#define EXPECTED_ER 6360000.0 /*Expected earth radius, in meters.*/
#define EXPECTED_HT 700000.0 /*Expected satellite height, in meters.*/
#define EXPECTED_POS 7000000.0 /*Expected satellite position, in m.*/
#define EXPECTED_VEL 7500.0 /*Expected satellite velocity, in m/s.*/
#define EXPECTED_FS 18000000.0 /*Expected sampling frequency, in Hz.*/
#define EXPECTED_RANGEGATE 0.0054 /*Expected range gate delay, in s.*/
#define EXPECTED_WAVELEN 0.056 /*Expected radar wavelength, in m.*/
#define EXPECTED_RSR 18.96 /* Expected range sampling rate, in MHz */
#define EXPECTED_PRF 1700 // Expected PRF, in Hz
#define EXPECTED_SAMP_RATE 19000000.0
double get_units(double value, double expectedValue);

