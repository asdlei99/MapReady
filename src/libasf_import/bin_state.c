/****************************************************************
FUNCTION NAME:
  Interface to the "bin_state" structure. This structure holds the
  state information used during the decoding/analysis of a Level
  Zero Product binary file.

  Also contains metadata interfaces for same.

SYNTAX:

PARAMETERS:
    NAME:       TYPE:           PURPOSE:
    --------------------------------------------------------

DESCRIPTION:

RETURN VALUE:

SPECIAL CONSIDERATIONS:

PROGRAM HISTORY:
  DATE:    AUTHOR:        DESCRIPTION:
  --------------------------------------------------------------------
  ??/????  Orion Lawlor  Original Creation
  10/2000  R. Gens       Modified updateAGC_window to allow for bit
                           errors
  03/2002  R. Gens       Modified earth radius and satellite height
                           calculation
  01/2004  R. Gens       Updated old to new metadata
  05/2004  P. Denny      Spruced up metadata & bin_state interaction
                           Initialize bin_state 'nLines' line
                           incrementer at 0

****************************************************************/

#include "decoder.h"
#include "missing.h"
#include "lzFetch.h"
#include "get_stf_names.h"

/*********************************************************************
 * new_bin_state:
 * Allocate, set to defaults, and return a new bin_state structure. */
bin_state *new_bin_state(void)
{
  int i;
  bin_state *s=(bin_state *)MALLOC(sizeof(bin_state));
  strcpy(s->satName,"Unknown");
  strcpy(s->beamMode,"STD");
  s->zeroDopSteered=0;
  s->nPulseInAir=0;
  s->nSamp=0;
  s->nBeams=1;    /*Normally, you have just one beam*/
  s->slope=0;     /*chirp slope, Hz/sec.*/
  s->frequency=0; /*radar wavelength, in m.*/
  s->fs=0;        /*Range sampling frequency, Hz*/
  s->prf=0;       /*Pulse Repetition Freqency*/
  s->prf_code=-1;
  s->dwp=0;
  s->dwp_code=-1;
  s->range_gate=0;
  s->time_code=0;

  s->binary=NULL;
  s->curFrame=0;
  s->bytesPerFrame=0;
  s->bytesInFile=0;
  s->missing=NULL;

  s->nValid=0;
  s->estDop=0.0;
  s->I_BIAS=s->Q_BIAS=0.0;

  // These defaults are for ERS...
  s->re=6363000.989; /*approximate earth radius at scene center.*/
  s->vel=7463.989;   /*satellite velocity, m/s.*/
  s->ht=792000.989;  /*satellite height above earth, m.*/
  s->pulsedur=0;     /*chirp length, in sec.*/
  s->lookDir='R';
  s->azres=8.0;      /* Desired azimuth resolution (m)*/
  s->nLooks=5;       /* Number of looks to square up data */
  s->nLines=0;       /* Start the line count at 0. */

  s->re=0.0; /*approximate earth radius at scene center.*/
  s->vel=0.0;   /*satellite velocity, m/s.*/
  s->ht=0.0;  /*satellite height above earth, m.*/
  s->pulsedur=0.0;     /*chirp length, in sec.*/
  s->lookDir='R';
  s->azres=0.0;      /* Desired azimuth resolution (m)*/
  s->nLooks=0;       /* Number of looks to square up data */
  s->nLines=0;       /* Start the line count at 0. */

  s->dotFMT=NULL;
  for (i=0;i<MAX_BEAMS;i++)
    s->firstFrame[i]=NULL;

  return s;
}

/*********************************************************************
 * Delete_bin_state:
 * destroys the given bin_state structure. */
void delete_bin_state(bin_state *s)
{
  int i;
  strcpy(s->satName,"Deleted...");
  if (s->binary)
    FCLOSE(s->binary);
  if (s->dotFMT)
    FCLOSE(s->dotFMT);
  for (i=0;i<MAX_BEAMS;i++)
    if (s->firstFrame[i]!=NULL)
      FREE(s->firstFrame[i]);
  if (s->missing!=NULL)
    freeMissing(s);
  FREE(s);
}

/*********************************************************************
 * updateMeta:
 * Writes the satellite (bin_state struct) fields into the given
 * meta_parameters structure.  */
