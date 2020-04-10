#pragma once

#include <Windows.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

extern CHAR MODULE_DIR[MAX_PATH];

template<typename A>
void LogWriteUnfold(std::ostream& stream, A a) {
	stream << a;
}

template<typename A, typename ...T>
void LogWriteUnfold(std::ostream& stream, A a, T ...param) {
	stream << a;
	LogWriteUnfold(stream, param...);
}

template<typename A, typename ...T>
void LogWrite(A a, T ...param) {
	std::tm timestamp;
	std::time_t now = std::time(nullptr);
	localtime_s(&timestamp, &now);
	std::string name(MODULE_DIR);
	name += "\\esws";
	std::string path = name + ".log";

	// 文件过大就重命名。
	auto flag = std::ios::app;
	std::ifstream input(path);
	if (input.good()) {
		input.seekg(0, input.end);
		auto size = input.tellg();
		if (size >= 100000) {
			input.seekg(0, input.beg);
			std::stringstream backupPath;
			backupPath << name << "-" << std::put_time(&timestamp, "%Y%m%d%H%M%S") << ".log";
			std::ofstream backup(backupPath.str(), std::ios::app);
			backup << input.rdbuf();
			flag = std::ios::ate;
		}
	}

	// 写入日志。
	std::ofstream output(path, flag);
	LogWriteUnfold(output, std::put_time(&timestamp, "[%H:%M:%S]"), a, param...);
}

template<typename A, typename ...T>
void LogWriteLn(A a, T ...param) {
	LogWrite(a, param..., "\r\n");
}