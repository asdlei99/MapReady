#include "asf.h"
#include "asf_nan.h"
#include "asf_meta.h"

/*******************************************************************************
 * meta_is_valid_char:
 *   Tests to see if the value is MAGIC_UNSET_CHAR. If so return FALSE (not
 *   valid) otherwise return TRUE (valid) */
int meta_is_valid_char(char value)
{
	return (value!=MAGIC_UNSET_CHAR);
}

/*******************************************************************************
 * meta_is_valid_string:
 *   Tests to see if the value is MAGIC_UNSET_STRING. If so return FALSE (not
 *   valid) otherwise return TRUE (valid) */
int meta_is_valid_string(char *value)
{
	return strcmp(value,MAGIC_UNSET_STRING);
}

/*******************************************************************************
 * meta_is_valid_int:
 *   Tests to see if the value is MAGIC_UNSET_INT. If so return FALSE (not
 *   valid) otherwise return TRUE (valid) */
int meta_is_valid_int(int value)
{
	return (value!=MAGIC_UNSET_INT);
}


/*******************************************************************************
 * meta_is_valid_double:
 *   tests to see if the value is real (not nan or +/- inf) */
 // FIXME:  Using isfinite() rather than finite() would probably get rid of the
 // compiler warning and would work as well...
// 5/27/08: Taking above suggestion and putting in isfinite()
// if it doesn't break any of the builds, let's go with it
int meta_is_valid_double(double value)
{
	return isfinite(value);
}
