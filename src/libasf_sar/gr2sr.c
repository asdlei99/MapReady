/******************************************************************************
NAME:   gr2sr - Remaps a ground range image to a slant range image

DESCRIPTION:
        Remaps a ground range image to a slant range image using resampling
        vectors calculated by the  gr2ml_vec (azimuth resampling vector) and
        gr2sr_vec (range resampling vector) subroutines.  The resampling
        uses bi-linear interpolation based on the vectors calculated.

FILE REFERENCES:
    NAME:               USAGE:
    ---------------------------------------------------------------
    infile.img          Input data file
    infile.meta         Input metadata about image file

HARDWARE/SOFTWARE LIMITATIONS:

ALGORITHM DESCRIPTION:

ALGORITHM REFERENCES:

BUGS:

******************************************************************************/
#include "asf_sar.h"
#include "asf_raster.h"
#include <asf.h>
#include <asf_meta.h>

#define VERSION 0.1

static void gr2sr_vec(meta_parameters *meta, float srinc, float *gr2sr,
                      int apply_pp_earth_radius_fix)
{
  int    i;             /* Counter                                       */
  float  r_sc;          /* radius from center of the earth for satellite */
  float  r_earth;       /* radius of the earth                           */
  float  r_close;       /* near slant range distance                     */
  float  rg0, rg;        /* Ground range to first pixel, G.R. to cur pix  */
  float  a, x, x2, y;   /* temporaries                                   */
  float  grinc;  /* Slant and Ground range pixel spacings         */
  float  rslant;        /* slant range distance to current pixel         */

  /* Radius from the center of the earth to the spacecraft, slant range to
     first pixel, and radius of the earth */
  r_sc = meta_get_sat_height(meta, 0, 0);
  r_close = meta_get_slant(meta,0,0);

  if (apply_pp_earth_radius_fix)
      r_earth = meta_get_earth_radius_pp(meta);
  else
      r_earth = meta_get_earth_radius(meta,0,0);

  /* Set the ground range and slant range increments */
  grinc = meta->general->x_pixel_size;

  /* calculate ground range to first point */
  a = (r_sc-r_earth)/r_earth;
  x = 1.0+a;
  x2 = x * x;
  y = r_close/r_earth;
  rg0 = r_earth * acos((1.0 + x2 - y*y) / (2.0*x));

  /* begin loop */
  for(i = 0; i<MAX_IMG_SIZE; i++) {
    rslant = r_close + i *srinc;
    y = rslant/r_earth;
    rg = r_earth*acos((1.0+x2-y*y)/(2.0*x));
    gr2sr[i] = (rg - rg0)/grinc;
  }
}

static char * replExt(const char *filename, const char *ext)
{
  char *ret = MALLOC(sizeof(char)*(strlen(filename)+strlen(ext)+5));
  strcpy(ret, filename);

  char *p = findExt(ret);
  if (p) *p = '\0';

  if (ext[0] != '.') strcat(ret, ".");
  strcat(ret, ext);

  return ret;
}

