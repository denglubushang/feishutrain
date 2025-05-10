#pragma once
#include<iostream>
#include<iomanip>
#include<string>
#include<chrono>

class ProgressBar {
public:
	ProgressBar();
	void update(size_t current, size_t total);
	void finish(size_t total);

private:
	struct SpeedUnit {
		double threshold;	//最低速率要求
		const char* unit;	//单位字符串
		double divisor;		//除数
	};

	static const SpeedUnit speed_units_[];
	static constexpr int unit_cont_ = 4;

	std::string format_speed(double bytes_per_sec) const;

	std::chrono::steady_clock::time_point start_time_;
	std::chrono::steady_clock::time_point last_time_;
	size_t last_bytes_;
};