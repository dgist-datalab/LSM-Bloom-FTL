#ifndef __LSM_SIMULATION_H__
#define __LSM_SIMULATION_H__
#include <stdint.h>
#include <stdio.h>
#include "lsm_params_module.h"
#include "ssd.h"

#define LPPB (PAGEPERBLOCK*L2PGAP)

typedef struct block{
	uint32_t now;
	uint32_t max;
	uint32_t array[LPPB];
}block;


typedef struct run{
	uint32_t now;
	uint32_t max;
	struct block *array;
}run;

typedef struct level{
	uint32_t now;
	uint32_t max;
	struct run *array;
}level;

typedef struct LSM{
	uint32_t sizefactor;
	uint32_t level;
	uint32_t now;
	uint32_t max;
	struct level *array;
	block *buffer;
}LSM;

#define for_each_target_max(target, idx, _now)\
	for(idx=0, _now=&((target)->array[idx]); idx<(target)->max; idx++, _now=&((target)->array[idx]))

#define for_each_target_now(target, idx, _now)\
	for(idx=0, _now=&((target)->array[idx]); idx<(target)->now; idx++, _now=&((target)->array[idx]))

#define full_check_target(target) ((target)->now>=(target)->max)
#define run_insert_block(r, b) ((r)->array[(r)->now++]=(b))
#define z_level_insert_block(l, b) (run_insert_block(&(l)->array[(l)->now],(b)))
#define level_insert_run(l, r) ((l)->array[(l)->now++]=(r))

#define run_free(r) \
	do{\
		free(r->array);\
	}while(0)


LSM* lsm_init(char t, uint32_t level, uint32_t size_factor, uint32_t blocknum);
int lsm_insert(LSM*, uint32_t lba);
void lsm_print_level(LSM *, uint32_t target_level);
void lsm_free(LSM *lsm);
#endif
