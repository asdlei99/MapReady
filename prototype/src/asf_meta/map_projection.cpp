/******************************************************************************
NAME:JPL_proj

SYNOPSIS:
	A set of map projection routines from JPL. Includes
	Along-Track/Cross-Track (AT/CT), Universal Transverse Mercator, Polar
	Sterographic, Albers Equal Area, and Lambert Conformal Conic.


******************************************************************************/
#include "map_projection.h"
#include "asf/units.h"

using namespace asf;

/*
These now live in the proj_parameters structure:
#define RE 	6378144.0   GEM-06 Ellipsoid.
#define ecc_e	8.1827385e-2 GEM-06 Eccentricity
#define ecc2	SQR(ecc_e)
*/
#define cspeed	299.792458e6
#define omega_e 7.29211585e-5
#define D2R RADIANS_FROM_DEGREES
#define R2D DEGREES_FROM_RADIANS

/* Degree versions of (normally radian) trancendental operations */
#define sind(x) sin((x)*D2R)
#define cosd(x) cos((x)*D2R)
#define tand(x) tan((x)*D2R)
#define asind(x) (asin(x)*R2D)
#define acosd(x) (acos(x)*R2D)
#define atand(x) (atan(x)*R2D)
#define atan2d(y,x) (atan2(y,x)*R2D)


#define SQR(A)	((A)*(A))
#define mag(x,y,z) sqrt(SQR(x)+SQR(y)+SQR(z))

#define RE (proj->re_major) /*Extract semimajor axis of earth (equatorial)*/
#define RP (proj->re_minor) /*Extract semiminor axis of earth (polar)*/
 /*Extract first eccentricity, squared*/
#define ecc2 (sqrt(1.0 - (proj->re_minor*proj->re_minor)/(proj->re_major*proj->re_major)) \
            * sqrt(1.0 - (proj->re_minor*proj->re_minor)/(proj->re_major*proj->re_major)))
 /*Extract first eccentricity*/
#define ecc_e (sqrt(1.0 - (proj->re_minor*proj->re_minor)/(proj->re_major*proj->re_major)))

/*Projection Prototypes;*/
void ll_ac(meta_projection *proj, char look_dir, double lat, double lon, double *c1, double *c2);
void ll_lamcc(meta_projection *proj,double lat,double lon,double *x,double *y);
void ll_lamaz(meta_projection *proj,double lat,double lon,double *x,double *y);
void ll_ps(meta_projection *proj,double lat, double lon, double *x, double *y);
void ll_utm(meta_projection *proj,double tlat, double tlon, double *p1, double *p2);
void ll_alb(meta_projection *proj,double lat, double lon, double *x, double *y);

void ac_ll(meta_projection *proj, char look_dir, double c1, double c2,double *lat_d, double *lon);
void lamcc_ll(meta_projection *proj,double x,double y,double *lat_d,double *lon);
void lamaz_ll(meta_projection *proj,double lat,double lon,double *x,double *y);
void ps_ll(meta_projection *proj,double xx,double yy,double *alat,double *alon);
void utm_ll(meta_projection *proj,double x,double y,double *lat_d,double *lon);
void alb_ll(meta_projection *proj,double xx,double yy,double *alat,double *alon);

/*Convert projection units (meters) from geodetic latitude and longitude (degrees).*/
void ll_to_proj(meta_projection *proj,char look_dir,double lat_d,double lon,double *p1,double *p2)
{
	if (proj==NULL)
		asf::die("NULL projection parameter structure passed to ll_to_proj!\n");
	switch (proj->type)
	{
		case SCANSAR_PROJECTION: ll_ac(proj,look_dir,lat_d,lon,p2,p1); break;
		case LAMBERT_CONFORMAL_CONIC: ll_lamcc(proj,lat_d,lon,p1,p2); break;
		case LAMBERT_AZIMUTHAL_EQUAL_AREA: ll_lamaz(proj,lat_d,lon,p1,p2); break;
		case POLAR_STEREOGRAPHIC: ll_ps(proj,lat_d,lon,p1,p2); break;
		case UNIVERSAL_TRANSVERSE_MERCATOR: ll_utm(proj,lat_d,lon,p1,p2); break;
	        case ALBERS_EQUAL_AREA: ll_alb(proj,lat_d,lon,p1,p2); break;
	        case LAT_LONG_PSEUDO_PROJECTION: *p2=lat_d; *p1=lon; break;
		default:
			printf("Unrecognized map projection '%d' passed to ll_to_proj!\n",proj->type);
			exit(1);
	}
}

/*Convert projection units (meters) to geodetic latitude and longitude (degrees).*/
void proj_to_ll(meta_projection *proj, char look_dir, double p1, double p2, double *lat_d, double *lon)
{
	if (proj==NULL)
		asf::die("NULL projection parameter structure passed to proj_to_ll!\n");
	switch(proj->type)
	{
		case SCANSAR_PROJECTION: ac_ll(proj,look_dir,p2,p1,lat_d,lon); break;
		case LAMBERT_CONFORMAL_CONIC: lamcc_ll(proj,p1,p2,lat_d,lon); break;
		case LAMBERT_AZIMUTHAL_EQUAL_AREA: lamaz_ll(proj,p1,p2,lat_d,lon); break;
		case POLAR_STEREOGRAPHIC: ps_ll(proj,p1,p2,lat_d,lon); break;
		case UNIVERSAL_TRANSVERSE_MERCATOR: utm_ll(proj,p1,p2,lat_d,lon); break;
	        case ALBERS_EQUAL_AREA: alb_ll(proj,p1,p2,lat_d,lon); break;
	        case LAT_LONG_PSEUDO_PROJECTION: *lat_d=p2; *lon=p1; break;
		default:
			printf("Unrecognized map projection '%d' passed to proj_to_ll!\n",proj->type);
			exit(1);
	}
}

