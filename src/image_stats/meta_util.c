#include "asf.h"
#include "asf_meta.h"
#include "image_stats.h"

double my_get_satellite_height(double time, stateVector stVec)
{
  return sqrt(stVec.pos.x*stVec.pos.x+stVec.pos.y*stVec.pos.y+
	      stVec.pos.z*stVec.pos.z);
}

double my_get_earth_radius(double time, stateVector stVec, double re, double rp)
{
  double er = sqrt(stVec.pos.x*stVec.pos.x+stVec.pos.y*stVec.pos.y+
		   stVec.pos.z*stVec.pos.z);
  double lat = asin(stVec.pos.z/er);
  return (re*rp)/sqrt(rp*rp*cos(lat)*cos(lat)+re*re*sin(lat)*sin(lat));
}

double my_get_slant_range(meta_parameters *meta, double er, double ht, 
		       int line, int sample)
{
  if (meta->sar->image_type=='P') {
    double time,slant;
    meta_get_timeSlantDop(meta, line, sample, &time, &slant, NULL);
    return slant+meta->sar->slant_shift;
  }
  else {
    double minPhi=acos((ht*ht + er*er -
			SQR(meta->sar->slant_range_first_pixel))/(2.0*ht*er));
    double phi=minPhi+sample*(meta->general->x_pixel_size/er);
    double slantRng=sqrt(ht*ht+er*er-2.0*ht*er*cos(phi));
    return slantRng+meta->sar->slant_shift;
  }
}

double my_get_look_angle(double er, double ht, double sr)
{
  return acos((sr*sr+ht*ht-er*er)/(2.0*sr*ht));
}

double my_get_incidence_angle(double er, double ht, double sr)
{
  return PI-acos((sr*sr+er*er-ht*ht)/(2.0*sr*er));
}


