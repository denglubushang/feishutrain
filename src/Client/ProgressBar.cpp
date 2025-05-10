#include "ProgressBar.h"

const ProgressBar::SpeedUnit ProgressBar::speed_units_[] = {
	{1024.0 * 1024.0 * 1024.0,"GB/s",1024.0 * 1024.0 * 1024.0},
	{1024.0 * 1024.0,		  "MB/s",1024.0 * 1024.0},
	{1024.0,				  "KB/s",1024.0 },
	{0.0,					  "Bytes/s",1.0},
};


ProgressBar::ProgressBar()
	: start_time_(std::chrono::steady_clock::now()),
	last_time_(start_time_),
	last_bytes_(0) {}


std::string ProgressBar::format_speed(double bytes_per_sec) const {
	for (int i = 0; i < unit_cont_; ++i) {
		if (bytes_per_sec >= speed_units_[i].threshold) {
			double value = bytes_per_sec / speed_units_[i].divisor;
			char buf[64];
			snprintf(buf, sizeof(buf), "%.2f %s", value, speed_units_[i].unit);
			return std::string(buf);
		}
	}
	return "0 Bytes/s";
}

void ProgressBar::update(size_t current, size_t total) {
	//计算百分比
	float progress = (float)current / total * 100.0f;

	//计算时间间隔
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	std::chrono::duration<double> elapsed_since_last = now - last_time_;
	if (elapsed_since_last.count() < 0.05) {
		//少于0.1秒，不刷
		return;
	}

	size_t bytes_sent = current - last_bytes_;
	double speed = 0.0;	// KB/s

	if (elapsed_since_last.count() > 0.0) {
		speed = bytes_sent / elapsed_since_last.count();	// B/s
	}
	std::string speed_str = format_speed(speed);

	//显示进度和速率
	std::cout << "\r发送进度[";
	int pos = static_cast<int>(progress / 2);


	for (int i = 0; i < 50; ++i) {
		if (i < pos) {
			std::cout << "=";
		}
		else if (i == pos) {
			std::cout << ">";
		}
		else {
			std::cout << " ";
		}
	}
	std::cout << "]" << std::fixed << std::setprecision(1) << progress << "%    "
		<< speed_str << "   ";
	std::cout.flush();

	last_time_ = now;
	last_bytes_ = current;
}

void ProgressBar::finish(size_t total) {
	// 补一个完整100%的进度条
	float progress = 100.0f;

	std::cout << "\r发送进度[";
	for (int i = 0; i < 50; ++i) {
		std::cout << "=";
	}
	std::cout << "]" << std::fixed << std::setprecision(1) << progress << "%   ";

	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	std::chrono::duration<double> total_time = now - start_time_;
	double seconds = total_time.count();

	double avg_speed = total / seconds;  // B/s
	std::string avg_speed_str = format_speed(avg_speed);

	std::cout << std::endl;
	std::cout << "传输完成！总时间：" << std::fixed << std::setprecision(2) << seconds
		<< "秒，平均速率：" << avg_speed_str << std::endl;
}
