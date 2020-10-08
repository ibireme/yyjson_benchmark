#include "benchmark.h"
#include "yyjson.h"


// -----------------------------------------------------------------------------
// reader

u64 reader_measure_yyjson(const char *json, size_t size, int repeat) {
    benchmark_tick_init();
    
    for (int i = 0; i < repeat; i++) {
        benchmark_tick_begin();
        yyjson_doc *doc = yyjson_read(json, size, YYJSON_READ_NOFLAG);
        benchmark_tick_end();
        if (!doc) return 0;
        yyjson_doc_free(doc);
    }
    
    return benchmark_tick_min();
}

u64 reader_measure_yyjson_fast(const char *json, size_t size, int repeat) {
    benchmark_tick_init();
    
    yyjson_read_flag flag = YYJSON_READ_FASTFP | YYJSON_READ_INSITU;
    
    // use custom allocator
    usize buf_size = yyjson_read_max_memory_usage(size, flag);
    void *buf = malloc(buf_size);
    yyjson_alc alc;
    yyjson_alc_pool_init(&alc, buf, buf_size);
    
    // use insitu
    char *dat = malloc(size + 4);
    
    for (int i = 0; i < repeat; i++) {
        memcpy(dat, json, size);
        memset(dat + size, 0, 4); // 4-byte padding
        
        benchmark_tick_begin();
        yyjson_doc *doc = yyjson_read_opts(dat, size, flag, &alc, NULL);
        benchmark_tick_end();
        
        if (!doc) return 0;
        yyjson_doc_free(doc);
    }
    
    // free memory
    free(buf);
    free(dat);
    
    return benchmark_tick_min();
}


// -----------------------------------------------------------------------------
// writer

u64 writer_measure_yyjson(const char *json, size_t size, size_t *out_size,
                          bool pretty, int repeat) {
    benchmark_tick_init();
    
    yyjson_doc *doc = yyjson_read(json, size, YYJSON_READ_NOFLAG);
    yyjson_write_flag flag = pretty ? YYJSON_WRITE_PRETTY : YYJSON_WRITE_NOFLAG;
    
    for (int i = 0; i < repeat; i++) {
        benchmark_tick_begin();
        char *str = yyjson_write(doc, flag, out_size);
        benchmark_tick_end();
        if (!str) return 0;
        free(str);
    }
    yyjson_doc_free(doc);
    
    return benchmark_tick_min();
}

u64 writer_measure_yyjson_mut(const char *json, size_t size, size_t *out_size,
                              bool pretty, int repeat) {
    benchmark_tick_init();
    
    yyjson_doc *doc = yyjson_read(json, size, YYJSON_READ_NOFLAG);
    yyjson_mut_doc *mdoc = yyjson_doc_mut_copy(doc, NULL);
    yyjson_write_flag flag = pretty ? YYJSON_WRITE_PRETTY : YYJSON_WRITE_NOFLAG;
    
    for (int i = 0; i < repeat; i++) {
        benchmark_tick_begin();
        char *str = yyjson_mut_write(mdoc, flag, out_size);
        benchmark_tick_end();
        if (!str) return 0;
        free(str);
    }
    yyjson_doc_free(doc);
    yyjson_mut_doc_free(mdoc);
    
    return benchmark_tick_min();
}


// -----------------------------------------------------------------------------
// stats

static yy_inline void stats_single(yyjson_val *val, stats_data *data) {
    if (yyjson_is_str(val)) {
        data->num_string++;
    } else if (yyjson_is_num(val)) {
        data->num_number++;
    } else if (yyjson_is_true(val)) {
        data->num_true++;
    } else if (yyjson_is_false(val)) {
        data->num_false++;
    } else if (yyjson_is_null(val)) {
        data->num_null++;
    }
}

static void stats_recursive(yyjson_val *val, stats_data *data) {
    if (!val) return;
    if (yyjson_is_arr(val)) {
        data->num_array++;
        usize idx, max;
        yyjson_val *tmp;
        yyjson_arr_foreach(val, idx, max, tmp) {
            if (yyjson_is_ctn(tmp)) stats_recursive(tmp, data);
            else stats_single(tmp, data);
        }
    } else if (yyjson_is_obj(val)) {
        data->num_object++;
        usize idx, max;
        yyjson_val *k, *v;
        yyjson_obj_foreach(val, idx, max, k, v) {
            data->num_string++;
            if (yyjson_is_ctn(v)) stats_recursive(v, data);
            else stats_single(v, data);
        }
    } else {
        stats_single(val, data);
    }
}

u64 stats_measure_yyjson(const char *json, size_t size, stats_data *data, int repeat) {
    benchmark_tick_init();
    
    yyjson_doc *doc = yyjson_read(json, size, YYJSON_READ_NOFLAG);
    yyjson_val *root = yyjson_doc_get_root(doc);
    for (int i = 0; i < repeat; i++) {
        memset(data, 0, sizeof(stats_data));
        benchmark_tick_begin();
        stats_recursive(root, data);
        benchmark_tick_end();
    }
    yyjson_doc_free(doc);
    
    return benchmark_tick_min();
}


u64 stats_measure_yyjson_fast(const char *json, size_t size, stats_data *data, int repeat) {
    benchmark_tick_init();
    
    yyjson_doc *doc = yyjson_read(json, size, YYJSON_READ_NOFLAG);
    yyjson_val *root = yyjson_doc_get_root(doc);
    usize val_count = yyjson_doc_get_val_count(doc);
    for (int i = 0; i < repeat; i++) {
        memset(data, 0, sizeof(stats_data));
        benchmark_tick_begin();
        for (usize v = 0; v < val_count; v++) {
            yyjson_val *val = root + v;
            if (yyjson_is_arr(val)) {
                data->num_array++;
            } else if (yyjson_is_obj(val)) {
                data->num_object++;
            } else if (yyjson_is_str(val)) {
                data->num_string++;
            } else if (yyjson_is_num(val)) {
                data->num_number++;
            } else if (yyjson_is_true(val)) {
                data->num_true++;
            } else if (yyjson_is_false(val)) {
                data->num_false++;
            } else if (yyjson_is_null(val)) {
                data->num_null++;
            }
        }
        benchmark_tick_end();
    }
    yyjson_doc_free(doc);
    
    return benchmark_tick_min();
}