/******************************************************************
  Module Name: ll_lamcc
 
  Abstract:
            convert lat/lon to Lambert Conformal Conic (Ellipsoid)
	    coordinates (x:easting,y: northing)
 
  User-defined Input parameters:
          1).tlat  - array of latitude coordinates
          2).tlon  - array of longitude coordinates
          3).plat1 - the first standard parallels
          4).plat2 - the second standard parallels
          5).lat0  - original lat
          6).lon0 - original long
 
  Output parameters:
          1).x - array of easting coordinates
          2).y - array of northing coordinates
 
  Reference: Synder, J. P. 1984, Map projection used by the US
             geological survey(bulletin 1532)

 ******************************************************************/
void lamcc_init(proj_lamcc *l,double e,
	double *m1,double *m2,double *t0,double *t1,double *t2,double *n)
{
	*m1 = cosd(l->plat1)/sqrt(1.0-SQR(e*sind(l->plat1)));
	*m2 = cosd(l->plat2)/sqrt(1.0-SQR(e*sind(l->plat2)));
	
	*t1=tand(45.0-l->plat1/2.0)
			/
		pow(((1.0-e*sind(l->plat1))/(1.0+e*sind(l->plat1))),(e/2.0));
	*t2=tand(45.0-l->plat2/2.0)
			/
		pow(((1.0-e*sind(l->plat2))/(1.0+e*sind(l->plat2))),(e/2.0));
	*t0=tand(45.0-l->lat0 /2.0)
			/
		pow(((1.0-e*sind(l->lat0 ))/(1.0+e*sind(l->lat0 ))),(e/2.0));
	
	if (l->plat1!=l->plat2) 
		*n = log(*m1/ *m2) / log(*t1/ *t2); 
	else 
		*n = sind(l->plat1);
}
void ll_lamcc(meta_projection *proj,double lat,double lon,double *x,double *y)
{
	double F, rho_0, t, rho, theta;
	double m1, m2, t0, t1, t2, n;
	double e=ecc_e;
	lamcc_init(&proj->param.lamcc,e,
		&m1,&m2,&t0,&t1,&t2,&n);
	
	F = m1/(n*pow(t1,n)); rho_0 = RE*F*pow(t0,n);
	
	t = tand(45.0-lat/2.0)/pow(((1.0-e*sind(lat))/(1.0+e*sind(lat))),(e/2.0));
	rho = RE*F*pow(t,n);
	theta = n*(lon-proj->param.lamcc.lon0);
	
	*x = rho*sind(theta);
	*y = rho_0-rho*cosd(theta);
}
/***********************************************************************
  Module Name: lamcc_ll
 
  Abstract: convert Lambert Conformal Conic(Ellipsoid) coordinates to lat/lon
 
  User-defined Input parameters:
          1).tlat  - array of latitude coordinates
          2).tlon  - array of longitude coordinates
          3).plat1 - the first standard parallels
          4).plat2 - the second standard parallels
          5).lat0  - original lat
          6).lon0 - original long
 
  Output parameters:
          1).x - array of easting coordinates
          2).y - array of northing coordinates
 
  Reference: Synder, J. P. 1984, Map projection used by the US
             geological survey(bulletin 1532)
 
***********************************************************************/
void lamcc_ll(meta_projection *proj,double x,double y,double *lat_d,double *lon)
{
	double F, rho_0, t, rho, theta, chi;
	double m1, m2, t0, t1, t2, n;
	double e=ecc_e,e2=ecc2;
	lamcc_init(&proj->param.lamcc,e,
		&m1,&m2,&t0,&t1,&t2,&n);
	
	F = m1 / ( n * pow(t1,n) );
	rho_0 = RE * F * pow(t0,n);
	rho = sqrt(x*x + SQR(rho_0-y));
	
	/* If n is negative, the signs of x,y and rho_0 must be reversed
	(see notes in "Map projections used by the USGS) */
	theta = atand(x/(rho_0-y));
	*lon   = theta / n + proj->param.lamcc.lon0;
	t = pow((rho/(RE*F)),(1.0/n));
	
	/* use a series to calculate the inverse formula */
	chi = (M_PI/2.0) - 2.0*atan(t);
	
	*lat_d = chi
	        +  (e2 /2.0 +  e2*e2* 5.0/24.0+ e2*e2*e2 /12.0)
	        	*sin(2.0*chi)
	        +  (e2*e2* 7.0/48.0 + e2*e2*e2* 9.0/240.0)
	        	*sin(4.0*chi)
	        +  (e2*e2*e2* 7.0/120.0)
	        	*sin(6.0*chi);
	
	*lat_d = (*lat_d) * R2D;	/* convert to degrees */
}


