#include "benchmark.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/reader.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

using namespace rapidjson;


template <typename Encoding = UTF8<> >
class StatHandler : public BaseReaderHandler<Encoding, StatHandler<Encoding> > {
public:
    typedef typename Encoding::Ch Ch;
    
    StatHandler(stats_data& stat) : stat_(stat) {}
    
    bool Null() { stat_.num_null++; return true; }
    bool Bool(bool b) { if (b) stat_.num_true++; else stat_.num_false++; return true; }
    bool Int(int) { stat_.num_number++; return true; }
    bool Uint(unsigned) { stat_.num_number++; return true; }
    bool Int64(int64_t) { stat_.num_number++; return true; }
    bool Uint64(uint64_t) { stat_.num_number++; return true; }
    bool Double(double) { stat_.num_number++; return true; }
    bool String(const Ch*, SizeType length, bool) { stat_.num_string++; return true; }
    bool StartObject() { return true; }
    bool EndObject(SizeType memberCount) { stat_.num_object++; return true; }
    bool StartArray() { return true; }
    bool EndArray(SizeType elementCount) { stat_.num_array++; return true; }
    
private:
    StatHandler& operator=(const StatHandler&);
    
    stats_data& stat_;
};

extern "C" {

// -----------------------------------------------------------------------------
// reader

u64 reader_measure_rapidjson(const char *json, size_t size, int repeat) {
    benchmark_tick_init();
    
    Document doc;
    for (int i = 0; i < repeat; i++) {
        benchmark_tick_begin();
        doc.Parse<kParseValidateEncodingFlag | kParseFullPrecisionFlag>(json, size);
        benchmark_tick_end();
        if (doc.HasParseError()) return 0;
    }
    
    return benchmark_tick_min();
}

u64 reader_measure_rapidjson_fast(const char *json, size_t size, int repeat) {
    benchmark_tick_init();
    
    char *buf = (char *)malloc(size + 1);
    Document doc;
    for (int i = 0; i < repeat; i++) {
        memcpy((void *)buf, (void *)json, size);
        buf[size] = '\0';
        benchmark_tick_begin();
        doc.ParseInsitu(buf);
        benchmark_tick_end();
        if (doc.HasParseError()) return 0;
    }
    free((void *)buf);
    
    return benchmark_tick_min();
}

// -----------------------------------------------------------------------------
// writer

u64 writer_measure_rapidjson(const char *json, size_t size, size_t *out_size,
                             bool *roundtrip, bool pretty, int repeat) {
    benchmark_tick_init();
    
    Document doc;
    doc.Parse<kParseValidateEncodingFlag | kParseFullPrecisionFlag>(json, size);
    
    bool processed = false;
    if (pretty) {
        for (int i = 0; i < repeat; i++) {
            StringBuffer sb;
            Writer<StringBuffer> writer(sb);
            benchmark_tick_begin();
            doc.Accept(writer);
            benchmark_tick_end();
            if (!processed) {
                processed = true;
                *out_size = sb.GetSize();
                *roundtrip = (*out_size == size && memcmp(json, sb.GetString(), size) == 0);
            }
        }
    } else {
        for (int i = 0; i < repeat; i++) {
            StringBuffer sb;
            PrettyWriter<StringBuffer> writer(sb);
            benchmark_tick_begin();
            doc.Accept(writer);
            benchmark_tick_end();
            *out_size = sb.GetSize();
            if (!processed) {
                processed = true;
                *out_size = sb.GetSize();
                *roundtrip = (*out_size == size && memcmp(json, sb.GetString(), size) == 0);
            }
        }
    }
    
    return benchmark_tick_min();
}


// -----------------------------------------------------------------------------
// stats

u64 stats_measure_rapidjson_fast(const char *json, size_t size, stats_data *data, int repeat) {
    benchmark_tick_init();
    
    Document doc;
    doc.Parse<kParseValidateEncodingFlag | kParseFullPrecisionFlag>(json, size);
    
    for (int i = 0; i < repeat; i++) {
        memset(data, 0, sizeof(stats_data));
        StatHandler<> handler(*data);
        benchmark_tick_begin();
        doc.Accept(handler);
        benchmark_tick_end();
    }
    
    return benchmark_tick_min();
}



static void GenStat(stats_data& stat, const Value& v) {
    switch (v.GetType()) {
        case kNullType:  stat.num_null++; break;
        case kFalseType: stat.num_false++; break;
        case kTrueType:  stat.num_true++; break;
            
        case kObjectType:
            stat.num_object++;
            for (Value::ConstMemberIterator m = v.MemberBegin(); m != v.MemberEnd(); ++m) {
                stat.num_string++;
                GenStat(stat, m->value);
            }
            break;
            
        case kArrayType:
            stat.num_array++;
            for (Value::ConstValueIterator i = v.Begin(); i != v.End(); ++i)
            GenStat(stat, *i);
            break;
            
        case kStringType:
            stat.num_string++;
            break;
            
        case kNumberType:
            stat.num_number++;
            break;
    }
}

u64 stats_measure_rapidjson(const char *json, size_t size, stats_data *data, int repeat) {
    benchmark_tick_init();
    
    Document doc;
    doc.Parse<kParseValidateEncodingFlag | kParseFullPrecisionFlag>(json, size);
    
    for (int i = 0; i < repeat; i++) {
        memset(data, 0, sizeof(stats_data));
        benchmark_tick_begin();
        GenStat(*data, doc);
        benchmark_tick_end();
    }
    
    return benchmark_tick_min();
}



}