void updateMeta(bin_state *s,meta_parameters *meta,char *inN,int stfFlag)
{
  char *parN, *ellipsoid;

  parN = (char *) MALLOC(sizeof(char)*512);

  strcpy(meta->general->sensor, s->satName);
  strcpy(meta->general->mode,   s->beamMode);
  meta->general->line_count   = s->nLines;
  meta->general->sample_count = s->nSamp;
  meta->general->x_pixel_size = 1.0/s->fs * (speedOfLight/2.0);
  meta->general->y_pixel_size = 1.0/s->prf * s->vel * (s->re/(s->re+s->ht));

  meta->sar->image_type              = 'S';          /*Slant range product*/
  meta->sar->look_direction          = s->lookDir;
  meta->sar->look_count              = s->nLooks;
  meta->sar->deskewed                = 0;           /*Not doppler deskewed*/
  meta->sar->range_time_per_pixel    = 1.0/s->fs;
  meta->sar->azimuth_time_per_pixel  = 1.0/s->prf;
  meta->sar->slant_shift             = 0.0;
  meta->sar->time_shift              = 0.0;
  meta->sar->slant_range_first_pixel = s->range_gate*speedOfLight/2.0;
  meta->sar->wavelength              = speedOfLight/s->frequency;
  meta->sar->prf                     = s->prf;
  if (!meta_is_valid_double(meta->sar->earth_radius))
    meta->sar->earth_radius          = s->re;
  if (!meta_is_valid_double(meta->sar->satellite_height))
    meta->sar->satellite_height      = s->re+s->ht;
  sprintf(meta->sar->satellite_binary_time,"%f",s->time_code);

  if (stfFlag) {
    get_stf_metadata_name(inN, &parN);
    strcpy(meta->general->processor, "ASF/IMPORT2ASF");
    meta->general->data_type = COMPLEX_REAL32;
    meta->general->image_data_type = RAW_IMAGE;
    meta->general->orbit = lzInt(parN,"prep_block.OrbitNr:",NULL);
    if (meta->state_vectors->vecs[0].vec.vel.z > 0)
      meta->general->orbit_direction  = 'A';
    else if (meta->state_vectors->vecs[0].vec.vel.z < 0)
      meta->general->orbit_direction  = 'D';
    meta->general->band_count   = 1;
    meta->general->start_line   = 0;
    meta->general->start_sample = 0;
    meta->general->bit_error_rate =
      lzDouble(parN,"prep_block.bit_error_rate:",NULL);
    meta->general->missing_lines =
      lzDouble(parN,"prep_block.missing_lines:",NULL);
    meta->sar->original_line_count   = s->nLines;
    meta->sar->original_sample_count = s->nSamp;
    meta->sar->line_increment        = 1.0;
    meta->sar->sample_increment      = 1.0;
    ellipsoid = lzStr(parN,"prep_block.ellipsoid_name:",NULL);
    if (strncmp(ellipsoid,"GEM6", 4)==0) {
      meta->general->re_major = 6378144.0;
      meta->general->re_minor = 6356754.9;
    }
    if (!meta_is_valid_double(meta->sar->earth_radius))
      meta->sar->earth_radius     = s->re;
    if (!meta_is_valid_double(meta->sar->satellite_height))
      meta->sar->satellite_height = s->re+s->ht;
    if (!meta_is_valid_string(meta->sar->satellite_clock_time)) {
      strcpy (meta->sar->satellite_clock_time, "0");
    }
  }
  FREE(parN);
}

/*********************************************************************
 * AddStateVector:
 * Updates the Earth Radius, spacecraft hieght, velocity, etc. using
 * the given state vector.
 * Format:
 *  stVec[0-2]: Earth-Fixed position, in meters.
 *  stVec[3-5]: Earth-Fixed velocity, in meters/second.  */
void addStateVector(bin_state *s,stateVector *stVec)
{
  double latCen; /*Geocentric latitude of state vector, in radians.*/
  double er;     /*Radius of earth under state vector, in m.*/

  /* Use state vector to estimate latitude.
   ---------------------------------------*/
  latCen=atan(stVec->pos.z/
    sqrt(stVec->pos.x*stVec->pos.x+stVec->pos.y*stVec->pos.y));

  /* Use the latitude to determine earth's (ellipsoidal) radius.
   -----------------------------------------------------------*/
  er=er_polar/sqrt(1-ecc2/(1+tan(latCen)*tan(latCen)));

  /* Now write all these parameters into satellite structure.
   --------------------------------------------------------*/
  s->re=er;
  s->ht=vecMagnitude(stVec->pos)-er;
  s->vel=vecMagnitude(stVec->vel);

  asfPrintStatus("   Updating for more accurate earth radius (%.2f), \n"
                 "   height (%.2f), and velocity (%.2f).\n",
                 s->re,s->ht,s->vel);

  /* Estimate the doppler value at beam center
   ------------------------------------------*/
  if (!s->zeroDopSteered) {
    GEOLOCATE_REC *g=init_geolocate(stVec);
    g->earth_radius=er;
    g->side=s->lookDir;
    g->lambda=speedOfLight/s->frequency;
    s->estDop=yaw2doppler(g,s->range_gate*speedOfLight/2.0,1.10)/s->prf;
    asfPrintStatus("   Estimated doppler: %f PRF\n",s->estDop);
    free_geolocate(g);
  }
}

/*********************************************************************
 * updateAGC_window:
 * Writes the given AGC value (floating-point amplitude amplification
 * that should be applied to this image) and window position (starting
 * offset of this row in pixels) to the .fmt file. Called by decoding
 * routines. */
void updateAGC_window(bin_state *s,float amplify,float startOff)
{
  static float sto_amp=-1000.0,sto_off=-1000.0,diff=5.0;
  diff=abs((10*log(sto_amp*sto_amp)/log(10))-(10*log(amplify*amplify)/log(10)));
  if (s->dotFMT==NULL) return;/*Skip it if no .fmt file exists*/
  if (((sto_amp!=amplify)&&(diff<3))||(sto_off!=startOff))
  {/*Amplification factor or window position has changed! Update .fmt*/
    sto_amp=amplify;
    sto_off=startOff;
    fprintf(s->dotFMT,"%-8d %12.3f %10.5g\n",s->nLines,startOff,amplify);
    fflush(s->dotFMT);/*Flush the format file*/
  }
}



