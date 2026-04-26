# MiniSSD - A Minimal SSD Simulator

A software simulator that models the internal behavior of a NAND Flash-based Solid State Drive, built from scratch in C/C++.

**MiniSSD - 简易固态硬盘模拟器**

用 C/C++ 从零构建的 NAND Flash SSD 软件模拟器，模拟真实 SSD 内部的存储结构和工作原理。

---

## Why This Project / 为什么做这个项目

We use SSDs every day, but rarely think about what happens between "save a file" and "charge stored in a transistor." This project builds the entire storage stack in software to understand the internals: how pages are written, why blocks must be erased as a unit, and how the firmware manages wear and garbage collection.

我们每天都在用 SSD，但很少有人想过从"保存文件"到"电荷存进晶体管"之间到底发生了什么。这个项目用软件把整个存储栈搭出来，搞明白原理：page 怎么写、block 为什么必须整体擦除、固件怎么管理磨损和垃圾回收。

---

## Architecture / 架构

```
┌─────────────────────────────────────────┐
│          CLI / Benchmark (C++)          │  ← Week 3
│         User-facing interface           │
├─────────────────────────────────────────┤
│        FTL - Flash Translation Layer    │  ← Week 2
│   Address Mapping / GC / Wear Leveling  │
├─────────────────────────────────────────┤
│        NAND Physical Layer (C)          │  ← Week 1 ✅
│     Page Read / Write / Block Erase     │
└─────────────────────────────────────────┘
```

The bottom layer (NAND) is written in **pure C** to stay close to a hardware-driver style. The upper layers (FTL, CLI) are written in **C++17**, calling the C layer through `extern "C"`. This mixed approach mirrors real-world embedded/firmware development.

底层 NAND 用**纯 C** 编写，贴近硬件驱动风格；上层 FTL 和 CLI 用 **C++17** 编写，通过 `extern "C"` 调用 C 底层。这种混合方式是真实嵌入式/固件开发中的常见模式。

---

## Project Structure / 项目结构

```
minissd/
├── CMakeLists.txt                  # Build configuration / 构建配置
├── README.md                       # This file / 本文件
│
├── include/                        # Header files / 头文件
│   └── nand/
│       ├── nand_types.h            # Type definitions & constants / 类型定义与常量
│       ├── nand_chip.h             # Chip operation interface / 芯片操作接口
│       └── nand_utils.h            # Utility functions / 辅助函数
│
├── src/                            # Source files / 源文件
│   ├── nand/
│   │   ├── nand_chip.c             # Core implementation / 核心实现
│   │   └── nand_utils.c            # Utilities implementation / 辅助函数实现
│   └── main.cpp                    # Entry point & tests / 入口与测试
│
├── tests/                          # Unit tests (coming) / 单元测试（待完成）
├── scripts/                        # Python visualization (coming) / Python 可视化（待完成）
└── build/                          # Build output / 编译输出
```

---

## Current Status / 当前进度

### Week 1: NAND Physical Layer ✅

The foundation is complete. The NAND layer simulates real flash memory behavior with the following constraints enforced:

Week 1 基础层已完成。NAND 层模拟了真实闪存的物理行为，强制执行以下约束：

| Rule / 规则 | Description / 说明 |
|---|---|
| **No overwrite** / 不能覆盖写 | A page must be FREE before writing. Flash cells can only go from 1→0, not 0→1. / Page 必须是 FREE 才能写入，闪存只能 1→0，不能 0→1 |
| **Sequential write** / 顺序写入 | Pages within a block must be written in order (0, 1, 2...). / 同一 block 内的 page 必须按序号递增写入 |
| **Block erase** / 块擦除 | Erase is the only way to reset pages, and it operates on an entire block. / 擦除是重置 page 的唯一方式，且以整个 block 为单位 |
| **Wear limit** / 磨损上限 | A block is marked bad after exceeding the maximum erase count (100,000). / 擦写次数超过上限后自动标记为坏块 |
| **Bad blocks** / 坏块 | Factory bad blocks are randomly generated at init; runtime bad blocks arise from wear. / 出厂坏块在初始化时随机生成，运行时坏块由磨损产生 |

### Week 2: FTL (Planned) 🔲

Address mapping, garbage collection, wear leveling, bad block management.

地址映射、垃圾回收、磨损均衡、坏块管理。

### Week 3: CLI & Benchmark (Planned) 🔲

Interactive command-line interface and performance testing tools.

交互式命令行界面和性能测试工具。

### Week 4: Testing & Visualization (Planned) 🔲

Google Test unit tests, Python performance charts, documentation.

Google Test 单元测试、Python 性能图表、完整文档。

---

## Build & Run / 编译与运行

### Prerequisites / 前置条件

