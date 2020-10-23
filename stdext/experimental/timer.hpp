#pragma once

#include <cstdint>

#ifdef _WIN32
#include <profileapi.h>
#else
#include <time.h>
#endif

namespace stdext
{

	/*int64_t get_current_time_nsecs()
	{
#ifdef _WIN32
		LARGE_INTEGER li;
		if (!QueryPerformanceCounter(&li))
			return 0;
		return int64_t(double(li.QuadPart) * static_qpc_freq.inv_freq);
#else
		struct timespec ts = {};
		if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
			return 0;
		return ts.tv_sec * 1000000000ll + ts.tv_nsec;
#endif
	}*/

	class timer
	{
	public:
		void start()
		{

		}

		double end()
		{

		}

	private:
		int64_t t = 0;
	};
}