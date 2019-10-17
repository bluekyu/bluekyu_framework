#pragma once

#include "main.hpp"

inline auto Engine::test_nested_observable() -> std::function<void(rxcpp::observable<size_t>)>
{
    return [this](rxcpp::observable<size_t> frames) {
        spdlog::error("test_nested_observable()");
        ++total_transactions_;

        frames |
            rxcpp::rxo::map([=](size_t frame) {
                spdlog::info("nested observable");
                return rxcpp::rxs::just(frame) |
                    calc_fps() |
                    rxcpp::rxo::map([=](double fps) {
                        spdlog::warn("FPS {} on {}", fps, frame);
                        return std::make_tuple(frame, fps);
                    });
            }) |
            rxcpp::rxo::merge() |
            rxcpp::rxo::map(rxcpp::rxu::apply_to([](size_t frame, double) {
                return static_cast<size_t>(frame);
            })) |
            rxcpp::rxo::observe_on(get_main_worker()) |
            rxcpp::rxo::subscribe<size_t>(
                lifetime_,
                [this](size_t frame) {
                    sub_transactions();
                },
                [](std::exception_ptr eptr) {
                    try
                    {
                        std::rethrow_exception(eptr);
                    }
                    catch (std::exception err)
                    {
                        spdlog::critical("{}", err.what());
                    }
                });
    };
}
