/******************************************************************************
NAME:	avg_in_dop

SYNOPSIS:

DESCRIPTION:
	Calculates the average doppler from two .in files and stores the 
  coefficients in specified file

EXTERNAL ASSOCIATES:
    NAME:			USAGE:
    -----------------------------------------------------------------

FILE REFERENCES:
    NAME:			USAGE:
    -----------------------------------------------------------------
    infile.in			Input .in file 1
    infile.in			Input .in file 2
    average_in_dop		Output, average doppler file
    [-log <file>]		Options to have output written to <file>

PROGRAM HISTORY:
    VERS:    DATE:   AUTHOR:
    -----------------------------------------------------------------
     1.0     6/00      D.Koster   Initial Development.
     1.01    7/01      R. Gens    Added logfile switch
     1.15    4/02      P. Denny   Update commandline parsing add usage()

HARDWARE/SOFTWARE LIMITATIONS:

ALGORITHM DESCRIPTION:

ALGORITHM REFERENCES:

BUGS:

*****************************************************************************/

/******************************************************************************
*                                                                             *
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

#include "asf.h"
#include "ardop_params.h"
#include "asf_insar.h"

#define VERSION 1.15

void usage(char *name)
{
 printf("\n"
	"USAGE:\n"
	"   %s [-log <file>] <infile1.in> <infile2.in> <outfile>\n",name); 
 printf("\n"
	"ARUGMENTS:\n"
	"   <infile1.in>  is the first .in file with the .in extension\n"
	"   <infile2.in>  is the second .in file with the .in extension\n"
	"   <outfile>     is the output file where the average doppler\n"
	"                   coefficients are stored.\n"
	"   -log <file>   Allows output to be written to a log file (optional).\n"); 
 printf("\n"
	"DESCRIPTION:\n"
	"   %s averages the doppler polynomials and writes the\n"
	"   results to outfile.\n",name);
 printf("\n"
	"Version %3.1f, ASF SAR Tools\n"
	"\n", VERSION);
 exit(1);
}

int main(int argc, char *argv[])
{
	char dotInFile1[255];		/* first .in file */	
	char dotInFile2[255];		/* second .in file */
	char goingOut[255];		/* Output file for average */

	logflag=0;
	currArg=1;

/* parse up optional arguments */
	while (currArg < (argc-3)) {
		char *key = argv[currArg++];
		if (strmatch(key,"-log")) {
			CHECK_ARG(1);
			strcpy(logFile,GET_ARG(1));
			fLog = FOPEN(logFile, "a");
			logflag=1;
		}
		else {printf("**Invalid option: %s\n\n",argv[currArg-1]); usage(argv[0]);}
	}
	if ((argc-currArg) < 3) {printf("Insufficient arguments.\n"); usage(argv[0]);}

/* Get file names from the command line. */
	strcpy(dotInFile1,argv[currArg]);
	strcpy(dotInFile2,argv[currArg+1]);
	strcpy(goingOut,argv[currArg+2]);

	return average_in_doppler(dotInFile1, dotInFile2, goingOut);
}

