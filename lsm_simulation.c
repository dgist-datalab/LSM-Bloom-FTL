#include <string.h>
#include <stdlib.h>
#include <cmath>
#include "lsm_simulation.h"
#include "iteration.h"
using namespace std;
extern uint64_t write_cnt;
extern uint32_t last_compaction_cnt;
extern monitor lsm_monitor;

LSM *tlsm;

LSM* lsm_init(char t, uint32_t level_num, uint32_t size_factor, uint32_t blocknum, uint64_t range){
	LSM *lsm=(LSM*)malloc(sizeof(LSM));
	if(level_num && size_factor){
		lsm->level=level_num;
		lsm->sizefactor=size_factor;
		lsm->now=0;
		lsm->max=level_num;
	}
	else{
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
	}
	lsm->max_block_num=blocknum;
	lsm->valid_block_num=blocknum;

	lsm->array=(level*)malloc(sizeof(level) * lsm->max);
	for(uint32_t i=0; i<lsm->max; i++){
		level_init(&lsm->array[i], i+1, lsm);
	}

	lsm->lba_run_id=(uint64_t*)calloc(sizeof(uint64_t), range);

	lsm->buffer=(block*)malloc(sizeof(block));
	block_init(lsm->buffer);
	lsm->last_level_valid=0;
	tlsm=lsm;
	return lsm;
}

run lsm_level_to_run(LSM *lsm, level *lev, uint32_t idx, iter **iter_set, uint32_t iter_num, uint32_t target_run_size){
	run new_run;
	run_init(&new_run, target_run_size?target_run_size:pow(lsm->sizefactor, idx+1), true);

	bool *empty=(bool*)calloc(iter_num, sizeof(bool));
	uint32_t t_run=0, insert_idx=0;
	uint32_t t_lba=0, *temp, temp_idx=0;
	while(!check_done(empty, iter_num)){
		t_lba=UINT32_MAX;
		for(uint32_t i=0; i<iter_num; i++){
			if(empty[i]) continue;
			temp=(uint32_t*)iter_pick(iter_set[i]);
			if(t_lba> *temp){
				t_lba= *temp;
				t_run=i;
			}
		}

		//remove same lba
		for(uint32_t i=0; i<iter_num; i++){
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

		run_insert_value(&new_run, t_lba, insert_idx++, &write_cnt, lsm->lba_run_id);
		iter_move(iter_set[t_run]);
		if(!iter_pick(iter_set[t_run])){
			empty[t_run]=true;
		}
	}

	new_run.now=insert_idx/LPPB+(insert_idx%LPPB?1:0);

	free(empty);
	return new_run;
}

void lsm_compaction(LSM *lsm, uint32_t idx){

	lsm_monitor.level_read_num[idx]+=level_data(&lsm->array[idx]);

	uint32_t before_write=write_cnt;
	iter **iter_set=(iter**)malloc(sizeof(iter*) * lsm->sizefactor);
	for(uint32_t i=0; i<lsm->sizefactor; i++){
		iter_set[i]=iter_init(0, lsm->array[idx].array[i].now, &lsm->array[idx].array[i], run_get_value_from_idx);
	}

	run new_run=lsm_level_to_run(lsm, NULL, idx, iter_set, lsm->sizefactor, 0);
	run *t=&lsm->array[idx+1].array[lsm->array[idx+1].now];
	run_free(t);
	
	run *now;
	uint32_t i;
	for_each_target_max(&lsm->array[idx], i, now){
		run_free(now);
	}
	free(lsm->array[idx].array);
	level_init(&lsm->array[idx], idx+1, lsm);
	for(uint32_t i=0; i<lsm->sizefactor; i++){
		free(iter_set[i]);
	}
	free(iter_set);
	level_insert_run(&lsm->array[idx+1], new_run);
	
	lsm_monitor.level_write_num[idx]+=write_cnt-before_write;
}

int lba_comp(const void *_a, const void *_b){
	uint32_t a=*(uint32_t*)_a;
	uint32_t b=*(uint32_t*)_b;
	return a-b;
}

int lsm_insert(LSM* lsm, uint32_t lba){
	lsm->buffer->array[lsm->buffer->now++]=lba;
	write_cnt++;

	if(!full_check_target(lsm->buffer)) return 1;
	qsort(lsm->buffer->array, lsm->buffer->now, sizeof(uint32_t), lba_comp);
	
	z_level_insert_block(&lsm->array[0], *lsm->buffer);
	lsm->array[0].now++;
	
	for(uint32_t i=0; i<lsm->max-1; i++){
		if(i==lsm->max-2 &&	full_check_target(&lsm->array[i])
				&& full_check_target(&lsm->array[i+1])){
		//	printf("compaction %d -> %d\n", i, i+1);
			last_compaction_cnt++;
			lsm_last_compaction(lsm, &lsm->array[i], i);
			continue;
		}

		if(full_check_target(&lsm->array[i])){
		//	printf("compaction %d -> %d\n", i, i+1);
			lsm_compaction(lsm, i);		
		}
		else break;
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

void lsm_free(LSM *lsm, uint64_t t){
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

	printf("lev read  write  WAF BF+PIN\n");
	printf("0  0  %lu  1\n", t);

	for(uint32_t i=0; i<lsm->max; i++){
		printf("%u %lu  %lu  %.3lf %.3lf\n", i+1,lsm_monitor.level_read_num[i], lsm_monitor.level_write_num[i], 
				(double) lsm_monitor.level_write_num[i]/lsm_monitor.level_read_num[i],
				(double) lsm_monitor.level_write_num[i]/lsm_monitor.level_read_num[i]/PACK
				);
	}
	printf("last level merge run\n");
	for(uint32_t i=0; i<=lsm->sizefactor; i++){
		printf("%u %lu\n", i, lsm_monitor.last_level_merg_run_num[i]);
	}
}

void run_init(run *r, uint32_t blocknum, bool isconsume){
	static int cnt=0;
	r->now=0;
	r->max=blocknum;
	r->id=cnt++;
	r->array=(block*)malloc(sizeof(block)* blocknum);
	for(uint32_t i=0; i<r->max; i++){
		if(isconsume && !tlsm->valid_block_num){
			lsm_last_gc(tlsm, NULL, UINT32_MAX, r->max-i, false);
		}
		block_init(&r->array[i]);
	}
}

void run_free(run *r){
	tlsm->valid_block_num+=r->now;
	free((r)->array);
}

void level_init(level *lev, uint32_t idx, LSM *lsm){
	lev->now=0;
	lev->max=lsm->sizefactor;
	lev->array=(run*)malloc(sizeof(run)*lev->max);
	for(uint32_t i=0; i<lev->max; i++){
		run_init(&lev->array[i], pow(lsm->sizefactor, idx-1), false);
	}
}

void run_insert_value(run *r, uint32_t lba, uint32_t idx, uint64_t *write_monitor_cnt, uint64_t* lba_run_id){
	uint32_t block_n=idx/LPPB;
	uint32_t offset=idx%LPPB;
	lba_run_id[lba]=r->id;
	if(offset % 64==0){
		r->array[block_n].guard[offset/64]=lba;
	}

	if(!r->array[block_n].used){
		r->array[block_n].used=true;
		tlsm->valid_block_num--;
		tlsm->now_block_num++;
	}

	r->array[block_n].array[offset]=lba;
	r->array[block_n].now++;
	(*write_monitor_cnt)++;
}
