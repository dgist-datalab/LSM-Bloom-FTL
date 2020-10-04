#include "lsm_simulation.h"
#include "lsm_params_module.h"
#include "ssd.h"

#include <unistd.h>
#include <stdlib.h>

#define LEVEL 5

int main(){
	uint32_t size_factor=get_size_factor(LEVEL, BLOCKNUMBER);
	float t=1-(float)(get_sparse_sorted_head(LEVEL,size_factor)*1)/BLOCKNUMBER;
	uint32_t range=(uint32_t)((double)(TOTALSIZE/4/K) * t);
	fprintf(stderr, "range :%lu blocknum:%lu\n", range, BLOCKNUMBER);
	fprintf(stderr, "size_factor: %u\n", size_factor);
	LSM *lsm=lsm_init(TIER, LEVEL, 0, BLOCKNUMBER);
	while(1){
		if(lsm_insert(lsm, rand()%range+1)==-1){
			break;
		}
	}
	
	lsm_print_level(lsm, LEVEL-1);
	lsm_free(lsm);
	return 0;
}
