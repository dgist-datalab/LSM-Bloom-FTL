#ifndef __ITERATION_H__
#define __ITERATION_H__

#include <stdio.h>
#include <stdint.h>

typedef struct iteration{
	uint32_t now;
	uint32_t max;
	void *target;
	void *(*get_value_from_idx)(void *target, uint32_t idx);
}iter;

iter *iter_init(uint32_t now, uint32_t max, void *, void *(*get_value_from_idx)(void *target, uint32_t idx));
void* iter_pick(iter* it);
void* iter_pick_move(iter *it);
void iter_move(iter* it);
void iter_free(iter *it);

#endif
