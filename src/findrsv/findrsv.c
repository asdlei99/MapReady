/******************************************************************
NAME:	    findrsv -- find the correct state vector in the orbit data file
			and write to RSV file (single state vec and time)
			for use with maskgen.

SYNOPSIS:   findrsv <inORBITfile> <inDEMfile> <outRSVfile>

DESCRIPTION:

	This program finds the raw state vectors in an ORBITfile, compares
them with the DEMfile and finally writes them to an RSVfile (.rsv).

FILE REFERENCES:
	---------------------------------------------------------
    inORBITfile:
    	name:  Vyyyy_ddd_RSV.D
    	--> contains list of orbit numbers and their corresponding
    		state vectors and time references, separated by eight minutes

    inDEMfile:
	The DEM file that will eventually be used with maskgen.

    outRSVfile:
		x(km)  y(km)  z(km)  x'(m/s)  y'(m/s)  z'(m/s)
		year, julian day, gmtSec
		delta time

EXTERNAL ASSOCIATES:
	---------------------------------------------------------
	propagate
	prop2ll
	ll2proj	
	
PROGRAM HISTORY:
    VERS:   DATE:  AUTHOR:        PURPOSE:
    --------------------------------------------------------------
    1.0      8/00   Mike Jessop    Find RSV for maskgen
    2.0     11/00   Joshua Badgley Modified to accept and use an Albers
                                   Conical Equal Area projection as well
				   as an Universal Transveral Mercator
                                   Projection.

*******************************************************************************

*******************************************************************************
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
#include "dateUtil.h"

#define  deltatime		30.0
#define  propahead		960.0	/* number of seconds to propagate ahead */

/*	nominal cut-off values, hilat & lolat--adjust appropriately 
	-->this program finds the state vector will cover from high latitude
	   to low latitude when propagated ahead by time 'propahead'  */
#define  hilat			71.5
#define  lolat			52.0


/* global declarations */
char RSVfile[255], DEMfile[225];

void execute(char *cmd)
{
	printf("%s",cmd);
	fflush(stdin);
	if (system(cmd)!=0) { printf("Program Aborted\n"); exit(1); }
}

void datestr_parse(const char *inStr,julian_date *juldate,hms_time *time)
{
	char buf[100];
	int sec,msec;
#define subStr(start,len,dest) strncpy(buf,&inStr[start],len);buf[len]=0;sscanf(buf,"%d",dest);
			
	subStr(0,3,&juldate->jd);
	subStr(4,2,&time->hour);
	subStr(7,2,&time->min);
	subStr(10,2,&sec);
	subStr(13,3,&msec);
	time->sec=sec+msec/1000.0;
}

