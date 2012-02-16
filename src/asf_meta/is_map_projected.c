#include <asf_meta.h>
#include <libasf_proj.h>

// Utility function written solely for the purpose of
// making if-statement (etc) conditional expressions
// cleaner/smaller/lighter/intuitive
//
// This function wouldn't be necessary except for the
// fact that ScanSAR images have the sar->image_type
// set to 'P' for 'projected', but unless geocoded,
// this means that they are projected to
// along-track/cross-track ...not map-projected.
// If 'P' exists, then additional checking is required
// to determine if the image is MAP-projected or not.
int is_map_projected(meta_parameters *md)
{
  // Convenience pointers
  meta_projection *mp = md->projection;

  // Return true if the image is projected and the
  // projection is one of the ASF-supported map
  // projection types
  return (mp     &&
          (mp->type == UNIVERSAL_TRANSVERSE_MERCATOR  ||
           mp->type == POLAR_STEREOGRAPHIC            ||
           mp->type == ALBERS_EQUAL_AREA              ||
           mp->type == LAMBERT_CONFORMAL_CONIC        ||
           mp->type == LAMBERT_AZIMUTHAL_EQUAL_AREA   ||
	   mp->type == EQUI_RECTANGULAR ||
	   mp->type == EQUIDISTANT ||
	   mp->type == MERCATOR ||
	   mp->type == SINUSOIDAL ||
           mp->type == STATE_PLANE ||
	   mp->type == LAT_LONG_PSEUDO_PROJECTION) &&
	  mp->type != SCANSAR_PROJECTION
         ) ? 1 : 0;
}

int is_lat_lon_pseudo(meta_parameters *md)
{
  meta_projection *mp = md->projection;
  return mp && mp->type == LAT_LONG_PSEUDO_PROJECTION;
}

