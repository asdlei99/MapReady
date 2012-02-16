
#include <time.h>

#include <asf.h>
#include <asf_export.h>
#include <envi.h>


void
export_as_envi (const char *metadata_file_name,
                const char *image_data_file_name,
                const char *output_file_name)
{
  /* Get the image metadata.  */
  meta_parameters *md = meta_read (metadata_file_name);
  char *envi_file_name = NULL;
  envi_header *envi;
  FILE *fp;
  time_t calendar_time;
  char t_stamp[15];

  /* Complex data generally can't be output into meaningful images, so
     we refuse to deal with it.  */
  if (   md->general->data_type == COMPLEX_BYTE
      || md->general->data_type == COMPLEX_INTEGER16
      || md->general->data_type == COMPLEX_INTEGER32
      || md->general->data_type == COMPLEX_REAL32
      || md->general->data_type == COMPLEX_REAL64)
  {
    asfPrintError("Input data cannot be complex.\n");
  }

  envi_file_name = appendExt(output_file_name, ".hdr");
  envi = meta2envi (md);

  /* Write ENVI header file */
  fp = FOPEN(envi_file_name, "w");
  calendar_time = time(NULL);
  strftime(t_stamp, 12, "%d-%b-%Y", localtime(&calendar_time));
  fprintf(fp, "ENVI\n");
  fprintf(fp, "description = {\n"
              "  Converted to ENVI format on (%s)}\n", t_stamp);
  fprintf(fp, "samples = %i\n", envi->samples);
  fprintf(fp, "lines = %i\n", envi->lines);
  fprintf(fp, "bands = %i\n", envi->bands);
  fprintf(fp, "header offset = %i\n", envi->header_offset);
  fprintf(fp, "file type = %s\n", envi->file_type);
  fprintf(fp, "data type = %i\n", envi->data_type);
  fprintf(fp, "interleave = %s\n", envi->interleave);
  fprintf(fp, "sensor type = %s\n", envi->sensor_type);
  fprintf(fp, "byte order = %i\n", envi->byte_order);
  if (md->projection) {
    switch (md->projection->type) {
    case UNIVERSAL_TRANSVERSE_MERCATOR:
      fprintf(fp,
              "map info = {%s, %i, %i, %.4f, %.4f, %.4f, %.4f, %i, %s}\n",
              envi->projection, envi->ref_pixel_x, envi->ref_pixel_y,
              envi->pixel_easting, envi->pixel_northing, envi->proj_dist_x,
              envi->proj_dist_y, envi->projection_zone, envi->hemisphere);
      fprintf(fp,
              "projection info = {3, %.4f, %.4f, %.4f, %.4f, "
              "0.0, 0.0, 0.99996, %s}\n",
              envi->semimajor_axis, envi->semiminor_axis, envi->center_lat,
              envi->center_lon, envi->projection);
      break;
    case POLAR_STEREOGRAPHIC:
      fprintf(fp,
              "map info = {%s, %i, %i, %.4f, %.4f, %.4f, %.4f, %s}\n",
              envi->projection, envi->ref_pixel_x, envi->ref_pixel_y,
              envi->pixel_easting, envi->pixel_northing, envi->proj_dist_x,
              envi->proj_dist_y, envi->hemisphere);
      fprintf(fp,
              "projection info = {31, %.4f, %.4f, %.4f, %.4f, "
              "0.0, 0.0, %s}\n",
              envi->semimajor_axis, envi->semiminor_axis, envi->center_lat,
              envi->center_lon, envi->projection);
      break;
    case ALBERS_EQUAL_AREA:
      fprintf(fp,
              "map info = {%s, %i, %i  , %.4f, %.4f, %.4f, %.4f, %s}\n",
              envi->projection, envi->ref_pixel_x, envi->ref_pixel_y,
              envi->pixel_easting, envi->pixel_northing, envi->proj_dist_x,
              envi->proj_dist_y, envi->hemisphere);
      fprintf(fp,
              "projection info = {9, %.4f, %.4f, %.4f, %.4f, "
                "0.0, 0.0, %.4f, %.4f, %s}\n",
              envi->semimajor_axis, envi->semiminor_axis, envi->center_lat,
              envi->center_lon, envi->standard_parallel1,
              envi->standard_parallel2, envi->projection);
      break;
    case LAMBERT_CONFORMAL_CONIC:
      fprintf(fp,
              "map info = {%s, %i, %i, %.4f, %.4f, %.4f, %.4f, %s}\n",
              envi->projection, envi->ref_pixel_x, envi->ref_pixel_y,
              envi->pixel_easting, envi->pixel_northing, envi->proj_dist_x,
              envi->proj_dist_y, envi->hemisphere);
      fprintf(fp,
              "projection info = {4, %.4f, %.4f, %.4f, %.4f, "
              "0.0, 0.0, %.4f, %.4f, %s}\n",
              envi->semimajor_axis, envi->semiminor_axis, envi->center_lat,
              envi->center_lon, envi->standard_parallel1,
              envi->standard_parallel2, envi->projection);
      break;
    case LAMBERT_AZIMUTHAL_EQUAL_AREA:
      fprintf(fp,
              "map info = {%s, %i, %i, %.4f, %.4f, %.4f, %.4f, %s}\n",
              envi->projection, envi->ref_pixel_x, envi->ref_pixel_y,
              envi->pixel_easting, envi->pixel_northing, envi->proj_dist_x,
              envi->proj_dist_y, envi->hemisphere);
      fprintf(fp,
              "projection info = {11, %.4f, %.4f, %.4f, %.4f, "
              "0.0, 0.0, %s}\n",
              envi->semimajor_axis, envi->semiminor_axis, envi->center_lat,
              envi->center_lon, envi->projection);
      break;
    case LAT_LONG_PSEUDO_PROJECTION:
      asfPrintError ("Exporting pseudoprojected images in ENVI format is not "
		     "implemented.");
      break;
    case STATE_PLANE:
      asfPrintError ("Exporting state plane images in ENVI format is not "
		     "implemented.");
      break;
    case SCANSAR_PROJECTION:
      asfPrintError ("Exporting scansar projected images probably doesn't "
		     "make sense and isn't supported.");
      break;
    case UNKNOWN_PROJECTION:
    default:
      asfPrintError ("Unknown or unsupported projection type encountered. "
		     "Cannot export.");
      break;
    }
  }
  fprintf(fp, "wavelength units = %s\n", envi->wavelength_units);
  /*** wavelength, data ignore and default stretch currently not used ***/
  FCLOSE(fp);
  FREE(envi_file_name);

  /* Clean and report */
  free (envi);
  meta_free (md);

  fileCopy(image_data_file_name, output_file_name);
}
