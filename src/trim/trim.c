/******************************************************************************
NAME: trim

SYNOPSIS:

    trim [-log <file>] [-h <trim height>] [-w <trim width>]
     <in> <out> <new top line> <new left sample>

DESCRIPTION:
    Trim will let you change the size of any LAS 6.0 single-band image.
It should work (but hasn't yet been tested!) on int data.
It does work on byte, short, float, and complex data.

    You specify the input and output filenames (WITH extension), and
a top left coordinate.  Trim will create a new image in outfile which
starts at these coordinates.  Optionally, you can also specify the new height
and width of the output image (if not specified, trim will use the old size
minus the top left coordinates-- i.e. minus the part that is trimmed off).

    Trim relies on DDR information to do the trimming, and maintains DDR
information through to the output.

    Trim can be used anytime you want to work with a smaller image, but
it also can be used to remove the CEOS header from an image (albeit the process
for doing so is a bit convoluted): since the RADARSAT-era CEOS header is one line
on top of the image, plus 192 bytes per line, we can just trim it off.  E.g.
if we have a byte CEOS image "bob.D" we wish to turn into a LAS image "bill.img", type

    trim bob.D bill.img 1 192

    Trim will generate bill.img and bill.ddr.  Note that for this to work, you
need a file called "bob.ddr" to describe bob.D's dimensions and data type-- but
CEOS files don't ship with such a file.  This is the purpose of the makeddr program,
which should also be available from ASF.

EXTERNAL ASSOCIATES:
    NAME:               USAGE:
    ---------------------------------------------------------------

FILE REFERENCES:
    NAME:               USAGE:
    ---------------------------------------------------------------

PROGRAM HISTORY:
    VERS:   DATE:  AUTHOR:      PURPOSE:
    ---------------------------------------------------------------
    1.0     6/97   Orion Lawlor Needed to trim various image files
                     for faster interferometry.
    1.1     5/98   Orion Lawlor Caplib/bug fix.
    1.11    7/01   Rudi Gens    Added logfile switch
    1.3     3/02   Pat Denny    Updated Command line parsing
    1.5     3/04   Rudi Gens    Removed DDR dependency
    2.0     10/05  Rudi Gens    Moved trim into libasf_raster.
                Tool now using function call.

HARDWARE/SOFTWARE LIMITATIONS: none

SEE ALSO:
    makeddr(1), trim_sic(1)

BUGS: none known

******************************************************************************/
/****************************************************************************
*                                           *
*   Trim: lets you trim/add any # of samples to any LAS 6.0 image file      *
*         (even complex .cpx) while maintaining the DDR.            *
* Copyright (c) 2004, Geophysical Institute, University of Alaska Fairbanks   *
* All rights reserved.                                                        *
*                                                                             *
* Redistribution and use in source and binary forms, with or without          *
* modification, are permitted provided that the following conditions are met: *
*                                                                             *
*    * Redistributions of source code must retain the above copyright notice, *
*      this list of conditions and the following disclaimer.                  *
*    * Redistributions in binary form must reproduce the above copyright      *
*      notice, this list of conditions and the following disclaimer in the    *
*      documentation and/or other materials provided with the distribution.   *
*    * Neither the name of the Geophysical Institute nor the names of its     *
*      contributors may be used to endorse or promote products derived from   *
*      this software without specific prior written permission.               *
*                                                                             *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" *
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE   *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE  *
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE    *
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR         *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF        *
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS    *
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN     *
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)     *
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE  *
* POSSIBILITY OF SUCH DAMAGE.                                                 *
*                                                                             *
*       For more information contact us at:                                   *
*                                                                             *
*       Alaska Satellite Facility                                             *
*       Geophysical Institute                   http://www.asf.alaska.edu     *
*       University of Alaska Fairbanks          uso@asf.alaska.edu            *
*       P.O. Box 757320                                                       *
*       Fairbanks, AK 99775-7320                                              *
*                                                                             *
******************************************************************************/

#include "asf.h"
#include "asf_meta.h"
#include "asf_raster.h"
#include "asf_license.h"
#include "trim_help.h"

int main(int argc, char *argv[])
{
  long long startX,startY,sizeX=-1,sizeY=-1;
  char *infile,*outfile;

  logflag=0;
  currArg=1;      /* from cla.h in asf.h, points to current argv string */
  printf("Program: trim\n\n");

/* Parse command line args */
  if (argc > 1) {
    check_for_help(argc, argv);
    handle_license_and_version_args(argc, argv, TOOL_NAME);
  }
  if (argc < 4) {
    asfPrintStatus("**Not enough arguments\n");
    usage();
    return 0;
  }

  while (currArg < (argc-4))
  {
    char *key=argv[currArg++];
    if (strmatch(key,"-width")) {
      CHECK_ARG(1) /*one integer argument: width */
      sizeX=atoi(GET_ARG(1));
    }
    else if (strmatch(key,"-height")) {
      CHECK_ARG(1) /*one integer argument: height */
      sizeY=atoi(GET_ARG(1));
    }
    else if (strmatch(key,"-log")) {
      CHECK_ARG(1) /*one string argument: logfile name */
      strcpy(logFile,GET_ARG(1));

      fLog = FOPEN(logFile, "a");
      logflag=1;
      StartWatchLog(fLog);
      printLog("Program: trim\n\n");
    }
    else {
      printf("   *****Unrecognized option keyword:  %s\n",argv[currArg-1]);
      usage(argv[0]);
    }
  }
  if ((argc-currArg) < 4) {
    printf("   Insufficient arguments.\n"); usage(argv[0]);
  }

 /*Compute filenames*/
  infile=argv[currArg];
  outfile=argv[currArg+1];
  startX=atoi(argv[currArg+3]);
  startY=atoi(argv[currArg+2]);

  /* Call library function */
  trim(infile, outfile, startX, startY, sizeX, sizeY);

  return(0);
}
