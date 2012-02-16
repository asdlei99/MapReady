/****************************************************************
NAME: stats

SYNOPSIS: stats [-mask <value>] [-log <logFile>] [-quiet]
                [-overmeta] [-nometa] [-overstat] [-nostat]
                [-startline <line>] [-startsample <sample>]
                [-width <width>] [-height <height>]
                [-trim <fraction>] <sar_name>

DESCRIPTION:
  Takes statistics on an image file and prints them out to
  a .stat file as well as inserting a stats structure in the
  .meta file.

EXTERNAL ASSOCIATES:
    NAME:                USAGE:
    ---------------------------------------------------------------

FILE REFERENCES:
    NAME:                USAGE:
    ---------------------------------------------------------------

PROGRAM HISTORY:

    VERS:   DATE:    AUTHOR:      PURPOSE:
    ---------------------------------------------------------------
    1.0     3/2003   P. Denny     Put sar image file statistics into
                                   meta file and stats file
    1.1     4/2003   B. Kerin     Added windowing functionality.
    1.2     5/2003   P. Denny     Fixed windowing bug,
                                    wrote stat_write and stat_read
    1.3     5/2004   R. Gens      Added trimming option

HARDWARE/SOFTWARE LIMITATIONS:
  None known

ALGORITHM DESCRIPTION:

ALGORITHM REFERENCES:

BUGS: None known

*******************************************************************************
*                                                                             *
*   Figures statistics for a SAR image file and dumps them to file            *
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
#include "stats.h"
#include "asf_license.h"
#include "asf_nan.h"

#define VERSION 1.3

/* For floating point comparisons.  */
#define MICRON 0.00000001
#ifdef FLOAT_EQUIVALENT
#  undef  FLOAT_EQUIVALENT
#  define FLOAT_EQUIVALENT(a, b) (fabs(a - b) < MICRON ? 1 : 0)
#endif

#define SQR(X) ((X)*(X))

stat_parameters calc_hist(stat_parameters stats, char *sar_name, int band, meta_parameters *meta,
                          double sum_of_samples, long samples_counted, int mask_flag);

