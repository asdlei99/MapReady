#include "asf_meta.h"
#include "esri.h"
#include "asf_nan.h"

esri_header* meta2esri(meta_parameters *meta)
{
  char pbuf[25];
  esri_header *esri;

  /* Allocate memory for ESRI header */
  esri = (esri_header *)MALLOC(sizeof(esri_header));

  /* Initialize the values */
  esri->nrows = MAGIC_UNSET_INT;
  esri->ncols = MAGIC_UNSET_INT;
  esri->nbands = 1;
  esri->nbits = 8;
  esri->byteorder = 'I';
  sprintf(esri->layout, "BIL"); /* Taken as the default value for now, 
				   since we don't have multiband imagery */
  esri->skipbytes = 0;  /* Our data don't have a header */
  esri->ulxmap = MAGIC_UNSET_DOUBLE;
  esri->ulymap = MAGIC_UNSET_DOUBLE;
  esri->xdim = MAGIC_UNSET_DOUBLE;
  esri->ydim = MAGIC_UNSET_DOUBLE;
  esri->bandrowbytes = MAGIC_UNSET_INT; /* not used yet */
  esri->totalrowbytes = MAGIC_UNSET_INT; /* not used yet */
  esri->bandgapbytes = MAGIC_UNSET_INT; /* not used yet */
  esri->nodata = MAGIC_UNSET_INT; /* no value assigned yet for metadata */

  /* Fill the values in from the metadata */
  esri->nrows = meta->general->line_count;
  esri->ncols = meta->general->sample_count;
  esri->nbands = meta->general->band_count;
  switch (meta->general->data_type) 
    {
    case BYTE: esri->nbits = 8; break;
    case INTEGER16: esri->nbits = 16; break;
    case INTEGER32: esri->nbits = 32; break;
    case REAL32: sprintf(pbuf, "REAL32"); break;
    case REAL64: sprintf(pbuf, "REAL64"); break;
    case COMPLEX_REAL32: sprintf(pbuf, "COMPLEX_REAL32"); break;
    default: 
      sprintf(errbuf,"\n   ERROR: Unsupported data type: %s\n\n", pbuf);
      printErr(errbuf);
      break;
    }
  // All our data are now generated as big-endian
  esri->byteorder = 'M'; /* Motorola byte order */
  if (meta->projection) {
    esri->ulxmap = meta->projection->startX;
    esri->ulymap = meta->projection->startY;
  }
  esri->xdim = meta->general->x_pixel_size;
  esri->ydim = meta->general->y_pixel_size;

  return esri;
}

meta_parameters* esri2meta(esri_header  *esri)
{
  meta_parameters *meta;

  /* Allocate memory for metadata structure */
  meta = raw_init();

  /* Fill metadata with valid ERSI header data */
  meta->general->line_count = esri->nrows;
  meta->general->sample_count = esri->ncols;
  meta->general->band_count = esri->nbands;

  switch (esri->nbits)
    {
    case 8: meta->general->data_type = BYTE; break;
    case 16: meta->general->data_type = INTEGER16; break;
    case 32: meta->general->data_type = INTEGER32; break;
    default:
      sprintf(errbuf,"\n   ERROR: Unsupported data type: %i bit\n\n", esri->nbits);
      printErr(errbuf);
      break;
    }

  if (!meta->projection) meta->projection = meta_projection_init();
  meta->projection->startX = esri->ulxmap;
  meta->projection->startY = esri->ulymap;

  meta->general->x_pixel_size = esri->xdim;
  meta->general->y_pixel_size = esri->ydim;

  return meta;
}
