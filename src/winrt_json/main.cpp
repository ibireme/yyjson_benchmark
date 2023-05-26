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
