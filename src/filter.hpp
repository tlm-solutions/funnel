#ifndef FUNNEL_FILTER_HPP
#define FUNNEL_FILTER_HPP

#include <algorithm>
#include <utility>
#include <vector>

#include <json_struct/json_struct.h>

struct Filter {
    std::vector<unsigned int> lines;
    std::vector<unsigned int> regions;

    JS_OBJ(lines, regions);

    Filter(
            std::vector<unsigned int> other_line,
            std::vector<unsigned int> other_region)
        : lines(std::move(other_line)), regions(std::move(other_region)) {}

    Filter() = default;

    [[nodiscard]] auto match(unsigned int other_line, unsigned int other_region) const noexcept -> bool {
        bool matches_line =
                (std::find(std::begin(lines), std::end(lines), other_line) != std::end(lines)) || lines.empty();
        bool matches_region = (std::find(std::begin(regions), std::end(regions), other_region) != std::end(regions)) ||
                              regions.empty();
        return matches_line && matches_region;
    }
};


#endif //FUNNEL_FILTER_HPP