void testssv(double rsv[], char *datestr, int year, int *itworks_flag)
{
    int			i;
    char		cmd[255], tmpstr[255],
    			inVec[255],outVec[255],
    			llfile[255],projfile[255];
    double		gmtSec,
    			tstlat;
    julian_date jld;
    ymd_date	ymd;
    hms_time	hms;
    
    FILE	*fpi, *fpo;
    
    *itworks_flag=1;  /* assume that it will work-- change if it doesn't */
    
    /* fix string length if required */
	strcpy(tmpstr, "");
	for (i=0; i<16-strlen(datestr); i++) { strcat(tmpstr, "0"); }
        strcat(tmpstr, datestr);
	strcpy(datestr, tmpstr);
	
    jld.year = year;
    datestr_parse(datestr, &jld, &hms);
    date_jd2ymd(&jld,&ymd);
    gmtSec = hms.hour*3600+hms.min*60+hms.sec;
        
    strcpy(inVec,"propIn.1");
	strcpy(outVec,"propOut.1");
	strcpy(llfile,"latlon.1");
	strcpy(projfile,"proj.1");
	
	/* Write the rsv file for use by prop2ll */
	fpo = fopen(RSVfile,"w");
	for (i=1; i<=6; i++) { fprintf(fpo, "%lf ", rsv[i]); }
	fprintf(fpo, "\n%d %d %lf",jld.year,jld.jd,gmtSec);
	fprintf(fpo, "\n%lf\n",propahead/2);
	fclose(fpo);
	
	/* Write input file for propagate and run */
	fpo = fopen(inVec,"w");
    for (i=1; i<=6; i++) { fprintf(fpo, "%lf\n", rsv[i]); }
    fprintf(fpo,"%d\n%d\n%d\n",ymd.year,ymd.month,ymd.day);
    fprintf(fpo,"%lf\n",gmtSec);
    fprintf(fpo,"%d\n%d\n%d\n",ymd.year,ymd.month,ymd.day);
    fprintf(fpo,"%lf\n",gmtSec+propahead);
    fprintf(fpo,"%d\n",(int)(propahead/deltatime));
    fclose(fpo);
    sprintf(cmd,"propagate %s %s\n",inVec,outVec); execute(cmd);
    sprintf(cmd,"prop2ll %s %s %s\n",RSVfile,outVec,llfile); execute(cmd);
    
    /* Failure of ll2proj indicates the state vector did not work */
    sprintf(cmd,"ll2proj %s %s %s\n",DEMfile,llfile,projfile);
    printf("%s",cmd); fflush(stdin);
    if (system(cmd)!=0) *itworks_flag=0;
    	else {  /* Second test to make sure it covers the whole scene */
    		fpi = fopen(llfile,"r");
	    	for (i=1; i<=5; i++) fscanf(fpi,"%lf",&tstlat);
	    	printf("LAT extreme 1: %lf\n",tstlat);
	    	if (tstlat<hilat&&tstlat>lolat) *itworks_flag=0;
	    	for (i=1; i<=(int)(5*(propahead/deltatime-1)); i++) 
	    		{ fscanf(fpi,"%lf",&tstlat); }
	    	printf("LAT extreme 2: %lf\n",tstlat);
	    	if (tstlat<hilat&&tstlat>lolat) *itworks_flag=0;
	    	fclose(fpi);
    	}
    
    /* Clean up temporary files */
    if (*itworks_flag) {
    	sprintf(cmd,"rm %s %s %s %s\n",inVec,outVec,llfile,projfile);
    	execute(cmd); }
    else {
    	printf("State vector failed: %s\n",datestr);
    	sprintf(cmd,"rm %s %s %s\n",inVec,outVec,llfile);
    	execute(cmd);
    	sprintf(cmd,"rm %s\n",RSVfile);
    	execute(cmd); }
    
}

int main (int argc, char *argv[]) {
    int		i, orbitnum, searchnum, year,
			found_flag=0, itworks_flag=0;
    char 	inFile[255], datestr[255];
    
    double	rsv[7];				/* raw state vector */
	
	FILE *fpi;
			
	/* Test for proper arguments */
    if (argc < 4 || argc > 5) { 
    	printf("Usage: %s <inORBITfile> <inDEMfile> <outRSVfile>  [ORBIT]\n",argv[0]);
    	exit(1); }
    if (argc == 4)  {
      printf("Enter orbit number to search for: "); scanf("%d", &searchnum);
    }
    else searchnum = atoi(argv[4]);

    strcpy(DEMfile,argv[2]);
    strcpy(inFile, argv[1]);
    strcat(strcpy(RSVfile,argv[3]),".rsv");
    
    /* Open input ORBIT file and parse the header */
    fpi = fopen(inFile, "r");
    if ( fpi == (FILE *) NULL) {
        printf("\n*** couldn't open data file: %s\n", inFile);
        exit(1); }
    for (i=1;i<=9;i++) fscanf(fpi, "%s", datestr);
    
    while (!feof(fpi) && !itworks_flag)
    {
		fscanf(fpi, "%d %d", &orbitnum, &year);
        fscanf(fpi, "%s", datestr);
		for (i=1; i<=6; i++) fscanf(fpi, "%lf", &rsv[i]);
		if (orbitnum==searchnum) {
	    	found_flag=1;
	    	testssv(rsv, datestr, year, &itworks_flag);
		}
    }
    fclose(fpi);
    if (!found_flag) { printf("Orbit number not found!!\n"); }
    else { 
    	if (!itworks_flag)	{printf("ERROR: No orbits cover the range\n");}
    		else {
    			printf("\n%s sucessfully completed!\n",argv[0]);
    			printf("Wrote to output file: %s\n",RSVfile);
    			printf("rsv date/time: %s\n\n",datestr); }
    	 }
    
    return(0);
}
