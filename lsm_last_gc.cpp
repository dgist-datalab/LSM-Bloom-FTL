#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <queue>
#include <sys/time.h>
#include "lsm_simulation.h"
#include "iteration.h"

#define FASTFINDING

using namespace std;
extern uint64_t write_cnt;
extern monitor lsm_monitor;
extern LSM *tlsm;
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

int __lsm_last_gc(LSM *lsm,uint32_t level_id,  uint32_t target_block){
	if(level_id==0 || lsm->array[level_id].now==0){
		printf("device is full!!!\n");
		abort();
	}
	uint64_t invalid_number=0, valid_number=0;
	uint32_t target_run_max=0;

	run *rset;
	uint32_t run_num=0;

	struct timeval a, b;
	run *now;
	static int gc_cnt=0;
	//printf("\nlast gc!:%d\n", gc_cnt++);
	uint32_t tr_it, r_it;
	uint32_t before_block_num=tlsm->valid_block_num;

	uint32_t retry_cnt=0;
retry:
	//printf("retry_cnt:%u\n", retry_cnt++);
	//lsm_print_summary(NULL);

	if(retry_cnt==lsm->array[level_id].now){
	//	printf("retry upper level!!:%u->%u\n", level_id, level_id-1);
		return __lsm_last_gc(lsm, level_id-1, target_block-tlsm->valid_block_num);
	}

	uint32_t id=UINT32_MAX;
	target_run_max=0;
	run_num=lsm->array[level_id].now;
	rset=(run*)malloc(sizeof(run) * lsm->array[level_id].now);
	uint32_t i;
	queue<uint32_t> *valid_lba_list=new queue<uint32_t>[run_num];
	for(i=0; i<run_num; i++){
		rset[i]=lsm->array[level_id].array[i];	
	}

	for(i=0; i<run_num; i++){
		now=&rset[i];
		if(now->id<id){
			id=now->id;
			tr_it=i;
		}
	}
	//printf("\t\ttr_it:%u\n",tr_it);
	gettimeofday(&a,NULL);
	
	r_it=tr_it;
	{
		now=&rset[r_it];
		block *b;
		uint32_t b_idx=0; 
		for_each_target_now(now, b_idx, b){
			uint32_t *lba;
			uint32_t l_idx=0;
			for_each_target_now(b, l_idx, lba){
				bool isvalid=true;
#ifdef FASTFINDING
				isvalid=(lsm->lba_run_id[*lba]==now->id);
#else
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

#endif
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
	}
	gettimeofday(&b, NULL);
//	printf("\nfiltering time : %lu\n", getsubtime(b, a));

	uint32_t ri,bi;
	run *r_now;
	block *b_now;
	for_each_target_now(&lsm->array[level_id], ri, r_now){
		//if(ri>=target_run_max) break;
		if(ri<tr_it) continue;
		if(ri>=tr_it+target_run_max)continue;
		for_each_target_now(&rset[run_num-1], bi, b_now){	
			lsm_monitor.level_read_num[level_id]+=b_now->now;
		}
	}
	lsm_monitor.last_level_merg_run_num[target_run_max]++;

	for_each_target_max(&lsm->array[level_id], ri, now){
		if(ri<tr_it) continue;
		if(ri>=tr_it+target_run_max)continue;
		tlsm->valid_block_num+=now->now;
		for_each_target_now(now, bi, b_now){
			b_now->used=false;
		}
		run_free(now);
	}

	gettimeofday(&a, NULL);
	run new_run;
	run_init(&new_run, valid_number/LPPB + (valid_number%LPPB?1:0));
	bool *empty=(bool*)calloc(target_run_max, sizeof(bool));
	uint32_t t_idx, t_lba;
	uint32_t insert_idx=0;

	volatile uint64_t before_write=write_cnt;
	while(!check_done(empty, target_run_max)){
		uint32_t temp;
		t_lba=UINT32_MAX;
		for(i=0; i<target_run_max; i++){
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

		run_insert_value(&new_run, t_lba, insert_idx++, &write_cnt, lsm->lba_run_id);
		valid_lba_list[t_idx].pop();
		empty[t_idx]=valid_lba_list[t_idx].empty();
	}	
	new_run.now=insert_idx/LPPB+(insert_idx%LPPB?1:0);
	free(empty);

	lsm_monitor.level_write_num[lsm->max]+=write_cnt-before_write;
	tlsm->gc_cnt+=write_cnt-before_write;
	gettimeofday(&b, NULL);
//	printf("merge time : %lu\n", getsubtime(b, a));


	level new_last_level;
	level_init(&new_last_level, level_id, lsm);

	for(i=0; i<tr_it; i++){
		level_insert_run(&new_last_level, rset[i]);
	}

	level_insert_run(&new_last_level, new_run);
	
	for(i=target_run_max+tr_it; i<run_num; i++){
		level_insert_run(&new_last_level, rset[i]);
	}

	free(lsm->array[level_id].array);
	lsm->array[level_id]=new_last_level;
	lsm->last_level_valid=lsm->array[level_id].now;

	free(rset);
	delete [] valid_lba_list;
	if(!(tlsm->valid_block_num-before_block_num >= target_block)) {
		goto retry;
	}


	//printf("iv/total ratio: %.2lf %lu, %lu\n", (double)invalid_number/(invalid_number+valid_number), invalid_number, valid_number);
	//printf("target_run :%u ,now: %u\n", target_run_max, lsm->array[level_id].now);
	lsm_monitor.last_run_valid+=valid_number;
	lsm_monitor.last_run_invalid+=invalid_number;
	return 1;
}

void __lsm_last_merge(LSM *lsm, run *rset, uint32_t run_num){
	//lsm_print_summary(&rset[run_num-1]);

	level *last_level=&lsm->array[lsm->max-1];
	uint64_t invalid_number=0, valid_number=0;
	uint32_t target_run_max=0;
	queue<uint32_t> *valid_lba_list=new queue<uint32_t>[2];
	run *now;
	run *target_runs[2];
	struct timeval a, b;
	static int gc_cnt=0;
	//printf("\nlast merge!:%d\n", gc_cnt++);

	uint32_t tr_it;
	uint32_t id=UINT32_MAX;
	for(uint32_t i=0; i<run_num-1; i++){
		now=&rset[i];
		if(now->id<id){
			id=now->id;
			tr_it=i;
			target_runs[0]=now;
		}
	}

	uint32_t tr_it2; id=UINT32_MAX;
	for(uint32_t i=0; i<run_num-1; i++){
		now=&rset[i];
		if(i==tr_it) continue;
		if(now->id<id){
			id=now->id;
			tr_it2=i;
			target_runs[1]=now;
		}
	}

	//printf("tr_it:%u, %u\n",tr_it, tr_it2);

	gettimeofday(&a,NULL);
	for(uint32_t r_it=0; r_it<2; r_it++){
		now=target_runs[r_it];
		block *b;
		uint32_t b_idx=0; 
		for_each_target_now(now, b_idx, b){
			uint32_t *lba;
			uint32_t l_idx=0;
			for_each_target_now(b, l_idx, lba){
				bool isvalid=true;
#ifdef FASTFINDING
				isvalid=(lsm->lba_run_id[*lba]==now->id);
#else
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

#endif
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
	}
	gettimeofday(&b, NULL);
//	printf("\nfiltering time : %lu\n", getsubtime(b, a));

	uint32_t ri,bi;
	run *r_now;
	block *b_now;
	for_each_target_now(&lsm->array[lsm->max-1], ri, r_now){
		if(!((r_now)->id==target_runs[0]->id || (r_now)->id==target_runs[1]->id))continue;
		for_each_target_now(&rset[run_num-1], bi, b_now){	
			lsm_monitor.level_read_num[lsm->max-1]+=b_now->now;
		}
	}
	lsm_monitor.last_level_merg_run_num[target_run_max]++;

	for_each_target_max(&lsm->array[lsm->max-1], ri, now){
		if(!(now->id==target_runs[0]->id || now->id==target_runs[1]->id)){
			continue;
		}
		tlsm->valid_block_num+=now->now;
		for_each_target_now(now, bi, b_now){
			b_now->used=false;
		}
		run_free(now);
	}

	gettimeofday(&a, NULL);
	run new_run;
	run_init(&new_run, valid_number/LPPB + (valid_number%LPPB?1:0));
	bool *empty=(bool*)calloc(target_run_max, sizeof(bool));
	uint32_t t_idx, t_lba;
	uint32_t insert_idx=0;

	volatile uint64_t before_write=write_cnt;
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

		run_insert_value(&new_run, t_lba, insert_idx++, &write_cnt, lsm->lba_run_id);
		valid_lba_list[t_idx].pop();
		empty[t_idx]=valid_lba_list[t_idx].empty();
	}	
	new_run.now=insert_idx/LPPB+(insert_idx%LPPB?1:0);
	free(empty);

	lsm_monitor.level_write_num[lsm->max-1]+=write_cnt-before_write;
	gettimeofday(&b, NULL);
	//printf("merge time : %lu\n", getsubtime(b, a));


	level new_last_level;
	level_init(&new_last_level, lsm->max-1, lsm);

	for(uint32_t i=0; i<run_num; i++){
		now=&rset[i];
		if((now->id==target_runs[0]->id || now->id==target_runs[1]->id))continue;
		level_insert_run(&new_last_level, rset[i]);
	}

	level_insert_run(&new_last_level, new_run);

	free(lsm->array[lsm->max-1].array);
	lsm->array[lsm->max-1]=new_last_level;
	lsm->last_level_valid=lsm->array[lsm->max-1].now;

	//printf("iv/total ratio: %.2lf %lu, %lu\n", (double)invalid_number/(invalid_number+valid_number), invalid_number, valid_number);
	//printf("\ttarget_run :%u ,now: %u\n", target_run_max, lsm->array[lsm->max-1].now);
	lsm_monitor.last_run_valid+=valid_number;
	lsm_monitor.last_run_invalid+=invalid_number;

	delete [] valid_lba_list;
}


void lsm_last_gc(LSM *lsm, run *rset,uint32_t run_num, uint32_t target_block, bool needmerge){
	if(needmerge){
		__lsm_last_merge(lsm, rset, run_num);
		//lsm_print_summary(NULL);
	}
	else{
		__lsm_last_gc(lsm, lsm->max-1, target_block);
		//lsm_print_summary(NULL);
	}
}