static int gr2sr_pixsiz_imp(const char *infile, const char *outfile,
                            float srPixSize, int apply_pp_earth_radius_fix)
{
  meta_parameters *inMeta, *outMeta;

  int   np, nl;         /* in number of pixels,lines       */
  int   onp, onl;       /* out number of pixels,lines      */
  int   nBands;         /* number of bands in input/output */
  int   ii;
  float *gr2sr;    /* GR 2 SR resampling vector for Range  */
  int   *lower;    /* floor of gr2sr vector                */
  int   *upper;    /* ceiling of gr2sr vector              */
  float *ufrac;    /* Upper fraction from gr2sr vector     */
  float *lfrac;    /* Lower fraction from gr2sr vector     */

  float *inBuf;          /* Input buffer                  */
  float *outBuf;         /* Output buffer                 */
  FILE  *fpi, *fpo;      /* File pointers                 */
  int   line;            /* Loop counter                  */
  int   band;            /* Loop counter                  */
  char  *iimgfile;       /* .img input file               */
  char  *oimgfile;       /* .img output file              */
 
  gr2sr = (float *) MALLOC(sizeof(float) * MAX_IMG_SIZE);
  upper = (int *) MALLOC(sizeof(int) * MAX_IMG_SIZE);
  lower = (int *) MALLOC(sizeof(int) * MAX_IMG_SIZE);
  ufrac = (float *) MALLOC(sizeof(float) * MAX_IMG_SIZE);
  lfrac = (float *) MALLOC(sizeof(float) * MAX_IMG_SIZE);

  inMeta = meta_read(infile);

  if (srPixSize < 0) {
    /*
      In an e-mail from Rick:
       Slant Range Pixel Size = C (speed of light) / [SampleRate (sample 
       rate) * 2,000,000.]

       SampleRate can be extracted from the L1 metadata :
          RNG CMPLX SAMPLE RATE        18.9599991

       The meta->sar->range_sampling_rate is 10^6 times the value above,
       so we use C/(2*meta->sar->range_sampling_rate)
    */
    srPixSize = SPD_LIGHT / ((2.0 * inMeta->sar->range_sampling_rate) *
      inMeta->general->sample_count / inMeta->sar->original_sample_count);
  }

  nl = inMeta->general->line_count;
  np = inMeta->general->sample_count;
  nBands = inMeta->general->band_count;

  char **band_name = extract_band_names(inMeta->general->bands, nBands);

  onl=nl;
  gr2sr_vec(inMeta, srPixSize, gr2sr, apply_pp_earth_radius_fix);
  
  /* Determine the output image size */
  onp = 0;
  for (ii=0; ii<MAX_IMG_SIZE; ii++) {
     if (gr2sr[ii]<np) onp=ii; /* gr input still in range-- keep output */
     else break; /* gr input is off end of image-- stop sr output */
  }
  
  /* Split gr2sr into resampling coefficients */
  for (ii=0; ii<onp; ii++) {
     lower[ii] = (int) gr2sr[ii];
     upper[ii] = lower[ii] + 1;
     ufrac[ii] = gr2sr[ii] - (float) lower[ii];
     lfrac[ii] = 1.0 - ufrac[ii];
     if (lower[ii]>=np) lower[ii]=np-1; /* range clip */
     if (upper[ii]>=np) upper[ii]=np-1; /* range clip */
  }
  
  outMeta = meta_read(infile);
  outMeta->sar->slant_shift += ((inMeta->general->start_sample)
                                * inMeta->general->x_pixel_size);
  outMeta->general->start_sample = 0.0;
  outMeta->sar->sample_increment = 1.0;
  outMeta->sar->image_type       = 'S';
  outMeta->general->x_pixel_size = srPixSize;
  outMeta->general->sample_count = onp;

  iimgfile = replExt(infile, "img");
  oimgfile = replExt(outfile, "img");

  fpi = FOPEN(iimgfile,"rb");
  fpo = FOPEN(oimgfile,"wb");
  inBuf = (float *) MALLOC (np*sizeof(float));
  outBuf = (float *) MALLOC (onp*sizeof(float));

  for (band = 0; band < nBands; band++) {
    if (inMeta->general->band_count != 1)
      asfPrintStatus("Converting to slant range: band %s\n", band_name[band]);
    for (line = 0; line < onl; line++) {
      get_float_line(fpi, inMeta, line + band*onl, inBuf);
      for (ii=0; ii<onp; ii++) { /* resample to slant range */
         outBuf[ii] = inBuf[lower[ii]]*lfrac[ii]+inBuf[upper[ii]]*ufrac[ii];
      }
      put_float_line(fpo,outMeta,line + band*onl,outBuf);
      asfLineMeter(line,onl);
    }
  }

  for (ii=0; ii < inMeta->general->band_count; ii++)
    FREE(band_name[ii]);
  FREE(band_name);

  meta_write(outMeta, outfile);
  meta_free(inMeta);
  meta_free(outMeta);

  FREE(ufrac);
  FREE(lfrac);
  FREE(gr2sr);
  FREE(upper);
  FREE(lower);

  FREE(inBuf);
  FREE(outBuf);
  FCLOSE(fpi);
  FCLOSE(fpo);
  FREE(iimgfile);
  FREE(oimgfile);

  return TRUE;
}

int gr2sr(const char *infile, const char *outfile)
{
  return gr2sr_pixsiz(infile, outfile, -1);
}

int gr2sr_pixsiz(const char *infile, const char *outfile,
                 float srPixSize)
{
    return gr2sr_pixsiz_imp(infile, outfile, srPixSize, FALSE);
}

int gr2sr_pixsiz_pp(const char *infile, const char *outfile,
                    float srPixSize)
{
    return gr2sr_pixsiz_imp(infile, outfile, srPixSize, TRUE);
}

