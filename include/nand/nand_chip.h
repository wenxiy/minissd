/*
 * nand_chip.h - NAND Flash 芯片操作接口
 *
 * 这一层模拟的是"硬件行为"：
 * 你对芯片能做的事只有三件 —— 读 page、写 page、擦 block。
 * 每个操作都带真实闪存的物理约束。
 *
 * C++ 代码通过 extern "C" 调用这些函数。
 */

#ifndef NAND_CHIP_H
#define NAND_CHIP_H

#include "nand_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================================================================
 *  芯片生命周期
 * ================================================================== */

/*
 * 初始化一颗芯片
 *
 * - 所有 page 数据设为 0xFF（闪存擦除后的默认值）
 * - 所有 page 状态设为 FREE
 * - 所有 block 擦写次数归零
 * - 按 factory_bad_rate 比例随机标记出厂坏块
 *
 * @param chip              指向芯片结构体
 * @param factory_bad_rate  出厂坏块率，如 0.02 表示 2%
 * @return                  NAND_OK
 */
nand_status_t nand_chip_init(nand_chip_t *chip, float factory_bad_rate);

/* ==================================================================
 *  三大核心操作
 * ================================================================== */

/*
 * 读取一个 page 的数据
 *
 * @param chip       芯片
 * @param block_idx  block 编号 [0, BLOCKS_PER_CHIP)
 * @param page_idx   page 编号 [0, PAGES_PER_BLOCK)
 * @param buf        输出缓冲区，至少 PAGE_DATA_SIZE 字节
 * @return           NAND_OK / NAND_ERR_BAD_BLOCK / NAND_ERR_OUT_OF_RANGE
 *
 * 行为：
 *   - 如果 page 是 FREE，buf 会被填满 0xFF（真实闪存就是这样）
 *   - 如果 page 是 VALID 或 INVALID，都能读（INVALID 只是逻辑标记）
 *   - 读操作不改变任何状态，但 total_reads 计数 +1
 */
nand_status_t nand_page_read(nand_chip_t *chip,
                              uint32_t block_idx,
                              uint32_t page_idx,
                              uint8_t *buf);

/*
 * 写入（编程）一个 page
 *
 * @param chip       芯片
 * @param block_idx  block 编号
 * @param page_idx   page 编号
 * @param data       要写入的数据，PAGE_DATA_SIZE 字节
 * @return           NAND_OK / NAND_ERR_WRITE_TO_PROGRAMMED /
 *                   NAND_ERR_BAD_BLOCK / NAND_ERR_OUT_OF_RANGE /
 *                   NAND_ERR_WRITE_ORDER
 *
 * 约束（模拟真实闪存规则）：
 *   1. 目标 page 必须是 FREE 状态（闪存不能覆盖写！）
 *   2. 同一 block 内必须按 page_idx 从小到大顺序写
 *      （真实 NAND 的物理限制：lower page 要先写）
 *   3. block 不能是坏块
 */
nand_status_t nand_page_write(nand_chip_t *chip,
                               uint32_t block_idx,
                               uint32_t page_idx,
                               const uint8_t *data);

/*
 * 擦除一整个 block
 *
 * @param chip       芯片
 * @param block_idx  block 编号
 * @return           NAND_OK / NAND_ERR_BAD_BLOCK /
 *                   NAND_ERR_ERASE_LIMIT / NAND_ERR_OUT_OF_RANGE
 *
 * 行为：
 *   - block 内所有 page：数据清为 0xFF，状态恢复为 FREE
 *   - erase_count += 1
 *   - valid_page_count 和 next_free_page 重置
 *   - 如果 erase_count 超过 MAX_ERASE_COUNT → 标记为 BLOCK_BAD_RUNTIME
 *   - total_erases 计数 +1
 */
nand_status_t nand_block_erase(nand_chip_t *chip, uint32_t block_idx);

/* ==================================================================
 *  辅助操作
 * ================================================================== */

/*
 * 将某个 page 标记为 INVALID（数据过期）
 *
 * FTL 做覆盖写时调用：旧数据不物理删除，只是标记无效，
 * 等 GC 来回收这个 block 时才真正擦除。
 */
nand_status_t nand_page_invalidate(nand_chip_t *chip,
                                    uint32_t block_idx,
                                    uint32_t page_idx);

/* ==================================================================
 *  查询接口
 * ================================================================== */

/* 查询单个 block */
block_state_t nand_block_get_state(const nand_chip_t *chip, uint32_t block_idx);
uint32_t      nand_block_get_erase_count(const nand_chip_t *chip, uint32_t block_idx);
uint32_t      nand_block_get_valid_pages(const nand_chip_t *chip, uint32_t block_idx);
uint32_t      nand_block_get_free_pages(const nand_chip_t *chip, uint32_t block_idx);

/* 全芯片统计 */
uint32_t nand_chip_get_bad_block_count(const nand_chip_t *chip);
uint32_t nand_chip_get_free_block_count(const nand_chip_t *chip);
uint32_t nand_chip_get_total_valid_pages(const nand_chip_t *chip);

/* 打印芯片摘要信息（调试用） */
void nand_chip_dump_info(const nand_chip_t *chip);

#ifdef __cplusplus
}
#endif

#endif /* NAND_CHIP_H */