/*********************************************************************
  Description:
 
  This subroutine converts from geodetic latitude and longitude to
  polar stereographic (x,y) coordinates for the polar regions. The
  standard parallel (latitude with no distortion) is +/- 70 degrees.
  The equations are from Synder, J. P., 1982, map projections used
  by the U.S. geological survey, geological survey bulletin 1532,
  U.S. government printing office. see jpl technical memorandum
  3349-85-101 for further details.
 
  Variable  Type          I/O     Descritpion
  --------------------------------------------------------------------------
  alat     double          I      geodetic latitude(degrees, +90 to -90)
  alon     double          I      geodetic longitude(degrees, 0 to 360)
  x        double          O      polar stereographic x coordinate(km)
  y        double          O      polar stereographic y coordinate(km)
 
  re       double          I      radius of the earth (km)
  e2       double          I      eccentricity square (m)
  slat     double          I      standard parallel
  slon     double          I      meridian along positive y-axis
 
  Written by C. S. Morris - April 29, 1985
  modified for ers-1 project- genreate the xy table
  instead of one (x,y) per call to the subroutine.    ssp  Jan 1989.
  add re, e2, slat and slon to the calling sequence.  ssp  Mar 1989.
  convert to c					      ASF  1/98
*****************************************************************************/
void ll_ps(meta_projection *proj,double lat, double lon, double *x, double *y)
{
	double re=RE;     /* radius of earth */
	double e=ecc_e,e2=ecc2;  /* eccentricities of earth*/
	double ta[2];
	double sn,rlat,cm,rho;
	double alon, alat;
	int    i;
	
	/* Set constants for ssm/i grid */
	if (lon <= 0) alon = lon + 360; else alon = lon;
	if (lat>=0.0) sn=1.0; else sn=-1.0;
	
	/* Compute x and y in grid coordinates */
	alat=sn*lat; alon=sn*alon; rlat=alat;
	for (i=0; i<2; i++)
	{
		if (i==1) rlat=proj->param.ps.slat;
		ta[i]= tan((M_PI/4.0)-(rlat/(2.0*R2D)))/
			pow(((1.0-e*sin(rlat*D2R))/(1.0+e*sin(rlat*D2R))),(e/2.0));
	}

	cm=cos(proj->param.ps.slat*D2R)/
		sqrt(1.0-e2*(SQR(sin(proj->param.ps.slat*D2R))));
	rho=re*cm*ta[0]/ta[1];
	
	*x=(rho*sn*sin((alon-proj->param.ps.slon)*D2R));
	*y=-(rho*sn*cos((alon-proj->param.ps.slon)*D2R));
}
/*****************************************************************************
  Description:
 
  This subroutine converts from polar stereographic (x,y) coordinates
  to geodetic latitude and longitude for the polar regions. The standard
  parallel (latitude with no distortion) is +/- 70 degrees. The equations
  are from Synder, J. P., 1982, map projections used by the U.S.
  geological survey, geological survey bulletin 1532, U.S. goverment
  printing office. See JPL technical memorandum 3349-85-101 for further
  details.
 
  Arguments:

  Variable    Type       I/O    Description
  xx,yy       real*8      I     polar sterographic x,y coordinate (m)
                                                                     ssp 8/89
  alat        real*8      O     geodetic latitude (degrees, +90 to -90)
  alon        real*8      O     geodetic longitude (degrees, 0 to 360)

  Written by C. S. Morris - April 29, 1985
******************************************************************************/

void ps_ll(meta_projection *proj,double x,double y,double *alat,double *alon)
{
	double re=RE;     /* radius of earth  */
	double e=ecc_e,e2=ecc2;  /* eccentricities of earth */
	double sn,rho,cm,t,chi,slatr;
	
	slatr=proj->param.ps.slat*D2R;
	if (proj->param.ps.slat>=0.0) sn=1.0; else sn=-1.0;
	
	slatr*=sn;/*Make South Hemisphere work-- O. Lawlor*/
	
	/* Compute the latitude and longitude */
	rho=sqrt(x*x+y*y);
	if (rho <= 0.01) { *alat=(sn < 0)?-90.0:90.0; *alon = 0.0; }
	else {
		cm = cos(slatr)/sqrt(1.0-e2*(SQR(sin(slatr))));
		t = tan(M_PI/4.0-slatr/2.0)/
			pow((1.0-e*sin(slatr))/(1.0+e*sin(slatr)),e/2.0);
		t = rho*t/(re*cm);
		chi=(M_PI/2.0)-2.0*atan(t);
		
		*alat = chi
	        +  (e2 /2.0 +  e2*e2* 5.0/24.0+ e2*e2*e2 /12.0)
	        	*sin(2.0*chi)
	        +  (e2*e2* 7.0/48.0 + e2*e2*e2* 9.0/240.0)
	        	*sin(4.0*chi)
	        +  (e2*e2*e2* 7.0/120.0)
	        	*sin(6.0*chi);
		
		*alat = sn * (*alat) * R2D;
		*alon = atan2(sn*x,-sn*y)*R2D+sn*proj->param.ps.slon;
		*alon = sn * (*alon);
		
		while (*alon <= -180.0) *alon += 360.0;  /* OSL 9/98 */
		while (*alon >   180.0) *alon -= 360.0;
	}
}

/**********************UTM Conversion Routines.*******************/
int UTM_zone(double lon) {return (int)floor(((180.0+lon)/6.0+1.0));} 

