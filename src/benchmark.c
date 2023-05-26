#include "benchmark.h"

static int reader_num = 0;
static const char *reader_names[64];
static int reader_name_max = 0;
static reader_measure_func reader_funcs[64];

static int writer_num = 0;
static const char *writer_names[64];
static int writer_name_max = 0;
static writer_measure_func writer_funcs[64];

static int stats_num = 0;
static const char *stats_names[64];
static int stats_name_max = 0;
static stats_measure_func stats_funcs[64];


static void func_register_all(void) {
#define register_reader(name) \
    extern u64 reader_measure_##name(const char *json, size_t size, int repeat); \
    reader_funcs[reader_num] = reader_measure_##name; \
    reader_names[reader_num] = #name; \
    reader_num++; \
    if ((int)strlen(#name) > reader_name_max) reader_name_max = (int)strlen(#name);
    
#define register_writer(name) \
    extern u64 writer_measure_##name(const char *json, size_t size, size_t *out_size, \
                                     bool *roundtrip, bool pretty, int repeat); \
    writer_funcs[writer_num] = writer_measure_##name; \
    writer_names[writer_num] = #name; \
    writer_num++; \
    if ((int)strlen(#name) > writer_name_max) writer_name_max = (int)strlen(#name);
        
#define register_stats(name) \
    extern u64 stats_measure_##name(const char *json, size_t size, stats_data *data, int repeat); \
    stats_funcs[stats_num] = stats_measure_##name; \
    stats_names[stats_num] = #name; \
    stats_num++; \
    if ((int)strlen(#name) > stats_name_max) stats_name_max = (int)strlen(#name);
    
    
    
    // fast
    register_reader(yyjson_fast); // validate_encoding, insitu, fast_fp
    register_reader(yyjson);      // validate_encoding, full_precision_fp
    //register_writer(yyjson);      // immutable writer
    //register_stats(yyjson);       // stats recursive
    
#if BENCHMARK_HAS_SIMDJSON
    register_reader(simdjson);
    //register_writer(simdjson); // immutable writer, minify only
    //register_stats(simdjson);
#endif
    
    register_reader(rapidjson);       // validate_encoding, full_precision_fp
    //register_writer(rapidjson);
    //register_stats(rapidjson);        // stats recursive

#if BENCHMARK_HAS_WINRT
    register_reader(winrt_json);
    //register_writer(winrt_json);
#endif
    
    // full
    /*
    register_reader(yyjson_fast); // validate_encoding, insitu, fast_fp
    register_reader(yyjson);      // validate_encoding, full_precision_fp
    register_writer(yyjson);      // immutable writer
    register_writer(yyjson_mut);  // mutable writer
    register_stats(yyjson_fast);  // stats iterator
    register_stats(yyjson);       // stats recursive

#if BENCHMARK_HAS_SIMDJSON
    register_reader(simdjson);
    register_writer(simdjson); // immutable writer, minify only
    register_stats(simdjson);
#endif
    
    register_reader(sajson);
    register_reader(sajson_dynamic);
    register_stats(sajson);

    register_reader(rapidjson);       // validate_encoding, full_precision_fp
    register_reader(rapidjson_fast);  // no_validate_encoding, insitu, fast_fp
    register_writer(rapidjson);
    register_stats(rapidjson_fast);   // stats with handler
    register_stats(rapidjson);        // stats recursive

    register_reader(cjson);
    register_writer(cjson);
    register_stats(cjson);

    register_reader(jansson);
    register_writer(jansson);
    register_stats(jansson);
     */
}

static void func_cleanup(void) {
    reader_num = 0;
    reader_name_max = 0;
    writer_num = 0;
    writer_name_max = 0;
    stats_num = 0;
    stats_name_max = 0;
}

static int get_repeat_count(usize len) {
    return (len > 16 * 1024 * 1024) ? 4 : 16;
}

static void setup_chart_column_option(yy_chart_options *op) {
    op->type = YY_CHART_COLUMN;
    op->h_axis.categories = reader_names;
    op->h_axis.allow_decimals = true;
    op->plot.value_labels_enabled = false;
    op->plot.value_labels_decimals = 2;
    op->plot.color_by_point = false;
    op->plot.group_padding = 0.1f;
    op->plot.point_padding = 0.0f;
    op->plot.border_width = 0.0f;
    op->legend.enabled = true;
    op->tooltip.value_decimals = 2;
    op->width = 800;
    op->height = 350;
}


static void run_reader_benchmark(yy_report *report, char **file_paths, int file_count) {
    yy_chart_options op;
    
    yy_chart_options_init(&op);
    setup_chart_column_option(&op);
    op.h_axis.categories = reader_names;
    
    // bytes per second
    op.title = "JSON reader";
    op.subtitle = "gigabytes per second (larger is better)";
    op.v_axis.title = "GB/s";
    op.tooltip.value_suffix = " GB/s";
    
    yy_chart *chart_bps = yy_chart_new();
    yy_chart_set_options(chart_bps, &op);
    yy_report_add_chart(report, chart_bps);
    
    // cycles per byte
    op.title = "JSON reader (cpb)";
    op.subtitle = "cycles per byte (smaller is better)";
    op.v_axis.title = "cycles";
    op.tooltip.value_suffix = " cycles/byte";
    
    yy_chart *chart_cpb = yy_chart_new();
    yy_chart_set_options(chart_cpb, &op);
    yy_report_add_chart(report, chart_cpb);
    
    printf("benchmark reader...\n");
    for (int f = 0; f < file_count; f++) {
        char file_name[YY_MAX_PATH];
        char *file_path = file_paths[f];
        yy_path_get_last(file_name, file_path);
        if (!yy_str_has_suffix(file_name, ".json")) continue;
        printf("    %s\n", file_name);
        yy_path_remove_ext(file_name, file_name);
        
        char *dat;
        usize len;
        if (!yy_file_read(file_path, (u8 **)&dat, &len)) {
            printf("cannot read file: %s\n", file_path);
            continue;
        }
        
        yy_chart_item_begin(chart_bps, file_name);
        yy_chart_item_begin(chart_cpb, file_name);
        
        for (int i = 0; i < reader_num; i++) {
            reader_measure_func func = reader_funcs[i];
            int repeat = get_repeat_count(len);
            
            u64 ticks = func(dat, len, repeat);
            
            f64 cycles = (f64)ticks * yy_cpu_get_cycle_per_tick();
            f64 cycles_per_byte = cycles / (f64)len;
            yy_chart_item_add_float(chart_cpb, (f32)cycles_per_byte);
            
            f64 gb_per_sec = (f64)len / ((f64)ticks / yy_cpu_get_tick_per_sec()) / 1024.0 / 1024.0 / 1024.0;
            yy_chart_item_add_float(chart_bps, (f32)gb_per_sec);
            
        }
        
        yy_chart_item_end(chart_bps);
        yy_chart_item_end(chart_cpb);
        free(dat);
    }
    
    yy_chart_free(chart_bps);
    yy_chart_free(chart_cpb);
}



static void run_writer_benchmark(yy_report *report, char **file_paths, int file_count) {
    yy_chart_options op;
    
    yy_chart_options_init(&op);
    setup_chart_column_option(&op);
    op.h_axis.categories = writer_names;
    
    // pretty
    op.title = "JSON writer pretty";
    op.subtitle = "gigabytes per second (larger is better)";
    op.v_axis.title = "GB/s";
    op.tooltip.value_suffix = " GB/s";
    
    yy_chart *chart_pretty = yy_chart_new();
    yy_chart_set_options(chart_pretty, &op);
    yy_report_add_chart(report, chart_pretty);
    
    // minify
    op.title = "JSON writer minify";
    op.subtitle = "gigabytes per second (larger is better)";
    op.v_axis.title = "GB/s";
    op.tooltip.value_suffix = " GB/s";
    
    yy_chart *chart_minify = yy_chart_new();
    yy_chart_set_options(chart_minify, &op);
    yy_report_add_chart(report, chart_minify);
    
    printf("benchmark writer...\n");
    for (int f = 0; f < file_count; f++) {
        char file_name[YY_MAX_PATH];
        char *file_path = file_paths[f];
        yy_path_get_last(file_name, file_path);
        if (!yy_str_has_suffix(file_name, ".json")) continue;
        printf("    %s\n", file_name);
        yy_path_remove_ext(file_name, file_name);
        
        char *dat;
        usize len;
        if (!yy_file_read(file_path, (u8 **)&dat, &len)) {
            printf("cannot read file: %s\n", file_path);
            continue;
        }
        
        yy_chart_item_begin(chart_pretty, file_name);
        yy_chart_item_begin(chart_minify, file_name);
        
        for (int i = 0; i < writer_num; i++) {
            writer_measure_func func = writer_funcs[i];
            int repeat = get_repeat_count(len);
            
            usize out_size;
            bool roundtrip;
            u64 ticks = func(dat, len, &out_size, &roundtrip, true, repeat);
            f64 gb_per_sec = (f64)out_size / ((f64)ticks / yy_cpu_get_tick_per_sec()) / 1024.0 / 1024.0 / 1024.0;
            yy_chart_item_add_float(chart_pretty, (f32)gb_per_sec);
            
            ticks = func(dat, len, &out_size, &roundtrip, false, repeat);
            gb_per_sec = (f64)out_size / ((f64)ticks / yy_cpu_get_tick_per_sec()) / 1024.0 / 1024.0 / 1024.0;
            yy_chart_item_add_float(chart_minify, (f32)gb_per_sec);
        }
        
        yy_chart_item_end(chart_pretty);
        yy_chart_item_end(chart_minify);
        free(dat);
    }
    
    yy_chart_free(chart_pretty);
    yy_chart_free(chart_minify);
}



static void run_stats_benchmark(yy_report *report, char **file_paths, int file_count) {
    yy_chart_options op;
    
    yy_chart_options_init(&op);
    setup_chart_column_option(&op);
    op.h_axis.categories = stats_names;
    
    op.title = "JSON stats";
    op.subtitle = "value count per seconds (larger is better)";
    op.v_axis.title = "value count";
    
    yy_chart *chart = yy_chart_new();
    yy_chart_set_options(chart, &op);
    yy_report_add_chart(report, chart);
    
    printf("benchmark stats...\n");
    for (int f = 0; f < file_count; f++) {
        char file_name[YY_MAX_PATH];
        char *file_path = file_paths[f];
        yy_path_get_last(file_name, file_path);
        if (!yy_str_has_suffix(file_name, ".json")) continue;
        printf("    %s\n", file_name);
        yy_path_remove_ext(file_name, file_name);
        
        char *dat;
        usize len;
        if (!yy_file_read(file_path, (u8 **)&dat, &len)) {
            printf("cannot read file: %s\n", file_path);
            continue;
        }
        
        yy_chart_item_begin(chart, file_name);
        
        int total_num_cmp = 0;
        for (int i = 0; i < stats_num; i++) {
            stats_measure_func func = stats_funcs[i];
            int repeat = get_repeat_count(len);
            stats_data data;
            u64 ticks = func(dat, len, &data, repeat);
            int total_num = data.num_null + data.num_true + data.num_false + data.num_number +
                            data.num_string + data.num_array + data.num_object;
            if (!total_num_cmp) total_num_cmp = total_num;
            else if (total_num_cmp != total_num) printf("stats not match: %s\n", stats_names[i]);
            f64 seconds = (f64)ticks / yy_cpu_get_tick_per_sec();
            yy_chart_item_add_float(chart, (f32)(total_num / seconds));
        }
        
        yy_chart_item_end(chart);
        free(dat);
    }
    
    yy_chart_free(chart);
}

// RFC 8259 JSON Test Suite
// https://github.com/nst/JSONTestSuite
static void run_conformance_benchmark(void) {
    printf("\n");
    printf("RFC 8259 JSON Test Suite: https://github.com/nst/JSONTestSuite\n");
    
    char path[YY_MAX_PATH];
    yy_path_combine(path, BENCHMARK_DATA_PATH, "data", "parsing", NULL);
    int file_count = 0;
    char **files = yy_dir_read(path, &file_count);
        
    for (int f = 0; f < reader_num; f++) {
        reader_measure_func func = reader_funcs[f];
        const char *func_name = reader_names[f];
        
        int used_count = 0;
        int valid_count = 0;
        for (int i = 0; i < file_count; i++) {
            char *json_name = files[i];
            if (!yy_str_has_suffix(json_name, ".json")) continue;
            
            char json_path[YY_MAX_PATH];
            yy_path_combine(json_path, path, json_name, NULL);
            char *dat;
            usize dat_len;
            if (!yy_file_read(json_path, (u8 **)&dat, &dat_len)) continue;
            used_count++;
            
            if (strncmp(func_name, "rapidjson", strlen("rapidjson")) == 0 &&
                (strcmp(json_name, "n_structure_100000_opening_arrays.json") == 0 ||
                 strcmp(json_name, "n_structure_open_array_object.json") == 0 ||
                 strcmp(json_name, "n_structure_100000_opening_arrays.json") == 0)) {
                // may crash
            } else {
                u64 suc = func(dat, dat_len, 1);
                if (yy_str_has_prefix(json_name, "y_")) { //  must be accepted
                    if (suc) valid_count++;
                } else if (yy_str_has_prefix(json_name, "n_")) { // must be rejected
                    if (!suc) valid_count++;
                } else { // free to accept or reject
                    valid_count++;
                }
            }
            
            free(dat);
        }
        
        printf("%*s  (%d/%d)%s\n",reader_name_max, func_name, valid_count, used_count, valid_count == used_count ? " [OK]" : "");
        
    }
    printf("\n");
    yy_dir_free(files);
}


static void run_all_benchmark(const char *output_path) {
    char path[YY_MAX_PATH];
    yy_path_combine(path, BENCHMARK_DATA_PATH, "data", "json", NULL);
    
    int file_count = 0;
    char **files = yy_dir_read_full(path, &file_count);
    
#if TWITTER_ONLY
    file_count = 1;
    yy_path_combine(path, BENCHMARK_DATA_PATH, "data", "json", "twitter.json", NULL);
    files[0] = path;
#endif

    yy_report *report = yy_report_new();
    yy_report_add_env_info(report);
    
    run_conformance_benchmark();
    run_reader_benchmark(report, files, file_count);
    run_writer_benchmark(report, files, file_count);
    run_stats_benchmark(report, files, file_count);
    
    bool suc = yy_report_write_html_file(report, output_path);
    if (!suc) {
        printf("write report file failed: %s\n", output_path);
    }
    yy_report_free(report);
    
#if !TWITTER_ONLY
    yy_dir_free(files);
#endif
}

void benchmark(const char *output_path) {

    printf("------[prepare]---------\n");
    printf("warmup...\n");
    yy_cpu_setup_priority();
    yy_cpu_spin(0.5);
    yy_cpu_measure_freq();
    func_register_all();
    
    printf("------[benchmark]------\n");
    run_all_benchmark(output_path);
    
    printf("------[finish]---------\n");
    func_cleanup();
}
