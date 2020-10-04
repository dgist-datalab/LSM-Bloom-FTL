#include <string.h>
#include <stdlib.h>
#include <cmath>
#include "lsm_simulation.h"
#include "iteration.h"
using namespace std;

static inline void block_init(block *b, uint32_t max=PAGEPERBLOCK){
	b->now=0;
	b->max=max*L2PGAP;
	memset(b->array, 0, sizeof(b->array));
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

LSM* lsm_init(char t, uint32_t level_num, uint32_t size_factor, uint32_t blocknum){
	LSM *lsm=(LSM*)malloc(sizeof(LSM));
	if(level_num){
		lsm->level=level_num;
		lsm->sizefactor=get_size_factor(level_num, blocknum);
		lsm->now=0;
		lsm->max=level_num;
	}
	else{
		lsm->sizefactor=size_factor;
		lsm->level=get_level(size_factor, blocknum);
		lsm->now=0;
		lsm->max=lsm->level;
	}

	lsm->array=(level*)malloc(sizeof(level) * lsm->max);
	for(uint32_t i=0; i<lsm->max; i++){
		level_init(&lsm->array[i], i+1, lsm);
	}

	lsm->buffer=(block*)malloc(sizeof(block));
	block_init(lsm->buffer);
	return lsm;
}

void *run_get_value_from_idx(void *r, uint32_t idx){
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

static inline void run_insert_value(run *r, uint32_t lba, uint32_t idx){
	uint32_t block_n=idx/LPPB;
	uint32_t offset=idx%LPPB;
	r->array[block_n].array[offset]=lba;
	r->array[block_n].now++;
}

void lsm_compaction(LSM *lsm, uint32_t idx){
	//static int cnt=0;
	//fprintf(stderr,"compaction cnt:%d %d\n", cnt++, idx);
	run new_run;
	run_init(&new_run, pow(lsm->sizefactor, idx+1));
	run *t=&lsm->array[idx+1].array[lsm->array[idx+1].now];
	run_free(t);

	iter **iter_set=(iter**)malloc(sizeof(iter*) * lsm->sizefactor);
	for(uint32_t i=0; i<lsm->sizefactor; i++){
		iter_set[i]=iter_init(0, lsm->array[idx].array[i].now, &lsm->array[idx].array[i], run_get_value_from_idx);
	}

	bool *empty=(bool*)calloc(lsm->sizefactor, sizeof(bool));
	uint32_t t_run=0, insert_idx=0;
	uint32_t t_lba=0, *temp, temp_idx=0;
	while(!check_done(empty, lsm->sizefactor)){
		t_lba=UINT32_MAX;
		for(uint32_t i=0; i<lsm->sizefactor; i++){
			if(empty[i]) continue;
			temp=(uint32_t*)iter_pick(iter_set[i]);
			if(t_lba> *temp){
				t_lba= *temp;
				t_run=i;
			}
		}

		//remove same lba
		for(uint32_t i=0; i<lsm->sizefactor; i++){
			if(empty[i]) continue;
			if(i==t_run) continue;
			temp=(uint32_t*)iter_pick(iter_set[i]);
			if(t_lba == *temp){
				iter_move(iter_set[i]);
				if(!iter_pick(iter_set[i])){
					empty[i]=true;
				}
			}
		}

		run_insert_value(&new_run, t_lba, insert_idx++);
		iter_move(iter_set[t_run]);
		if(!iter_pick(iter_set[t_run])){
			empty[t_run]=true;
		}
	}
	
	new_run.now=insert_idx/LPPB+(insert_idx%LPPB?1:0);

	run *now;
	uint32_t i;
	for_each_target_max(&lsm->array[idx], i, now){
		run_free(now);
	}
	free(lsm->array[idx].array);
	level_init(&lsm->array[idx], idx+1, lsm);

	free(empty);
	for(uint32_t i=0; i<lsm->sizefactor; i++){
		free(iter_set[i]);
	}
	free(iter_set);
	level_insert_run(&lsm->array[idx+1], new_run);
}

int lba_comp(const void *_a, const void *_b){
	uint32_t a=*(uint32_t*)_a;
	uint32_t b=*(uint32_t*)_b;
	return a-b;
}

int lsm_insert(LSM* lsm, uint32_t lba){
	lsm->buffer->array[lsm->buffer->now++]=lba;

	if(!full_check_target(lsm->buffer)) return 1;
	qsort(lsm->buffer->array, lsm->buffer->now, sizeof(uint32_t), lba_comp);
	
	z_level_insert_block(&lsm->array[0], *lsm->buffer);
	lsm->array[0].now++;
	

	for(uint32_t i=0; i<lsm->max-1; i++){
		if(full_check_target(&lsm->array[i])){
			lsm_compaction(lsm, i);		
		}
		else break;
	}

	if(full_check_target(&lsm->array[lsm->max-1])){
		fprintf(stderr,"last level full!!\n");
		return -1;
	}
	free(lsm->buffer);
	lsm->buffer=(block*)malloc(sizeof(block));
	block_init(lsm->buffer);
	return 1;
}

void lsm_print_level(LSM *lsm, uint32_t target_level){
	run *now;
	uint32_t idx=0;
	static uint32_t idx_t=1;
	for_each_target_now(&lsm->array[target_level], idx, now){
		block *b;
		uint32_t idx2=0;
		for_each_target_now(now, idx2, b){
			//printf("block #%d\n0 :",idx2);
			for(uint32_t j=1; j<=b->max; j++){
				printf("%u %u\n",idx_t++, b->array[j-1]);
			}
		}
	}
}

void lsm_free(LSM *lsm){
	for(uint32_t idx=0; idx<lsm->max; idx++){
		uint32_t i=0;
		run *now;
		for_each_target_max(&lsm->array[idx], i, now){
			run_free(now);
		}
		free(lsm->array[idx].array);
	}
	free(lsm->array);
	free(lsm->buffer);
}
