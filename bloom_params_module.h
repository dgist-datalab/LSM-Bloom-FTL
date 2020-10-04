#ifndef __BLOOMF_PM_H__
#define __BLOOMF_PM_H__
#include <stdint.h>
double get_target_each_fpr(float block_fpr);
double get_target_each_fpr_w_mem(float block_fpr, uint32_t membernum);
uint32_t get_number_of_bits(float target_fpr);
uint32_t get_number_of_bits_for_block_fpr(float block_fpr);
uint32_t get_best_target_bits(float fpr, uint32_t max_depth);
#endif
