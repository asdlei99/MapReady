/****************************************************************************
*								            *
*   ardop_params.h   Parameters needed to run ARDOP                           *
* Copyright (c) 2004, Geophysical Institute, University of Alaska Fairbanks   *
* All rights reserved.                                                        *
*                                                                             *
* You should have received an ASF SOFTWARE License Agreement with this source *
* code. Please consult this agreement for license grant information.          *
*                                                                             *
*                                                                             *
*       For more information contact us at:                                   *
*                                                                             *
*	Alaska Satellite Facility	    	                              *
*	Geophysical Institute			www.asf.alaska.edu            *
*       University of Alaska Fairbanks		uso@asf.alaska.edu	      *
*	P.O. Box 757320							      *
*	Fairbanks, AK 99775-7320					      *
*									      *
******************************************************************************/

/*-------------------------------------------------------------------------*/
/*   This structure contains all of the parameters needed to run asp.c     */
/*-------------------------------------------------------------------------*/
struct ARDOP_PARAMS {
  char	in1[1024];	     /* First input file basename		   */
  char	out[1024];	     /* Output File basename                       */
  char  status[1024];         /* Kludgey status file, for STP               */
  char	CALPRMS[1024];        /* Calibration Parameters file (if desired)   */
  char	srm[4];		     /* Secondary range migration correction (y/n) */
  char	iqflip[4];	     /* Flip i/q or offset video (y/n flip;o off)  */ 
  int	iflag;		     /* Debugging Flag 	   			   */
  int	npatches;	     /* Number of range input patches 		   */
  int	na_valid;	     /* Number of valid points in the Azimuth	   */
  int	deskew; 	     /* Deskew flag				   */
  int	isave;		     /* Start range bin				   */
  int	nla;		     /* Number of range bins to process		   */
  int	hamFlag;	     /* Use a Hamming window for az ref func.      */
  int	kaiFlag;	     /* Use a Kaiser window for az ref func.       */
  int	pwrFlag;	     /* Create a power image			   */
  int	sigmaFlag;	     /* Create a sigma naught image		   */
  int	gammaFlag;	     /* Create a gamma naught image		   */
  int	betaFlag;	     /* Create a beta naught image		   */
  int	ifirstline;	     /* First line to read (from 0)  		   */
  int	ifirst;		     /* First sample pair to use (start 0)	   */
  int	nlooks;		     /* Number of looks in the azimuth		   */
  int	nbytes;		     /* Number of input bytes per line (w/ header) */
  int	ngood;		     /* Number of input bytes per line (no header) */
  float	fd;		     /* Doppler centroid quadratic coefs (PRFs)    */
  float	fdd;		     /* Doppler centroid quadratic coefs (PRFs/rangepixel) */
  float	fddd;	             /* Doppler centroid quadratic coefs (PRFs/(rangepixel^2)) */
  float	re;		     /* Earth Radius (m)			   */
  float	vel;		     /* Body fixed S/C velocity (m/s)		   */
  float	ht;		     /* Spacecraft height (m)			   */
  float	r00;		     /* Range of first sample in raw data file (m) */
  float	prf;		     /* Pulse Repitition Frequency (pps)	   */
  float	azres;		     /* Desired azimuth resolution (m)		   */
  float	fs;		     /* Range sampling rate (Hz)	  	   */
  float	slope;		     /* Chirp Slope (Hz/s)			   */
  float	pulsedur;	     /* Pulse Duration (s)			   */
  float	nextend;	     /* Chirp Extension Points			   */
  float	wavl;		     /* Radar Wavelength			   */
  float	rhww;		     /* Range spectrum wt. (1.0=none;0.54=hamming) */
  float	pctbw;		     /* Fraction of range bandwidth to remove  	   */
  float	pctbwaz;	     /* Fraction of azimuth bandwidth to remove	   */
  float	sloper, interr,	     /* 1st Patch Slope and Inter. for range       */
	slopea, intera;      /* 1st Patch Slope and Inter. for azimuth     */
  float	dsloper, dinterr,    /* Delta slope,inter per patch in range       */
	dslopea, dintera;    /* Delta slope,inter per patch in azimith     */
  float	caltone;	     /* Caltone % of sample rate		   */
  float	timeOff,slantOff; /*Characteristic Timing Offset (manually entered.*/
  float	xmi;		     /* bias value for i values			   */
  float	xmq;		     /* bias value for q values			   */
};

struct INPUT_ARDOP_PARAMS {
  char	in1[1024];
  char	out[1024];
  char  status[1024];
  char	CALPRMS[1024];
  int *pwrFlag;
  int *sigmaFlag;
  int *gammaFlag;
  int *betaFlag;
  int *hamFlag;
  int *kaiFlag;
  int *ifirstline;
  int *npatches;
  int *isave;
  int *ifirst;
  int *nla;
  float *azres;
  int *deskew;
  int *na_valid;
  float *sloper;
  float *interr;
  float *slopea;
  float *intera;
  float *dsloper;
  float *dinterr;
  float *dslopea;
  float *dintera;
  float *fd;
  float *fdd;
  float *fddd;
  int *iflag;
};

struct INPUT_ARDOP_PARAMS *get_input_ardop_params_struct(char *in1, char *out);
void apply_in_ardop_params_to_ardop_params(struct INPUT_ARDOP_PARAMS *in,
                                           struct ARDOP_PARAMS *a);

void print_params(const char *in,struct ARDOP_PARAMS *a,const char *sourceProgram);
void read_params(const char *in,struct ARDOP_PARAMS *);

