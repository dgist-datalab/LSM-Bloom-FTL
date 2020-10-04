#include "lsm_params_module.h"
#include "ssd.h"
#include <cmath>
static inline int checking(uint32_t level, uint32_t size_factor, uint32_t blocknumber){
	uint32_t head_num=0;
	uint32_t level_head_num=1;
	for(uint32_t i=0; i<=level; i++){
		head_num+=level_head_num;
		level_head_num*=size_factor;
	}
	if(head_num<blocknumber){
		return -1;
	}
	else if(head_num > blocknumber){
		return 1;
	}
	else return 0;
}

uint32_t get_size_factor(uint32_t level,  uint32_t blocknumber){
	uint32_t target=(uint32_t)(std::ceil((std::pow(blocknumber, (double)1.0/level))));
	int result;
	int retry_cnt=0;
retry:
	switch((result=checking(level, target, blocknumber))){
		case 1:
		case 0:
			return target;
		case -1:
			target++;
			retry_cnt++;
			if(retry_cnt>2) return UINT32_MAX;
			goto retry;
	}
	return UINT32_MAX;
}

uint32_t get_level(uint32_t sizefactor, uint32_t blocknumber){
	uint32_t target=(uint32_t)(std::ceil(std::log(blocknumber)/std::log((double)sizefactor)));
	int result;
	int retry_cnt=0;
retry:
	switch((result=checking(target, sizefactor, blocknumber))){
		case 1:
		case 0:
			return target;
		case -1:
			target++;
			retry_cnt++;
			if(retry_cnt>2) return UINT32_MAX;
			goto retry;
	}
	return UINT32_MAX;
}

uint32_t get_waf(char t, uint32_t level, uint32_t size_factor){
	switch (t){
		case TIER:
			return level;
		case LEVEL:
			return level*size_factor;
		case HYBRID:
			return level-1+size_factor;
	}
	return UINT32_MAX;
}

uint32_t get_raf(char t, uint32_t level, uint32_t size_factor){
	switch (t){
		case TIER:
			return 	level*size_factor;
		case LEVEL:
			return level;
		case HYBRID:
			return (level-1)*size_factor+1;
	}
	return UINT32_MAX;
}

uint32_t get_sparse_sorted_head(uint32_t level, uint32_t size_factor){
	uint32_t head_num=0;
	uint32_t level_head_num=1;
	for(uint32_t i=0; i<=level-1; i++){
		head_num+=level_head_num;
		level_head_num*=size_factor;
	}
	return head_num;
}
