#include "lsm_simulation.h"
#include "iteration.h"
extern uint64_t write_cnt;
extern monitor lsm_monitor;


static inline uint32_t num_block_run(run r){
	return r.now;
}

void lsm_last_compaction(LSM *lsm, level *lev, uint32_t idx){
	lsm_monitor.level_read_num[idx]+=level_data(&lsm->array[idx]);
	/*upper level merge*/
	uint32_t iter_num=lsm->sizefactor;
	iter **iter_set=(iter**)malloc(sizeof(iter*)*iter_num);
	for(uint32_t i=0; i<lsm->sizefactor; i++){
		iter_set[i]=iter_init(0, lsm->array[idx].array[i].now, &lsm->array[idx].array[i], run_get_value_from_idx);
	}
	lsm->gc_cnt=0;
	uint64_t before_write=write_cnt;
	run upper_run=lsm_level_to_run(lsm, NULL, idx, iter_set, iter_num, 0);
	lsm_monitor.level_write_num[idx]+=write_cnt-before_write-lsm->gc_cnt;

	uint32_t i;
	run *now;
	for_each_target_max(&lsm->array[idx], i, now){
		run_free(now);
	}
	free(lsm->array[idx].array);
	level_init(&lsm->array[idx], lsm->max-2, lsm);

	for(uint32_t i=0; i<iter_num; i++){
		free(iter_set[i]);
	}
	free(iter_set);
	iter_num=0;
	run *run_set=(run*)malloc(sizeof(run) * (lsm->array[idx+1].now +1));
	for(uint32_t i=0; i<lsm->array[idx+1].now; i++){
		run_set[iter_num++]=lsm->array[idx+1].array[i];
	}
	run_set[iter_num++]=upper_run;

	lsm_last_gc(lsm, run_set, iter_num, num_block_run(upper_run), true);
}
