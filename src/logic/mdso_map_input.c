/****************************************************************/
/*  mdso: midipix dso scavenger                                 */
/*  Copyright (C) 2015--2016  Z. Gilboa                         */
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

int mdso_map_input(
	int			fd,
	const char *		path,
	int			prot,
	struct mdso_input *	map)
{
	struct stat	st;
	bool		fnew;
	int		ret;

	if ((fnew = (fd < 0)))
		fd  = open(path,O_RDONLY | O_CLOEXEC);

	if (fd < 0)
		return -1;

	if ((ret = fstat(fd,&st) < 0) && fnew)
		close(fd);

	if (ret < 0)
		return -1;

	map->size = st.st_size;
	map->addr = mmap(0,map->size,prot,MAP_PRIVATE,fd,0);

	if (fnew)
		close(fd);

	return (map->addr == MAP_FAILED) ? -1 : 0;
}

int mdso_unmap_input(struct mdso_input * map)
{
	return munmap(map->addr,map->size);
}
