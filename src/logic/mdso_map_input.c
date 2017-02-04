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
#include "mdso_errinfo_impl.h"

int mdso_map_input(
	const struct mdso_driver_ctx *	dctx,
	int				fd,
	const char *			path,
	int				prot,
	struct mdso_input *		map)
{
	struct stat	st;
	bool		fnew;
	int		ret;

	if ((fnew = (fd < 0)))
		fd  = open(path,O_RDONLY | O_CLOEXEC);

	if (fd < 0)
		return MDSO_SYSTEM_ERROR(dctx);

	if ((ret = fstat(fd,&st) < 0) && fnew)
		close(fd);

	else if ((st.st_size == 0) && fnew)
		close(fd);

	if (ret < 0)
		return MDSO_SYSTEM_ERROR(dctx);

	else if (st.st_size == 0)
		return MDSO_CUSTOM_ERROR(
			dctx,MDSO_ERR_SOURCE_SIZE_ZERO);

	map->size = st.st_size;
	map->addr = mmap(0,map->size,prot,MAP_PRIVATE,fd,0);

	if (fnew)
		close(fd);

	return (map->addr == MAP_FAILED)
		? MDSO_SYSTEM_ERROR(dctx)
		: 0;
}

int mdso_unmap_input(struct mdso_input * map)
{
	return munmap(map->addr,map->size);
}