void ll_utm(meta_projection *proj,double tlat, double tlon, double *p1, double *p2)
{
	double   k0 = 0.9996;       /* Center meridian scale factor */
	double   epsq, lat, /*lon,*/ lon0, a1, a2, a3, rn;
	double   t, b1, b2, rm, rm0;
	double   tanlat, c;
	double   yadj, xadj = 500000.0;
	double   e4, e6, c1, c2, c3, c4;
	double   x,y;

	epsq = ecc2/(1.0 - ecc2);
	lon0 = (double)(proj->param.utm.zone-1)*6.0 + 3.0 - 180.0;   /* zone from proj constants */
	if (proj->hem=='N') yadj = 0.0; else yadj = 1.0E7; /* Ref pt Southern Hemisphere */

	lat = tlat * D2R; /*lon = tlon * D2R;*/

	rn =  RE / sqrt(1.0 - ecc2*SQR(sin(lat)));
	tanlat = tan(lat);
	t = SQR(tanlat);
	c = epsq * SQR(cos(lat));
	a1 = cos(lat) * ((tlon-lon0)*D2R);
	a2 = (1.0 - t + c) * (a1*a1*a1) / 6.0;
	a3 = (5.0 - 18.0*t + t*t + 72.0*c - 58.0*epsq) * ((a1*a1*a1*a1*a1) / 120.0);

	x = k0 * rn * (a1+a2+a3) + xadj;

	e4 = ecc2 * ecc2;
	e6 = e4 * ecc2;
	c1 = 1.0 - ecc2/4.0 - 3.0*e4/64.0 - 5.0*e6/256.0;
	c2 = 3.0*ecc2/8.0 + 3.0*e4/32.0 + 45.0*e6/1024.0;
	c3 = 15.0*e4/256.0 + 45.0*e6/1024.0;
	c4 = 35.0*e6/3072.0;
	rm = RE*(c1*lat-c2*sin(2.0*lat)+c3*sin(4.0*lat)- c4*sin(6.0*lat));
	rm0 = 0.0;
	b1 = (SQR(a1))/2.0 + (5.0 - t + 9.0*c + 4.0 *SQR(c)) * (a1*a1*a1*a1) / 24.0;
	b2 = (61.0-58.0*t+SQR(t)+600.0*c+330.0*epsq) * (a1*a1*a1*a1*a1*a1) / 720.0;

	y = k0 * (rm - rm0 + rn*tanlat*(b1 + b2)) + yadj;

	*p1 = x;
	*p2 = y;
}
void utm_ll(meta_projection *proj,double x,double y,double *lat_d,double *lon)
{
	double esq = ecc2;        /* Earth eccentricity squared */
	double k0  = 0.9996;      /* Center meridian scale factor */
	double u1, u2, u3, lat1, esqsin2, lat1d, long0;
	double rm, e1, u, epsq, t1, c1, rn1, r1, rm0;
	double tanlat1, d;
	double xadj = 500000.0, yadj;    /* Northing origin:(5.d5,0.d0) */
	                               /* Southing origin:(5.d5,1.d7) */
	rm0 = 0.0;
	if (proj->hem=='N') 
		yadj=0.0; 
	else 
		yadj=1.0E7; /* Ref pt Southern Hemisphere */
	long0 = (double)(proj->param.utm.zone)*6.0-183.0;     /* zone from proj_const.inc   */
	
	rm = (y - yadj)/k0 + rm0;
	e1 = (1.0 - sqrt(1.0 - esq))/(1.0 + sqrt(1.0 - esq));
	u = rm/(RE*(1.0-esq/4.0-(3.0*esq*esq/64.0) - (5.0*esq*esq*esq/256.0)));
	u1 = (3.0 * e1 / 2.0 - (27.0 * e1*e1*e1)/32.0) * sin(2.0*u);
	u2 = (21.0 * (e1*e1)/16.0 - (55.0 * e1*e1*e1*e1)/32.0) * sin(4.0*u);
	u3 = (151.0 * (e1*e1*e1) / 96.0) * sin(6.0*u);
	lat1 = u + u1 + u2 + u3;
	lat1d = lat1/D2R;
	
	esqsin2 = 1.0 - esq*SQR(sin(lat1));
	epsq = esq/(1.0 - esq);
	c1 = epsq * SQR(cos(lat1));
	tanlat1 = sin(lat1)/cos(lat1);
	t1 = tanlat1 * tanlat1;
	rn1 = RE/sqrt(esqsin2);
	r1 = RE*(1.0 - esq)/sqrt(esqsin2 * esqsin2 * esqsin2);
	d = (x - xadj)/(rn1 * k0);
	
	*lat_d = lat1d - ((rn1 * tanlat1/r1) * (d*d*0.50
	              - (5.0 + 3.0*t1 - 10.0*c1 + 4.0*c1*c1
	                      - 9.0*epsq) * (d*d*d*d)/24.0
	              + (61.0 + 90.0*t1 + 298.0*c1 + 45.0*t1*t1
	                       - 252.0*epsq - 3.0*c1*c1)
	                  *(d*d*d*d*d*d)/720.0) )/D2R;
	
	*lon = long0 + ((1.0/cos(lat1)) * (d
	      - (1.0 + 2.0*t1 + c1) * (d*d*d)/6.0
	      + (5.0 - 2.0*c1 + 28.0*t1 - 3.0*c1*c1 + 8.0*epsq + 24.0*t1*t1)
	             *(d*d*d*d*d)/120.0) )/D2R;
}

/***************************Azimuth Equal Area Conversion Routines*******
Copied from the LAS routine asf_geolib/alberfor.c, which has the comment:

PROGRAMMER              DATE
----------              ----
T. Mittan,              Feb, 1992

ALGORITHM REFERENCES

1.  Snyder, John P., "Map Projections--A Working Manual", U.S. Geological
    Survey Professional Paper 1395 (Supersedes USGS Bulletin 1532), United
    State Government Printing Office, Washington D.C., 1987.

2.  Snyder, John P. and Voxland, Philip M., "An Album of Map Projections",
    U.S. Geological Survey Professional Paper 1453 , United State Government
    Printing Office, Washington D.C., 1989.
*/

#define EPSLN 1.0e-8
#define HALF_PI (M_PI*0.5)

/* Function to calculate the sine and cosine in one call.  Some computer
   systems have implemented this function, resulting in a faster implementation
   than calling each function separately.  It is provided here for those
   computer systems which don`t implement this function
  ----------------------------------------------------*/
#if !defined(sunos)
void sincos(double val,double *sin_val,double *cos_val)
{
	*sin_val = sin(val);
	*cos_val = cos(val);
	return;
}
#endif

/* Function to adjust a longitude angle to range from -PI to PI radians
   added if statments 
  -----------------------------------------------------------------------*/
