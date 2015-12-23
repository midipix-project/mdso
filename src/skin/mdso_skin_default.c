#include "mdso_driver_impl.h"
#include "argv/argv.h"

const struct argv_option mdso_default_options[] = {
	{"version",		'v',TAG_VERSION,ARGV_OPTARG_NONE,0,0,
				"show version information"},

	{"help",		'h',TAG_HELP,ARGV_OPTARG_OPTIONAL,"short|long",0,
				"show usage information [listing %s options only]"},

	{"machine",		'm',TAG_QUAD_PTR,ARGV_OPTARG_REQUIRED,"32|64",0,
				"set machine bits to %s"},

	{"libpath",		'l',TAG_LIBPATH,ARGV_OPTARG_REQUIRED,"loader|peb|system32",0,
				"runtime loader should search for the library either "
				"according to its internal/inherited path (loader), "
				"or according to the library path in the process PEB block (peb); "
				"alternatively, the loader may only search for the library "
				"in the system library directory (system32)."},

	{"libname",		'n',TAG_LIBNAME,ARGV_OPTARG_REQUIRED,0,"<libname>",
				"set dependency library name to %s"},

	{"dstdir",		'd',TAG_DSTDIR,ARGV_OPTARG_REQUIRED,0,"<dstdir>",
				"save generated assembly files under %s"},

	{"pretty",		'p',TAG_PRETTY,ARGV_OPTARG_REQUIRED,"yaml",0,
				"format output for parsing by %s"},

	{"expsyms",		'e',TAG_EXPSYMS,ARGV_OPTARG_NONE,0,0,
				"print exported symbols" },

	{0}
};
