#include "lsm_simulation.h"
#include "iteration.h"
extern uint32_t write_cnt;
extern monitor lsm_monitor;


static inline uint32_t num_block_run(run r){
	return r.now;
}

void lsm_last_compaction(LSM *lsm, level *lev, uint32_t idx){
	uint32_t before_cnt=write_cnt;
	lsm_monitor.level_read_num[idx]+=level_data(&lsm->array[idx]);
	/*upper level merge*/
	uint32_t iter_num=lsm->sizefactor;
	iter **iter_set=(iter**)malloc(sizeof(iter*)*iter_num);
	for(uint32_t i=0; i<lsm->sizefactor; i++){
		iter_set[i]=iter_init(0, lsm->array[idx].array[i].now, &lsm->array[idx].array[i], run_get_value_from_idx);
	}
	run upper_run=lsm_level_to_run(lsm, NULL, idx, iter_set, iter_num, 0);

	lsm_monitor.level_write_num[idx]+=write_cnt-before_cnt;
	for(uint32_t i=0; i<iter_num; i++){
		free(iter_set[i]);
	}
	free(iter_set);
	uint32_t middle_cnt=write_cnt;
	iter_num=0;
	run *run_set=(run*)malloc(sizeof(run) * (lsm->array[idx+1].now +1));
	for(uint32_t i=0; i<lsm->array[idx+1].now; i++){
		run_set[iter_num++]=lsm->array[idx+1].array[i];
	}
	run_set[iter_num++]=upper_run;

	before_cnt=write_cnt;
	lsm_last_gc(lsm, run_set, iter_num, num_block_run(upper_run));

	lsm_monitor.level_write_num[idx+1]+=write_cnt-before_cnt;
}
