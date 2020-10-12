#include <stdlib.h>
#include "lsm_simulation.h"
#include "iteration.h"

extern LSM *tlsm;

iter *iter_init(uint32_t now, uint32_t max,void *target, void *(*get_value_from_idx)(void *target,uint32_t idx)){
	iter *res=(iter*)malloc(sizeof(iter));
	res->now=now;
	res->max=max;
	res->get_value_from_idx=get_value_from_idx;
	res->target=target;
	return res;
}

void* iter_pick(iter* it){
	void *res=it->get_value_from_idx(it->target, it->now);
	if(!res){
		uint32_t block_n=it->now/LPPB;
		uint32_t offset=it->now%LPPB;
		run *tr=(run*)it->target;
		if(!(it->now >=tr->now *LPPB) && tr->array[block_n].used){
			tlsm->valid_block_num++;
			if(tlsm->valid_block_num > tlsm->max_block_num){
				printf("valid block can't be over max block num %s:%d\n", __FILE__, __LINE__);
				abort();		
			}
			tr->array[block_n].used=false;
		}
	}
	return res;
}

void* iter_pick_move(iter *it){
	return it->get_value_from_idx(it->target, it->now++);
}

void iter_move(iter* it){
	it->now++;
	if(it->now % LPPB == 0){
		tlsm->valid_block_num++;
		if(tlsm->valid_block_num > tlsm->max_block_num){
			printf("valid block can't be over max block num %s:%d\n", __FILE__, __LINE__);
			abort();		
		}
		run *tr=(run*)it->target;
		tr->array[(it->now-1)/LPPB].used=false;
	}
}

void iter_free(iter *it){
	free(it);
}
