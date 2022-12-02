#ifndef FUNNEL_API_REQUESTS_HPP
#define FUNNEL_API_REQUESTS_HPP

#include "protobuf/telegram.pb.h"

#include <algorithm>
#include <vector>
#include <map>
#include <cinttypes>
#include <unordered_map>

#include <json_struct/json_struct.h>

struct RequestInterpolated {
    unsigned int line;
    unsigned int run;

    RequestInterpolated(unsigned int input_line, unsigned int input_run): line(input_line), run(input_run) {};
    RequestInterpolated() = default;

    JS_OBJ(line, run);
};

struct Epsg3857 {
    float x;
    float y;

    JS_OBJ(x, y);
};

struct Position {
    float lat;
    float lon;
    std::unordered_map<std::string, Epsg3857> properties;

    JS_OBJ(lat, lon, properties);
};

struct ResponseInterpolated {
    unsigned int historical_time;
    unsigned int next_reporting_point;
    std::unordered_map<std::string, Position> positions;

    JS_OBJ(historical_time, next_reporting_point, positions);
};


#endif //FUNNEL_API_REQUESTS_HPP
