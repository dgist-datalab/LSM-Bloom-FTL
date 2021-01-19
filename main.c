#include "lsm_params_module.h"
#include "bloom_params_module.h"
#include "ssd.h"

#define TESTLEVELSTART 4
#define TESTLEVELDEPTH 10
#define BLOOMFPR 0.1f
int main(){
	uint32_t size_factor;
	for(uint32_t j=1; j<=16; j*=2){
		printf("TYPE\tLEVEL\tSF\tWAF\tRAF\tOP\tB-MEM\tMON\tGB-MEM\tGB-MON(BN:%ld, TREE NUM:%u)\n", BLOCKNUMBER/j, j);
		for(uint32_t i=TESTLEVELSTART; i<= TESTLEVELSTART+TESTLEVELDEPTH; i++){
			size_factor=get_size_factor(i, BLOCKNUMBER/j);
			printf("TIER\t%d\t%d\t%d\t%d\t%.2f\t%ubits\t%ubits\t%ubits\t%ubits\n",i,size_factor,get_waf(TIER, i ,size_factor), 
					get_raf(TIER, i, size_factor), 
					(float)(get_sparse_sorted_head(i, size_factor)*j)/BLOCKNUMBER,
					get_number_of_bits_for_block_fpr(BLOOMFPR/get_raf(TIER,i ,size_factor)),
					get_number_of_bits_for_block_fpr(BLOOMFPR),
					get_best_target_bits(BLOOMFPR/get_raf(TIER,i,size_factor),11),
					get_best_target_bits(BLOOMFPR,11)
					);

		}
		printf("\n");
		for(uint32_t i=TESTLEVELSTART; i<=TESTLEVELSTART+TESTLEVELDEPTH; i++){
			size_factor=get_size_factor(i, BLOCKNUMBER/j);
			printf("LEVEL\t%d\t%d\t%d\t%d\t%.2f\t%ubits\t%ubits\t%ubits\t%ubits\n",i,size_factor,get_waf(LEVEL,i , size_factor), 
					get_raf(LEVEL, i, size_factor),
					(float)(get_sparse_sorted_head(i,size_factor)*j)/BLOCKNUMBER,
					get_number_of_bits_for_block_fpr(BLOOMFPR/get_raf(LEVEL,i ,size_factor)),
					get_number_of_bits_for_block_fpr(BLOOMFPR),
					get_best_target_bits(BLOOMFPR/get_raf(LEVEL,i,size_factor),11),
					get_best_target_bits(BLOOMFPR,11)
					);
		}
		printf("=================================================\n");
	}

	return 0;
}
