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

typedef struct monitor{
	uint32_t level_read_num[100];
	uint32_t level_write_num[100];
	uint32_t last_level_merg_run_num[100];
}monitor;

typedef struct block{
	uint32_t now;
	uint32_t max;
	uint32_t guard[LPPB/64+1];
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
	uint32_t last_level_valid;
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
		free((r)->array);\
	}while(0)



static inline void block_init(block *b, uint32_t max=PAGEPERBLOCK){
	b->now=0;
	b->max=max*L2PGAP;
	memset(b->array, 0, sizeof(b->array));
	memset(b->guard, -1, sizeof(b->guard));
}

static inline void run_init(run *r, uint32_t blocknum){
	r->now=0;
	r->max=blocknum;
	r->array=(block*)malloc(sizeof(block)* blocknum);
	for(uint32_t i=0; i<r->max; i++){
		block_init(&r->array[i]);
	}
}

static inline void level_init(level *lev, uint32_t idx, LSM *lsm){
	lev->now=0;
	lev->max=lsm->sizefactor;
	lev->array=(run*)malloc(sizeof(run)*lev->max);
	/*
	if(idx>2){
		static int i=0;
		printf("%d - target_block_nunm %lf X %u blocksize:%u\n",i++,pow(lsm->sizefactor, idx-1), lev->max, sizeof(block));
	}*/
	for(uint32_t i=0; i<lev->max; i++){
		run_init(&lev->array[i], pow(lsm->sizefactor, idx-1));
	}
}

static inline void *run_get_value_from_idx(void *r, uint32_t idx){
	run *tr=(run*)r;
	if(idx>=tr->now * LPPB) return NULL;
	uint32_t block_n=idx/LPPB;
	uint32_t offset=idx%LPPB;
	uint32_t * target=&tr->array[block_n].array[offset];
	if(*target==0) return NULL;
	else return target;
}

static inline bool check_done(bool *a, uint32_t idx){
	for(uint32_t i=0; i<idx; i++){
		if(!a[i]) return false;
	}
	return true;
}

static inline uint32_t level_data(level *lev){
	run *r_now;
	block *b_now;
	uint32_t i,j, result=0;
	for_each_target_now(lev, i, r_now){
		for_each_target_now(r_now, j, b_now){
			result+=b_now->now;
		}
	}
	return result;
}

static inline void run_insert_value(run *r, uint32_t lba, uint32_t idx, uint32_t *write_monitor_cnt){
	uint32_t block_n=idx/LPPB;
	uint32_t offset=idx%LPPB;
	if(offset % 64==0){
		r->array[block_n].guard[offset/64]=lba;
	}
	r->array[block_n].array[offset]=lba;
	r->array[block_n].now++;
	(*write_monitor_cnt)++;
}

LSM* lsm_init(char t, uint32_t level, uint32_t size_factor, uint32_t blocknum);
int lsm_insert(LSM*, uint32_t lba);
void lsm_print_level(LSM *, uint32_t target_level);
void lsm_last_compaction(LSM*, level *, uint32_t idx);
void lsm_last_gc(LSM *lsm, run *rset, uint32_t run_num, uint32_t target_block);
run lsm_level_to_run(LSM *lsm, level *lev, uint32_t idx, iter**, uint32_t iter_num, uint32_t target_run_size);
void lsm_free(LSM *lsm);

#endif
