#ifndef FUNNEL_API_REQUESTS_HPP
#define FUNNEL_API_REQUESTS_HPP

#include "protobuf/telegram.pb.h"

#include <json_struct/json_struct.h>

struct RequestInterpolated {
    unsigned int line;
    unsigned int run;

    RequestInterpolated(unsigned int input_line, unsigned int input_run) : line(input_line), run(input_run) {};

    RequestInterpolated() = default;

    JS_OBJ(line, run);
};

#endif //FUNNEL_API_REQUESTS_HPP
