//
// Created by revol-xut on 11/19/22.
//

#include "prometheus.hpp"

PrometheusExporter::PrometheusExporter() noexcept {
    unsigned short exporter_port = static_cast<unsigned short>(std::stoi(std::getenv("EXPORTER_PORT")));

    exposer_ = std::make_unique<prometheus::Exposer>("127.0.0.1:" + std::to_string(exporter_port));
    registry_ = std::make_shared<prometheus::Registry>();


    exposer_->RegisterCollectable(registry_);
}

auto PrometheusExporter::get_opened_connections() noexcept -> prometheus::Family<prometheus::Counter>& {
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

