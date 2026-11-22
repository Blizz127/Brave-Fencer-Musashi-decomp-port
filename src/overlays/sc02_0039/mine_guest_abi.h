#ifndef BFM_MINE_GUEST_ABI_H
#define BFM_MINE_GUEST_ABI_H

/* The PS1 routines can dereference a 32-bit game address directly. */
#if defined(__mips__) && !defined(__LP64__)
#define BFM_MINE_DIRECT_GUEST_ADDRESS 1
#else
#define BFM_MINE_DIRECT_GUEST_ADDRESS 0
#endif

#endif