double adjust_lon(double x)		/* Angle in radians			*/
{
	while (x>M_PI) x-=2*M_PI;
	while (x<=-M_PI) x+=2*M_PI;

	return(x);
}
/* Function to eliminate roundoff errors in asin
----------------------------------------------*/
double asinz (double con)
{
	if (fabs(con) > 1.0)
	{
		if (con > 1.0)
			con = 1.0;
		else
			con = -1.0;
	}
	return(asin(con));
}

/* Function to compute the constant small m which is the radius of
   a parallel of latitude, phi, divided by the semimajor axis.
---------------------------------------------------------------*/
double msfnz (double eccent,double sinphi,double cosphi)
{
	double con;

	con = eccent * sinphi;
	return((cosphi / (sqrt (1.0 - con * con))));
}

/* Function to compute constant small q which is the radius of a 
   parallel of latitude, phi, divided by the semimajor axis. 
------------------------------------------------------------*/
double qsfnz (double eccent,double sinphi,double cosphi)
{
	double con;

	if (eccent > 1.0e-7)
	{
		con = eccent * sinphi;
		return (( 1.0- eccent * eccent) * (sinphi /(1.0 - con * con) - (.5/eccent)*
		    log((1.0 - con)/(1.0 + con))));
	}
	else
		return(2.0 * sinphi);
}

/* Function to compute phi1, the latitude for the inverse of the
   Albers Conical Equal-Area projection.
-------------------------------------------*/
double phi1z (double eccent,double qs,int  *flag)
{
	double eccnts;
	double dphi;
	double con;
	double com;
	double sinpi;
	double cospi;
	double phi;
	int i;

	phi = asinz(.5 * qs);
	if (eccent < EPSLN)
		return(phi);
	eccnts = eccent * eccent;
	for (i = 1; i <= 25; i++)
	{
		sincos(phi,&sinpi,&cospi);
		con = eccent * sinpi;
		com = 1.0 - con * con;
		dphi = .5 * com * com / cospi * (qs / (1.0 - eccnts) - sinpi / com + 
		    .5 / eccent * log ((1.0 - con) / (1.0 + con)));
		phi = phi + dphi;
		if (fabs(dphi) <= 1e-7)
			return(phi);
	}
	asf::die("Albers Projection Convergence error");
	return(-1);
}

/** Silly local map projection class */
class map_proj {
public:
	/** Convert lat/lon (in radians) to x,y projection coordinates (in meters) */
	virtual void xy_from_ll(double lat, double lon,double *x, double *y) const =0;
	/** Convert x,y projection coordinates (in meters) to lat/lon (in radians) */
	virtual void ll_from_xy(double x, double y, double *lat, double *lon) const =0;
};

/** Lambert Azimuthal Equal-Area
http://mathworld.wolfram.com/LambertAzimuthalEqual-AreaProjection.html
http://www.3dsoftware.com/Cartography/USGS/MapProjections/Azimuthal/LambertAzimuthal/
*/
class lamaz_proj : public map_proj {
public:
	double lon_center;	 /* Center longitude (projection center) */
	double lat_center;	 /* Center latitude (projection center)  */
	double R;		 /* Radius of the earth (sphere)	 */
	double sin_lat_o;	 /* Sine of the center latitude 	 */
	double cos_lat_o;	 /* Cosine of the center latitude	 */
	double false_easting;	 /* x offset in meters  		 */
	double false_northing;   /* y offset in meters  		 */

	lamaz_proj(
		double r, 			/* (I) Radius of the earth (sphere) 	*/
		double center_long,		/* (I) Center longitude 		*/
		double center_lat,		/* (I) Center latitude 			*/
		double false_east,		/* x offset in meters			*/
		double false_north		/* y offset in meters			*/
		)
	{
		R = r;
		lon_center = center_long;
		lat_center = center_lat;
		false_easting = false_east;
		false_northing = false_north;
		sincos(center_lat, &sin_lat_o, &cos_lat_o);
	}
	
	/** Convert lat/lon (in radians) to x,y projection coordinates (in meters) */
	void xy_from_ll(double lat, double lon,double *x, double *y) const {
		double delta_lon;	/* Delta longitude (Given longitude - center 	*/
		double sin_delta_lon;	/* Sine of the delta longitude 			*/
		double cos_delta_lon;	/* Cosine of the delta longitude 		*/
		double sin_lat;		/* Sine of the given latitude 			*/
		double cos_lat;		/* Cosine of the given latitude 		*/
		double g;		/* temporary varialbe				*/
		double ksp;		/* heigth above elipsiod			*/

		/* Forward equations
		  -----------------*/
		delta_lon = adjust_lon(lon - lon_center);
		sincos(lat, &sin_lat, &cos_lat);
		sincos(delta_lon, &sin_delta_lon, &cos_delta_lon);
		g = sin_lat_o * sin_lat + cos_lat_o * cos_lat * cos_delta_lon;
		if (g == -1.0) 
			asf::die("Lamaz point projects to invalid circle");/* of radius = %f\n", 2.0 * R);*/
		ksp = R * sqrt(2.0 / (1.0 + g));
		*x = ksp * cos_lat * sin_delta_lon + false_easting;
		*y = ksp * (cos_lat_o * sin_lat - sin_lat_o * cos_lat * cos_delta_lon) + 
			false_northing;
	}
	/** Convert x,y projection coordinates (in meters) to lat/lon (in radians) */
	void ll_from_xy(double x, double y, double *lat, double *lon) const {
		double Rh;
		double z;		/* Great circle dist from proj center to given point */
		double sin_z;		/* Sine of z */
		double cos_z;		/* Cosine of z */
		double temp;		/* Re-used temporary variable */

		/* Inverse equations
		  -----------------*/
		x -= false_easting;
		y -= false_northing;
		Rh = sqrt(x * x + y * y);
		temp = Rh / (2.0 * R);
		if (temp > 1) 
			asf::die("Lamaz invalid point");
		z = 2.0 * asin(temp);
		sincos(z, &sin_z, &cos_z);
		*lon = lon_center;
		if (fabs(Rh) > EPSLN)
		   {
		   *lat = asin(sin_lat_o * cos_z + cos_lat_o * sin_z * y / Rh);
		   temp = fabs(lat_center) - HALF_PI;
		   if (fabs(temp) > EPSLN)
		      {
		      temp = cos_z - sin_lat_o * sin(*lat);
		      if(temp!=0.0)*lon=adjust_lon(lon_center+atan2(x*sin_z*cos_lat_o,temp*Rh));
		      }
		   else if (lat_center < 0.0) *lon = adjust_lon(lon_center - atan2(-x, y));
		   else *lon = adjust_lon(lon_center + atan2(x, -y));
		   }
		else *lat = lat_center;
	}
};

