#ifndef __SSD_H__
#define __SSD_H__

#define K (1024L)
#define M (K*1204)
#define G (M*1204)
#define T (G*1204)

#define PAGESIZE (K*16)
#define L2PGAP (PAGESIZE/4/K)
#define PAGEPERBLOCK (512L)
#define BLOCKSIZE (PAGESIZE*PAGEPERBLOCK)
#define TOTALSIZE (G*64)
#define BLOCKNUMBER (TOTALSIZE/BLOCKSIZE)

#endif
