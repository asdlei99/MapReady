/********************************************************************
NAME:    metadata.c -- MAIN PROGRAM TO READ/TEST CEOS METADATA

SYNOPSIS:     metadata [ <record type> -all -save ] infile
 Where, the rectypes are:
   -dssr        Data Set Summary record
   -shr         Scene Header record
   -mpdr        Map Projection Data Record
   -ppdr        Platform Position Data record
   -atdr        Attitude Data record
   -ampr        ALOS Map Projection Data record
   -radr        Radiometric Data record
   -rcdr        Radiometric Compensation Data record
   -dqsr        Data Quality Summary record
   -pdhr        Processed Data Histograms record
   -sdhr        Signal Data Histograms record
   -rasr        Range Spectra record
   -ppr         Processing Parameter record
   -dqsr        Data Quality Summary record
   -facdr       ASF or ESA Facility Related Data record
   -asf_facdr   ASF Facility Related Data record
   -esa_facdr   ESA Facility Related Data record
   -jaxa_facdr  JAXA Facility Related Data record
   -ifdr        Image File Descriptor record
   -lfdr        Leader File Descriptor record
   -tfdr        Trailer File Descriptor record
   infile       Base name of the SAR image

 NOTE: NOT ALL OF THESE OPTIONS ARE INCLUDED IN THIS RELEASE. SEE USAGE.

DESCRIPTION:
    Reads infile.ext, where ext is one of L, D, ldr, trl, tlr, or dat
        to find the record type specified.  Once found, converts the data
        to a structure of values and prints the values out, either to the
        screen or to file as specified by the -f switch.

        This program handles both old ASF format image triplet (leader,
        data, trailer) and new ASF format pair (leader, data) files.
        New format files are "looked for" first.

EXTERNAL ASSOCIATES:
    NAME:               USAGE:
    ---------------------------------------------------------------

FILE REFERENCES:
    NAME:               USAGE:
    ---------------------------------------------------------------

PROGRAM HISTORY:
VERSION         DATE   AUTHOR
-------         ----   ------
  1.0           3/96   T. Logan (ASF)
  1.01          7/96   M. Shindle (ASF) - Corrected minor bug
  2.00          8/96   T. Logan (ASF)   - Expanded to access RADARSAT data
  2.01         12/96   T. Logan (ASF)   - Added check_cal() call
  2.02          7/97   D. Corbett       - Don't print mpdr if get_mpdr erred
  2.03          3/98   O. Lawlor (ASF)  - Moved prn_ceos types over to metadata,
                                          which is the only program that ever
                      uses them.
  2.04      6/99   M. Ayers (ASF)   - Added prn_fdr to print file descriptor
                      records.
  2.05      9/01   S. Watts     - Added case statements to display
                      Signal Data Histogram Records.
  2.3           3/02   P. Denny         - Update command line arguments
  2.31         12/06   B. Dixon         - Do not require .D and .L files (both)
  3.0           1/07   R. Gens, et al   - CLA cleanup

HARDWARE/SOFTWARE LIMITATIONS:

ALGORITHM DESCRIPTION:

ALGORITHM REFERENCES:

BUGS:

*********************************************************************/
/****************************************************************************
*                                           *
*   Metadata retrieves ceos structures from SAR metadata            *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include <ceos.h>
#include <asf.h>
#include <asf_meta.h>
#include <get_ceos_names.h>
#include <cla.h>
#include "asf_license.h"
#include <asf_version.h>
#include "metadisplay.h"
#include "metadata_help.h"

/*void usage(char *name)
{
   fprintf(stderr,"\n");
   fprintf(stderr,"USAGE:\n");
   fprintf(stderr,"  %s [ <option> ] infile\n",name);
   fprintf(stderr,"\n");
   fprintf(stderr,"OPTIONS:\n");
   fprintf(stderr,"  -save        Write the records to an output file\n");
   fprintf(stderr,"  -all         All records\n");
   fprintf(stderr,"  -dssr        Data Set Summary record\n");
   fprintf(stderr,"  -shr         Scene Header record\n");
   fprintf(stderr,"  -mpdr        Map Projection Data Record\n");
   fprintf(stderr,"  -ppdr        Platform Position Data record\n");
   fprintf(stderr,"  -atdr        Attitude Data record\n");
   fprintf(stderr,"  -ampr        ALOS Map Projection Data\n");
   fprintf(stderr,"  -radr        Radiometric Data record\n");
   fprintf(stderr,"  -ardr        ALOS Radiometric Data record\n");
   fprintf(stderr,"  -rcdr        Radiometric Compensation Data record\n");
   fprintf(stderr,"  -dqsr        Data Quality Summary record\n");
   fprintf(stderr,"  -pdhr        Processed Data Histograms record\n");
   fprintf(stderr,"  -sdhr        Signal Data Histograms record\n");
   fprintf(stderr,"  -rasr        Range Spectra record\n");
   fprintf(stderr,"  -ppr         Processing Parameter record\n");
   fprintf(stderr,"  -facdr       ASF or ESA Facility Related Data record\n");
   fprintf(stderr,"  -asf_facdr   Force ASF Facility Related Data record\n");
   fprintf(stderr,"  -esa_facdr   Force ESA Facility Related Data record\n");
   fprintf(stderr,"  -jaxa_facdr  Force JAXA Facility Related Data record\n");
   fprintf(stderr,"  -ifdr        Image File Descriptor record\n");
   fprintf(stderr,"  -lfdr        Leader File Descriptor record\n");
   fprintf(stderr,"  -tfdr        Trailer File Descriptor record\n");
   fprintf(stderr,"  -meta        Generate ASF internal metadata file\n");
   fprintf(stderr,"  -license     Print the ASF licensing statement and quit\n");
   fprintf(stderr,"  -version     Print the version number information and quit\n");
   fprintf(stderr,"\n");
   fprintf(stderr,"  infile    The base name of the CEOS image\n\n");
   fprintf(stderr,"\n");
   fprintf(stderr,"DESCRIPTION:\n");
   fprintf(stderr,"  The metadata program retrieves data record contents from CEOS format"
                  "  metadata files (a.k.a. \"leader files\") ...if the requested record\n"
                  "  exists.\n\n");
//   fprintf(stderr,"Version %s,  ASF SAR Tools\n\n",
//           CONVERT_PACKAGE_VERSION_STRING);
   fprintf(stderr,"\n");
   print_version(TOOL_NAME);
   exit (EXIT_FAILURE);
}*/

