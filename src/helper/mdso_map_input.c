/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2017  Z. Gilboa                         */
/*  Released under GPLv2 and GPLv3; see COPYING.MDSO.           */
/****************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mdso/mdso.h>
#include "mdso_driver_impl.h"
#include "mdso_errinfo_impl.h"

int mdso_map_input(
	const struct mdso_driver_ctx *	dctx,
	int				fd,
	const char *			path,
	int				prot,
	struct mdso_input *		map)
{
	int		ret;
	struct stat	st;
	bool		fnew;
	int		fdcwd;

	fdcwd = mdso_driver_fdcwd(dctx);

	if ((fnew = (fd < 0)))
		fd  = openat(fdcwd,path,O_RDONLY | O_CLOEXEC);

	if (fd < 0)
		return MDSO_SYSTEM_ERROR(dctx);

	if ((ret = fstat(fd,&st) < 0) && fnew)
		close(fd);

	else if ((st.st_size == 0) && fnew)
		close(fd);

	if (ret < 0)
		return MDSO_SYSTEM_ERROR(dctx);

	if (st.st_size == 0) {
		map->size = 0;
		map->addr = 0;
	} else {
		map->size = st.st_size;
		map->addr = mmap(0,map->size,prot,MAP_PRIVATE,fd,0);
	}

	if (fnew)
		close(fd);

	return (map->addr == MAP_FAILED)
		? MDSO_SYSTEM_ERROR(dctx)
		: 0;
}

int mdso_unmap_input(struct mdso_input * map)
{
	return map->size ? munmap(map->addr,map->size) : 0;
}