inline double sqr(double x) {return x*x;}


/** 
Determine the Earth ellipsoid radius at this geodetic latitude, in degrees.  
Several possibilities from
	http://www.gmat.unsw.edu.au/snap/gps/clynch_pdfs/radiigeo.pdf
	http://mathforum.org/library/drmath/view/51788.html


For comparison, the version above reads:
	delta_lon = adjust_lon(lon - lon_center);
	sincos(lat, &sin_lat, &cos_lat);
	sincos(delta_lon, &sin_delta_lon, &cos_delta_lon);
	g = sin_lat_o * sin_lat + cos_lat_o * cos_lat * cos_delta_lon;
	ksp = R * sqrt(2.0 / (1.0 + g));
	*x = ksp * cos_lat * sin_delta_lon + false_easting;
	*y = ksp * (cos_lat_o * sin_lat - sin_lat_o * cos_lat * cos_delta_lon) + 
		false_northing;
Here "R" is the value we need to calculate.

The libproj source theoretically describes this, in
	proj-4.4.8/src/PJ_laea.c
but the code is macro-heavy, comment-free, and pretty close to gibberish.
The oblique (normal) forward transform seems to read:
	coslam = cos(lp.lam);
	sinlam = sin(lp.lam); // Longitude
	sinphi = sin(lp.phi); // Latitude 
	q = pj_qsfn(sinphi, P->e, P->one_es); // "determine small q".  OK!  Great!  WTF!?!?
	sinb = q / P->qp;
	cosb = sqrt(1. - sinb * sinb);
	b = 1. + P->sinb1 * sinb + P->cosb1 * cosb * coslam; // g below +1
	b = sqrt(2. / b);
	xy.x = P->xmf * b * cosb * sinlam;
	xy.y = P->ymf * b * (P->cosb1 * sinb - P->sinb1 * cosb * coslam);
Looks like P->xmf/P->ymf play the role of R here.  

xmf/ymf are computed later in the file as:
	e = sqrt(es); // eccentricity?
	qp = pj_qsfn(1., e, one_es);
	rq = sqrt(.5 * qp);
	sinphi = sin(phi0); // Center latitude
	sinb1 = pj_qsfn(sinphi, e, one_es) / qp;
	cosb1 = sqrt(1. - sinb1 * sinb1);
	dd = cos(phi0) / (sqrt(1. - es * sinphi * sinphi) * rq * cosb1);
	ymf = (xmf = rq) / dd;
	xmf *= dd;
	
This can be translated as:
static double pj_qsfn(double sinphi, double e, double one_es) {
	double con = e * sinphi;
	return (one_es * (sinphi / (1. - con * con) -
		 (.5 / e) * log ((1. - con) / (1. + con))));
}
	double es=sqrt(e);
	double one_es=1.0-es;
	double lat=lat_deg*D2R;
	double qp = pj_qsfn(1., e, one_es);
	double rq = sqrt(.5 * qp);
	double sinphi = sin(lat); // Center latitude
	double sinb1 = pj_qsfn(sinphi, e, one_es) / qp;
	double cosb1 = sqrt(1. - sinb1 * sinb1);
	double dd = cos(lat) / (sqrt(1. - es * sinphi * sinphi) * rq * cosb1);
	double xmf;
	double ymf = (xmf = rq) / dd;
	xmf *= dd;
	out_radii[0]=a*xmf; out_radii[1]=a*ymf;
and this runs, but it doesn't give the same results as libproj.

I can't find any documentation on libproj's scaling, and I'm not convinced
it's even right.  Hence I'm sticking with the asf_libgeocode version for now.
*/

double lamaz_radius(double lat_deg,double re,double rp) {
	
/* Sensible but not-matching-libproj options from:
	http://www.gmat.unsw.edu.au/snap/gps/clynch_pdfs/radiigeo.pdf
	http://mathforum.org/library/drmath/view/51788.html

	double a=re, b=rp;
	double e=(sqr(a)-sqr(b))/sqr(a);
	double lat=lat_deg*D2R;	
	double r1= a*sqrt((sqr(1-sqr(e))*sqr(sin(lat))+sqr(cos(lat)))/(1-sqr(e*sin(lat)))); // radius of ellipsoid
	double r2= a*(1-sqr(e))*pow(1-sqr(e*sin(lat)),-1.5); // radius of curvature
	printf("Radius 1: %f    Radius 2: %f\n",r1,r2);
*/
	return re;
}


lamaz_proj make_lamaz(meta_projection *proj) {
	return lamaz_proj(
		lamaz_radius(proj->param.lamaz.center_lat,RE,RP),
		proj->param.lamaz.center_lon * D2R,
		proj->param.lamaz.center_lat * D2R,
		proj->param.lamaz.false_easting,proj->param.lamaz.false_northing);
}

