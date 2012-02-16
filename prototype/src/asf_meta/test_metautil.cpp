/*
Verify meta.h routines against known values.

Orion Sky Lawlor, olawlor@acm.org, 2006/06/12
*/
#include "asf_meta/util.h"
#include "asf_meta/meta_parameters.h"

using namespace asf;

int verbose=1;

/* testing infrastructure */
void diff(std::string what,double a,double b,double tol=1.0e-10) {
	double absdiff=fabs(a-b);
	double reldiff=absdiff/(0.5*(fabs(a)+fabs(b)));
	if (verbose) printf("  difference in %s: %g relative, %g absolute\n",
		what.c_str(),reldiff,absdiff);
	if (absdiff>tol) {
		fprintf(stderr,"ERROR in %s test!  Expected (%.18g), but got (%.18g), which has an absolute difference of %g!\n",
			what.c_str(), a,b,absdiff);
		abort();
	}
}
void diff(std::string what,asf::meta3D_t a,asf::meta3D_t b,double tol=1.0e-10) {
	double absdiff=a.dist(b);
	double reldiff=absdiff/(0.5*(a.mag()+b.mag()));
	if (verbose) printf("   difference in %s: %g relative, %g absolute\n",
		what.c_str(),reldiff,absdiff);
	if (absdiff>tol) {
		fprintf(stderr,"ERROR in %s test!  Expected (%f,%f,%f), but got (%f,%f,%f), which has a absolute difference of %g!\n",
			what.c_str(), a.x,a.y,a.z, b.x,b.y,b.z,absdiff);
		abort();
	}
}
void diff(std::string what,asf::meta_state_t a,asf::meta_state_t b,double tol=1.0e-10) 
{
	diff(what+" position",a.pos,b.pos,tol);
	diff(what+" velocity",a.vel,b.vel,tol);
}

void vassert(std::string what,bool value) {
	if (!value) {
		fprintf(stderr,"ERROR in %s assert!\n",what.c_str());
		abort();
	}
	if (verbose) printf("  passed %s assert\n",what.c_str());
}

/*********************** state vectors **********************/

void old_fixed2gei(stateVector *stateVec,double gha)
{
	double radius,phi,phiVel;
	double angularVelocity=(366.225/365.225)*2.0*M_PI/86400.0;
	double ghaRads=gha*M_PI/180.0;
	stateVector stateOut;
	
	/*Compute the radius, longitude, and rotation amount for the position.*/
	radius=sqrt(stateVec->pos.x*stateVec->pos.x+stateVec->pos.y*stateVec->pos.y);
	phi=atan2(stateVec->pos.y,stateVec->pos.x);
	phi+=ghaRads;
	stateOut.pos.x=cos(phi)*radius;
	stateOut.pos.y=sin(phi)*radius;
	stateOut.pos.z=stateVec->pos.z;
	
	/*Compute the radius, longitude, and rotation amount for the velocity.
		(this equation has an extra term since the earth is rotating).*/
	radius=sqrt(stateVec->vel.x*stateVec->vel.x+stateVec->vel.y*stateVec->vel.y);
	phiVel=atan2(stateVec->vel.y,stateVec->vel.x);
	phiVel+=ghaRads;
	stateOut.vel.x=cos(phiVel)*radius-angularVelocity*stateOut.pos.y;
	stateOut.vel.y=sin(phiVel)*radius+angularVelocity*stateOut.pos.x;
	stateOut.vel.z=stateVec->vel.z;
	
	/*Finally, copy stateOut into stateVec.*/
	(*stateVec)=stateOut;
}


void old_gei2fixed(stateVector *stateVec,double gha)
{
	double radius,phi,phiVel;
	double angularVelocity=(366.225/365.225)*2.0*M_PI/86400.0;
	double ghaRads=gha*M_PI/180.0;
	stateVector stateOut;
	
	/*Compute the radius, longitude, and rotation amount for the position.*/
	radius=sqrt(stateVec->pos.x*stateVec->pos.x+stateVec->pos.y*stateVec->pos.y);
	phi=atan2(stateVec->pos.y,stateVec->pos.x);
	phi-=ghaRads;
	stateOut.pos.x=cos(phi)*radius;
	stateOut.pos.y=sin(phi)*radius;
	stateOut.pos.z=stateVec->pos.z;
	
	/*Compute the radius, longitude, and rotation amount for the velocity.
		(this equation has an extra term since the earth is rotating).*/
	radius=sqrt(stateVec->vel.x*stateVec->vel.x+stateVec->vel.y*stateVec->vel.y);
	phiVel=atan2(stateVec->vel.y,stateVec->vel.x);
	phiVel-=ghaRads;
	stateOut.vel.x=cos(phiVel)*radius+angularVelocity*stateOut.pos.y;
	stateOut.vel.y=sin(phiVel)*radius-angularVelocity*stateOut.pos.x;
	stateOut.vel.z=stateVec->vel.z;
	
	/*Finally, copy stateOut into stateVec.*/
	(*stateVec)=stateOut;
}


