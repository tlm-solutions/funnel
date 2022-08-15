
#ifndef FUNNEL_FILTER_HPP
#define FUNNEL_FILTER_HPP

#include <algorithm>
#include <vector>

#include <json_struct/json_struct.h>

struct Filter {
    std::vector<unsigned int> lines;
    std::vector<unsigned int> positions;
    std::vector<unsigned int> regions;

    JS_OBJ(lines, positions, regions);

    Filter(
            const std::vector<unsigned int>& other_line, 
            const std::vector<unsigned int>& other_position, 
            const std::vector<unsigned int>& other_region
    ): lines(other_line), positions(other_position), regions(other_region) {}

    Filter() = default;

    auto match(unsigned int other_line, unsigned int other_position, unsigned int other_region) const noexcept -> bool {
        if (lines.empty() && positions.empty() && regions.empty()) {
            return true;
        }

        bool matches_line = std::find(std::begin(lines), std::end(lines), other_line) == std::end(lines);
        bool matches_position = std::find(std::begin(positions), std::end(positions), other_position) == std::end(positions);
        bool matches_region = std::find(std::begin(regions), std::end(regions), other_region) == std::end(regions);
        std::cout << "data:" << other_line << "|" << other_position << "|" << other_region << std::endl;
        std::cout << "matches:" << matches_line << "|" << matches_position << "|" << matches_region << std::endl;
        return matches_line && matches_region && matches_position;
    }
};



#endif //FUNNEL_FILTER_HPP
