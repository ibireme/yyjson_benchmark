#include "benchmark.h"
#include "cJSON.h"

// -----------------------------------------------------------------------------
// reader

u64 reader_measure_cjson(const char *json, size_t size, int repeat) {
    benchmark_tick_init();
    
    for (int i = 0; i < repeat; i++) {
        benchmark_tick_begin();
        cJSON *doc = cJSON_ParseWithLength(json, size);
        benchmark_tick_end();
        if (!doc) return 0;
        cJSON_Delete(doc);
    }
    
    return benchmark_tick_min();
}


// -----------------------------------------------------------------------------
// writer

u64 writer_measure_cjson(const char *json, size_t size, size_t *out_size,
                          bool pretty, int repeat) {
    benchmark_tick_init();
    
    cJSON *doc = cJSON_ParseWithLength(json, size);
    
    if (pretty) {
        for (int i = 0; i < repeat; i++) {
            benchmark_tick_begin();
            char *str = cJSON_Print(doc);
            benchmark_tick_end();
            if (!str) return 0;
            *out_size = strlen(str);
            free(str);
        }
    } else {
        for (int i = 0; i < repeat; i++) {
            benchmark_tick_begin();
            char *str = cJSON_PrintUnformatted(doc);
            benchmark_tick_end();
            if (!str) return 0;
            *out_size = strlen(str);
            free(str);
        }
    }
    
    cJSON_Delete(doc);
    
    return benchmark_tick_min();
}


// -----------------------------------------------------------------------------
// stats

static yy_inline void stats_single(cJSON *val, stats_data *data) {
    if (cJSON_IsString(val)) {
        data->num_string++;
    } else if (cJSON_IsNumber(val)) {
        data->num_number++;
    } else if (cJSON_IsTrue(val)) {
        data->num_true++;
    } else if (cJSON_IsFalse(val)) {
        data->num_false++;
    } else if (cJSON_IsNull(val)) {
        data->num_null++;
    }
}

static void stats_recursive(cJSON *val, stats_data *data) {
    if (!val) return;
    if (cJSON_IsArray(val)) {
        data->num_array++;
        cJSON *tmp;
        cJSON_ArrayForEach(tmp, val) {
            if (cJSON_IsObject(tmp) || cJSON_IsArray(tmp)) stats_recursive(tmp, data);
            else stats_single(tmp, data);
        }
    } else if (cJSON_IsObject(val)) {
        data->num_object++;
        cJSON *tmp;
        cJSON_ArrayForEach(tmp, val) {
            data->num_string++;
            if (cJSON_IsObject(tmp) || cJSON_IsArray(tmp)) stats_recursive(tmp, data);
            else stats_single(tmp, data);
        }
    } else {
        stats_single(val, data);
    }
}

u64 stats_measure_cjson(const char *json, size_t size, stats_data *data, int repeat) {
    benchmark_tick_init();
    
    cJSON *doc = cJSON_ParseWithLength(json, size);
    for (int i = 0; i < repeat; i++) {
        memset(data, 0, sizeof(stats_data));
        benchmark_tick_begin();
        stats_recursive(doc, data);
        benchmark_tick_end();
    }
    cJSON_Delete(doc);
    
    return benchmark_tick_min();
}
