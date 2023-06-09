#include "pch.h"
#include "../benchmark.h"
#include <winrt/Windows.Data.Json.h>

using namespace winrt;
using namespace Windows::Foundation;

extern "C"
{
    u64 reader_measure_winrt_json(const char* json, size_t size, int repeat);
    u64 writer_measure_winrt_json(const char* json, size_t size, size_t* out_size,
        bool* roundtrip, bool pretty, int repeat);
    u64 stats_measure_winrt_json(const char* json, size_t size, stats_data* data, int repeat);
}
u64 reader_measure_winrt_json(const char* json, size_t size, int repeat)
{
    benchmark_tick_init();
    winrt::hstring jsonW = winrt::to_hstring(json);

    try {
        for (int i = 0; i < repeat; ++i)
        {
            benchmark_tick_begin();
            auto parsed = winrt::Windows::Data::Json::JsonObject::Parse(jsonW);
            benchmark_tick_end();
        }
        return benchmark_tick_min();
    }
    catch (...)
    {
        return 0;
    }
}

u64 writer_measure_winrt_json(const char* json, size_t size, size_t* out_size,
    bool* roundtrip, bool pretty, int repeat)
{
    benchmark_tick_init();
    auto jsonW = winrt::to_hstring(json);
    try {
        auto parsed = winrt::Windows::Data::Json::JsonObject::Parse(jsonW);

        for (int i = 0; i < repeat; ++i)
        {
            auto str = parsed.ToString();
            *out_size = str.size() * 2;
            *roundtrip = (*out_size == jsonW.size() && memcmp(jsonW.data(), str.data(), size));
        }
        return benchmark_tick_min();
    }
    catch (...)
    {
        return 0;
    }
}

static void stats_json_object_recursive(winrt::Windows::Data::Json::JsonObject object, stats_data* data)
{
    for (auto pair : object)
    {
        auto value = pair.Value();
        switch (value.ValueType())
        {
            case winrt::Windows::Data::Json::JsonValueType::Array: ++data->num_array; break;
            case winrt::Windows::Data::Json::JsonValueType::Boolean:
                if (value.GetBoolean())
                    ++data->num_true;
                else
                    ++data->num_false;
                break;
            case winrt::Windows::Data::Json::JsonValueType::Null: ++data->num_null; break;
            case winrt::Windows::Data::Json::JsonValueType::Number: ++data->num_number; break;
            case winrt::Windows::Data::Json::JsonValueType::Object: 
                ++data->num_object; 
                stats_json_object_recursive(value.GetObjectW(), data); 
                break;
            case winrt::Windows::Data::Json::JsonValueType::String: ++data->num_string; break;
            default:
                break;
        }
    }
}

u64 stats_measure_winrt_json(const char* json, size_t size, stats_data* data, int repeat)
{
    benchmark_tick_init();
    auto jsonW = winrt::to_hstring(json);
    try {
        auto parsed = winrt::Windows::Data::Json::JsonObject::Parse(jsonW);
        for (int i = 0; i < repeat; ++i)
        {
            memset(data, 0, sizeof(stats_data));
            benchmark_tick_begin();
            stats_json_object_recursive(parsed, data);
            benchmark_tick_end();
        }
    }
    catch (...) {}
    return benchmark_tick_min();
}
