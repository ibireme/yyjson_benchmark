#include "yy_test_utils.h"


/**
 Function prototype to meansure a JSON reader performance.
 A wrapper should define the function with this format: reader_measure_<name>.
 For example: reader_measure_yyjson_insitu.
 
 @param json JSON data in UTF-8 with null-terminator.
 @param size JSON data size in bytes.
 @param repeat Loop count for meansure.
 @return The ticks cost of one loop.
 */
typedef u64 (*reader_measure_func)(const char *json, size_t size, int repeat);


/**
 Function prototype to meansure a JSON writer performance.
 A wrapper should define the function with this format: writer_measure_<name>.
 For example: writer_measure_yyjson.
 
 @param json JSON data in UTF-8 with null-terminator.
 @param size JSON data size in bytes.
 @param out_size JSON output size in bytes.
 @param roundtrip JSON output same as input.
 @param repeat Loop count for meansure.
 @param pretty Pretty or minify.
 @return The ticks cost of one loop.
 */
typedef u64 (*writer_measure_func)(const char *json, size_t size, size_t *out_size,
                                   bool *roundtrip, bool pretty, int repeat);


typedef struct {
    int num_null;
    int num_true;
    int num_false;
    int num_number;
    int num_string;
    int num_array;
    int num_object;
} stats_data;

/**
 Function prototype to meansure the performance of traverse a JSON document.
 A wrapper should define the function with this format: stats_measure_<name>.
 For example: stats_measure_yyjson.
 
 @param json JSON data in UTF-8 with null-terminator.
 @param size JSON data size in bytes.
 @param data JSON stats data output.
 @param repeat Loop count for meansure.
 @return The ticks cost of one loop.
 */
typedef u64 (*stats_measure_func)(const char *json, size_t size, stats_data *data, int repeat);



/**
 The benchmark data directory path.
 */
#ifndef BENCHMARK_DATA_PATH
extern const char *benchmark_get_data_path(void);
#define BENCHMARK_DATA_PATH benchmark_get_data_path()
#endif

/**
 Benchmark tick timer
 */

#define benchmark_tick_init() \
    u64 __tmin = UINT64_MAX, __t1, __t2;

#define benchmark_tick_begin() \
    __t1 = yy_time_get_ticks();

#define benchmark_tick_end() \
    __t2 = yy_time_get_ticks(); \
    if (__t2 - __t1 < __tmin) __tmin = __t2 - __t1;

#define benchmark_tick_min() \
    __tmin