void test_statevecs(void) {
	meta_state_t a,bn,bo,cn,co,d;
	a.pos=meta3D_t(-2423929.8125,-2113451.3611,6391908.6914);
	a.vel=meta3D_t(-6520.2572727,-2099.3559969,-3160.1381836);
	double gha=123.0;
	bn=a; fixed2gei(&bn,gha);
	bo=a; old_fixed2gei(&bo,gha);
	diff("fixed2gei state vector",bo,bn,1.0e-3);
	cn=bn; gei2fixed(&cn,gha);
	co=bo; old_gei2fixed(&co,gha);
	diff("gei2fixed state vector",co,cn,1.0e-3);
	diff("new gei2fixed(fixed2gei) state vector",a,cn,1.0e-6);
	diff("old gei2fixed(fixed2gei) state vector",a,co,1.0e-6);
}

/******** Date and GHA handling ****************/
void test_dates(void) {
	vassert("1999",false==gregorian_leap_year(1999));
	vassert("2000",true==gregorian_leap_year(2000));
	vassert("2004",true==gregorian_leap_year(2004));
	vassert("2100",false==gregorian_leap_year(2100));
	
	/* Known-good julian days */
	diff("julian day of 1900",2415020.5,julian_day_from_year(1900),0);
	diff("julian day of 1975",2442413.5,julian_day_from_year(1975),0);
	diff("julian day of 2000",2451544.5,julian_day_from_year(2000),0);
	diff("julian day of 2101",2488434.5,julian_day_from_year(2101),0);
	diff("julian day of 2006/6/6",2453892.5, julian_day_from_ymd(2006,6,6),0);
	diff("julian day of 2008/6/6",2454623.5, julian_day_from_ymd(2008,6,6),0);
	
	/* UT1 at Midnight, January 1, 2003 was -.2892390 seconds */
	double UTC=0.0;
	double UT1=UT1_from_UTC(2003,1,0.0);
	diff("UT1 from UTC on 2003",-.2892390,UT1-UTC,1.0e-6);
	
	/* TAI was 33 seconds ahead of UTC starting January 1 2006 */
	double base=12345.6;
	diff("TAI from UTC in late 2005",base+32,TAI_from_UTC(2005,360,base+0.0),0);
	diff("TAI from UTC on Jan 1, 2006 (just after leap second)",base+33,TAI_from_UTC(2006,1,base+0.0),0);
	
	{ // Date-to-GHA pair from CEOS image E122590290G1U014.L (PPDR) 
	double ceos_gha=2.400339841842651;
	double our_gha=utc2gha(1995,313,0,0,75322.359375);
	diff("ceos vs our gha, 1995",ceos_gha,our_gha,1.0e-5);
	}
	{ // Date-to-GHA pair from CEOS image R147672136G1Q002.L (PPDR) 
	double ceos_gha=164.044052124023440;
	double our_gha=utc2gha(2004,357,0,0,17510.265625);
	diff("ceos vs our gha, 2004",ceos_gha,our_gha,1.0e-5);
	}
}


/************** Geolocation *******************/

double meter_to_latlon=1.0e-5; /* lat/lon degrees per meter */

