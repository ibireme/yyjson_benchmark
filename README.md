# JSON Benchmark

Benchmark JSON library performance in C/C++.

# Requirement

- A modern compiler or IDE supporting C11 and C++17.
- CMake 3.5+ for building this project.
- Git for interacting with the submodule in this repository.

# Building

Clone this repository and initialize submodules:
```shell
git clone https://github.com/ibireme/yyjson_benchmark.git
cd yyjson_benchmark
git submodule update --init
```

Build and run:
```shell
mkdir build
cd build
cmake ..
cmake --build . --config Release
./run_benchmark -o report.html
```

If you want to build with other compiler or IDE, try these commands:
```shell
# Clang for Linux/Unix:
cmake .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++

# Microsoft Visual Studio for Windows:
cmake .. -G "Visual Studio 16 2019" -A x64
cmake .. -G "Visual Studio 16 2019" -A Win32

# Xcode for macOS:
cmake .. -G Xcode

# Xcode for iOS:
cmake .. -G Xcode -DCMAKE_SYSTEM_NAME=iOS
```

If you want to build for arm64 device, you should add flag`-DSIMDJSON_IMPLEMENTATION=arm64` for `simdjson`.

# Results

Benchmark reports with interactive charts. All reports are stored in
[docs/reports](docs/reports/); click "Charts" to open one.

Note: reports produced before 2026-11 were generated with the December 2020
submodule pins (yyjson 0.1.0, simdjson v0.7.0 — AVX2 only, rapidjson
v1.1.0-567); the current pins are the 2026 releases (yyjson 0.13.0,
simdjson v5.2.0, jansson v2.15.1, cJSON v1.7.19, json-c 0.19), so older
reports understate simdjson on AVX-512 CPUs.

|Platform|CPU|Compiler|OS|Report|
|---|---|---|---|---|
|KVM VPS|AMD Ryzen 9 9950X|gcc 15.2|Ubuntu 26.04|[Charts](docs/reports/AMD_Ryzen_9_9950X_gcc_15.html), [run 2](docs/reports/AMD_Ryzen_9_9950X_gcc_15_run2.html)|
|Intel NUC 8i5|Core i5-8259U|msvc 2019|Windows 10 2004|[Charts](docs/reports/Intel_NUC_8i5_msvc_2019.html)|
|Intel NUC 8i5|Core i5-8259U|clang 10|Ubuntu 20.04|[Charts](docs/reports/Intel_NUC_8i5_clang_10.html)|
|Intel NUC 8i5|Core i5-8259U|gcc 9.3|Ubuntu 20.04|[Charts](docs/reports/Intel_NUC_8i5_gcc_9.html)|
|MacBook Pro|Core i7-4870HQ|clang 12|macOS|[Charts](docs/reports/MacbookPro_i7-4870HQ_clang_12.html)|
|MacBook Pro|Core i7-4870HQ|gcc 9.3|macOS|[Charts](docs/reports/MacbookPro_i7-4870HQ_gcc_9.html)|
|AWS EC2 c5n.large|Xeon Platinum 8124M|clang 10|Ubuntu 20.04|[Charts](docs/reports/EC2_c5n.large_clang_10.html)|
|AWS EC2 c5n.large|Xeon Platinum 8124M|gcc 9.3|Ubuntu 20.04|[Charts](docs/reports/EC2_c5n.large_gcc_9.html)|
|AWS EC2 c5a.large|AMD EPYC 7R32|gcc 9.3|Ubuntu 20.04|[Charts](docs/reports/EC2_c5a.large_gcc_9.html)|
|AWS EC2 t4g.medium|Graviton2 (ARM64)|gcc 9.3|Ubuntu 20.04|[Charts](docs/reports/EC2_t4g.medium_gcc_9.html)|
|Apple iPhone 12 Pro|A14 (ARM64)|clang 12|iOS 14|[Charts](docs/reports/Apple_A14_clang_12.html)|
|Raspberry Pi 4B|Cortex-A72|gcc 9.3|Linux 64-bit|[Charts](docs/reports/Raspberry_Pi_4B_gcc9.html)|

# Methodology notes (allocator sensitivity)

The plain `yyjson` series calls `yyjson_read()` which allocates the document
(roughly 2-3x the input size) through the global allocator on every call.
With glibc malloc, MB-scale allocations are served with fresh `mmap` pages that
are returned to the OS on free, so every iteration pays page faults and kernel
zeroing. Measured consequences (twitter.json):

|Condition|Throughput|% system CPU|Minor faults/iter|
|---|---|---|---|
|fresh process, glibc|1.2-1.5 GB/s|61%|~230|
|same process after an insitu+pool pass (glibc)|~5 GB/s|0%|~0|
|jemalloc or tcmalloc via LD_PRELOAD|4.4-5.2 GB/s|0-2%|~3|
|insitu + pool allocator|5.5-6 GB/s|0%|0|

In other words, the plain-read number reflects allocator behavior as much as
parser speed, and depends on measurement order within the process (this suite
runs the insitu variant first, which warms the glibc heap). For large files
(46-63 MB) the effect is even stronger: 0.4-0.5 GB/s plain vs 1.3-2.1 GB/s
with a pool. If you compare parsers, prefer `yyjson (insitu)` for yyjson or
neutralize the allocator (pool, `mallopt`, or an alternative malloc).

simdjson is not affected: its DOM parser reuses internal buffers across calls,
so the timed region performs no global allocations.

Peak memory overhead over the input buffer (fgo, 46 MB): yyjson DOM ~3.1x
(plain: allocated per call; insitu+pool: same size, allocated once and reused),
simdjson DOM ~4.6x (buffers held and reused between calls), simdjson On-Demand
with struct deserialization ~1.0x (only requested values materialize).

# JSON Datasets

|File|Size|Format|Content|Info|
|---|---|---|---|---|
|twitter|616.7KB|pretty|The most commonly used test data from [MiliYip](https://github.com/miloyip/nativejson-benchmark).|common|
|twitterescaped|549.2KB|minify|Same as twitter, with unicode escaped.|common|
|github_events|63.6KB|pretty|GitHub event data from GitHub API.|common|
|canada|2.1MB|minify|Contour of Canada border from [MiloYip](https://github.com/miloyip/nativejson-benchmark).|full-length double|
|citm_catalog|1.6MB|pretty|A big benchmark file with indentation from [RichardHightower](https://github.com/RichardHightower/json-parsers-benchmark)|repeated integers<br/> and strings|
|lottie|282.2KB|minify|Lottie animation data downloaded from [LottieFiles](https://lottiefiles.com/29-motorcycle)|short string and number|
|gsoc-2018|3.2MB|pretty|GSoC Data of 2018.|long ASCII string|
|poet|3.4MB|pretty|Poet data from [chinese-poetry](https://github.com/chinese-poetry/chinese-poetry)|long CJK string|
|fgo|46.5MB|minify|Config data dumped from Japanese mobile game "Fate/Grand Order"|real-world large file|
|otfcc|63.3MB|minify|OpenType data dumped from [NotoSansJP-Regular.otf](https://www.google.com/get/noto/help/cjk/) with [otfcc](https://github.com/caryll/otfcc)|real-world large file|