static void
usage(char *name)
{
  printf("\n"
   "USAGE:\n"
   "   %s [-mask <value>] [-log <logFile>] [-quiet]\n"
   "         [-overmeta] [-nometa] [-overstat] [-nostat]\n"
   "         [-startline <line>] [-startsample <sample>]\n"
   "         [-width <width>] [-height <height>]\n"
   "         [-trim <fraction>] <sar_name>\n", name);
  printf("\n"
   "REQUIRED ARGUMENTS:\n"
   "   sar_name   Name of input image file (including extension)\n");
  printf("\n"
   "OPTIONAL ARGUMENTS:\n"
   "   -mask         Value to ignore while taking statistics.\n"
   "   -log          Copy terminal output to <logFile>.\n"
   "   -quiet        Supress terminal output.\n"
   "   -overmeta     Force overwrite of existing .meta file.\n"
   "   -nometa       Do not write a .meta file.\n"
   "   -overstat     Force overwrite of existing .stat file.\n"
   "   -nostat       Do not write a .stat file.\n"
   "   -startline    Start counting at image line <line> (implies -nometa).\n"
   "   -startsample  Start counting at image sample <sample> (implies -nometa).\n"
   "   -height       Only process <height> lines (implies -nometa).\n"
   "   -width        Only process <width> samples per line (implies -nometa).\n"
   "   -trim         Fraction used to trim the histogram.\n");
  printf("\n"
   "DESCRIPTION:\n"
   "   This program takes statistics on a SAR data file and writes them\n"
   "   with a histogram out to a .stat file. It also inserts a stats block\n"
   "   in the .meta file.\n");
  printf("\n"
   "Version %.2f, ASF SAR Tools\n"
   "\n",VERSION);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
  double min, max;             /* Minimum & maximum sample values       */
  double sum_of_samples=0.0;   /* Sum of all samples accounted for      */
  double sum_of_squared_samples=0.0; /* Sum of all squared samples accounted for*/
  double trim_fraction;        /* Fraction used to trim the histogram   */
  int ii;                      /* Loop index                            */
  long samples_counted=0;      /* Number of all samples accounted for   */
  float *data_line;           /* Buffer for a line of samples          */
  long line, sample;            /* Line and sample indices               */
  long num_lines, num_samples;  /* Number of lines and samples           */
  int percent_complete=0;      /* Percent of data sweep completed       */
  int overmeta_flag=FALSE;     /* If TRUE write over current .meta file */
  int overstat_flag=FALSE;     /* If TRUE write over current .stat file */
  int nometa_flag=FALSE;       /* If TRUE do not write .meta file       */
  int nostat_flag=FALSE;       /* If TRUE do not write .stat file       */
  int mask_flag=FALSE;         /* TRUE if user specifies a mask value   */
  int trim_flag=FALSE;         /* If TRUE trim histogram                */
  double mask=NAN;             /* Value to ignore while caculating stats*/
  char meta_name[261];         /* Meta file name                        */
  meta_parameters *meta;       /* SAR meta data structure               */
  char sar_name[256];          /* SAR file name WITH extention          */
  FILE *sar_file;              /* SAR data file pointer to take stats on*/
  stat_parameters *stats;      /* Statistics structure                  */
  char stat_name[261];         /* Stats file name                       */
  extern int currArg;          /* Pre-initialized to 1                  */

  /* We initialize these to a magic number for checking. */
  long start_line = -1;         /* Window starting line.                 */
  long start_sample = -1;       /* Window starting sample.               */
  long window_height = -1;      /* Window height in lines.               */
  long window_width = -1;       /* Window width in samples.              */

/* parse command line */
  handle_license_and_version_args(argc, argv, "stats");
  logflag=quietflag=FALSE;
  while (currArg < (argc-1)) {
    char *key = argv[currArg++];
    if (strmatch(key,"-quiet")) {
      quietflag=TRUE;
    }
    else if (strmatch(key,"-log")) {
      CHECK_ARG(1);
      strcpy(logFile,GET_ARG(1));
      fLog = FOPEN(logFile, "a");
      logflag=TRUE;
    }
    else if (strmatch(key,"-mask")) {
      CHECK_ARG(1);
      mask = atof(GET_ARG(1));
      mask_flag=TRUE;
    }
    else if (strmatch(key,"-overmeta")) {
      overmeta_flag=TRUE;
    }
    else if (strmatch(key,"-overstat")) {
      overstat_flag=TRUE;
    }
    else if (strmatch(key,"-nometa")) {
      nometa_flag=TRUE;
    }
    else if (strmatch(key,"-nostat")) {
      nostat_flag=TRUE;
    }
    else if (strmatch(key,"-startline")) {
      CHECK_ARG(1);
      nometa_flag=TRUE; /* Implied.  */
      start_line = atol(GET_ARG(1));
      if ( start_line < 0 ) {
        printf("error: -startline argument must be greater than or equal to zero\n");
        usage(argv[0]);
      }
    }
    else if (strmatch(key,"-startsample")) {
      CHECK_ARG(1);
      nometa_flag=TRUE; /* Implied.  */
      start_sample = atol(GET_ARG(1));
      if ( start_sample < 0 ) {
        printf("error: -startsample argument must be greater than or equal to zero\n");
        usage(argv[0]);
      }
    }
    else if (strmatch(key,"-width")) {
      CHECK_ARG(1);
      nometa_flag=TRUE; /* Implied.  */
      window_width = atol(GET_ARG(1));
      if ( window_width < 0 ) {
        printf("error: -width argument must be greater than or equal to zero\n");
        usage(argv[0]);
      }
    }
    else if (strmatch(key,"-height")) {
      CHECK_ARG(1);
      nometa_flag=TRUE; /* Implied.  */
      window_height = atol(GET_ARG(1));
      if ( window_height < 0 ) {
        printf("error: -height argument must be greater than or equal to zero\n");
        usage(argv[0]);
      }
    }
    else if (strmatch(key,"-trim")) {
      CHECK_ARG(1);
      trim_flag=TRUE; /* Implied.  */
      trim_fraction = atof(GET_ARG(1));
    }
    else {printf( "\n**Invalid option:  %s\n",argv[currArg-1]); usage(argv[0]);}
  }

  if ((argc-currArg)<1) {printf("Insufficient arguments.\n"); usage(argv[0]);}
  strcpy (sar_name, argv[currArg]);
  char *ext = findExt(sar_name);
  if (ext == NULL || strcmp("IMG", uc(ext)) != 0) {
    strcpy(sar_name, appendExt(sar_name, ".img"));
  }
  create_name(meta_name, sar_name, ".meta");
  create_name(stat_name, sar_name, ".stat");

  printf("\nProgram: stats\n\n");
  if (logflag) {
    fprintf(fLog, "\nProgram: stats\n\n");
  }
  printf("\nCalculating statistics for %s\n\n", sar_name);
  if (logflag) {
    fprintf(fLog,"\nCalculating statistics for %s\n\n", sar_name);
  }
  meta = meta_read(meta_name);
  num_lines = meta->general->line_count;
  num_samples = meta->general->sample_count;

  if ( start_line == -1 ) start_line = 0;
  if ( start_line > num_lines ) {
    printf("error: -startline argument is larger than index of last line in image\n");
    exit(EXIT_FAILURE);
  }
  if ( start_sample == -1 ) start_sample = 0;
  if ( start_sample > num_samples ) {
    printf("error: -startsample argument is larger than index of last sample in image\n");
    exit(EXIT_FAILURE);
  }
  if ( window_height == -1 ) window_height = num_lines;
  if ( start_line + window_height > num_lines ) {
    printf("warning: window specified with -startline, -height options doesn't fit in image\n");
  }
  if ( window_width == -1 ) window_width = num_samples;
  if ( start_sample + window_width > num_samples ) {
    printf("warning: window specified with -startsample, -width options doesn't fit in image\n");
  }

/* Make sure we don't over write any files that we don't want to */
  if (meta->stats && !overmeta_flag && !nometa_flag) {
    printf(" ** The meta file already has a populated statistics structure.\n"
           " ** If you want to run this program and replace that structure,\n"
           " ** then use the -overmeta option to do so. If you want to run\n"
           " ** this program, but don't want to replace the structure, use\n"
           " ** the -nometa option.\n");
    if (logflag) {
      fprintf(fLog,
      " ** The meta file already has a populated statistics structure.\n"
      " ** If you want to run this program and replace that structure,\n"
      " ** then use the -overmeta option to do so. If you want to run\n"
      " ** this program, but don't want to replace the structure, use\n"
      " ** the -nometa option.\n");
    }
    exit(EXIT_FAILURE);
  }
  if (fileExists(stat_name) && !overstat_flag && !nostat_flag) {
    printf(" ** The file, %s, already exists. If you want to\n"
           " ** overwrite it, then use the -overstat option to do so.\n"
           " ** If you want to run the progam but don't want to write\n"
           " ** over the current file, then use the -nostat option.\n",
           stat_name);
    if (logflag) {
      fprintf(fLog,
      " ** The file, %s, already exists. If you want to\n"
      " ** overwrite it, then use the -overstat option to do so.\n"
      " ** If you want to run the progam but don't want to write\n"
      " ** over the current file, then use the -nostat option.\n",
      stat_name);
    }
    exit(EXIT_FAILURE);
  }

/* Let user know the window in which the stats will be taken */
  if ((start_line!=0) || (start_sample!=0)
      || (window_height!=num_lines) || (window_width!=num_samples)) {
        if (!quietflag) {
      printf("Taking statistics on a window with upper left corner (%ld,%ld)\n"
      "  and lower right corner (%ld,%ld)\n",
      start_sample, start_line,
      window_width+start_sample, window_height+start_line);
    }
    if (logflag && !quietflag) {
      fprintf(fLog,
        "Taking statistics on a window with upper left corner (%ld,%ld)\n"
      "  and lower right corner (%ld,%ld)\n",
      start_sample, start_line,
      window_width+start_sample, window_height+start_line);
    }

  }

/* Allocate line buffer */
  data_line = (float *)MALLOC(sizeof(float)*num_samples);
  if (meta->stats) FREE(meta->stats);
  if (meta->general->band_count <= 0) {
    printf(" ** Band count in the existing data is missing or less than zero.\nDefaulting to one band.\n");
    if (logflag) {
      fprintf(fLog, " ** Band count in the existing data is missing or less than zero.\nDefaulting to one band.\n");
    }
    meta->general->band_count = 1;
  }
  meta->stats = meta_statistics_init(meta->general->band_count);
  if (!meta->stats) {
    printf(" ** Cannot allocate memory for statistics data structures.\n");
    if (logflag) {
      fprintf(fLog, " ** Cannot allocate memory for statistics data structures.\n");
    }
    exit(EXIT_FAILURE);
  }
  stats = (stat_parameters *)MALLOC(sizeof(stat_parameters) * meta->stats->band_count);
  if (!stats) {
    printf(" ** Cannot allocate memory for statistics data structures.\n");
    if (logflag) {
      fprintf(fLog, " ** Cannot allocate memory for statistics data structures.\n");
    }
    exit(EXIT_FAILURE);
  }

  int  band;
  long band_offset;
  for (band = 0; band < meta->stats->band_count; band++) {
    /* Find min, max, and mean values */
    if (!quietflag) printf("\n");
    if (logflag && !quietflag) fprintf(fLog,"\n");
    min = 100000000;
    max = -100000000;
    sum_of_samples=0.0;
    sum_of_squared_samples=0.0;
    percent_complete=0;
    band_offset = band * meta->general->line_count;
    sar_file = FOPEN(sar_name, "r");
    for (line=start_line+band_offset; line<start_line+window_height+band_offset; line++) {
      if (!quietflag) asfPercentMeter((float)(line-start_line-band_offset)/(float)(window_height-start_line));
      get_float_line(sar_file, meta, line, data_line);
      for (sample=start_sample; sample<start_sample+window_width; sample++) {
        if ( mask_flag && FLOAT_EQUIVALENT(data_line[sample],mask) )
          continue;
        if (data_line[sample] < min) min=data_line[sample];
        if (data_line[sample] > max) max=data_line[sample];
        sum_of_samples += data_line[sample];
        sum_of_squared_samples += SQR(data_line[sample]);
        samples_counted++;
      }
    }
    if (!quietflag) asfPercentMeter(1.0);
//    if (!quietflag) printf("\rFirst data sweep: 100%% complete.\n");
    FCLOSE(sar_file);

    stats[band].min = min;
    stats[band].max = max;
    stats[band].upper_left_line = start_line;
    stats[band].upper_left_samp = start_sample;
    stats[band].lower_right_line = start_line + window_height;
    stats[band].lower_right_samp = start_sample + window_width;
    stats[band].mask = mask;

    stats[band] = calc_hist(stats[band], sar_name, band, meta, sum_of_samples,
                      samples_counted, mask_flag);


  /* Remove outliers and trim the histogram by resetting the minimum and
    and maximum */
    if (trim_flag) {
      register int sum=0, num_pixels, minDex=0, maxDex=255;
      double overshoot, width;

      num_pixels = (int)(samples_counted*trim_fraction);
      minDex = 0;
      while (sum < num_pixels)
        sum += stats[band].histogram[minDex++];
      if (minDex-1>=0)
        overshoot = (double)(num_pixels-sum)/stats[band].histogram[minDex-1];
      else
        overshoot = 0;
      stats[band].min = (minDex-overshoot-stats[band].offset)/stats[band].slope;

      sum=0;
      while (sum < num_pixels)
        sum += stats[band].histogram[maxDex--];
      if (maxDex+1<256)
        overshoot = (double)(num_pixels-sum)/stats[band].histogram[maxDex+1];
      else
        overshoot = 0;
      stats[band].max = (maxDex+1+overshoot-stats[band].offset)/stats[band].slope;

      /* Widening the range for better visual effect */
      width = (stats[band].max-stats[band].min)*(1/(1.0-2*trim_fraction)-1);
      stats[band].min -= width/2;
      stats[band].max += width/2;

      /* Couple useful corrections borrowed from SARview */
      if ((stats[band].max-stats[band].min) < 0.01*(max-min)) {
        stats[band].max = max;
        stats[band].min = min;
      }
      if (min == 0.0)
        stats[band].min=0.0;
      if (stats[band].min == stats[band].max)
        stats[band].max = stats[band].min + MICRON;

      stats[band].slope = 255.0/(stats[band].max-stats[band].min);
      stats[band].offset = -stats[band].slope*stats[band].min;

      stats[band] = calc_hist(stats[band], sar_name, band, meta, sum_of_samples,
                        samples_counted, mask_flag);
    }
  }
  if(data_line)FREE(data_line);

  /* Populate meta->stats structure */
  char **band_names = NULL;
  if (meta_is_valid_string(meta->general->bands) &&
      strlen(meta->general->bands)               &&
      meta->general->band_count > 0)
  {
    band_names = extract_band_names(meta->general->bands, meta->general->band_count);
  }
  else {
    if (meta->general->band_count <= 0) meta->general->band_count = 1;
    band_names = (char **) MALLOC (meta->general->band_count * sizeof(char *));
    int i;
    for (i=0; i<meta->general->band_count; i++) {
      band_names[i] = (char *) MALLOC (64 * sizeof(char));
      sprintf(band_names[i], "%02d", i);
    }
  }
  int band_no;
  for (band_no = 0; band_no < meta->stats->band_count; band_no++) {
    strcpy(meta->stats->band_stats[band_no].band_id, band_names[band_no]);
    meta->stats->band_stats[band_no].min = stats[band_no].min;
    meta->stats->band_stats[band_no].max = stats[band_no].max;
    meta->stats->band_stats[band_no].mean = stats[band_no].mean;
    meta->stats->band_stats[band_no].rmse = stats[band_no].rmse;
    meta->stats->band_stats[band_no].std_deviation = stats[band_no].std_deviation;
    meta->stats->band_stats[band_no].mask = stats[band_no].mask;
  }
  if (band_names) {
    int i;
    for (i=0; i<meta->general->band_count; i++) {
      if (band_names[i]) FREE (band_names[i]);
    }
    FREE(band_names);
  }

/* Print findings to the screen (and log file if applicable)*/
  if (!quietflag) {
    printf("\n");
    printf("Statistics found:\n");
    if (mask_flag)
      { printf("Used mask %-16.11g\n",mask); }
    printf("Number of bands: %d\n", meta->stats->band_count);
    for (band=0; band<meta->stats->band_count; band++) {
      printf("\n\nBand name = \"%s\"\n", meta->stats->band_stats[band].band_id);
      printf("Minimum = %-16.11g\n",stats[band].min);
      printf("Maximum = %-16.11g\n",stats[band].max);
      printf("Mean = %-16.11g\n",stats[band].mean);
      printf("Root mean squared error = %-16.11g\n",
            stats[band].rmse);
      printf("Standard deviation = %-16.11g\n",
            stats[band].std_deviation);
      printf("\n");
      printf("Data fit to [0..255] using equation:  byte = %g * sample + %g\n",
            stats[band].slope, stats[band].offset);
                  if (trim_flag)
                    printf("Trimming fraction = %.3g\n", trim_fraction);
      printf("\n");
      printf("Histogram:\n");
      for (ii=0; ii<256; ii++) {
        if (ii%8 == 0) {
          printf("%s%3i-%3i:",
            (ii==0) ? "" : "\n",
            ii, ii+7);
        }
        printf(" %8i", stats[band].histogram[ii]);
      }
      printf("\n");
    }
  }
  if (logflag && !quietflag) {
    fprintf(fLog,"Statistics found:\n");
    if (mask_flag)
      { fprintf(fLog,"Used mask %-16.11g\n",mask); }
    fprintf(fLog,"Number of bands: %d\n", meta->stats->band_count);
    for (band=0; band<meta->stats->band_count; band++) {
      fprintf(fLog,"\n\nBand name = \"%s\"\n", meta->stats->band_stats[band].band_id);
      fprintf(fLog,"Minimum = %-16.11g\n",stats[band].min);
      fprintf(fLog,"Maximum = %-16.11g\n",stats[band].max);
      fprintf(fLog,"Mean = %-16.11g\n",stats[band].mean);
      fprintf(fLog,"Root mean squared error = %-16.11g\n",
             stats[band].rmse);
      fprintf(fLog,"Standard deviation = %-16.11g\n",
             stats[band].std_deviation);
      fprintf(fLog,"\n");
      fprintf(fLog,"Data fit to [0..255] using equation:  byte = %g * sample + %g\n",
             stats[band].slope, stats[band].offset);
      if (trim_flag)
        fprintf(fLog,"Trimming fraction = %.3g\n", trim_fraction);
      fprintf(fLog,"\n");
      fprintf(fLog,"Histogram:\n");
      for (ii=0; ii<256; ii++) {
        if (ii%8 == 0) {
          fprintf(fLog,"%s%3i-%3i:",
                 (ii==0) ? "" : "\n",
                 ii, ii+7);
        }
        fprintf(fLog," %8i", stats[band].histogram[ii]);
      }
      fprintf(fLog,"\n");
    }
  }

/* Write out .meta and .stat files */
  if (!nometa_flag) meta_write(meta, meta_name);
  if (!nostat_flag) stat_write(stats, stat_name, meta->stats->band_count);

/* Free the metadata structure */
  meta_free(meta);

/* Report */
  if (!quietflag) {
    printf("\n");
    printf("Statistics taken on image file %s.\n",sar_name);
    if (!nometa_flag)
      printf("Statistics written to the stats block in %s.\n",
        meta_name);
    if (!nostat_flag)
      printf("Statistics plus histogram written to %s.\n",
        stat_name);
    printf("\n");
  }
  if (logflag && !quietflag) {
    fprintf(fLog,"\n");
    fprintf(fLog,"Statistics taken on image file '%s'\n",sar_name);
    if (!nometa_flag)
      fprintf(fLog,"Statistics written to the stats block in %s\n",
        meta_name);
    if (!nostat_flag)
      fprintf(fLog,"Statistics plus histogram written to %s\n",
        stat_name);
    fprintf(fLog,"\n");
  }

  if (fLog) FCLOSE(fLog);
  return 0;
}

