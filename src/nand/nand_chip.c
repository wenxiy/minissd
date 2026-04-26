/*
 * nand_chip.c - NAND Flash 芯片操作实现
 *
 * 这是整个模拟器最底层的代码。
 * 每个函数都在模拟真实闪存芯片的物理行为和约束。
 */

#include "nand/nand_chip.h"
#include "nand/nand_utils.h"
#include <string.h>
#include <stdio.h>

/* ==================================================================
 *  芯片初始化
 * ================================================================== */

nand_status_t nand_chip_init(nand_chip_t *chip, float factory_bad_rate)
{
    /* 清零整个芯片结构体 */
    memset(chip, 0, sizeof(nand_chip_t));

    for (uint32_t b = 0; b < BLOCKS_PER_CHIP; b++) {
        nand_block_t *block = &chip->blocks[b];

        block->state            = BLOCK_GOOD;
        block->erase_count      = 0;
        block->valid_page_count = 0;
        block->next_free_page   = 0;

        /* 初始化每个 page */
        for (uint32_t p = 0; p < PAGES_PER_BLOCK; p++) {
            nand_page_t *page = &block->pages[p];

            /* 闪存擦除后数据全是 0xFF */
            memset(page->data,  NAND_ERASED_BYTE, PAGE_DATA_SIZE);
            memset(page->spare, NAND_ERASED_BYTE, PAGE_SPARE_SIZE);
            page->state = PAGE_FREE;
        }

        /*
         * 按概率标记出厂坏块
         * 真实闪存芯片出厂时就有少量坏块（制造瑕疵），
         * SSD 固件在初始化时会扫描并记录这些坏块，永远不使用它们。
         */
        if (nand_rand_float() < factory_bad_rate) {
            block->state = BLOCK_BAD_FACTORY;
        }
    }

    return NAND_OK;
}

/* ==================================================================
 *  Page 读取
 * ================================================================== */

nand_status_t nand_page_read(nand_chip_t *chip,
                              uint32_t block_idx,
                              uint32_t page_idx,
                              uint8_t *buf)
{
    /* 1. 地址合法性检查 */
    if (!nand_is_valid_address(block_idx, page_idx)) {
        return NAND_ERR_OUT_OF_RANGE;
    }

    /* 2. 坏块检查 */
    nand_block_t *block = &chip->blocks[block_idx];
    if (block->state != BLOCK_GOOD) {
        return NAND_ERR_BAD_BLOCK;
    }

    /* 3. 读取数据到缓冲区 */
    nand_page_t *page = &block->pages[page_idx];
    memcpy(buf, page->data, PAGE_DATA_SIZE);

    /*
     * 注意：不管 page 状态是 FREE / VALID / INVALID 都能读
     * - FREE：返回 0xFF（初始化时就设好了）
     * - VALID：返回有效数据
     * - INVALID：返回旧数据（物理上数据还在，只是逻辑上无效了）
     */

    chip->total_reads++;
    return NAND_OK;
}

/* ==================================================================
 *  Page 写入（编程）
 * ================================================================== */

nand_status_t nand_page_write(nand_chip_t *chip,
                               uint32_t block_idx,
                               uint32_t page_idx,
                               const uint8_t *data)
{
    /* 1. 地址合法性检查 */
    if (!nand_is_valid_address(block_idx, page_idx)) {
        return NAND_ERR_OUT_OF_RANGE;
    }

    /* 2. 坏块检查 */
    nand_block_t *block = &chip->blocks[block_idx];
    if (block->state != BLOCK_GOOD) {
        return NAND_ERR_BAD_BLOCK;
    }

    /* 3. 目标 page 必须是 FREE 状态 */
    nand_page_t *page = &block->pages[page_idx];
    if (page->state != PAGE_FREE) {
        return NAND_ERR_WRITE_TO_PROGRAMMED;
    }

    /*
     * 4. 顺序写检查
     *
     * 真实 NAND Flash 要求同一 block 内 page 按序号从小到大写入。
     * 这是物理结构决定的：word line 共享电压，乱序写会干扰已编程的 page。
     *
     * 我们用 next_free_page 来追踪：
     * 如果你要写 page 5，那 page 0-4 要么已经写过，要么就跳过不写了。
     */
    if (page_idx < block->next_free_page) {
        return NAND_ERR_WRITE_ORDER;
    }

    /* 5. 执行写入 */
    memcpy(page->data, data, PAGE_DATA_SIZE);
    page->state = PAGE_VALID;

    block->valid_page_count++;
    block->next_free_page = page_idx + 1;

    chip->total_writes++;
    return NAND_OK;
}

/* ==================================================================
 *  Block 擦除
 * ================================================================== */

