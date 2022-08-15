
#ifndef FUNNEL_FILTER_HPP
#define FUNNEL_FILTER_HPP

#include <algorithm>
#include <vector>

class Filter {
    private:
        std::vector<unsigned int> lines_;
        std::vector<unsigned int> positions_;
        std::vector<unsigned int> regions_; 

    public:
        Filter(
                const std::vector<unsigned int>& line, 
                const std::vector<unsigned int>& position, 
                const std::vector<unsigned int>& region
        ): lines_(line), positions_(position), regions_(region) {}

        Filter() = default;

        auto match(unsigned int line, unsigned int position, unsigned int region) const noexcept -> bool {
            if (lines_.empty() && positions_.empty() && regions_.empty()) {
                return true;
            }

            bool matches_line = std::find(std::begin(lines_), std::end(lines_), line) == std::end(lines_);
            bool matches_position = std::find(std::begin(positions_), std::end(positions_), position) == std::end(positions_);
            bool matches_region = std::find(std::begin(regions_), std::end(regions_), region) == std::end(regions_);

            return matches_line && matches_region && matches_position;
        }
};



#endif //FUNNEL_FILTER_HPP
