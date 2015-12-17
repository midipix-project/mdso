#include "mdso_driver_impl.h"
#include "argv/argv.h"

const struct argv_option mdso_default_options[] = {
	{"version",		'v',TAG_VERSION,ARGV_OPTARG_NONE,0,0,
				"show version information"},

	{"help",		'h',TAG_HELP,ARGV_OPTARG_OPTIONAL,"short|long",0,
				"show usage information [listing %s options only]"},

	{0}
};
