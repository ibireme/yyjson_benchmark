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
make
./yyjson_benchmark -o report.html
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

# Reports
See [yyjson_benchmark/report](https://github.com/ibireme/yyjson_benchmark/tree/master/docs/reports) for some benchmark reports.

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