void ll_lamaz(meta_projection *proj,double lat, double lon, double *x, double *y)
{
	lamaz_proj p=make_lamaz(proj);
	p.xy_from_ll(lat*D2R,lon*D2R,x,y);
}

void lamaz_ll(meta_projection *proj,double xx,double yy,
	    double *alat,double *alon)
{
	lamaz_proj p=make_lamaz(proj);
	p.ll_from_xy(xx,yy,alat,alon);
	*alat *= R2D;
	*alon *= R2D;
}


/*******************************
 Albers equal-area projection */
class alber_proj : public map_proj {
public:
	/* Variables common to forward and inverse transforms
	  -----------------------------------------------------*/
	double r_major;        /* major axis			       */
	double r_minor;        /* minor axis			       */
	double c;	       /* constant c			       */
	double e3;	       /* eccentricity  		       */
	double es;		/* eccentricity squared			*/
	double rh;	       /* heigth above elipsoid 	       */
	double ns0;	       /* ratio between meridians	       */
	double lon_center;     /* center longitude		       */
	double false_easting;  /* x offset in meters		       */
	double false_northing; /* y offset in meters		       */

	/* Initialize the Albers projection
	  -------------------------------*/
	alber_proj(
	    double r_maj,			/* major axis				*/
	    double r_min,			/* minor axis				*/
	double lat1,			/* first standard parallel		*/
	double lat2,			/* second standard parallel		*/
	double lon0,			/* center longitude			*/
	double lat0,			/* center lattitude			*/
	double false_east,		/* x offset in meters			*/
	double false_north		/* y offset in meters			*/
	)
	{
		double sin_po,cos_po;		/* sin and cos values			*/
		double con;			/* temporary variable			*/
		double temp;			/* eccentricity squared and temp var	*/
		double ms1;			/* small m 1				*/
		double ms2;			/* small m 2				*/
		double qs0;			/* small q 0				*/
		double qs1;			/* small q 1				*/
		double qs2;			/* small q 2				*/

		false_easting = false_east;
		false_northing = false_north;
		lon_center = lon0;
		if (fabs(lat1 + lat2) < EPSLN)
		{
			asf::die("jpl_proj.c> Albers projeciton: Equal latitudes for St. Parallels on opposite sides of equator");
		}
		r_major = r_maj;
		r_minor = r_min;
		temp = r_minor / r_major;
		es = 1.0 - temp*temp;
		e3 = sqrt(es);

		sincos(lat1, &sin_po, &cos_po);
		con = sin_po;

		ms1 = msfnz(e3,sin_po,cos_po);
		qs1 = qsfnz(e3,sin_po,cos_po);

		sincos(lat2,&sin_po,&cos_po);

		ms2 = msfnz(e3,sin_po,cos_po);
		qs2 = qsfnz(e3,sin_po,cos_po);

		sincos(lat0,&sin_po,&cos_po);

		qs0 = qsfnz(e3,sin_po,cos_po);

		if (fabs(lat1 - lat2) > EPSLN)
			ns0 = (ms1 * ms1 - ms2 *ms2)/ (qs2 - qs1);
		else
			ns0 = con;
		c = ms1 * ms1 + ns0 * qs1;
		rh = r_major * sqrt(c - ns0 * qs0)/ns0;
	}


	/* Albers Conical Equal Area forward equations--mapping lat,long to x,y
double lat;	  (I) Latitude (radians)	       
double lon;	  (I) Longitude (radians)	       
double *x;	  (O) X projection coordinate (meters) 
double *y;	  (O) Y projection coordinate (meters) 
  -------------------------------------------------------------------*/
	void xy_from_ll(double lat, double lon,double *x, double *y) const
	{
		double sin_phi,cos_phi;		/* sine and cos values		*/
		double qs;			/* small q			*/
		double theta;			/* angle			*/

		double rh1;			/* height above ellipsoid	*/

		sincos(lat,&sin_phi,&cos_phi);
		qs = qsfnz(e3,sin_phi,cos_phi);
		rh1 = r_major * sqrt(c - ns0 * qs)/ns0;
		theta = ns0 * adjust_lon(lon - lon_center);
		*x = rh1 * sin(theta) + false_easting;
		*y = rh - rh1 * cos(theta) + false_northing;

	}

	/* Albers Conical Equal Area inverse equations--mapping x,y to lat/long
double x;	  (I) X projection coordinate (meters) 
double y;	  (I) Y projection coordinate (meters) 
double *lat;	  (O) Latitude (radians)	       
double *lon;	  (O) Longitude (radians)	       
  -------------------------------------------------------------------*/
	void ll_from_xy(double x, double y, double *lat, double *lon) const
	{
		double rh1;			/* height above ellipsoid	*/
		double qs;			/* function q			*/
		double con;			/* temporary sign value		*/
		double theta;			/* angle			*/
		int   flag;			/* error flag;			*/


		flag = 0;
		x -= false_easting;
		y = rh - y + false_northing;
		;
		if (ns0 >= 0)
		{
			rh1 = sqrt(x * x + y * y);
			con = 1.0;
		}
		else
		{
			rh1 = -sqrt(x * x + y * y);
			con = -1.0;
		}
		theta = 0.0;
		if (rh1 != 0.0)
			theta = atan2(con * x, con * y);
		con = rh1 * ns0 / r_major;
		qs = (c - con * con) / ns0;
		if (e3 >= 1e-10)
		{
			con = 1 - .5 * (1.0 - es) * log((1.0 - e3) / 1.0 + e3)/e3;
			if (fabs(fabs(con) - fabs(qs)) > .0000000001 )
			{
				*lat = phi1z(e3,qs,&flag);
			}
			else
			{
				if (qs >= 0)
					*lat = .5 * M_PI;
				else
					*lat = -.5 * M_PI;
			}
		}
		else
		{
			*lat = phi1z(e3,qs,&flag);
		}

		*lon = adjust_lon(theta/ns0 + lon_center);

	}
};

