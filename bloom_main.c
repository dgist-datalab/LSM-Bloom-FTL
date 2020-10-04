#include "bloom_params_module.h"
#include <stdio.h>
int main(){
	uint32_t target_member=1;
	uint32_t bits=0;
	double range_bits=48.0f*2;
	for(uint32_t i=1; i<=10; i++){
		target_member*=2;
		bits=get_number_of_bits(get_target_each_fpr_w_mem(0.1, target_member));
		printf("mem:%u bits:%u w/ range_bits:%lf\n", target_member, bits, range_bits/target_member+bits);
	}
	return 0;
}
