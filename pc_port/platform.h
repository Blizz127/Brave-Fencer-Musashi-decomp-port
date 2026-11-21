#ifndef MUSASHI_PORT_PLATFORM_H
#define MUSASHI_PORT_PLATFORM_H

#include <stddef.h>

const char *musashi_platform_name(void);

/* Host services the port needs from the operating system, one adapter per
 * host (platform/linux.c, platform/windows.c). The native lane's guest window
 * itself is musashi_native_lane_map_guest (musashi_native_lane.h), which the
 * adapter also defines. */

/* Make [addr, addr+size) inaccessible; 0 on success. */
int musashi_host_protect_none(void *addr, size_t size);

/* A host access fault names the running lane function on stderr
 * ("native_boot: HOST_FAULT addr=... lane_pc=...") and the process exits as
 * a SIGSEGV would (status 139). */
void musashi_host_install_fault_report(void);

/* A symbol the process may or may not have (optional SDL entry points);
 * NULL when absent. */
void *musashi_host_symbol(const char *name);

#endif