alber_proj make_alber(meta_projection *proj) {
	return alber_proj(RE,RP,
		proj->param.albers.std_parallel1 * D2R,
		proj->param.albers.std_parallel2 * D2R,
		proj->param.albers.center_meridian * D2R,
		proj->param.albers.orig_latitude * D2R,
		proj->param.albers.false_easting,proj->param.albers.false_northing);
}

void ll_alb(meta_projection *proj,double lat, double lon, double *x, double *y)
{
	alber_proj p=make_alber(proj);
	p.xy_from_ll(lat*D2R,lon*D2R,x,y);
}

void alb_ll(meta_projection *proj,double xx,double yy,
	    double *alat,double *alon)
{
	alber_proj p=make_alber(proj);
	p.ll_from_xy(xx,yy,alat,alon);
	*alat *= R2D;
	*alon *= R2D;
}

/***************************AT/CT Conversion Routines*********************/
/*Along-Track/Cross-Track utilities:*/
void cross(double x1, double y1, double z1, 
	double x2, double y2, double z2,
	double *u1, double *u2, double *u3)
{
  double r, t1, t2, t3;
  t1=y1*z2-z1*y2; t2=z1*x2-x1*z2; t3=x1*y2-y1*x2;
  r=sqrt(SQR(t1)+SQR(t2)+SQR(t3));
  *u1=t1/r; *u2=t2/r; *u3=t3/r;
}

void rotate_z(vector *v,double theta)
{
	double xNew,yNew;
	
	xNew = v->x*cosd(theta)+v->y*sind(theta);
	yNew = -v->x*sind(theta)+v->y*cosd(theta);
	v->x = xNew; v->y = yNew;
}

void rotate_y(vector *v,double theta)
{
	double xNew,zNew;
	
	zNew = v->z*cosd(theta)+v->x*sind(theta);
	xNew = -v->z*sind(theta)+v->x*cosd(theta);
	v->x = xNew; v->z = zNew;
}


/**************************************************************************
 * atct_init:
 * calculates alpha1, alpha2, and alpha3, which are some sort of coordinate
 * rotation amounts, in degrees.  This creates a latitude/longitude-style
 * coordinate system centered under the satellite at the start of imaging.
 * You must pass it a state vector from the start of imaging.            */
 
void atct_init(meta_projection *proj,stateVector st)
{
	vector up(0.0,0.0,1.0);
	vector z_orbit, y_axis, a, nd;
	double alpha3_sign;
	double alpha1,alpha2,alpha3;
	
	vecCross(st.pos,st.vel,&z_orbit);vecNormalize(&z_orbit);
	
	vecCross(z_orbit,up,&y_axis);vecNormalize(&y_axis);
	
	vecCross(y_axis,z_orbit,&a);vecNormalize(&a);
	
/*	printf("ALPHA123 new x-vector: %.3f,%.3f,%.3f\n",a.x,a.y,a.z);*/
	
	alpha1 = atan2_check(a.y,a.x)*R2D;
	alpha2 = -1.0 * asind(a.z);
	if (z_orbit.z < 0.0) 
	{
		alpha1 +=  180.0;
		alpha2 = -1.0*(180.0-fabs(alpha2));
	}
	
	vecCross(a,st.pos,&nd);vecNormalize(&nd);
	alpha3_sign = vecDot(nd,z_orbit);
	alpha3 = acosd(vecDot(a,st.pos)/vecMagnitude(st.pos));
	if (alpha3_sign<0.0) 
		alpha3 *= -1.0;
	
/*	printf("alpha1, alpha2, alpha3  %f %f %f\n",alpha1,alpha2,alpha3);*/
	proj->param.atct.alpha1=alpha1;
	proj->param.atct.alpha2=alpha2;
	proj->param.atct.alpha3=alpha3;
}

void ac_ll(meta_projection *proj, char look_dir, double c1, double c2, double *lat_d, double *lon)
{
	double qlat, qlon;
	double lat,radius;
	vector pos;
	
	if (look_dir=='R')
		qlat = -c2/proj->param.atct.rlocal; /* Right looking sar */
	else
		qlat =  c2/proj->param.atct.rlocal; /* Left looking sar */
	qlon = c1/(proj->param.atct.rlocal*cos(qlat));
	
	sph2cart(proj->param.atct.rlocal, qlat, qlon, &pos);
	
	rotate_z(&pos,-proj->param.atct.alpha3);
	rotate_y(&pos,-proj->param.atct.alpha2);
	rotate_z(&pos,-proj->param.atct.alpha1);
	
	cart2sph(pos,&radius,&lat,lon);
	*lon *= R2D;
	lat *= R2D;
	*lat_d = atand(tand(lat) / (1-ecc2));
}

void ll_ac(meta_projection *proj, char look_dir, double lat_d, double lon, double *c1, double *c2)
{
	double qlat, qlon;
	double lat,radius;
	vector pos;
	
	lat= atand(tand(lat_d)*(1 - ecc2));
	sph2cart(proj->param.atct.rlocal,lat*D2R,lon*D2R,&pos);
	
	rotate_z(&pos,proj->param.atct.alpha1);
	rotate_y(&pos,proj->param.atct.alpha2);
	rotate_z(&pos,proj->param.atct.alpha3);
	
	cart2sph(pos,&radius,&qlat,&qlon);
	
	*c1 = qlon*proj->param.atct.rlocal*cos(qlat);
	if (look_dir=='R')
		*c2 = -1.0*qlat*proj->param.atct.rlocal;  /* right looking */
	else
		*c2 = qlat * proj->param.atct.rlocal;   /* left looking */
}
