//
// Created by revol-xut on 11/19/22.
//

#ifndef FUNNEL_PROMETHEUS_HPP
#define FUNNEL_PROMETHEUS_HPP

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include <memory>

class PrometheusExporter {
private:
    std::shared_ptr<prometheus::Registry> registry_;
    std::unique_ptr<prometheus::Exposer> exposer_;
public:

    PrometheusExporter() noexcept;
    ~PrometheusExporter() noexcept = default;

    auto get_open_connections() noexcept -> prometheus::Family<prometheus::Counter>&;

};


#endif //FUNNEL_PROMETHEUS_HPP
