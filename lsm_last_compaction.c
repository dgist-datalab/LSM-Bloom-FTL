#include "lsm_simulation.h"
#include "iteration.h"
extern uint32_t write_cnt;
static inline void run_insert_value(run *r, uint32_t lba, uint32_t idx){
	uint32_t block_n=idx/LPPB;
	uint32_t offset=idx%LPPB;
	r->array[block_n].array[offset]=lba;
	r->array[block_n].now++;
	write_cnt++;
}

void lsm_last_compaction(LSM *lsm, level *lev, uint32_t idx){
	uint32_t before_cnt=write_cnt;
	/*upper level merge*/
	uint32_t iter_num=lsm->sizefactor;
	iter **iter_set=(iter**)malloc(sizeof(iter*)*iter_num);
	for(uint32_t i=0; i<lsm->sizefactor; i++){
		iter_set[i]=iter_init(0, lsm->array[idx].array[i].now, &lsm->array[idx].array[i], run_get_value_from_idx);
	}
	run upper_run=lsm_level_to_run(lsm, NULL, idx, iter_set, iter_num, 0); 
	for(uint32_t i=0; i<lsm->sizefactor; i++){
		free(iter_set[i]);
	}
	free(iter_set);
	uint32_t middle_cnt=write_cnt;

	/*upper & bottom level*/
	idx++;
	iter_num=lsm->sizefactor+1;
	iter_set=(iter**)malloc(sizeof(iter*)*iter_num);
	for(uint32_t i=0; i<lsm->sizefactor; i++){;
		iter_set[i]=iter_init(0, lsm->array[idx].array[i].now, &lsm->array[idx].array[i], run_get_value_from_idx);
	}
	iter_set[lsm->sizefactor]=iter_init(0, upper_run.now, &upper_run, run_get_value_from_idx);
	run new_run=lsm_level_to_run(lsm, NULL, idx, iter_set, iter_num, BLOCKNUMBER);

	run_free(&upper_run);
	for(uint32_t i=0; i<lsm->sizefactor; i++){
		free(iter_set[i]);
	}
	free(iter_set);

	/*new run*/
	idx--;
	uint32_t i;
	run *now;
	for_each_target_max(&lsm->array[idx], i, now){
		run_free(now);
	}
	free(lsm->array[idx].array);
	level_init(&lsm->array[idx], idx+1, lsm);
	
	idx++;
	for_each_target_max(&lsm->array[idx], i, now){
		run_free(now);
	}
	free(lsm->array[idx].array);
	level_init(&lsm->array[idx], idx, lsm);
	level_insert_run(&lsm->array[idx], new_run);
	uint32_t after_cnt=write_cnt;
	write_cnt-=(middle_cnt-before_cnt);
	printf("last_cnt write_cnt: %u, %u\n",after_cnt-middle_cnt, middle_cnt-before_cnt);
}
