#include <stdlib.h>
#include "iteration.h"

iter *iter_init(uint32_t now, uint32_t max,void *target, void *(*get_value_from_idx)(void *target,uint32_t idx)){
	iter *res=(iter*)malloc(sizeof(iter));
	res->now=now;
	res->max=max;
	res->get_value_from_idx=get_value_from_idx;
	res->target=target;
	return res;
}

void* iter_pick(iter* it){
	return it->get_value_from_idx(it->target, it->now);
}

void* iter_pick_move(iter *it){
	return it->get_value_from_idx(it->target, it->now++);
}

void iter_move(iter* it){
	it->now++;
}

void iter_free(iter *it){
	free(it);
}
