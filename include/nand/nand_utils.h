/*
 * nand_utils.h - NAND 底层辅助函数
 *
 * 一些通用的工具函数，不直接操作芯片，
 * 但被 nand_chip.c 和其他模块用到。
 */

#ifndef NAND_UTILS_H
#define NAND_UTILS_H

#include "nand_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 地址合法性检查 */
bool nand_is_valid_block(uint32_t block_idx);
bool nand_is_valid_page(uint32_t page_idx);
bool nand_is_valid_address(uint32_t block_idx, uint32_t page_idx);

/* 返回状态码对应的字符串描述（调试/日志用） */
const char* nand_status_to_str(nand_status_t status);
const char* nand_page_state_to_str(page_state_t state);
const char* nand_block_state_to_str(block_state_t state);

/* 简易随机数（用于出厂坏块模拟，不依赖外部库） */
void nand_rand_seed(uint32_t seed);
uint32_t nand_rand(void);
float nand_rand_float(void);   /* 返回 [0.0, 1.0) */

#ifdef __cplusplus
}
#endif

#endif /* NAND_UTILS_H */
