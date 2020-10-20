#include "benchmark.h"

#if BENCHMARK_HAS_SIMDJSON
#include "simdjson.h"
extern "C" {

// -----------------------------------------------------------------------------
// reader

u64 reader_measure_simdjson(const char *json, size_t size, int repeat) {
    benchmark_tick_init();
    
    simdjson::dom::parser parser;
    simdjson::dom::element root;
    simdjson::error_code error;
    for (int i = 0; i < repeat; i++) {
        benchmark_tick_begin();
        parser.parse(json, size).tie(root, error);
        benchmark_tick_end();
        if (error) return 0;
    }
    
    return benchmark_tick_min();
}



// -----------------------------------------------------------------------------
// writer

u64 writer_measure_simdjson(const char *json, size_t size, size_t *out_size,
                            bool *roundtrip, bool pretty, int repeat) {
    if (pretty) return 0;
    
    benchmark_tick_init();
    
    simdjson::dom::parser parser;
    simdjson::dom::element doc;
    doc = parser.parse(json, size);
    
    bool processed = false;
    for (int i = 0; i < repeat; i++) {
        benchmark_tick_begin();
        auto str = simdjson::minify(doc);
        benchmark_tick_end();
        if (str.length() == 0) return 0;
        if (!processed) {
            processed = true;
            *out_size = str.length();
            *roundtrip = (*out_size == size && memcmp(json, str.c_str(), size) == 0);
        }
    }
    
    return benchmark_tick_min();
}



// -----------------------------------------------------------------------------
// stats

static yy_inline void stats_single(simdjson::dom::element val, stats_data *data) {
    if (val.is<std::string_view>()) {
        data->num_string++;
    } else if (val.is<int64_t>() ||
               val.is<double>()) {
        data->num_number++;
    } else if (val.is<bool>()) {
        simdjson::error_code err;
        bool v;
        err = val.get(v);
        if (v) {
            data->num_true++;
        } else {
            data->num_false++;
        }
    } else if (val.is_null()) {
        data->num_null++;
    }
}

static void stats_recursive(simdjson::dom::element val, stats_data *data) {
    simdjson::error_code error;
    if (val.is<simdjson::dom::array>()) {
        data->num_array++;
        simdjson::dom::array array;
        if ((error = val.get(array))) { std::cerr << error << std::endl; abort(); }
        for (auto child : array) {
            if (child.is<simdjson::dom::array>() || child.is<simdjson::dom::object>()) {
                stats_recursive(child, data);
            } else {
                stats_single(child, data);
            }
        }
    } else if (val.is<simdjson::dom::object>()) {
        data->num_object++;
        simdjson::dom::object object;
        if ((error = val.get(object))) { std::cerr << error << std::endl; abort(); }
        for (auto field : object) {
            data->num_string++;
            if (field.value.is<simdjson::dom::array>() || field.value.is<simdjson::dom::object>()) {
                stats_recursive(field.value, data);
            } else {
                stats_single(field.value, data);
            }
        }
    } else {
        stats_single(val, data);
    }
}

u64 stats_measure_simdjson(const char *json, size_t size, stats_data *data, int repeat) {
    benchmark_tick_init();
    
    simdjson::dom::parser parser;
    simdjson::dom::element doc = parser.parse(json, size);
        
    for (int i = 0; i < repeat; i++) {
        memset(data, 0, sizeof(stats_data));
        benchmark_tick_begin();
        stats_recursive(doc, data);
        benchmark_tick_end();
    }
    
    return benchmark_tick_min();
}



}
#endif
