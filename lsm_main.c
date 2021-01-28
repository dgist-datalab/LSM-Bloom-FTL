#include "lsm_simulation.h"
#include "lsm_params_module.h"
#include "ssd.h"

#include <unistd.h>
#include <stdlib.h>

//#define LEVEL 4
uint32_t NOL;
#define ROUND 8
#define OP 0.20f

uint64_t write_cnt=0;
uint64_t last_compaction_cnt=0;
monitor lsm_monitor;

int main(int argc, char *argv[]){
	
	if(argc!=2){
		NOL=4;
	}
	else{
		NOL=atoi(argv[1]);
	}

	uint32_t size_factor=0, level=NOL;
	float t;
	uint32_t *list=get_blocknum_list(&level, &size_factor, BLOCKNUMBER, &t);

	uint64_t range=(uint32_t)((double)(TOTALSIZE/4/K) * (1-OP));
	fprintf(stderr, "range :%lu origin:%lu, blocknum:%lu\n", range, (uint64_t)BLOCKNUMBER*LPPB,BLOCKNUMBER);
	fprintf(stderr, "level:%u size_factor: %u\n", NOL, size_factor);
	fprintf(stderr, "ratio: %.2lf, OP: %f\n", t, OP);
	LSM *lsm=lsm_init(TIER, NOL, size_factor, BLOCKNUMBER, range);

	uint32_t max=0;
	for(uint64_t i=0; i<range*ROUND; i++){
		uint32_t target_lba=rand()%range+1;
		if(max<target_lba) max=target_lba;
		if(lsm_insert(lsm, target_lba)==-1){
			break;
		}
		if((i+1)%10000==0){
			printf("\r progresee %.2f%%",(float)i/(range*ROUND)*100);
		}
	}
	
	printf("WAF: %.2lf, %lu, %lu\n", (double)write_cnt/(range*ROUND),write_cnt, range*ROUND);
	printf("last_compaction_cnt: %lu, max lba:%u\n", last_compaction_cnt,max);

	lsm_print_level(lsm, LEVEL-1);
	
	lsm_free(lsm, range*ROUND);
	return 0;
}
