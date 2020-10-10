#include "lsm_simulation.h"
#include "lsm_params_module.h"
#include "ssd.h"

#include <unistd.h>
#include <stdlib.h>

#define LEVEL 4
#define ROUND 4

uint32_t write_cnt=0;
uint32_t last_compaction_cnt=0;
monitor lsm_monitor;

int main(){
	uint32_t size_factor=get_size_factor(LEVEL, BLOCKNUMBER);
	float t=1-(float)(get_sparse_sorted_head(LEVEL,size_factor)*1)/BLOCKNUMBER;
	uint32_t range=(uint32_t)((double)(TOTALSIZE/4/K) * 0.93f);
	fprintf(stderr, "range :%lu blocknum:%lu\n", range, BLOCKNUMBER);
	fprintf(stderr, "size_factor: %u\n", size_factor);
	fprintf(stderr, "ratio: %.2lf\n", t);
	LSM *lsm=lsm_init(TIER, LEVEL, 0, BLOCKNUMBER);

	uint32_t max=0;
	for(uint32_t i=0; i<range*ROUND; i++){
		uint32_t target_lba=rand()%range+1;
		if(max<target_lba) max=target_lba;
		if(lsm_insert(lsm, target_lba)==-1){
			break;
		}
		if((i+1)%10000==0){
			printf("\r progresee %.2f%%",(float)i/(range*ROUND)*100);
		}
	}
	
	printf("WAF: %.2lf, %u, %u\n", (float)write_cnt/(range*ROUND),write_cnt, range*ROUND);
	printf("last_compaction_cnt: %u, max lba:%u\n", last_compaction_cnt,max);

	//lsm_print_level(lsm, LEVEL-1);

	printf("lev read  write  WAF\n");
	printf("0  0  %u  1\n", range*ROUND);
	for(uint32_t i=0; i<LEVEL; i++){
		printf("%u %u  %u  %.3f\n", i+1,lsm_monitor.level_read_num[i], lsm_monitor.level_write_num[i], (float) lsm_monitor.level_write_num[i]/lsm_monitor.level_read_num[i]);
	}
	printf("last level merge run\n");
	for(uint32_t i=0; i<=size_factor; i++){
		printf("%u %u\n", i, lsm_monitor.last_level_merg_run_num[i]);
	}

	lsm_free(lsm);
	return 0;
}
