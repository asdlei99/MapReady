#include "asf.h"
#include "asf_meta.h"

#include <stdio.h>
#include <stdlib.h>

#define VERSION 0.2

static void usage(char *progname)
{
    asfPrintStatus("\n");
    asfPrintStatus("Usage: %s <image file> <lat> <lon> [height]\n",progname);
    asfPrintStatus("   image file     Name of a CEOS or ASF Internal file\n");
    asfPrintStatus("   lat            Latitude (in degrees)\n");
    asfPrintStatus("   lon            Longitude (in degrees)\n");
    asfPrintStatus("   height         Height (in meters), optional with default 0\n");
    asfPrintStatus("\n");
    exit(EXIT_FAILURE);
}

int main(int argc,char *argv[])
{
  double lat, lon;  
  double height = 0;
  double line, samp;

  handle_common_asf_args(&argc, &argv, "llh2ls");

  if (argc != 4 && argc != 5) {
    usage(argv[0]);
  }

  lat = atof(argv[2]);
  lon = atof(argv[3]);
  if (argc == 5) height = atof(argv[4]);

  meta_parameters *meta=NULL;
  char *ext = findExt(argv[1]);
  if (ext && strcmp_case(ext, ".L")==0)
    meta = meta_create(argv[1]);
  else if (ext && strcmp_case(ext, ".meta")==0)
    meta = meta_read(argv[1]);
  else {
    char *meta_file = appendExt(argv[1], ".meta");
    if (fileExists(meta_file))
      meta = meta_read(meta_file);
    else
      meta = meta_create(argv[1]);
  }

  if (!meta)
    asfPrintError("Could not find metadata file: %s\n", argv[1]);

  if (!meta_get_lineSamp(meta, lat, lon, height, &line, &samp)) {
    printf("Line:   %f\n", line);
    printf("Sample: %f\n", samp);
  } else {
    printf("Couldn't determine line/sample values.\n");
  }

  meta_free(meta);
  exit(EXIT_SUCCESS);
}

