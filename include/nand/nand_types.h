/*
 * nand_types.h - NAND Flash 基础类型定义与常量
 *
 * 这个文件定义了整个模拟器最底层的"规则"：
 * 一个 page 多大、一个 block 有多少 page、最多能擦多少次……
 * 所有其他模块都依赖这些定义。
 */

#ifndef NAND_TYPES_H
#define NAND_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 *  可配置参数
 *  改这里就能模拟不同规格的闪存芯片
 * ============================================================ */

#define PAGE_DATA_SIZE      4096        /* 4KB 数据区（用户数据） */
#define PAGE_SPARE_SIZE     128         /* 128B 备用区（ECC + 元数据） */
#define PAGE_TOTAL_SIZE     (PAGE_DATA_SIZE + PAGE_SPARE_SIZE)

#define PAGES_PER_BLOCK     64          /* 每个 block 包含 64 个 page */
#define BLOCKS_PER_CHIP     1024        /* 每个 chip 包含 1024 个 block */

/* 总容量 = 1024 blocks * 64 pages * 4KB = 256MB */

#define MAX_ERASE_COUNT     100000      /* 每个 block 最多擦写 10 万次 */
#define NAND_ERASED_BYTE    0xFF        /* 闪存擦除后的默认值（全1） */

/* ============================================================
 *  状态枚举
 * ============================================================ */

/* Page 状态 */
typedef enum {
    PAGE_FREE,          /* 空闲：擦除后的初始状态，可以写入 */
    PAGE_VALID,         /* 有效：已写入数据，数据有效 */
    PAGE_INVALID        /* 无效：数据已过期（被新版本覆盖），等待 GC 回收 */
} page_state_t;

/* Block 状态 */
typedef enum {
    BLOCK_GOOD,         /* 正常可用 */
    BLOCK_BAD_FACTORY,  /* 出厂坏块：生产时就标记的，永远不用 */
    BLOCK_BAD_RUNTIME   /* 运行时坏块：擦写次数超限或 ECC 无法纠正 */
} block_state_t;

/* 操作返回码 */
typedef enum {
    NAND_OK = 0,                        /* 操作成功 */
    NAND_ERR_WRITE_TO_PROGRAMMED,       /* 错误：写入已编程的 page */
    NAND_ERR_BAD_BLOCK,                 /* 错误：操作的是坏块 */
    NAND_ERR_ECC_UNCORRECTABLE,         /* 错误：ECC 无法纠正（预留） */
    NAND_ERR_ERASE_LIMIT,              /* 错误：擦写次数已达上限 */
    NAND_ERR_OUT_OF_RANGE,             /* 错误：地址越界 */
    NAND_ERR_WRITE_ORDER               /* 错误：page 写入顺序不对 */
} nand_status_t;

/* ============================================================
 *  核心数据结构
 * ============================================================ */

/* 一个 Page：闪存最小的读写单位 */
typedef struct {
    uint8_t         data[PAGE_DATA_SIZE];       /* 用户数据区 */
    uint8_t         spare[PAGE_SPARE_SIZE];     /* 备用区 */
    page_state_t    state;                      /* 当前状态 */
} nand_page_t;

/* 一个 Block：闪存最小的擦除单位 */
typedef struct {
    nand_page_t     pages[PAGES_PER_BLOCK];    /* 本 block 内的 page 数组 */
    block_state_t   state;                      /* block 状态 */
    uint32_t        erase_count;               /* 累计擦写次数 */
    uint32_t        valid_page_count;          /* 有效 page 计数（给 GC 用） */
    uint32_t        next_free_page;            /* 下一个可写 page 的索引（顺序写用）*/
} nand_block_t;

/* 一个 Chip：一颗完整的闪存芯片 */
typedef struct {
    nand_block_t    blocks[BLOCKS_PER_CHIP];   /* 所有 block */
    uint64_t        total_reads;               /* 全芯片读操作计数 */
    uint64_t        total_writes;              /* 全芯片写操作计数 */
    uint64_t        total_erases;              /* 全芯片擦操作计数 */
} nand_chip_t;

#endif /* NAND_TYPES_H */
