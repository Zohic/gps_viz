#ifndef GPS_UTILS_H
#define GPS_UTILS_H

#include <string>
#include <vector>
#include <type_traits>
#include <memory>
#include <cmath>

#include "result.h"

namespace gps
{
    struct ca_code {
		std::vector<char> code;
		unsigned int num;
		bool operator==(const ca_code& other) const {
			return num == other.num;
		}
	};

	struct ca_code_hash {
		size_t operator()(const ca_code& c) const {
			return c.num;
		}
	};

	static constexpr size_t ca_length = 1023;
	result<ca_code> GetCACodeFast(unsigned int number, unsigned int periods);

	template<typename T>
	constexpr std::string as_std_str(T value) {
		if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
			return std::to_string(value);
		}
		else if constexpr (std::is_same_v<T, const char*>) {
			return std::string(value);
		}
		else if constexpr (std::is_same_v<std::remove_reference_t<std::remove_cv_t<T>>, std::string>) {
			return std::move(value);
		}
		return std::string("WTHELL");
	}

	template<typename FT>
	constexpr std::string cat(FT first) {
		return as_std_str(first);
	}

	template<typename FT, typename ...RT>
	constexpr std::string cat(FT first, RT... rest) {
		return as_std_str(first) + cat(rest...);
	}

    template<typename T>
	struct range {
		T min, max, step;
		
		size_t count() const {
			if constexpr(std::is_integral_v<T>)
				return static_cast<size_t>((max - min)/step + 1);
			else
				return static_cast<size_t>(floor((max - min) / step));
		}
	};
	
	struct system_params {
		double Fs = 1.023e6;
		size_t accum_length = ca_length;
		ptrdiff_t accum_count = 30;
		range<double> comp_freq = { -6000, 6000, 100 };
		range<int> checkCA = { 1, 10, 1 };
	};

} // namespace gps

#endif //GPS_UTILS_H