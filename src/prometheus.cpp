//
// Created by revol-xut on 11/19/22.
//

#include "prometheus.hpp"
#include <iostream>

PrometheusExporter::PrometheusExporter() noexcept {
    std::string exporter_host;
    if (const char* host = std::getenv("EXPORTER_HOST")) {
        exporter_host = std::string(host);
    } else {
        std::cout << "no EXPORTER_HOST specified defaulting to 127.0.0.1:9010" << std::endl;
        exporter_host = "127.0.0.1:9010";
    }

    exposer_ = std::make_unique<prometheus::Exposer>(exporter_host);
    registry_ = std::make_shared<prometheus::Registry>();


    exposer_->RegisterCollectable(registry_);
}

auto PrometheusExporter::get_opened_connections() noexcept -> prometheus::Family<prometheus::Counter> & {
    return prometheus::BuildCounter()
            .Name("open_funnel_connections")
            .Help("Number of Open Websocket Connections")
            .Register(*registry_);
}

auto PrometheusExporter::get_closed_connections() noexcept -> prometheus::Family<prometheus::Counter> & {
    return prometheus::BuildCounter()
            .Name("closed_funnel_connections")
            .Help("Number of Closed Websocket Connections")
            .Register(*registry_);
}

