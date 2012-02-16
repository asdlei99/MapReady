/*
"meta_parameters" is a struct that represents in memory what the 
.meta file stores on disk.

Orion Sky Lawlor, olawlor@acm.org, 2006/07/13 (ASF)
*/

#ifndef __ASF_META_META_PARAMETERS_H__
#define __ASF_META_META_PARAMETERS_H__

#include "asf_meta/util.h"
#include "asf_meta/metadata.h"

namespace asf {

/* These are the dummy values that meta variables get set to at initialization */
#define MAGIC_UNSET_CHAR ('?')
#define MAGIC_UNSET_STRING ("???")
#define MAGIC_UNSET_INT (-999999999)
/* For MAGIC_UNSET_DOUBLE to work, you must include asf_nan.h */
#define MAGIC_UNSET_DOUBLE (NAN)

/* Maximum length of most string fields, including trailing null.  */
#define FIELD_STRING_MAX 256

/* Maximum length of mode field, including trailing null.  In case its
   so short for some good reason.  */
#define MODE_FIELD_STRING_MAX 5

/********************************************************************
 * meta_general: General RAdio Detection And Ranging parameters
 */
typedef struct {
  char sensor[FIELD_STRING_MAX];    /* Name of imaging sensor.             */
  char mode[MODE_FIELD_STRING_MAX]; /* Mode of imaging sensor.             */
  char processor[FIELD_STRING_MAX]; /* Name and version of SAR processor.  */
  data_type_t data_type;            /* Type of samples (e.g. "REAL4").     */
/*  Possible values for data_type:                                         *
 *   META STRING          WHAT IT IS                 DATA TYPE             *
 *    BYTE                1 byte                   = unsigned char         *
 *    INTEGER16           2 byte integer           = short int             *
 *    INTEGER32           4 byte integer           = int                   *
 *    REAL32              4 byte floating point    = float                 *
 *    REAL64              8 byte floating point    = double                *
 *    COMPLEX_BYTE        two 1 byte samples       = unsigned char         *
 *    COMPLEX_INTEGER16   two 2 byte integers      = short int             *
 *    COMPLEX_INTEGER32   two 4 byte integers      = int                   *
 *    COMPLEX_REAL32      two 4 byte floats        = float                 *
 *    COMPLEX_REAL64      two 8 byte floats        = double                *
 *    ???                 unknown (-999999999)                             */
  image_data_type_t image_data_type;/* Image data type                     */
/* Possible values for image type:                                         *
 *   META STRING          IMAGE DATA TYPE                                  *
 *    RAW                 Level 0 raw data (STF or CEOS)                   *
 *    COMPLEX             Level 1 complex data                             *
 *    AMPLITUDE           Amplitude image                                  *
 *    PHASE               Phase image (e.g. interferogram)                 *
 *    POWER               Power (magnitude) image                          *
 *    SIGMA               Sigma image (calibrated amplitude) [dB]          *
 *    GAMMA               Gamma image (calibrated amplitude) [dB]          *
 *    BETA                Beta image (calibrated amplitude) [dB]           *
 *    COHERENCE           Coherence image                                  *
 *    GEOCODED_IMAGE      Geocoded image                                   *
 *    DEM                 Digital elevation model                          *
 *    IMAGE               Generic image                                    */
  char system[FIELD_STRING_MAX];    /* System of samples (e.g. "ieee-std") */
/*  Possible string values for system:                                     *
 *    OUR NAME      DDR EQUIVALENT      WHAT IT IS                         *
 *    big_ieee        ieee-std       IEEE standard (ref 754)               *
 *    lil_ieee        ieee-lil       IEEE standard (ref 754) little-endian *
 *      ???           ibm-mvs        IBM MVS                               *
 *   cray_float       cray-unicos    Cray Y-MP Unicos                      *
 *      ???           other-msc      Misc. Other systems not covered       */
  int orbit;                 /* Orbit number of satellite.                 */
  char orbit_direction;	     /* Ascending 'A', or descending 'D'.          */
  int frame;                 /* Frame for this image or -1 if inapplicable.*/
  int band_number;           /* Band number; first band is 0               */
  int line_count;            /* Number of lines in image.                  */
  int sample_count;          /* Number of samples in image.                */
  int start_line;            /* First line relative to original image.     */
  int start_sample;          /* First sample relative to original image.   */
  double x_pixel_size;       /* Range pixel size, in meters                */
  double y_pixel_size;       /* Azimuth pixel size, in meters              */
  double center_latitude;    /* Approximate image center latitude.         */
  double center_longitude;   /* Approximage image center longitude.        */
  double re_major;           /* Semimajor axis length (equator) (meters).  */
  double re_minor;           /* Semiminor axis length (poles) (meters).    */
  double bit_error_rate;     /* Fraction of bits which are in error.       */
  int missing_lines;         /* Number of missing lines in data take       */
} meta_general;


/********************************************************************
 * meta_sar: SAR specific parameters
 */
typedef struct {
  /* 'S'-> Slant Range; 'G'-> Ground Range; 'P'-> Map Projected.  */
  char image_type;
  char look_direction;            /* 'L'-> Left Looking; 'R'-> Right Looking*/
  int look_count;                 /* Number of looks to take from SLC.      */
  int deskewed;                   /* True if image moved to zero doppler.   */
  int original_line_count;        /* Number of lines in original image      */
  int original_sample_count;      /* Number of samples in original image    */
  double line_increment;          /* line increment for sampling            */
  double sample_increment;        /* sample increment for sampling          */
  double range_time_per_pixel;    /* Time per pixel in range.               */
  double azimuth_time_per_pixel;  /* Time per pixel in azimuth.             */
  double slant_shift;             /* Error correction factor, in slant range*/
  double time_shift;              /* Error correction factor, in time.      */
  double slant_range_first_pixel; /* Slant range to first pixel.            */
  double wavelength;              /* SAR carrier wavelength, in meters.     */
  double prf;                     /* Pulse Repition Frequency.              */
  double earth_radius;            /* Earth radius at scene center.          */
  double earth_radius_pp;         /* Earth radius at scene center,          */
                                  /* as used by the PP.                     */
  double satellite_height;        /* Satellite height from earth's center.  */
  char satellite_binary_time[FIELD_STRING_MAX];  /* Satellite binary time   */
  char satellite_clock_time[FIELD_STRING_MAX];   /* Satellite UTC clock time*/
    /* Doppler centroid, doppler per pixel, and doppler per pixel squared.  */
  double range_doppler_coefficients[3];
    /* Doppler centroid, doppler per pixel, and doppler per pixel squared.  */
  double azimuth_doppler_coefficients[3];
  double azimuth_processing_bandwidth; /* version 1.4 */
  double chirp_rate;                   /* version 1.4 */
  double pulse_duration;               /* version 1.4 */
  double range_sampling_rate;          /* version 1.4 */
} meta_sar;


/********************************************************************
 * meta_optical: paramenters specific to optical images
 * NOT YET IN USE
 */
typedef struct {
  double cloud_pct;                  /* Cloud cover percentage */
  double sun_az_angle;               /* Sun azimuth angle      */
  double sun_elev_angle;             /* Sun elevation angle    */
} meta_optical;


/********************************************************************
 * meta_thermal: paramenters specific to thermal images
 * NOT YET IN USE
 */
typedef struct {
  double band_gain;                  /* band gain                   */
  double band_gain_change;           /* band gain change            */
  int day;                           /* Day/Night flag 1=day 0=night*/
} meta_thermal;


/********************************************************************
 * meta_projection / proj_parameters: These describe a map projection.
 * Projection parameter components: one for each projection.
 */
 /* Albers Conical Equal Area. */
  typedef struct {
    double std_parallel1;     /* First standard parallel           */
    double std_parallel2;     /* Second standard parallel          */
    double center_meridian;   /* Longitude of center meridian      */
    double orig_latitude;     /* Latitude of the projection origin */
    double false_easting;     /* False Easting                     */
    double false_northing;    /* False Northing                    */
  } proj_albers;
 /* Along-track/cross-track.*/
  typedef struct {
    double rlocal;              /* Radius of earth at scene center (meters)*/
    double alpha1,alpha2,alpha3;/* Rotation angles, in degrees             */
  } proj_atct;
 /* Lambert Azimuthal Equal Area. */
  typedef struct {
    double center_lon;        /* Longitude at center of projection */
    double center_lat;        /* Latitude at center of projection  */
    double false_easting;     /* False Easting                     */
    double false_northing;    /* False Northing                    */
  } proj_lamaz;
 /* Lambert Conformal Conic.*/
  typedef struct {
    double plat1;             /* First standard parallel  */
    double plat2;             /* Second standard parallel */
    double lat0;              /* Latitude of Origin       */
    double lon0;              /* Center Meridian          */
    double false_easting;     /* False Easting            */
    double false_northing;    /* False Northing           */
    double scale_factor;      /* Scale Factor             */
  } proj_lamcc;
 /* Polar Sterographic.  */
  typedef struct {
    double slat;              /* Standard Parallel 1                        */
    double slon;              /* Center Meridian                            */
    int is_north_pole;        /* 1 if centered on North Pole, 0 if South    */
    double false_easting;     /* False Easting                              */
    double false_northing;    /* False Northing                             */
  } proj_ps;
 /* Universal Transverse Mercator.*/
  typedef struct {
    int zone;                 /* Zone                                       */
    double false_easting;     /* False Easting                              */
    double false_northing;    /* False Northing                             */
    double lat0;              /* Latitude                                   */
    double lon0;              /* Longitude                                  */
    double scale_factor;      /* Scale factor: 0.9996 by definition         */
  } proj_utm;
 /* State Plane. */
  typedef struct {
    int zone;
  } proj_state;
/* For lat long pseudo projected images, no additional parameters are
   required, so they don't have their own structure type.  */

