/****************************************************************************
*								            *
*   aisp_def.h - Global type, function, and constants definitions           *
*   Copyright (C) 1997  Alaska SAR Facility 			   	    *
*									    *
*   ASF APD Contacts:						    	    *
*	Rick Guritz				rguritz@asf.alaska.edu      *
*	Tom Logan				tlogan@asf.alaska.edu       *
* 									    *
*	Alaska SAR Facility			ASF APD Web Site:	    *	
*	Geophysical Institute			www.asf.alaska.edu	    *
*       University of Alaska Fairbanks					    *
*	P.O. Box 757320							    *
*	Fairbanks, AK 99775-7320					    *
*									    *
****************************************************************************/

/*-----------------------------------------------------*/
/* define complex variable type if not already defined */
/*-----------------------------------------------------*/
#include "asf_insar.h"

/*-----------------------------*/
/* Simple function definitions */
/*-----------------------------*/
#define   NINT(a)       ((a)>=0.0 ? (int)((a)+0.5):(int)((a)-0.5)) /* nearest int */
#define   MIN(a,b)      (((a) < (b)) ? (a) : (b))                    /* minimum     */
#define   MAX(a,b)      (((a) > (b)) ? (a) : (b))                    /* maximum     */

float        Cabs(complexFloat); 
complexFloat Cadd (complexFloat,complexFloat);
complexFloat Cconj(complexFloat);
complexFloat Cmplx(float,float);
complexFloat Czero();
complexFloat Csmul(float,complexFloat);
complexFloat Cmul (complexFloat,complexFloat);

/*cfft1d: Perform FFT, 1 dimentional:
	dir=0 -> init; 
	dir<0 -> forward; 
	dir>0 -> backward*/
void cfft1d(int n, complexFloat *c, int dir);

/*-----------------------*/
/* Constants Definitions */
/*-----------------------*/
#define   default_n_az 4096
#define   speedOfLight 299792458.0 /*Speed of light in vacuum, m/s */
#define   pi      	3.14159265358979323
#define   pi2     	(2*pi)