- **Windows:** MSYS2 + MinGW-w64 (gcc, cmake, make)
- **Linux/macOS:** gcc/g++, cmake, make

### Build / 编译

```bash
cd minissd
mkdir -p build && cd build

# Windows (MinGW)
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
mingw32-make

# Linux / macOS
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Run / 运行

```bash
./minissd          # or minissd.exe on Windows
```

### Sample Output / 示例输出

```
=== MiniSSD - NAND Layer Test ===

[INIT] Chip initialized: OK
========== NAND Chip Info ==========
  Blocks:       1024
  Pages/Block:  64
  Page Size:    4096 bytes
  Total Size:   256 MB
  Bad Blocks:   10 (1.0%)
  Free Blocks:  1014
====================================

[WRITE] Block 0, Page 0 (data=0xAA): OK
[WRITE] Block 0, Page 1 (data=0xBB): OK
[WRITE] Block 0, Page 2 (string): OK

[READ]  Block 0, Page 0: OK, first 16 bytes:
AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA AA
[READ]  Block 0, Page 2: OK, content:
  "Hello from MiniSSD! This is page 2."
[READ]  Block 0, Page 10 (free): OK, first 16 bytes:
FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF

--- Error Scenario Tests ---
[WRITE] Overwrite Page 0: ERR: write to programmed page  (expected: ERR)
[WRITE] Skip to Page 5: OK  (expected: OK, skipping is allowed)
[WRITE] Write Page 3 after Page 5: ERR: page write order violation  (expected: ERR write order)
[WRITE] Write to bad block 52: ERR: bad block  (expected: ERR)
[READ]  Out-of-range block: ERR: address out of range  (expected: ERR)

--- Erase Test ---
[ERASE] Block 0: OK
[READ]  After erase, Block 0, Page 0: OK, first 16 bytes:
FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
[WRITE] Re-write after erase, Page 0: OK

=== All tests completed! ===
```

---

## Key Design Decisions / 关键设计决策

| Decision / 决策 | Choice / 选择 | Reason / 原因 |
|---|---|---|
| Low-level language / 底层语言 | C | Closer to hardware driver style; demonstrates C/C++ interop / 贴近硬件驱动风格，展示 C/C++ 混合能力 |
| Memory allocation / 内存分配 | Heap (`malloc`) | `nand_chip_t` is ~277MB — too large for the stack / 芯片结构体约 277MB，栈放不下 |
| Random number generator / 随机数 | Custom LCG | Deterministic with seed — reproducible test results / 固定种子可复现测试结果 |
| Erased byte value / 擦除默认值 | `0xFF` | Matches real NAND flash behavior / 与真实闪存一致 |
| Write order enforcement / 写入顺序检查 | Forward-only | Real NAND requires sequential page programming within a block / 真实 NAND 要求同 block 内按序写入 |

---

## Flash Memory Basics / 闪存基础知识

For those new to NAND flash, here's a quick primer:

对于不熟悉 NAND Flash 的读者，这里有一个快速入门：

```
                    Chip (256 MB)
                    ├── Block 0        ← Smallest ERASE unit / 最小擦除单位
                    │   ├── Page 0     ← Smallest READ/WRITE unit / 最小读写单位 (4KB)
                    │   ├── Page 1
                    │   ├── ...
                    │   └── Page 63
                    ├── Block 1
                    │   ├── Page 0
                    │   └── ...
                    ├── ...
                    └── Block 1023
```

**Key differences from a hard drive / 与硬盘的关键区别：**

1. **No in-place update / 不能原地更新：** To change data in a page, you must write to a *new* page and mark the old one invalid. / 要修改一个 page 的数据，必须写到新位置，旧位置标记无效。

2. **Erase before write / 写前必须擦：** Pages start as `0xFF`. Writing can only flip bits from 1→0. To write again, the entire block must be erased (all bits back to 1). / 写入只能把 bit 从 1 变 0，要重新写必须先擦除整个 block。

3. **Limited endurance / 有限寿命：** Each block can only be erased ~10,000–100,000 times before it wears out. / 每个 block 只能擦写数万到十万次。

This is why SSDs need a **Flash Translation Layer (FTL)** — firmware that hides all this complexity and makes the flash chip look like a normal disk to the operating system.

这就是为什么 SSD 需要 **FTL（闪存转换层）** —— 固件隐藏所有这些复杂性，让闪存芯片对操作系统来说看起来像一块普通硬盘。

---

## Tech Stack / 技术栈

- **C11** — NAND physical layer (hardware simulation)
- **C++17** — FTL, CLI, benchmark (upper layers)
- **CMake** — Cross-platform build system
- **Google Test** — Unit testing (planned)
- **Python** — Performance visualization (planned)

---

## License

This is a personal learning project. Feel free to reference or learn from the code.

这是个人学习项目，欢迎参考和学习。
