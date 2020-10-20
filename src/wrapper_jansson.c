#include "benchmark.h"
#include "jansson.h"

// -----------------------------------------------------------------------------
// reader

u64 reader_measure_jansson(const char *json, size_t size, int repeat) {
    benchmark_tick_init();
    
    for (int i = 0; i < repeat; i++) {
        benchmark_tick_begin();
        json_error_t error;
        json_t *root = json_loadb(json, size, JSON_DECODE_ANY | JSON_ALLOW_NUL, &error);
        benchmark_tick_end();
        if (!root) return 0;
        json_decref(root);
    }
    
    return benchmark_tick_min();
}


// -----------------------------------------------------------------------------
// writer

u64 writer_measure_jansson(const char *json, size_t size, size_t *out_size, 
                           bool *roundtrip, bool pretty, int repeat) {
    benchmark_tick_init();
    
    json_error_t error;
    json_t *root = json_loadb(json, size, JSON_DECODE_ANY | JSON_ALLOW_NUL, &error);
    if (!root) return 0;
    
    bool processed = false;
    if (pretty) {
        for (int i = 0; i < repeat; i++) {
            benchmark_tick_begin();
            char *str = json_dumps(root, JSON_ENCODE_ANY | JSON_INDENT(4));
            benchmark_tick_end();
            if (!str) return 0;
            if (!processed) {
                processed = true;
                *out_size = strlen(str);
                *roundtrip = (*out_size == size && memcmp(json, str, size) == 0);
            }
            free(str);
        }
    } else {
        for (int i = 0; i < repeat; i++) {
            benchmark_tick_begin();
            char *str = json_dumps(root, JSON_ENCODE_ANY | JSON_COMPACT);
            benchmark_tick_end();
            if (!str) return 0;
            if (!processed) {
                processed = true;
                *out_size = strlen(str);
                *roundtrip = (*out_size == size && memcmp(json, str, size) == 0);
            }
            free(str);
        }
    }
    
    json_decref(root);
    
    return benchmark_tick_min();
}


// -----------------------------------------------------------------------------
// stats

static yy_inline void stats_single(json_t *val, stats_data *data) {
    if (json_is_string(val)) {
        data->num_string++;
    } else if (json_is_number(val)) {
        data->num_number++;
    } else if (json_is_true(val)) {
        data->num_true++;
    } else if (json_is_false(val)) {
        data->num_false++;
    } else if (json_is_null(val)) {
        data->num_null++;
    }
}

static void stats_recursive(json_t *val, stats_data *data) {
    if (!val) return;
    if (json_is_array(val)) {
        data->num_array++;
        
        size_t idx;
        json_t *tmp;
        json_array_foreach(val, idx, tmp) {
            if (json_is_object(tmp) || json_is_array(tmp)) stats_recursive(tmp, data);
            else stats_single(tmp, data);
        }
    } else if (json_is_object(val)) {
        data->num_object++;
        
        const char *k;
        json_t *v;
        json_object_foreach(val, k, v) {
            data->num_string++;
            if (json_is_object(v) || json_is_array(v)) stats_recursive(v, data);
            else stats_single(v, data);
        }
    } else {
        stats_single(val, data);
    }
}

u64 stats_measure_jansson(const char *json, size_t size, stats_data *data, int repeat) {
    benchmark_tick_init();
    
    json_error_t error;
    json_t *root = json_loadb(json, size, JSON_DECODE_ANY | JSON_ALLOW_NUL, &error);
    for (int i = 0; i < repeat; i++) {
        memset(data, 0, sizeof(stats_data));
        benchmark_tick_begin();
        stats_recursive(root, data);
        benchmark_tick_end();
    }
    json_decref(root);
    
    return benchmark_tick_min();
}