nand_status_t nand_block_erase(nand_chip_t *chip, uint32_t block_idx)
{
    /* 1. 地址合法性检查 */
    if (!nand_is_valid_block(block_idx)) {
        return NAND_ERR_OUT_OF_RANGE;
    }

    /* 2. 坏块检查 */
    nand_block_t *block = &chip->blocks[block_idx];
    if (block->state != BLOCK_GOOD) {
        return NAND_ERR_BAD_BLOCK;
    }

    /* 3. 擦除所有 page */
    for (uint32_t p = 0; p < PAGES_PER_BLOCK; p++) {
        nand_page_t *page = &block->pages[p];
        memset(page->data,  NAND_ERASED_BYTE, PAGE_DATA_SIZE);
        memset(page->spare, NAND_ERASED_BYTE, PAGE_SPARE_SIZE);
        page->state = PAGE_FREE;
    }

    /* 4. 更新 block 元数据 */
    block->erase_count++;
    block->valid_page_count = 0;
    block->next_free_page   = 0;

    /*
     * 5. 检查擦写寿命
     *
     * 真实闪存每擦写一次，氧化层就稍微退化一点。
     * 超过上限后读写就不可靠了，必须标记为坏块。
     */
    if (block->erase_count >= MAX_ERASE_COUNT) {
        block->state = BLOCK_BAD_RUNTIME;
        return NAND_ERR_ERASE_LIMIT;
    }

    chip->total_erases++;
    return NAND_OK;
}

/* ==================================================================
 *  Page 无效化
 * ================================================================== */

nand_status_t nand_page_invalidate(nand_chip_t *chip,
                                    uint32_t block_idx,
                                    uint32_t page_idx)
{
    /* 1. 地址合法性检查 */
    if (!nand_is_valid_address(block_idx, page_idx)) {
        return NAND_ERR_OUT_OF_RANGE;
    }

    /* 2. 坏块检查 */
    nand_block_t *block = &chip->blocks[block_idx];
    if (block->state != BLOCK_GOOD) {
        return NAND_ERR_BAD_BLOCK;
    }

    /* 3. 只有 VALID 的 page 才能标记为 INVALID */
    nand_page_t *page = &block->pages[page_idx];
    if (page->state == PAGE_VALID) {
        page->state = PAGE_INVALID;
        block->valid_page_count--;
    }
    /* FREE 或已经 INVALID 的 page 不做操作，也不报错 */

    return NAND_OK;
}

/* ==================================================================
 *  查询接口
 * ================================================================== */

block_state_t nand_block_get_state(const nand_chip_t *chip, uint32_t block_idx)
{
    if (!nand_is_valid_block(block_idx)) {
        return BLOCK_BAD_RUNTIME;  /* 越界当坏块处理 */
    }
    return chip->blocks[block_idx].state;
}

uint32_t nand_block_get_erase_count(const nand_chip_t *chip, uint32_t block_idx)
{
    if (!nand_is_valid_block(block_idx)) return 0;
    return chip->blocks[block_idx].erase_count;
}

uint32_t nand_block_get_valid_pages(const nand_chip_t *chip, uint32_t block_idx)
{
    if (!nand_is_valid_block(block_idx)) return 0;
    return chip->blocks[block_idx].valid_page_count;
}

uint32_t nand_block_get_free_pages(const nand_chip_t *chip, uint32_t block_idx)
{
    if (!nand_is_valid_block(block_idx)) return 0;
    const nand_block_t *block = &chip->blocks[block_idx];
    if (block->state != BLOCK_GOOD) return 0;

    uint32_t free_count = 0;
    for (uint32_t p = 0; p < PAGES_PER_BLOCK; p++) {
        if (block->pages[p].state == PAGE_FREE) {
            free_count++;
        }
    }
    return free_count;
}

uint32_t nand_chip_get_bad_block_count(const nand_chip_t *chip)
{
    uint32_t count = 0;
    for (uint32_t b = 0; b < BLOCKS_PER_CHIP; b++) {
        if (chip->blocks[b].state != BLOCK_GOOD) {
            count++;
        }
    }
    return count;
}

uint32_t nand_chip_get_free_block_count(const nand_chip_t *chip)
{
    uint32_t count = 0;
    for (uint32_t b = 0; b < BLOCKS_PER_CHIP; b++) {
        const nand_block_t *block = &chip->blocks[b];
        if (block->state == BLOCK_GOOD && block->next_free_page == 0) {
            count++;
        }
    }
    return count;
}

uint32_t nand_chip_get_total_valid_pages(const nand_chip_t *chip)
{
    uint32_t count = 0;
    for (uint32_t b = 0; b < BLOCKS_PER_CHIP; b++) {
        count += chip->blocks[b].valid_page_count;
    }
    return count;
}

/* ==================================================================
 *  调试信息打印
 * ================================================================== */

void nand_chip_dump_info(const nand_chip_t *chip)
{
    uint32_t bad_count  = nand_chip_get_bad_block_count(chip);
    uint32_t free_count = nand_chip_get_free_block_count(chip);

    printf("========== NAND Chip Info ==========\n");
    printf("  Blocks:       %u\n", BLOCKS_PER_CHIP);
    printf("  Pages/Block:  %u\n", PAGES_PER_BLOCK);
    printf("  Page Size:    %u bytes\n", PAGE_DATA_SIZE);
    printf("  Total Size:   %u MB\n",
           (BLOCKS_PER_CHIP * PAGES_PER_BLOCK * PAGE_DATA_SIZE) / (1024 * 1024));
    printf("  Bad Blocks:   %u (%.1f%%)\n",
           bad_count, (float)bad_count / BLOCKS_PER_CHIP * 100);
    printf("  Free Blocks:  %u\n", free_count);
    printf("  Total Reads:  %llu\n", (unsigned long long)chip->total_reads);
    printf("  Total Writes: %llu\n", (unsigned long long)chip->total_writes);
    printf("  Total Erases: %llu\n", (unsigned long long)chip->total_erases);
    printf("====================================\n");
}
