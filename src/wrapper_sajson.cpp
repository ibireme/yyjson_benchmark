#include "benchmark.h"
#include "sajson.h"
extern "C" {

// -----------------------------------------------------------------------------
// reader

u64 reader_measure_sajson(const char *json, size_t size, int repeat) {
    benchmark_tick_init();
    
    char *buf = (char *)malloc(size);
    size_t *ast_buf = (size_t *)malloc(size * sizeof(size_t));
    for (int i = 0; i < repeat; i++) {
        memcpy((void *)buf, (void *)json, size);
        benchmark_tick_begin();
        const sajson::document& doc = sajson::parse(sajson::bounded_allocation(ast_buf, size),
                                                    sajson::mutable_string_view(size, buf));
        benchmark_tick_end();
        if (!doc.is_valid()) return 0;
    }
    free((void *)buf);
    
    return benchmark_tick_min();
}


u64 reader_measure_sajson_dynamic(const char *json, size_t size, int repeat) {
    benchmark_tick_init();
    
    char *buf = (char *)malloc(size);
    for (int i = 0; i < repeat; i++) {
        memcpy((void *)buf, (void *)json, size);
        benchmark_tick_begin();
        const sajson::document& doc = sajson::parse(sajson::dynamic_allocation(),
                                                    sajson::mutable_string_view(size, buf));
        benchmark_tick_end();
        if (!doc.is_valid()) return 0;
    }
    free((void *)buf);
    
    return benchmark_tick_min();
}



// -----------------------------------------------------------------------------
// stats

static yy_inline void stats_single(const sajson::value& val, stats_data *data) {
    if (val.get_type() == sajson::TYPE_STRING) {
        data->num_string++;
    } else if (val.get_type() == sajson::TYPE_INTEGER ||
               val.get_type() == sajson::TYPE_DOUBLE) {
        data->num_number++;
    } else if (val.get_type() == sajson::TYPE_TRUE) {
        data->num_true++;
    } else if (val.get_type() == sajson::TYPE_FALSE) {
        data->num_false++;
    } else if (val.get_type() == sajson::TYPE_NULL) {
        data->num_null++;
    }
}

static void stats_recursive(const sajson::value& val, stats_data *data) {
    if (val.get_type() == sajson::TYPE_ARRAY) {
        data->num_array++;
        auto length = val.get_length();
        for (size_t i = 0; i < length; ++i) {
            const sajson::value& tmp = val.get_array_element(i);
            if (tmp.get_type() == sajson::TYPE_ARRAY ||
                tmp.get_type() == sajson::TYPE_OBJECT)
                stats_recursive(tmp, data);
            else stats_single(tmp, data);
        }
        
    } else if (val.get_type() == sajson::TYPE_OBJECT) {
        data->num_object++;
        auto length = val.get_length();
        for (size_t i = 0; i < length; ++i) {
            const sajson::value& tmp = val.get_object_value(i);
            data->num_string++;
            if (tmp.get_type() == sajson::TYPE_ARRAY ||
                tmp.get_type() == sajson::TYPE_OBJECT)
                stats_recursive(tmp, data);
            else stats_single(tmp, data);
        }
    } else {
        stats_single(val, data);
    }
}

u64 stats_measure_sajson(const char *json, size_t size, stats_data *data, int repeat) {
    benchmark_tick_init();
    
    char *buf = (char *)malloc(size);
    memcpy((void *)buf, (void *)json, size);
    const sajson::document& doc = sajson::parse(sajson::dynamic_allocation(),
                                                sajson::mutable_string_view(size, buf));
    const sajson::value& root = doc.get_root();
    
    for (int i = 0; i < repeat; i++) {
        memset(data, 0, sizeof(stats_data));
        benchmark_tick_begin();
        stats_recursive(root, data);
        benchmark_tick_end();
    }
    
    free(buf);
    
    return benchmark_tick_min();
}



}