 /* Projection parameters for the projection in use.  */
  typedef union {
    proj_albers   albers;   /* Albers Conical Equal Area     */
    proj_atct     atct;     /* Along-track/cross-track       */
    proj_lamaz    lamaz;    /* Lambert Azimuthal Equal Area  */
    proj_lamcc    lamcc;    /* Lambert Conformal Conic       */
    proj_ps       ps;       /* Polar Sterographic            */
    proj_utm      utm;      /* Universal Transverse Mercator */
    proj_state    state;    /* State Plane                   */
  } param_t;
typedef param_t project_parameters_t;
typedef struct {
  projection_type_t type;  /* Projection types */
  double startX,startY;  /* Projection coordinates of top, lefthand corner.*/
  double perX,perY;      /* Projection coordinates per X and Y pixel.      */
  char units[12];        /* Units of projection (meters, arcsec, or degrees) */
  char hem;              /* Hemisphere Code: 'S'->southern; other northern.*/
  spheroid_type_t spheroid; /* Spheroid - will replace re_major and re_minor */
  double re_major;       /* Semimajor axis length (equator) (meters).      */
  double re_minor;       /* Semiminor axis length (poles) (meters).        */
  /* Note: we compute ecc=sqrt(1-re_major^2/re_minor^2).  This field
     is therefore redundant and should be eliminated.  DEPRECATED.         */
/*  double ecc;           * First eccentricity of earth ellipsoid.         */
  datum_type_t datum;    /* Geodetic datum - height reference system       */
  double height;         /* Height of the image is geocoded to             */
  project_parameters_t param;   /* Projection parameters for each projection.*/
} meta_projection;
 /* Compatibility alias.  proj_parameters is DEPRECATED.  */
typedef meta_projection proj_parameters;


/********************************************************************
 * meta_stats: statistical info about the image
 */
typedef struct {
  double min, max;           /* Minimum and maximum image values      */
  double mean;               /* Mean average of image values          */
  double rmse;               /* Root mean squared error               */
  double std_deviation;      /* Standard deviation                    */
  double mask;               /* Value ignored while taking statistics */
} meta_stats;


/********************************************************************
 * State_vectors: Some collection of fixed-earth state vectors around
 * the image.  These are always increasing in time; but beyond that,
 * have no assumptions.
 */
typedef struct {
  double time;     /* Time of state vector, in seconds from the
                      start of the image.  */
  stateVector vec; /* Fixed-earth state vector.  */
} state_loc;
typedef struct {
  int year;           /* Year for first state vector                  */
  int julDay;         /* Julian day of year for first state vector.   */
  double second;      /* Seconds of day for first state vector.       */
  int vector_count;   /* Number of state vectors.                     */
  int num;            /* Same as vector_count.  For backward compat.  */
  state_loc vecs[1];    /* Array sized at run-time.                     */
} meta_state_vectors;


/*********************************************************************
 * Location: Stores the corner coordinates of the image in latitude
 * and longitude. This feature has been requested by users a large
 * number of times. Having these coordinates around can be handy.
 */
typedef struct {
  double lat_start_near_range;   /* Latitude at image start in near range */
  double lon_start_near_range;   /* Longitude at image start in near range */
  double lat_start_far_range;    /* Latitude at image start in far range */
  double lon_start_far_range;    /* Longitude at image start in far range */
  double lat_end_near_range;     /* Latitude at image end in near range */
  double lon_end_near_range;     /* Longitude at image end in near range */
  double lat_end_far_range;      /* Latitude at image end in far range */
  double lon_end_far_range;      /* Longitude at image end in far range */
} meta_location;


/* DEPRECATED */
/*Geo_parameters: These are used in geolocating the image.*/
typedef struct {
  char type;                    /* 'S'-> Slant Range; 'G'-> Ground Range;
                                   'P'-> Map Projected.                         */
  proj_parameters *proj;        /* Projection parameters, for map-projected images*/
  char lookDir;                 /* 'L'-> Left Looking; 'R'-> Right Looking.     */
  int deskew;                   /* Image moved to zero-doppler? (1-> yes; 0->no)*/
  double xPix,yPix;             /* Range, azimuth pixel size, in m              */
  double rngPixTime,azPixTime;  /* Range, Azimuth pixel time, in s.             */
  double timeShift, slantShift; /* Image correction (fudge)
                                   factors in azimuth and range, in s and m.    */
  double slantFirst;            /* Slant range to first pixel, in m.            */
  double wavelen;               /* Satellite wavelength, in meters.             */
  double dopRange[3], dopAz[3]; /* Doppler centroid constant, linear, and
                                   quadratic terms, in azimuth and range (Hz)   */
} geo_parameters;

/* DEPRECATED */
/*Ifm_parameters: These are used only for interferometry.*/
typedef struct {
  double er;  /* Earth radius at scene center.                               */
  double ht;  /* Satellite height from earth's center.                       */
  int nLooks; /* Number of looks to take on SLC data to make square pixels   */
  int orig_nLines,orig_nSamples;
/*  double lookCenter; * Look angle to image center (CALCULATED but not used)*/
} ifm_parameters;

/* DEPRECATED */
/*extra_info: extra information needed to re-create CEOS files.*/
typedef struct {
  char   sensor[FIELD_STRING_MAX];      /* Name of imaging sensor.          */
  char   mode[MODE_FIELD_STRING_MAX];   /* Mode of imaging sensor.          */
  char   processor[FIELD_STRING_MAX];   /* Name and version of SAR processor*/
  int    orbit;                         /* Orbit number of satellite.       */
  double bitErrorRate;                  /* Exactly what it says.            */
  char   satBinTime[FIELD_STRING_MAX];  /* Satellite binary clock time.     */
  char   satClkTime[FIELD_STRING_MAX];  /* Satellite UTC time.              */
  double prf;                           /* Pulse Repition Frequency.        */
} extra_info;

/********************************************************************
 * General ASF metadta structure.  Collection of all above.
 */
/**
"meta_parameters" is a struct that represents in memory what the 
.meta file stores on disk.
*/
typedef struct {
  double meta_version;     /* Version of metadata format conformed to*/

  meta_general       *general;
  meta_sar           *sar;             /* Can be NULL (check!).  */
  meta_optical       *optical;         /* Can be NULL (check!).  */
  meta_thermal       *thermal;         /* Can be NULL (check!).  */
  meta_projection    *projection;      /* Can be NULL (check!).  */
  meta_stats         *stats;
  meta_state_vectors *state_vectors;   /* Can be NULL (check!).  */
  meta_location      *location;
  
#if 0
    /* Deprecated elements from old metadata format.  */
  meta_state_vectors *stVec;	       /* Can be NULL (check!).  */
  geo_parameters  *geo;
  ifm_parameters  *ifm;
  extra_info      *info;               /* Can be NULL (check!).  */
#endif
} meta_parameters;


/****************** Creation/IO **********************/
/**  Create a new totally empty metadata structure. */
ASF_COREDLL meta_parameters *meta_raw_init(void);

/** Read a brand new metadata structure from this .meta file */
ASF_COREDLL meta_parameters *meta_read(const char *inName);

/** Read a brand new metadata source from this .meta file */
ASF_COREDLL metadata_source *meta_read_source(const char *inName);

/** Write this meta_parameters object to a .meta file. */
ASF_COREDLL void meta_write(meta_parameters *meta,const char *outName);

/** Destroy and deallocate this meta_parameters object. */
ASF_COREDLL void meta_free(meta_parameters *meta);



#if 0 /*********** FIXME: implement these in C++! ***********/

/* Create meta struct from a CEOS file */
meta_parameters *meta_create(const char *fName);

/* Return true if the file base name given has a corresponding new
   style .meta file.  */
int meta_is_new_style(const char *file_base_name);

/*************************************************************
These routines all return various parameters from the
relevant metadata.  Values are always in m, m/s, s, and radians.
All arrays and coordinates are zero-based.
*/

/******************** General ***********************
General Calls: in meta_get.c*/

/* Figure out byte ordering system */
char *meta_get_system(void);

/* Convert a line, sample pair to a time, slant-range pair.
These only use the geo_parameters structure, and work
for all images.  They apply the time and slant range
correction fudge factors. Returns seconds and meters.
*/
double meta_get_time(meta_parameters *sar,double yLine,double xSample);
double meta_get_slant(meta_parameters *sar,double yLine, double xSample);

/*Converts a line, sample pair to the doppler value
at that location. Returns Hz.  Only works for SR & GR.
*/
double meta_get_dop(meta_parameters *sar,double yLine, double xSample);

/*Return fixed-earth state vector for the given time.*/
stateVector meta_get_stVec(meta_parameters *sar,double time_arg);

/*Return the incidence angle: this is the angle measured
	by the target between straight up and the satellite.
	Returns radians.*/
double meta_incid(meta_parameters *sar,double y,double x);

/*Return the look angle: this is the angle measured
	by the satellite between earth's center and the target point.
	Returns radians.*/
double meta_look(meta_parameters *sar,double y,double x);

/************* Geolocation ***********************
Geolocation Calls: in meta_get_geo.c.
Here, latitude and longitude are always in degrees.*/

/* Converts a given line and sample to that for the non-multilooked,
   non-windowed, equivalent image.*/
void meta_get_original_line_sample(meta_parameters *meta, int line,
				   int sample, int *original_line,
				   int *original_sample);

/* DEPRECATED.  You probably want meta_get_original_line_sample.  */
void meta_get_orig(void *ddr,
	int y, int x,int *yOrig,int *xOrig);

/* Converts given line and sample to geodetic
latitude and longitude.  Works with all image types.
*/
void meta_get_latLon(meta_parameters *sar,
	double yLine, double xSample,double elev,double *lat,double *lon);

/* Finds line and sample corresponding to given
latitude and longitude. */
void meta_get_lineSamp(meta_parameters *meta,
	double lat,double lon,double elev,double *yLine,double *xSample);

/* Converts a given line and sample in image into time,
slant-range, and doppler.  Works with all image types.
*/
void meta_get_timeSlantDop(meta_parameters *sar,
	double yLine,double xSample,
	double *time_arg,double *slant,double *dop);

/*Converts the given time, slant range, doppler, and elevation
off earth's surface into a latitude and longitude.*/
void meta_timeSlantDop2latLon(meta_parameters *sar,
	double time_arg, double slant,double dop,double elev,
	double *lat,double *lon);

/*Convert given latitude and longitude to time, slant
range, and doppler, using state vectors.*/
void latLon2timeSlant(meta_parameters *sar,
	double lat,double lon,
	double *time_arg,double *slant,double *dop);

void getLatLongMeta(const stateVector stVec,meta_parameters *meta,
	double range,double doppler,double elev,
	double *targLat, double *targLon, double *targRadius);

/* This low-level routine is used internally by asf_meta.  */
GEOLOCATE_REC *init_geolocate_meta(const stateVector *stVec,meta_parameters *meta);

/* Determines the geographic corner coordinates and popluates the location block */
void meta_get_corner_coords(meta_parameters *meta);


/************* Interferometry *******************
Interferometry Calls: in meta_get_ifm.c*/

/*Return satellite height.*/
double meta_get_sat_height(meta_parameters *meta, long line, long sample);

/*Return earth radius at scene center.*/
double meta_get_earth_radius(meta_parameters *meta, long line, long sample);

/*Return the earth radius we estimate to have been used by the PP
  during L0 processing.  If the data was not processed by the PP,
  or this esimation has not yet been done (pp_get_corrected_vals),
  then this just forwards the call to meta_get_earth_radius(meta, 0, 0) */
double meta_get_earth_radius_pp(meta_parameters *meta);

/*Return slant range to first (leftmost) pixel, and slant range per pixel.*/
void meta_get_slants(meta_parameters *sar,double *slantFirst, double *slantPer);

/*return satellite wavenumber k=2*PI/wavelength.*/
double meta_get_k(meta_parameters *sar);

/*Return the fraction of the scene that is after line y*/
double meta_scene_frac(meta_parameters *sar,int y);

/*Interpolate the given baseline for the (original image) line y.*/
void meta_interp_baseline(meta_parameters *sar,const baseline base,
	int y,double *Bn_y,double *Bp_y);

/*Return the "flat earth" look deviation--
	this is the x look angle minus the center look angle.*/
double meta_flat(meta_parameters *sar,double y,double x);

/*Return the expected phase of the given point for a bald earth-- elevation 0.*/
double meta_flat_phase(meta_parameters *sar,const baseline base,int y,int x);

/*Return the "phase rate"-- the number of meters of elevation per radian of phase.*/
double meta_phase_rate(meta_parameters *sar,const baseline base,int y,int x);

/* Convert meta->general->data_type field from complex to polar and visa versa.
 * If data is already in the correct format, leave it.*/
int meta_polar2complex(int data_type);
int meta_complex2polar(int data_type);

/* meta_is_valid_*:
 * Tests to see if the value is MAGIC_UNSET_*. If so return FALSE (not valid)
 * otherwise return TRUE (valid) */
int meta_is_valid_char(char value);
int meta_is_valid_string(char *value);
int meta_is_valid_int(int value);
int meta_is_valid_double(double value);

/* Propagate state vector source from time sourceSec to time destSec,
   returning the new propagated state vector.  */
stateVector propagate(stateVector source, double sourceSec, double destSec);

/*Propagate the state vectors in the given meta_parameters structure so they
 * start at the image start. Make nStVec of them, data_int seconds apart.*/
void propagate_state(meta_parameters *meta,int nStVec,double data_int);

/* Reads a subset of an image */
void readSubset(char *fileName, int width, int height, int posX, int posY,
		float *subset);
void readComplexSubset(char *fileName, int width, int height, int posX, int posY, 
		       complexFloat *subset);

/* From pp_corrected_vals.c */
void pp_get_corrected_vals(char *sarName, double *corrected_earth_radius,
                           double *corrected_azimuth_time_per_pixel);

/* Keep track of open meta and ddr structures, so that all updated
 * metadata can be written to the metafile, initialized in meta_init.c Nov '02 */
typedef struct {
    char             base_name[1024];
    meta_parameters *meta;
    struct DDR      *ddr;
} META_DDR_STRUCT;

#define NUM_META_DDR_STRUCTS 10
extern META_DDR_STRUCT meta_ddr_structs[NUM_META_DDR_STRUCTS];

#endif /* end disabled metadata code */

}; /* end asf namespace */

#endif
