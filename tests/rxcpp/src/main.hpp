#pragma once

#include <rxcpp/rx.hpp>

#include <spdlog/spdlog.h>

class Engine
{
public:
    Engine();

    void setup();
    void update();
    bool render();

    auto test_group_by() -> std::function<void(rxcpp::observable<size_t>)>;
    auto test_nested_observable() -> std::function<void(rxcpp::observable<size_t>)>;

    // misuse cases
    /*
    auto test_filter() -> std::function<rxcpp::observable<size_t>(rxcpp::observable<size_t>)>;
    */

    /**
     * custom operators
     */
    auto calc_fps() -> std::function<rxcpp::observable<double>(rxcpp::observable<size_t>)>;

private:
    rxcpp::identity_one_worker get_main_worker() const;
    rxcpp::identity_one_worker get_job_worker() const;

    void sub_transactions();

    rxcpp::composite_subscription lifetime_;

    rxcpp::schedulers::run_loop loop_;
    rxcpp::rxsc::worker main_worker_;
    rxcpp::rxsc::worker job_worker_;

    size_t current_frame_ = 0;
    bool is_interactive_mode_ = true;
    std::atomic_int32_t transactions_;
    std::atomic_int32_t total_transactions_;

    std::chrono::high_resolution_clock::time_point start_time_;

    rxcpp::subjects::subject<size_t> frame_subject_;
    rxcpp::subscriber<size_t> frames_;
};

inline rxcpp::identity_one_worker Engine::get_main_worker() const
{
    return rxcpp::identity_same_worker(main_worker_);
}

inline rxcpp::identity_one_worker Engine::get_job_worker() const
{
    return rxcpp::identity_same_worker(job_worker_);
}

inline void Engine::sub_transactions()
{
    if (!is_interactive_mode_)
        --transactions_;
}