/** Perform a geolocation test on this .meta file, where (x,y) corresponds
to this lat/lon. */
void image_match(std::string metaName, int x,int y, double lat,double lon,double elev,double ll_tol=100.0*meter_to_latlon,bool with_map=true)
{
	asf::metadata_source *m=asf::meta_read_source(metaName.c_str());
	asf::meta3D_t img(x,y,elev);
	asf::meta3D_t lle=m->transform(asf::LONGITUDE_LATITUDE_DEGREES,img);
	asf::meta3D_t lle_good(lon,lat,elev);
	
	// Check the output elevation
	diff(metaName + " z (meters)",elev,lle.z,1.0e-2);
	// Check the output lat/lon
	diff(metaName + " lat/lon (degrees)",lle_good,lle,ll_tol);
	
	if (with_map) {
		// Translate lat/lon and image pixels back to map coords
		asf::meta3D_t map_img=m->transform(asf::MAP_COORDINATES,img,asf::IMAGE_PIXELS);
		asf::meta3D_t map_lle=m->transform(asf::MAP_COORDINATES,lle,asf::LONGITUDE_LATITUDE_DEGREES);
		diff(metaName + " projection roundtrip (map meters)",map_img,map_lle,0.2);
	}
	
	// Translate lat/lon back to pixels
	asf::meta3D_t img_back=m->transform(asf::IMAGE_PIXELS,lle,asf::LONGITUDE_LATITUDE_DEGREES);
	diff(metaName + " lat/lon roundtrip (pixels)",img,img_back,0.01);
	
}


void test_geolocations(void) {
	
/* Test SAR image geolocations */
	asf::metadata_source &m1=*asf::meta_read_source("../test_data/delta_1.meta");
	/* This is the location of corner reflector DJ1 in this image */
	double elev=452.1;
	asf::meta3D_t img(2238,12465,  elev);
	
	asf::meta3D_t std=m1(asf::SLANT_TIME_DOPPLER,img);
	asf::meta3D_t std_good(855186.9809,7.4200757940396,312.6913);
	diff("delta_1 DJ1 STD (meters,seconds,Hz)",std_good,std,1.0e-3);
	
	asf::meta3D_t xyz=m1(asf::TARGET_POSITION,img);
	asf::meta3D_t xyz_good(-2312667.89, -1618876.50, 5700719.17);
	diff("delta_1 DJ1 XYZ (meters)",xyz_good,xyz,10.0); /* <- ellipsoids are slightly different (why?) */
	
	asf::meta3D_t lle=m1(asf::LONGITUDE_LATITUDE_DEGREES,img);
	asf::meta3D_t lle_good(-145.00790111, 63.80828389, elev);
	diff("delta_1 DJ1 elevation (meters)",lle_good.z,lle.z,0.1);
	lle.z=lle_good.z; /* 1mm elevation roundoff screws up relative error */
	diff("delta_1 DJ1 position (degrees lat/lon)",lle_good,lle,10*meter_to_latlon);
	
	image_match("../test_data/delta_1.meta", img.x,img.y, lle.y,lle.x,elev, 1000.0*meter_to_latlon,false);
	
/* Test map-projected image geolocations */
	/* Geographic coordinates */
	if (1) {
	asf::metadata_source &mf=*asf::meta_read_source("../test_data/sf_ned1.meta");
	asf::meta3D_t imgf(123,234,0.0);
	asf::meta3D_t llf=mf(asf::LONGITUDE_LATITUDE_DEGREES,imgf);
	asf::meta3D_t llf_good(-122.513333+imgf.x*0.00027777777,
	                         37.836944-imgf.y*0.00027777777,0);
	diff("sf_ned1 position (degrees lat/lon)",llf_good,llf,1.0e-5);
	}
	
	if (1) /* Try out all map projections */
	{
	image_match("../test_data/map/akned_geo.meta", 75,57, 63.068333,-145.957778,892.0);
	double lat=63.066240, lon=-145.958271, elev=771.0; 
	image_match("../test_data/map/akned_utm.meta", 35,58, lat,lon,elev);
	image_match("../test_data/map/akned_ps.meta", 46,61, lat,lon,elev,200*meter_to_latlon);
	image_match("../test_data/map/akned_lamaz.meta", 46,61, lat,lon,elev,2000*meter_to_latlon); /* FIXME: libproj and map_projection.cpp disagree on the definition of lamaz by about 1.4km.  I don't know which one's right.  */
	image_match("../test_data/map/akned_lamcc.meta", 46,61, lat,lon,elev);
	image_match("../test_data/map/akned_albers.meta", 46,61, lat,lon,elev);
	}
}



ASF_PLUGIN_EXPORT int main() {
	test_statevecs();
	test_dates();
	test_geolocations();
	
	printf("All meta tests passed.  Good.\n");
}
