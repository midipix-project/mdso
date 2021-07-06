/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2021  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <mdso/mdso.h>

#define MDSO_UNUSED_PARAMETER(p) (void)p

int main(int argc, char ** argv, char ** envp)
{
	MDSO_UNUSED_PARAMETER(argc);
	return mdso_main(argv,envp,0);
}
