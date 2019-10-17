#include "main.hpp"

#include "test_group_by.hpp"
#include "test_nested_observable.hpp"

Engine::Engine() : frames_(frame_subject_.get_subscriber())
{
    transactions_ = 0;
    total_transactions_ = 0;
}

void Engine::setup()
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("%^[%H:%M:%S.%e] [%t] [%l] %v%$");

    main_worker_ = rxcpp::rxsc::make_run_loop(loop_).create_worker();
    job_worker_ = rxcpp::schedulers::make_scheduler<rxcpp::schedulers::new_thread>().create_worker();

    auto frame_stream = frame_subject_.get_observable() |
        rxcpp::rxo::observe_on(rxcpp::identity_same_worker(job_worker_)) |
        rxcpp::rxo::tap([](size_t frame) {
            spdlog::debug("Start stream: {}", frame);
        }) |
        rxcpp::rxo::publish() |
        rxcpp::rxo::ref_count();

    frame_stream | test_group_by();
    frame_stream | test_nested_observable();

    ++total_transactions_;
    frame_stream |
        rxcpp::rxo::subscribe<size_t>(
            lifetime_,
            [this](size_t frame) {
                spdlog::debug("End stream: {}", frame);
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

    start_time_ = std::chrono::high_resolution_clock::now();
}

void Engine::update()
{
    if (is_interactive_mode_)
    {
        frames_.on_next(++current_frame_);
    }
    else
    {
        if (transactions_ == 0)
        {
            transactions_ = total_transactions_.load();
            frames_.on_next(++current_frame_);
        }
    }

    while (!loop_.empty() && loop_.peek().when < loop_.now())
    {
        loop_.dispatch();
    }
}

bool Engine::render()
{
    if (!is_interactive_mode_)
    {
        auto transactions = transactions_.load();
        if (transactions < 0)
        {
            spdlog::error("Invalid transactions: {}", transactions);
            transactions_ = 0;
        }
        else if (transactions > 0)
        {
            return false;
        }
    }

    spdlog::info("Swap Buffer: {}", current_frame_);
    std::this_thread::sleep_for(std::chrono::microseconds(10));

    return true;
}

auto Engine::calc_fps() -> std::function<rxcpp::observable<double>(rxcpp::observable<size_t>)>
{
    return [this](rxcpp::observable<size_t> frames) {
        return frames |
            rxcpp::rxo::map([this](size_t frame) {
                spdlog::info("Calculate FPS on frame {}", frame);
                return frame / std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time_).count() * 1000.0;
            });
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
        engine.update();
        engine.render();
    }

    return 0;
}
