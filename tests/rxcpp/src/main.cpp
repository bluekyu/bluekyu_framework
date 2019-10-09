#include <rxcpp/rx.hpp>

#include <spdlog/spdlog.h>

class Engine
{
public:
    Engine() : frames_(frame_subject_.get_subscriber())
    {
    }

    void setup()
    {
        spdlog::set_level(spdlog::level::debug);
        spdlog::set_pattern("%^[%H:%M:%S.%e] [%t] [%l] %v%$");

        main_worker_ = std::make_unique<rxcpp::observe_on_one_worker>(rxcpp::rxsc::make_run_loop(loop_));
        job_worker_ = std::make_unique<rxcpp::observe_on_one_worker>(rxcpp::schedulers::make_scheduler<rxcpp::schedulers::new_thread>());

        frame_subject_.get_observable() |
            rxcpp::rxo::observe_on(*job_worker_) |
            rxcpp::rxo::tap([](size_t frame) {
                spdlog::debug("Start frame {}", frame);
            }) |

            test_group_by() |
            //test_nested_observable() |

            //test_filter() |

            rxcpp::rxo::subscribe<size_t>(
                lifetime_,
                [](size_t frame) {
                    spdlog::debug("End frame {}", frame);
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

        start_time_ = std::chrono::high_resolution_clock::now();
    }

    auto test_group_by() -> std::function<rxcpp::observable<size_t>(rxcpp::observable<size_t>)>;
    auto test_nested_observable() -> std::function<rxcpp::observable<size_t>(rxcpp::observable<size_t>)>;

    // misuse cases
    /*
    auto test_filter() -> std::function<rxcpp::observable<size_t>(rxcpp::observable<size_t>)>;
    */

    void render()
    {
        spdlog::info("Call {}-th render()", ++current_frame_);
        frames_.on_next(current_frame_);

        while (!loop_.empty() && loop_.peek().when < loop_.now())
        {
            loop_.dispatch();
        }
    }

private:
    rxcpp::composite_subscription lifetime_;

    rxcpp::schedulers::run_loop loop_;
    std::unique_ptr<rxcpp::observe_on_one_worker> main_worker_;
    std::unique_ptr<rxcpp::observe_on_one_worker> job_worker_;

    size_t current_frame_ = 0;
    std::chrono::high_resolution_clock::time_point start_time_;

    rxcpp::subjects::subject<size_t> frame_subject_;
    rxcpp::subscriber<size_t> frames_;
};

auto Engine::test_group_by() -> std::function<rxcpp::observable<size_t>(rxcpp::observable<size_t>)>
{
    return [this](rxcpp::observable<size_t> frames) {
        return frames |
            rxcpp::rxo::group_by(
                [](size_t frame) {
                    return frame % 2 == 0;
                },
                [](size_t frame) {
                    return frame;
                }) |
            rxcpp::rxo::map([this](rxcpp::grouped_observable<bool, size_t> group) -> rxcpp::observable<size_t> {
                if (group.get_key())
                {
                    spdlog::info("{}", __LINE__);  // NOTE: this is called only once
                    return group |
                        rxcpp::rxo::observe_on(*main_worker_) |
                        rxcpp::rxo::map([](size_t frame) {
                            spdlog::info("Even frame {}", frame);
                            std::this_thread::sleep_for(std::chrono::milliseconds(5));
                            return frame;
                        });
                }
                else
                {
                    spdlog::info("{}", __LINE__);  // NOTE: this is called only once
                    return group |
                        rxcpp::rxo::observe_on(*job_worker_) |
                        rxcpp::rxo::map([](size_t frame) {
                            spdlog::warn("Odd frame {}", frame);
                            return frame;
                        });
                }
            }) |
            rxcpp::rxo::merge() |
            rxcpp::rxo::observe_on(*job_worker_);
        // stream is out of order.
    };
}

auto Engine::test_nested_observable() -> std::function<rxcpp::observable<size_t>(rxcpp::observable<size_t>)>
{
    return [this](rxcpp::observable<size_t> frames) {
        spdlog::error("{}", __LINE__);

        return frames |
            rxcpp::rxo::map([this](size_t frame) {
                spdlog::info("FPS Calc on {}", frame);
                return frame / std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time_).count() * 1000.0;
            }) |
            rxcpp::rxo::map([=](double fps) -> rxcpp::observable<std::tuple<std::string, double>> {
                spdlog::error("{}", __LINE__);
                return rxcpp::rxs::just(size_t(1)) |
                    rxcpp::rxo::map([=](size_t frame) {
                        spdlog::warn("FPS {} on {}", fps, frame);
                        return std::make_tuple(std::to_string(frame), fps);
                    });
            }) |
            rxcpp::rxo::merge() |
            rxcpp::rxo::map(rxcpp::rxu::apply_to([](std::string message, double fps) {
                return static_cast<size_t>(std::stol(message));
            }));
    };
}

// misuse case
/*
auto Engine::test_filter() -> std::function<rxcpp::observable<size_t>(rxcpp::observable<size_t>)>
{
    return [this](rxcpp::observable<size_t> frames) {
        spdlog::error("{}", __LINE__);

        auto even_stream =
            frames |
            rxcpp::rxo::filter([](size_t frame) {
                return frame % 2 == 0;
            }) |
            rxcpp::rxo::observe_on(*main_worker_) |
            rxcpp::rxo::map([](size_t frame) {
                spdlog::info("Even frame {}", frame);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                return frame;
            }) |
            rxcpp::rxo::publish() |
            rxcpp::rxo::ref_count() | rxcpp::rxo::as_dynamic();

        auto odd_stream =
            frames |
            rxcpp::rxo::filter([](size_t frame) {
                return frame % 2 != 0;
            }) |
            rxcpp::rxo::observe_on(*job_worker_) |
            rxcpp::rxo::map([](size_t frame) {
                spdlog::warn("Odd frame {}", frame);
                return frame;
            }) |
            rxcpp::rxo::publish() |
            rxcpp::rxo::ref_count() | rxcpp::rxo::as_dynamic();

        return even_stream |
            rxcpp::rxo::merge(odd_stream) |
            rxcpp::rxo::observe_on(*job_worker_);
            // stream is out of order.
    };
}
*/

int main(int, char**)
{
    Engine engine;

    engine.setup();

    while (true)
    {
        engine.render();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
