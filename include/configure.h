#ifndef CONFIGURE_INCLUDED
#define CONFIGURE_INCLUDED

#include "librtl_433.h"

typedef enum {
	CFG_EXITCODE_MINUS1 = -1,
	CFG_EXITCODE_NULL = 0,
	CFG_EXITCODE_ONE = 1,
	CFG_SUCCESS_GO_ON = 2
} CfgResult;

CfgResult configure_librtl433(r_cfg_t *cfg, int argc, char **argv, int allow_default_cfgfile);
void add_infile(r_cfg_t *cfg, char *in_file);
void clear_infiles(r_cfg_t *cfg);

#endif // CONFIGURE_INCLUDED