/* Create histogram of the data and get the summation of the
 * square of (sample-mean) to use in calculation of rmse & stdev */
stat_parameters calc_hist(stat_parameters stats, char *sar_name, int band, meta_parameters *meta,
                          double sum_of_samples, long samples_counted, int mask_flag)
{
  FILE   *fp;
  long   start_line, start_sample, window_height, window_width;
  long   percent_complete, line, sample, ii, band_offset;
  double diff_squared_sum=0.0;
  float  *data_line;

  start_line = stats.upper_left_line;
  start_sample = stats.upper_left_samp;
  window_height = stats.lower_right_line - start_line;
  window_width = stats.lower_right_samp - start_sample;

  data_line = (float *)MALLOC(sizeof(float)*meta->general->sample_count);
  fp = FOPEN(sar_name, "r");

  /* Initialize the histogram array */
  for (ii=0; ii<256; ii++) stats.histogram[ii] = 0;

  if (meta->general->data_type != BYTE) {
    /* Set slope and offset, to map pixels to [0..255].
     * byte = slope * in + offset
     * 0    = slope * min + offset
     * 255  = slope * max + offset
     * Therefore: */
    stats.slope = 255.0 / (stats.max-stats.min);
    stats.offset = -stats.slope * stats.min;
  }

  /* Get histogram of the data.  If its byte data just slap it into the
   * histogram; otherwise use slope & offset to scale the data */
  stats.mean = sum_of_samples / (double)samples_counted;
  percent_complete=0;
  band_offset = band * meta->general->line_count;
  for (line=start_line+band_offset; line<start_line+window_height+band_offset; line++) {
    if (!quietflag) asfPercentMeter((float)(line-start_line-band_offset)/(float)(window_height-start_line));
    get_float_line(fp, meta, line, data_line);
    for (sample=start_sample; sample<start_sample+window_width; sample++) {
      int bin = (meta->general->data_type == BYTE)
        ? (int)data_line[sample]
        : (int)(stats.slope*data_line[sample]+stats.offset);
      if (bin < 0) bin = 0;
      else if (bin > 255) bin = 255;
      if ( mask_flag && FLOAT_EQUIVALENT(data_line[sample],stats.mask) )
        continue;
      stats.histogram[bin]++;
      diff_squared_sum += SQR(data_line[sample] - stats.mean);
    }
  }
  if (!quietflag) asfPercentMeter(1.0);

  FREE(data_line);
  FCLOSE(fp);

/* Populate stats structure */
  stats.rmse =
    sqrt( fabs( diff_squared_sum / (double)samples_counted ) );
  stats.std_deviation =
    sqrt( fabs( diff_squared_sum / (double)(samples_counted-1) ) );

  return stats;
}
