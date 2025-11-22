#ifndef COMMON_DEBUGCON_HH
#define COMMON_DEBUGCON_HH

#include <Windows.h>
#include <cstdio>
#include <format>
#include <mutex>
#include <string>
#include <string_view>
#include <chrono>
#include <ctime>

class cDebugConsole {
public:
	static cDebugConsole& instance() {
		static cDebugConsole instance;
		return instance;
	}

	template <typename... Args>
	void print(std::format_string<Args...> fmt, Args&&... args) {
		const auto msg = std::format(fmt, std::forward<Args>(args)...);
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (kDebugConsoleTimestamps) {
			const std::string ts = makeTimestamp();
			writeRaw(m_hOut, std::format("[{}] ", ts), false);
		}
		writeRaw(m_hOut, msg, true);
	}

	template <typename... Args>
	void success(std::format_string<Args...> fmt, Args&&... args) {
		const auto msg = std::format(fmt, std::forward<Args>(args)...);
		writeLineWithPrefix(m_hOut, foreground(0, 1, 0, true), "[+] ", msg);
	}

	template <typename... Args>
	void info(std::format_string<Args...> fmt, Args&&... args) {
		const auto msg = std::format(fmt, std::forward<Args>(args)...);
		writeLineWithPrefix(m_hOut, foreground(0, 1, 1, true), "[*] ", msg);
	}

	template <typename... Args>
	void error(std::format_string<Args...> fmt, Args&&... args) {
		const auto msg = std::format(fmt, std::forward<Args>(args)...);
		writeLineWithPrefix(m_hErr, foreground(1, 0, 0, true), "[-] ", msg);
	}

private:
	static WORD foreground(WORD r, WORD g, WORD b, bool bright = true) {
		WORD v = 0;
		if (r) v |= FOREGROUND_RED;
		if (g) v |= FOREGROUND_GREEN;
		if (b) v |= FOREGROUND_BLUE;
		if (bright) v |= FOREGROUND_INTENSITY;
		return v;
	}

	static std::string makeTimestamp() {
		using namespace std::chrono;

		const auto now = system_clock::now();
		const auto t = system_clock::to_time_t(now);

		std::tm tm{};
		localtime_s(&tm, &t);
		char hhmmss[16];

		std::strftime(hhmmss, sizeof(hhmmss), "%H:%M:%S", &tm);
		const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
		return std::format("{}.{:03}", hhmmss, static_cast<int>(ms.count()));
	}

	FILE* handleToFile(HANDLE h) const {
		return (h == m_hErr) ? stderr : stdout;
	}

	void setAttr(HANDLE h, WORD color) {
		if (h && h != INVALID_HANDLE_VALUE) {
			SetConsoleTextAttribute(h, color);
		}
	}

	WORD getDefaultAttr(HANDLE h) const {
		if (!h || h == INVALID_HANDLE_VALUE) return (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		CONSOLE_SCREEN_BUFFER_INFO csbi{};
		if (GetConsoleScreenBufferInfo(h, &csbi)) return csbi.wAttributes;
		return (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}

	void writeRaw(HANDLE h, std::string_view s, bool newline = true) {
		FILE* f = handleToFile(h);
		std::fwrite(s.data(), 1, s.size(), f);
		if (newline) std::fputc('\n', f);
		std::fflush(f);
	}

	void writeLineWithPrefix(HANDLE h, WORD prefixColor, std::string_view prefix, std::string_view msg) {
		std::lock_guard<std::mutex> lock(m_Mutex);
		const WORD defAttr = (h == m_hErr) ? m_errDefaultAttr : m_outDefaultAttr;

		if (kDebugConsoleTimestamps) {
			const std::string ts = makeTimestamp();
			writeRaw(h, std::format("[{}] ", ts), false);
		}

		setAttr(h, prefixColor);
		writeRaw(h, prefix, false);

		setAttr(h, defAttr);
		writeRaw(h, msg, true);
	}

	cDebugConsole() {
		if constexpr (kEnableConsole) {
			if (!AllocConsole())
				std::abort();

			FILE* fp = nullptr;
			freopen_s(&fp, "CONOUT$", "w", stdout);
			freopen_s(&fp, "CONOUT$", "w", stderr);
			freopen_s(&fp, "CONIN$", "r", stdin);
		}

		m_hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		m_hErr = GetStdHandle(STD_ERROR_HANDLE);

		m_outDefaultAttr = getDefaultAttr(m_hOut);
		m_errDefaultAttr = getDefaultAttr(m_hErr);
	}

	HANDLE m_hOut = nullptr;
	HANDLE m_hErr = nullptr;
	WORD m_outDefaultAttr = 0;
	WORD m_errDefaultAttr = 0;
	std::mutex m_Mutex;
};

#endif /* COMMON_DEBUGCON_HH */
