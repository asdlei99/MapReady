/****************************************************************
NAME: escher

SYNOPSIS:  escher [-log <file>] [-x seedX] [-y seedY] 
                  <source.phase> <output.phase>
  	
	<source.phase>  is an input phase file (.phase and .ddr).
  	<output.phase>  is an output unwrapped phase file.
        [-log <file>]   allows the output to be written to a log file.
        [-x seedX ]	is the unwrapping start location in x. (default: center)
        [-y seedY ]     is the unwrapping start location in y. (default: center)
    
DESCRIPTION:
       Escher, named after the famous Dutch artist M.C. Escher, 
  performs phase unwrapping via the Goldstien method.  This is
  used to turn a wrapped phase file (for example, from igram(1))
  into an unwrapped phase file for later conversion into
  a DEM-- a Digital Elevation Map.

       Escher is critical during interferometry.
  
  Here is Mike Shindle's original description (for entertainment value):
  Here's a cool idea:  When making branch cuts...  while doing the branch
  cut from X to Y and looping through coordinates, if you hit a point which 
  has already been cut, then just stop the cut at that point (call it Z) 
  and call the branch cut function (recursive function call), this time 
  making a cut from Z to Y.  You'll have to use some sort of semaphor to
  indicate 'remainder of cut'.  The point is, if on the first leg you hit
  a cut, and on the second part you hit a different cut, but they are all
  part of the current tree, then you don't need to do anymore; everything
  is attached.  This will eliminate, we hope and pray, a lot of intermediate
  cutting.

EXTERNAL ASSOCIATES:
    NAME:                USAGE:
    ---------------------------------------------------------------

FILE REFERENCES:
    NAME:                USAGE:
    ---------------------------------------------------------------

PROGRAM HISTORY:
    VERS:   DATE:        AUTHOR:	PURPOSE:
    ---------------------------------------------------------------
    1.0                M. Shindle - Original Development
    2.0                M. Shindle - Default Phase value is value
			            at seed location, not 0.0.
    2.1		       O. Lawlor  - Get image size from DDR.
     "      07/11/97   D. Corbett - updated version number
    2.11	       O. Lawlor  - Check to see if seed point is out of bounds.
     "      03/08/98   D. Corbett - updated version number
    2.2		       O. Lawlor  - Ground areas of zero phase.
    2.3	     2/16/99   O. Lawlor  - Check more carefully for bad seed points.
                                    Use only 1 phase array (50% less memory!)
    2.5         6/05   R. Gens    - Added some logfile capabilities.
                                    Removed dependency of DDR. Took care of
                                    endianess.
 
HARDWARE/SOFTWARE LIMITATIONS:

  Coherence function not yet implemented.


ALGORITHM DESCRIPTION:
  
  1. generate a mask
  2. install cuts in the mask
  3. integrate the remaining un-cut phase

ALGORITHM REFERENCES:

BUGS:

****************************************************************/
/****************************************************************************
*								            *
*   escher  -  unwrapped an interferogram phase file using the branch cut   *
*	       method.							    *
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

#include "escher.h"

#define VERSION         2.5

/* globals */
PList list;
Uchar *mask;     /* phase-state mask  */
Uchar *im;       /* integration mask  */
float *phase;   /* input phase       */
float *coh;     /* coherence 'rho'   */
/*This is now "phase": float *uwp;   unwrapped phase   */
int wid;
int len;
int size;


int
main(int argc, char *argv[])
{
  int seedX=-1,seedY=-1; 
  char szWrap[MAXNAME], szUnwrap[MAXNAME];
  meta_parameters *meta;

  logflag=0;

  while (currArg < (argc-2)) {
    char *key = argv[currArg++];
    if (strmatch(key,"-log")) {
      CHECK_ARG(1);
      strcpy(logFile,GET_ARG(1));
      fLog = FOPEN(logFile, "a");
      logflag = 1;
    }
    else if (strmatch(key,"-x")) {
      CHECK_ARG(1);
      sscanf(GET_ARG(1), "%d", &seedX);
    }
    else if (strmatch(key,"-y")) {
      CHECK_ARG(1);
      sscanf(GET_ARG(1), "%d", &seedY);
    }
    else {
      printf("\n   ***Invalid option: %s\n", argv[currArg-1]);
      usage(argv[0]);
    }
  }
  create_name(szWrap, argv[currArg++], ".img");
  create_name(szUnwrap, argv[currArg], ".img");

  printf("%s\n",date_time_stamp());
  printf("Program: escher\n\n");
  if (logflag) {
    StartWatchLog(fLog);
    printLog("Program: escher\n\n");
  }

  meta = meta_read(szWrap);
  wid = meta->general->sample_count;
  len = meta->general->line_count;
  if ((seedX==-1)&&(seedY==-1))
  {
  	seedX=wid/2;
  	seedY=len/2;
  }
  
  meta_write(meta, szUnwrap);
  

  size = wid*len;
  mask  = (Uchar *)calloc(size, sizeof(Uchar));
  im    = (Uchar *)calloc(size, sizeof(Uchar));
  phase = (float *)MALLOC(sizeof(float)*size);
  /*coh   = (float *)MALLOC(sizeof(float)*size);*/

  /* perform steps*/
  printf("\n   begin unwrapping phase...\n");
  loadWrappedPhase(szWrap);
  groundBorder();
  makeMask();
  
  doStats("   after makeMask():");

  installCordon("cordon");
  cutMask();

  doStats("   after cutMask():");

#if DO_DEBUG_CHECKS
  saveMask((char *)mask, "test");

  verifyCuts();                                           
#endif

  checkSeed(&seedX, &seedY);

  integratePhase(seedX, seedY);

  doStats("   after integratePhase():");

  finishUwp();

  saveMask(mask, szUnwrap);

  saveUwp(szUnwrap);
  
  /* clean up & save*/
  free(mask);
  free(im);
  free(phase);
  return(0);
}


void usage(char *name)
{
  printf("\n");
  printf("USAGE:  %s [-log <file>] [-x seedX] [-y seedY] <source.phase> <output.phase>\n\n",name);
  printf("  <source.phase> is an input phase file (.phase and .ddr).\n");
  printf("  <output.phase> is an output unwrapped phase file.\n");
  printf("  [-log <file>]  allows the output to be written to a log file.\n");
  printf("  [-x seedX]     is the unwrapping start location in x. (default: center)\n");
  printf("  [-y seedY]     is the unwrapping start location in y. (default: center)\n");
  printf("  (escher will also generate a mask file 'output_phase.mask.)\n");
  printf("\n"
  "Escher uses Goldstein branch-cut phase unwrapping\n"
  "to unwrap the given [-pi,pi) input file into the phase-\n"
  "unwrapped output file.\n");
  printf("\nVersion: %.2f, ASF SAR Tools\n\n",VERSION);
  exit(1);
}
