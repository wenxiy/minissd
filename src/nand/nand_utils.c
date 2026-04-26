/*
 * nand_utils.c - NAND 底层辅助函数实现
 */

#include "nand/nand_utils.h"

/* ============================================================
 *  地址合法性检查
 * ============================================================ */

bool nand_is_valid_block(uint32_t block_idx)
{
    return block_idx < BLOCKS_PER_CHIP;
}

bool nand_is_valid_page(uint32_t page_idx)
{
    return page_idx < PAGES_PER_BLOCK;
}

bool nand_is_valid_address(uint32_t block_idx, uint32_t page_idx)
{
    return nand_is_valid_block(block_idx) && nand_is_valid_page(page_idx);
}

/* ============================================================
 *  状态码转字符串
 * ============================================================ */

const char* nand_status_to_str(nand_status_t status)
{
    switch (status) {
        case NAND_OK:                       return "OK";
        case NAND_ERR_WRITE_TO_PROGRAMMED:  return "ERR: write to programmed page";
        case NAND_ERR_BAD_BLOCK:            return "ERR: bad block";
        case NAND_ERR_ECC_UNCORRECTABLE:    return "ERR: ECC uncorrectable";
        case NAND_ERR_ERASE_LIMIT:          return "ERR: erase limit exceeded";
        case NAND_ERR_OUT_OF_RANGE:         return "ERR: address out of range";
        case NAND_ERR_WRITE_ORDER:          return "ERR: page write order violation";
        default:                            return "ERR: unknown";
    }
}

const char* nand_page_state_to_str(page_state_t state)
{
    switch (state) {
        case PAGE_FREE:     return "FREE";
        case PAGE_VALID:    return "VALID";
        case PAGE_INVALID:  return "INVALID";
        default:            return "UNKNOWN";
    }
}

const char* nand_block_state_to_str(block_state_t state)
{
    switch (state) {
        case BLOCK_GOOD:        return "GOOD";
        case BLOCK_BAD_FACTORY: return "BAD(factory)";
        case BLOCK_BAD_RUNTIME: return "BAD(runtime)";
        default:                return "UNKNOWN";
    }
}

/* ============================================================
 *  简易随机数生成器（LCG 算法）
 *
 *  不用 rand() 是为了：
 *  1. 可控的种子 → 可复现的测试结果
 *  2. 不依赖平台的 stdlib 实现
 * ============================================================ */

static uint32_t s_rand_state = 12345;

void nand_rand_seed(uint32_t seed)
{
    s_rand_state = seed;
}

uint32_t nand_rand(void)
{
    /* LCG: Numerical Recipes 推荐的参数 */
    s_rand_state = s_rand_state * 1664525u + 1013904223u;
    return s_rand_state;
}

float nand_rand_float(void)
{
    return (float)nand_rand() / (float)UINT32_MAX;
}
