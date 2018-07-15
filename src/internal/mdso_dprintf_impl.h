#ifndef MDSO_DPRINTF_IMPL_H
#define MDSO_DPRINTF_IMPL_H

#ifdef  ARGV_DRIVER
#define argv_dprintf mdso_dprintf
#endif

int mdso_dprintf(int fd, const char * fmt, ...);

#endif
