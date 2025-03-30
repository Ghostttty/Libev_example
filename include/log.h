#ifndef H_LOG
#define H_LOG

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <format>

#define LOG_I(log) log_event(INFO, log)
#define LOG_E(log) log_event(ERROR, log)

const std::string LOG_FILE = "server.log";

enum logging_level_e
{
	INFO = 0,
	ERROR
};

constexpr std::string_view logging_type[] = {"INFO", "ERROR"};

std::string get_timestamp()
{
	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);

	struct tm tm_buf;
	localtime_r(&t, &tm_buf);

	char buf[20];
	strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
	return buf;
}

void log_event(enum logging_level_e logging_lvl, const std::string &data)
{
	std::ofstream file_stream(LOG_FILE, std::ios::app);
	if(!file_stream)
	{
		std::cerr << "Failed to open log file" << std::endl;
		return;
	}

	std::string log_message = std::format("[{}][{}] {}", logging_type[logging_lvl], get_timestamp(), data);

    auto& stream = (logging_lvl == INFO) ? std::cout : std::cerr;
    stream << log_message << '\n';

	file_stream << log_message << std::endl;
}

#endif /* H_LOG */