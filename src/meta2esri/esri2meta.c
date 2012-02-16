/******************************************************************************
*                                                                             *
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
#include "asf_nan.h"
#include "esri.h"
#include "asf_meta.h"
#include <ctype.h>

#define VERSION 1.0


void usage(char *name)
{
  printf("\n"
	 "USAGE:\n"
	 "   %s [ -log <logFile> ] <esri_name> <meta_name>\n",name);
  printf("\n"
	 "REQUIRED ARGUMENTS:\n"
	 "   esri_name   Base name of the ESRI header file.\n"
	 "   meta_name   Base name of the new style meta data.");
  printf("\n"
	 "DESCRIPTION:\n"
	 "   %s converts an ESRI header file to a new style metadata.\n",
	 name);
  printf("\n"
	 "Version %.2f, ASF SAR Tools\n"
	 "\n",VERSION);
  exit(EXIT_FAILURE);
}


int main(int argc, char **argv)
{
  char meta_name[255];
  char esri_name[255];
  meta_parameters *meta=NULL;
  esri_header *esri=NULL;
  FILE *fp;
  char line[255]="", key[25]="", value[25]="";
  extern int currArg; /* from cla.h in asf.h... initialized to 1 */
  logflag = 0;

  /* Parse command line args */
  while (currArg < (argc-2))
    {
      char *key=argv[currArg++];
      if (strmatch(key,"-log")) {
	sprintf(logFile, "%s", argv[currArg]);
	logflag = 1;
      }
      else {
	printf("\n   ***Invalid option:  %s\n\n",
	       argv[currArg-1]);
	usage(argv[0]);
      }
    }
  if ((argc-currArg) < 2) {
    printf("Insufficient arguments.\n"); 
    usage(argv[0]);
  }

  create_name(esri_name, argv[currArg], ".hdr");
  create_name(meta_name, argv[currArg+1], ".meta");

  asfSplashScreen(argc, argv);
  
  /* Allocate memory for ESRI header structure */
  esri = (esri_header *)MALLOC(sizeof(esri_header));

  /* Read .hdr and fill meta structures */ 
  fp = FOPEN(esri_name, "r");
  while (NULL != fgets(line, 255, fp)) {
    sscanf(line, "%s %s", key, value);
    if (strncmp(key, "NROWS", 5)==0) esri->nrows = atoi(value);
    else if (strncmp(key, "NCOLS", 5)==0) esri->ncols = atoi(value);
    else if (strncmp(key, "NBITS", 5)==0) {
      esri->nbits = atoi(value);
      if (esri->nbits < 8) {
        sprintf(errbuf, "\n   ERROR: metadata do not support data less than 8 bit\n\n");
        printErr(errbuf);
      }
    } 
    else if (strncmp(key, "NBANDS", 6)==0)
      esri->nbands = atoi(value);
    else if (strncmp(key, "BYTEORDER", 9)==0) esri->byteorder = value[0];
    else if (strncmp(key, "LAYOUT", 6)==0) {
      sprintf(esri->layout, "%s", value);
      if (strncmp(uc(esri->layout), "BSQ", 3)!=0) {
        sprintf(errbuf, "\n   ERROR: metadata do not support data other than BSQ format\n\n");
        printErr(errbuf);
      }
    }
    else if (strncmp(key, "SKIPBYTES", 9)==0) {
      esri->skipbytes = atoi(value);
      if (esri->skipbytes > 0) {
        sprintf(errbuf, "\n   ERROR: metadata only support generic binary data\n\n");
        printErr(errbuf);
      }
    }
    else if (strncmp(key, "ULXMAP", 6)==0) esri->ulxmap = atof(value);
    else if (strncmp(key, "ULYMAP", 6)==0) esri->ulymap = atof(value);
    else if (strncmp(key, "XDIM", 4)==0) esri->xdim = atof(value);
    else if (strncmp(key, "YDIM", 4)==0) esri->ydim = atof(value);
    /* bandrowbytes, totalrowbytes, bandgapdata and nodata currently not used */
  }
  FCLOSE(fp);

  /* Fill metadata structure with valid data */
  meta = esri2meta(esri);
  
  /* Write metadata file */
  meta_write(meta, meta_name);

  /* Clean and report */
  meta_free(meta);
  asfPrintStatus("   Converted ESRI header (%s) to metadata file (%s)\n\n",
		 esri_name, meta_name);
  
  return 0;
}
