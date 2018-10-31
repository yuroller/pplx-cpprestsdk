#pragma once

#include <chrono>

class StopWatch
{
public:
	StopWatch()
		: start_time_(std::chrono::steady_clock::now())
	{}

	std::chrono::steady_clock::duration Duration() const {
		return std::chrono::steady_clock::now() - start_time_;
	}

private:
	std::chrono::steady_clock::time_point start_time_;
};
