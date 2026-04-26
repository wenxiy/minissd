/*
 * main.cpp - MiniSSD 入口
 *
 * Week 1 阶段：先做一个简单的手动测试，
 * 验证 NAND 底层的读/写/擦是否正确。
 * 后续 Week 3 会替换成完整的 CLI。
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "nand/nand_chip.h"
#include "nand/nand_utils.h"
}

/* 辅助：打印一段数据的前 N 字节（十六进制） */
static void print_hex(const uint8_t *data, int len)
{
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (len % 16 != 0) printf("\n");
}

/* 辅助：创建测试数据（用一个字节值填满整个 page） */
static void fill_page_data(uint8_t *buf, uint8_t fill_value)
{
    memset(buf, fill_value, PAGE_DATA_SIZE);
}

int main(void)
{
    printf("=== MiniSSD - NAND Layer Test ===\n\n");

    /* ---- 1. 初始化芯片 ---- */
    /*
     * 注意：nand_chip_t 非常大（~277MB），不能放栈上，必须用 malloc 分配到堆。
     * 真实 SSD 固件也是动态分配这块内存的。
     */
    nand_chip_t *chip = (nand_chip_t *)malloc(sizeof(nand_chip_t));
    if (!chip) {
        printf("[ERROR] Failed to allocate memory for chip (%zu bytes)\n",
               sizeof(nand_chip_t));
        return 1;
    }

    nand_rand_seed(42);   /* 固定种子，结果可复现 */
    nand_status_t status = nand_chip_init(chip, 0.02f);  /* 2% 出厂坏块 */
    printf("[INIT] Chip initialized: %s\n", nand_status_to_str(status));
    nand_chip_dump_info(chip);
    printf("\n");

    /* ---- 2. 找一个好的 block 来测试 ---- */
    uint32_t test_block = UINT32_MAX;
    for (uint32_t b = 0; b < BLOCKS_PER_CHIP; b++) {
        if (nand_block_get_state(chip, b) == BLOCK_GOOD) {
            test_block = b;
            break;
        }
    }
    if (test_block == UINT32_MAX) {
        printf("[ERROR] No good block found! Aborting.\n");
        free(chip);
        return 1;
    }
    printf("[TEST] Using block %u for testing\n\n", test_block);

    /* ---- 3. 写入测试 ---- */
    uint8_t write_buf[PAGE_DATA_SIZE];
    uint8_t read_buf[PAGE_DATA_SIZE];

    /* 写 page 0: 全填 0xAA */
    fill_page_data(write_buf, 0xAA);
    status = nand_page_write(chip, test_block, 0, write_buf);
    printf("[WRITE] Block %u, Page 0 (data=0xAA): %s\n",
           test_block, nand_status_to_str(status));

    /* 写 page 1: 全填 0xBB */
    fill_page_data(write_buf, 0xBB);
    status = nand_page_write(chip, test_block, 1, write_buf);
    printf("[WRITE] Block %u, Page 1 (data=0xBB): %s\n",
           test_block, nand_status_to_str(status));

    /* 写 page 2: 写一段文字 */
    memset(write_buf, 0, PAGE_DATA_SIZE);
    const char *msg = "Hello from MiniSSD! This is page 2.";
    memcpy(write_buf, msg, strlen(msg));
    status = nand_page_write(chip, test_block, 2, write_buf);
    printf("[WRITE] Block %u, Page 2 (string): %s\n",
           test_block, nand_status_to_str(status));

    printf("\n");

    /* ---- 4. 读取验证 ---- */
    status = nand_page_read(chip, test_block, 0, read_buf);
    printf("[READ]  Block %u, Page 0: %s, first 16 bytes:\n",
           test_block, nand_status_to_str(status));
    print_hex(read_buf, 16);

    status = nand_page_read(chip, test_block, 1, read_buf);
    printf("[READ]  Block %u, Page 1: %s, first 16 bytes:\n",
           test_block, nand_status_to_str(status));
    print_hex(read_buf, 16);

    status = nand_page_read(chip, test_block, 2, read_buf);
    printf("[READ]  Block %u, Page 2: %s, content:\n",
           test_block, nand_status_to_str(status));
    printf("  \"%s\"\n", (char *)read_buf);

    /* 读一个 FREE 的 page（应该全是 0xFF） */
    status = nand_page_read(chip, test_block, 10, read_buf);
    printf("[READ]  Block %u, Page 10 (free): %s, first 16 bytes:\n",
           test_block, nand_status_to_str(status));
    print_hex(read_buf, 16);

    printf("\n");

    /* ---- 5. 错误场景测试 ---- */
    printf("--- Error Scenario Tests ---\n");

    /* 尝试覆盖写已编程的 page */
    fill_page_data(write_buf, 0xCC);
    status = nand_page_write(chip, test_block, 0, write_buf);
    printf("[WRITE] Overwrite Page 0: %s  (expected: ERR)\n",
           nand_status_to_str(status));

    /* 尝试乱序写（跳过 page 3-4，直接写 page 5 → 应该可以） */
    fill_page_data(write_buf, 0x55);
    status = nand_page_write(chip, test_block, 5, write_buf);
    printf("[WRITE] Skip to Page 5: %s  (expected: OK, skipping is allowed)\n",
           nand_status_to_str(status));

    /* 尝试往回写 page 3（已经跳过了 → 应该报错） */
    fill_page_data(write_buf, 0x33);
    status = nand_page_write(chip, test_block, 3, write_buf);
    printf("[WRITE] Write Page 3 after Page 5: %s  (expected: ERR write order)\n",
           nand_status_to_str(status));

    /* 尝试操作一个坏块 */
    uint32_t bad_block = UINT32_MAX;
    for (uint32_t b = 0; b < BLOCKS_PER_CHIP; b++) {
        if (nand_block_get_state(chip, b) == BLOCK_BAD_FACTORY) {
            bad_block = b;
            break;
        }
    }
    if (bad_block != UINT32_MAX) {
        status = nand_page_write(chip, bad_block, 0, write_buf);
        printf("[WRITE] Write to bad block %u: %s  (expected: ERR)\n",
               bad_block, nand_status_to_str(status));
    } else {
        printf("[SKIP]  No factory bad block found (lucky chip!)\n");
    }

    /* 越界地址 */
    status = nand_page_read(chip, BLOCKS_PER_CHIP + 1, 0, read_buf);
    printf("[READ]  Out-of-range block: %s  (expected: ERR)\n",
           nand_status_to_str(status));

    printf("\n");

    /* ---- 6. 擦除测试 ---- */
    printf("--- Erase Test ---\n");
    printf("[INFO]  Block %u valid pages before erase: %u\n",
           test_block, nand_block_get_valid_pages(chip, test_block));

    status = nand_block_erase(chip, test_block);
    printf("[ERASE] Block %u: %s\n", test_block, nand_status_to_str(status));
    printf("[INFO]  Block %u valid pages after erase: %u\n",
           test_block, nand_block_get_valid_pages(chip, test_block));
    printf("[INFO]  Block %u erase count: %u\n",
           test_block, nand_block_get_erase_count(chip, test_block));

    /* 擦除后读 page 0（应该全是 0xFF） */
    status = nand_page_read(chip, test_block, 0, read_buf);
    printf("[READ]  After erase, Block %u, Page 0: %s, first 16 bytes:\n",
           test_block, nand_status_to_str(status));
    print_hex(read_buf, 16);

    /* 擦除后可以重新写入 */
    fill_page_data(write_buf, 0xDD);
    status = nand_page_write(chip, test_block, 0, write_buf);
    printf("[WRITE] Re-write after erase, Page 0: %s\n",
           nand_status_to_str(status));

    printf("\n");

    /* ---- 7. Page 无效化测试 ---- */
    printf("--- Invalidate Test ---\n");
    printf("[INFO]  Block %u valid pages: %u\n",
           test_block, nand_block_get_valid_pages(chip, test_block));

    status = nand_page_invalidate(chip, test_block, 0);
    printf("[INVAL] Page 0: %s\n", nand_status_to_str(status));
    printf("[INFO]  Block %u valid pages after invalidate: %u\n",
           test_block, nand_block_get_valid_pages(chip, test_block));

    printf("\n");

    /* ---- 最终状态 ---- */
    nand_chip_dump_info(chip);

    printf("\n=== All tests completed! ===\n");
    free(chip);
    return 0;
}
