#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <queue>
#include <sys/time.h>
#include "lsm_simulation.h"
#include "iteration.h"

using namespace std;
extern uint32_t write_cnt;
extern monitor lsm_monitor;
bool debug;

static inline uint32_t get_guard_idx(uint32_t lba, block *b){
	uint32_t s=0, e=b->now/64+1;
	uint32_t *array=b->guard;
	uint32_t mid=0;
	while(s<=e){
		mid=(s+e)/2;

		__builtin_prefetch(&array[(mid+1+e)/2], 0, 1);
		__builtin_prefetch(&array[(s+mid-1)/2], 0, 1);
		__builtin_prefetch(&array[mid+1], 0, 1);

		if(array[mid]<= lba && array[mid+1]>lba) return mid;

		if(array[mid]<lba){
			s=mid+1;
		}
		else if(array[mid]>lba){
			e=mid-1;
		}
	}
	//return array[mid]>lba? mid-1:mid;
}

static inline bool isexist(uint32_t lba, block *b){
	uint32_t i=get_guard_idx(lba, b);
	uint32_t s=i*64, e=(i+1)*64-1;
	e=e>b->now?b->now-1:e;
	uint32_t mid;
	uint32_t *array=b->array;
	while(s<=e){
		mid=(s+e)/2;

		__builtin_prefetch(&array[(mid+1+e)/2], 0, 1);
		__builtin_prefetch(&array[(s+mid-1)/2], 0, 1);

		if(array[mid]<lba){
			s=mid+1;
		}
		else if(array[mid]>lba){
			e=mid-1;
		}
		else{
			return true;	
		}	
	}
	return false;
}


static inline bool isexist_run(uint32_t lba, run *r){
	block *now;
	uint32_t i;
	for_each_target_now(r, i , now){
		if(lba > now->array[now->now-1]) continue;
		else if(lba < now->array[0]) break;
		if(isexist(lba, now)) return true;
	}
	return false;
}

uint64_t getsubtime(struct timeval big, struct timeval small){
	uint64_t big_time=big.tv_sec * 1000000+ big.tv_usec;
	uint64_t small_time=small.tv_sec *1000000+small.tv_usec;
	return big_time-small_time;
}

void lsm_last_gc(LSM *lsm, run *rset,uint32_t run_num, uint32_t target_block){
	level *last_level=&lsm->array[lsm->max-1];
	uint32_t invalid_number=0, valid_number=0;
	uint32_t target_run_max=0;
	queue<uint32_t> *valid_lba_list=new queue<uint32_t>[run_num];
	run *now;

	struct timeval a, b;
	

	static int gc_cnt=0;
	printf("\nlast gc!:%d\n", gc_cnt++);

	gettimeofday(&a,NULL);
	for(uint32_t r_it=0; r_it<run_num-1; r_it++){
		now=&rset[r_it];
		block *b;
		uint32_t b_idx=0; 
		for_each_target_now(now, b_idx, b){
			uint32_t *lba;
			uint32_t l_idx=0;
			for_each_target_now(b, l_idx, lba){
				bool isvalid=true;
				if(r_it==0){
					for(uint32_t t_r_idx=(lsm->last_level_valid?lsm->last_level_valid:1); t_r_idx<run_num; t_r_idx++){
						if(isexist_run(*lba, &rset[t_r_idx])){
							isvalid=false;
							break;
						}
					}
				}
				else{
					for(uint32_t t_r_idx=r_it+1; t_r_idx<run_num; t_r_idx++){
						if(isexist_run(*lba, &rset[t_r_idx])){
							isvalid=false;
							break;
						}
					}
				}

				if(!isvalid){
					invalid_number++;
				}
				else{
					valid_number++;
					valid_lba_list[target_run_max].push(*lba);
				}
			}
		}
		target_run_max++;
		if(invalid_number/LPPB - (valid_number%LPPB?1:0) >= target_block) {
			if(target_run_max==1 && valid_number!=0){
				continue;
			}
			break;
		}
	}
	gettimeofday(&b, NULL);
	printf("filtering time : %lu\n", getsubtime(b, a));

	uint32_t ri,bi;
	run *r_now;
	block *b_now;
	for_each_target_now(&lsm->array[lsm->max-1], ri, r_now){
		if(ri>=target_run_max) break;
		for_each_target_now(&rset[run_num-1], bi, b_now){	
			lsm_monitor.level_read_num[lsm->max-1]+=b_now->now;
		}
	}
	lsm_monitor.last_level_merg_run_num[target_run_max]++;


	gettimeofday(&a, NULL);
	run new_run;
	if(target_run_max!=1){
		run_init(&new_run, valid_number/LPPB + (valid_number%LPPB?1:0));
		bool *empty=(bool*)calloc(target_run_max, sizeof(bool));
		uint32_t t_idx, t_lba;
		uint32_t insert_idx=0;
		while(!check_done(empty, target_run_max)){
			uint32_t temp;
			t_lba=UINT32_MAX;
			for(int i=0; i<target_run_max; i++){
				if(empty[i]) continue;
				temp=valid_lba_list[i].front();
				if(temp==t_lba){
					printf("it can't have same lba:%u %s:%d\n", temp,__FILE__, __LINE__);
					abort();
				}
				if(temp < t_lba){
					t_lba=temp;
					t_idx=i;
				}
			}

			run_insert_value(&new_run, t_lba, insert_idx++, &write_cnt);
			valid_lba_list[t_idx].pop();
			empty[t_idx]=valid_lba_list[t_idx].empty();
		}	


		new_run.now=insert_idx/LPPB+(insert_idx%LPPB?1:0);
		free(empty);
	}
	gettimeofday(&b, NULL);
	printf("merge time : %lu\n", getsubtime(b, a));

	uint32_t i;
	level new_last_level;
	level_init(&new_last_level, lsm->max-1, lsm);

	if(target_run_max!=1){
		level_insert_run(&new_last_level, new_run);
	}
	for(uint32_t i=target_run_max; i<run_num; i++){
		level_insert_run(&new_last_level, rset[i]);
	}

	for_each_target_max(&lsm->array[lsm->max-2], i, now){
		run_free(now);
	}
	free(lsm->array[lsm->max-2].array);
	level_init(&lsm->array[lsm->max-2], lsm->max-2, lsm);
	
	for_each_target_max(&lsm->array[lsm->max-1], i, now){
		if(i>=target_run_max && lsm->array[lsm->max-1].now>i) continue;
		run_free(now);
	}
	free(lsm->array[lsm->max-1].array);
	lsm->array[lsm->max-1]=new_last_level;
	lsm->last_level_valid=lsm->array[lsm->max-1].now;
	delete [] valid_lba_list;
}
