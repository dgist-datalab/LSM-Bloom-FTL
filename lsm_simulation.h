#ifndef __LSM_SIMULATION_H__
#define __LSM_SIMULATION_H__
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "iteration.h"
#include "lsm_params_module.h"
#include "ssd.h"

#define LPPB (PAGEPERBLOCK*L2PGAP)
#define PACK (PAGESIZE/(2*sizeof(uint32_t)))

typedef struct monitor{
	uint64_t buffer_write;
	uint64_t level_read_num[100];
	uint64_t level_write_num[100];
	uint64_t last_level_merg_run_num[100];
	uint64_t last_run_valid;
	uint64_t last_run_invalid;
}monitor;

typedef struct block{
	bool used;
	uint32_t now;
	uint32_t max;
	uint32_t guard[LPPB/64+1];
	uint32_t array[LPPB];
}block;


typedef struct run{
	uint64_t id;
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
	int32_t max;
	struct level *array;
	uint32_t last_level_valid;
	uint64_t *lba_run_id;
	block *buffer;
	int32_t max_block_num;
	int32_t valid_block_num;
	int32_t now_block_num;
	uint64_t gc_cnt;
	uint64_t range;
}LSM;

#define for_each_target_max(target, idx, _now)\
	for(idx=0, _now=&((target)->array[idx]); idx<(target)->max; idx++, _now=&((target)->array[idx]))

#define for_each_target_now(target, idx, _now)\
	for(idx=0, _now=&((target)->array[idx]); idx<(target)->now; idx++, _now=&((target)->array[idx]))

#define full_check_target(target) ((target)->now>=(target)->max)
#define run_insert_block(r, b) ((r)->array[(r)->now++]=(b))

#define level_insert_run(l, r) ((l)->array[(l)->now++]=(r))


static inline void block_init(block *b, uint32_t max=PAGEPERBLOCK){
	b->now=0;
	b->max=max*L2PGAP;
	b->used=0;
	memset(b->array, 0, sizeof(b->array));
	memset(b->guard, -1, sizeof(b->guard));
}

void run_init(run *r, uint32_t blocknum);

void level_init(level *lev, uint32_t idx, LSM *lsm);

void *run_get_value_from_idx(void *r, uint32_t idx);

static inline bool check_done(bool *a, uint32_t idx){
	for(uint32_t i=0; i<idx; i++){
		if(!a[i]) return false;
	}
	return true;
}

static inline uint64_t level_data(level *lev){
	run *r_now;
	block *b_now;
	uint64_t i,j, result=0;
	for_each_target_now(lev, i, r_now){
		for_each_target_now(r_now, j, b_now){
			result+=b_now->now;
		}
	}
	return result;
}

void run_insert_value(run *r, uint32_t lba, uint32_t idx, uint64_t *write_monitor_cnt, uint64_t* lba_run_id);
LSM* lsm_init(char t, uint32_t level, uint32_t size_factor, uint32_t blocknum, uint64_t range);
int lsm_insert(LSM*, uint32_t lba);
void lsm_print_level(LSM *, uint32_t target_level);
void lsm_print_run(LSM *, uint32_t target_level);
void lsm_print_summary(run *);
void lsm_last_compaction(LSM*, level *, uint32_t idx);
void lsm_last_gc(LSM *lsm, run *rset, uint32_t run_num, uint32_t target_block, bool needmerge);
run lsm_level_to_run(LSM *lsm, level *lev, uint32_t idx, iter**, uint32_t iter_num, uint32_t target_run_size);
void lsm_free(LSM *lsm, uint64_t);
void run_free(run *r);
void z_level_insert_block(level *l, block b);

#endif