int main(int argc, char **argv)
{
  char *fileName, *outName;
  int found = 0;
  meta_parameters *meta;

  if (argc > 1) {
      check_for_help(argc, argv);
      handle_license_and_version_args(argc, argv, TOOL_NAME);
  }
  if (argc<2) {
      usage(TOOL_NAME);
      return 1;
  }

  int dssr_flag = extract_flag_options(&argc, &argv, "-dssr", "--dssr", NULL);
  int shr_flag = extract_flag_options(&argc, &argv, "-shr", "--shr", NULL);
  int mpdr_flag = extract_flag_options(&argc, &argv, "-mpdr", "--mpdr", NULL);
  int ppdr_flag = extract_flag_options(&argc, &argv, "-ppdr", "--ppdr", NULL);
  int atdr_flag = extract_flag_options(&argc, &argv, "-atdr", "--atdr", NULL);
  int ampr_flag = extract_flag_options(&argc, &argv, "-ampr", "--ampr", NULL);
  int radr_flag = extract_flag_options(&argc, &argv, "-radr", "--radr", NULL);
  int ardr_flag = extract_flag_options(&argc, &argv, "-ardr", "--ardr", NULL);
  int rcdr_flag = extract_flag_options(&argc, &argv, "-rcdr", "--rcdr", NULL);
  int dqsr_flag = extract_flag_options(&argc, &argv, "-dqsr", "--dqsr", NULL);
  int pdhr_flag = extract_flag_options(&argc, &argv, "-pdhr", "--pdhr", NULL);
  int sdhr_flag = extract_flag_options(&argc, &argv, "-sdhr", "--sdhr", NULL);
  int rasr_flag = extract_flag_options(&argc, &argv, "-rasr", "--rasr", NULL);
  int ppr_flag = extract_flag_options(&argc, &argv, "-ppr", "--ppr", NULL);
  int ifdr_flag = extract_flag_options(&argc, &argv, "-ifdr", "--ifdr", NULL);
  int facdr_flag =
          extract_flag_options(&argc, &argv, "-facdr", "--facdr", NULL);
  int asf_facdr_flag =
          extract_flag_options(&argc, &argv, "-asf_facdr", "--asf_facdr", NULL);
  int esa_facdr_flag =
          extract_flag_options(&argc, &argv, "-esa_facdr", "--esa_facdr", NULL);
  int jaxa_facdr_flag =
    extract_flag_options(&argc, &argv, "-jaxa_facdr", "--jaxa_facdr", NULL);
  int lfdr_flag = extract_flag_options(&argc, &argv, "-lfdr", "--lfdr", NULL);
  int tfdr_flag = extract_flag_options(&argc, &argv, "-tfdr", "--tfdr", NULL);
  int all_flag = extract_flag_options(&argc, &argv, "-all", "--all", NULL);
  int save = extract_flag_options(&argc, &argv, "-save", "--save", NULL);
  int meta_flag = extract_flag_options(&argc, &argv, "-meta", "--meta", NULL);

  if (dssr_flag || shr_flag || mpdr_flag || ppdr_flag || atdr_flag ||
      ampr_flag || radr_flag || rcdr_flag || dqsr_flag || pdhr_flag ||
      sdhr_flag || rasr_flag || ppr_flag || ifdr_flag || facdr_flag ||
      asf_facdr_flag || esa_facdr_flag || jaxa_facdr_flag || lfdr_flag ||
      tfdr_flag || ardr_flag || all_flag || meta_flag)
    found = 1;

  if (argc == 1 || !found)
    usage(argv[0]);

  fileName = (char *) MALLOC(sizeof(char)*(strlen(argv[1])+1));
  strcpy(fileName, argv[1]);

  if (meta_flag) {
    meta = meta_read(fileName);
    outName = appendExt(fileName, ".meta");
    meta_write(meta, outName);
    meta_free(meta);
  }
  if (dssr_flag || all_flag)
    output_record(fileName, ".dssr", 10, save);
  if (shr_flag || all_flag)
    output_record(fileName, ".shr", 18, save);
  if (mpdr_flag || all_flag)
    output_record(fileName, ".mpdr", 20, save);
  if (ppdr_flag || all_flag)
    output_record(fileName, ".ppdr", 30, save);
  if (atdr_flag || all_flag)
    output_record(fileName, ".atdr", 40, save);
  if (ampr_flag || all_flag)
    output_record(fileName, ".ampr", 44, save);
  if (radr_flag || all_flag)
    output_record(fileName, ".radr", 50, save);
  if (ardr_flag || all_flag)
    output_record(fileName, ".ardr", 50, save);
  if (rcdr_flag || all_flag)
    output_record(fileName, ".rcdr", 51, save);
  if (dqsr_flag || all_flag)
    output_record(fileName, ".dqsr", 60, save);
  if (pdhr_flag || all_flag)
    output_record(fileName, ".pdhr", 70, save);
  if (sdhr_flag || all_flag)
    output_record(fileName, ".shdr", 71, save);
  if (rasr_flag || all_flag)
    output_record(fileName, ".rasr", 80, save);
  if (ppr_flag || all_flag)
    output_record(fileName, ".ppr", 120, save);
  if (ifdr_flag || all_flag)
    output_record(fileName, ".ifdr", 192, save);
// Automatically choose which facility data record to look for
  if (facdr_flag || all_flag)
    output_record(fileName, ".facdr", 200, save);
// This option is left for users to force the program to look for this specific
// facility data record (not used with 'all' flag)
  if (asf_facdr_flag)
    output_record(fileName, ".asf_facdr", 210, save);
// This option is left for users to force the program to look for this specific
// facility data record (not used with 'all' flag)
  if (esa_facdr_flag)
    output_record(fileName, ".esa_facdr", 220, save);
  // This option is left for users to force the program to look for this
  // specific facility data record (not used with 'all' flag)
  if (jaxa_facdr_flag)
    output_record(fileName, ".jaxa_facdr", 230, save);
  if (lfdr_flag || all_flag)
    output_record(fileName, ".lfdr", 300, save);
  if (tfdr_flag || all_flag)
    output_record(fileName, ".tfdr", 193, save);

  exit (EXIT_SUCCESS);
}
