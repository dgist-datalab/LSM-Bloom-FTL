#include <cmath>
#include <stdio.h>
#include "ssd.h"
#include "bloom_params_module.h"
#define ABS(a) (a<0? -a:a)
#define EPSILON 0.00000000001f
using namespace std;
static inline double func(double x, uint32_t member, float target_fpr){
	return pow(x, member)+member*(target_fpr-1)*x-(target_fpr*member-member+1);
}

static inline double deriv_func(double x, uint32_t member, float target_fpr){
	return pow(x, member-1)+member*(target_fpr-1);
}

static inline double newton(double init_value, uint32_t member, float target_fpr){
	double h, res;
	do{
		h=func(res, member, target_fpr) / deriv_func(res, member, target_fpr); 
//		printf("h: %lf res: %lf\n", h, res);
		if(ABS(h) >= EPSILON){
			res-=h;
		}
		else break;
	}while(1);

	return 1-res;
}

double get_target_each_fpr(float block_fpr){
	return newton(0.5, PAGEPERBLOCK, block_fpr);
}

double get_target_each_fpr_w_mem(float block_fpr,uint32_t membernum){
	return newton(0.5, membernum, block_fpr);
}

uint32_t get_number_of_bits(float target_fpr){
	float t=1/target_fpr;
	uint32_t tt=ceil(log2(t));
	return pow(2,tt) < t ? tt+1: tt;
}

uint32_t get_number_of_bits_for_block_fpr(float block_fpr){
	return get_number_of_bits(get_target_each_fpr(block_fpr));
}

uint32_t get_best_target_bits(float fpr, uint32_t max_depth){
	uint32_t target_member=1;
	uint32_t bits=0;
	double range_bits=48.0f*2;
	uint32_t min=UINT32_MAX;
	for(uint32_t i=1; i<=max_depth; i++){
		target_member*=2;
		bits=get_number_of_bits(get_target_each_fpr_w_mem(fpr, target_member));
		if(min>range_bits/target_member+bits){
			min=range_bits/target_member+bits;
		}
	}
	return min;
}
