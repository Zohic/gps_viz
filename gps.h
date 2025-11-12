#ifndef GPS_H
#define GPS_H

#include <cstring>
#include <string>
#include <vector>
#include <complex>
#include <fstream>
#include <type_traits>
#include "result.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <atomic>

extern "C" {
#include "dspl.h"
}



namespace gps {

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

	
	
	
	class data_proc {
		std::vector<double> _data;
	public:
		template<typename T, typename CallBackType>
		result<bool> load(const std::string& filename, int parts, CallBackType callback) {
			using namespace std::string_literals;
			std::ifstream file;

			file.open(filename, std::ios_base::binary | std::ios_base::out);
			if (!file.is_open() || !file.good())
				return "cant open file";

			file.seekg(0, std::ios_base::end);
			auto byte_size = static_cast<size_t>(file.tellg());
			//elements = iqs = complex values
			const size_t elements = byte_size / sizeof(T) / 2;

			_data.resize(elements * 2);
			
			size_t load_size = elements / parts;
			size_t leftover = elements  - load_size * parts;

			file.seekg(0, std::ios_base::beg);
			std::unique_ptr<T[]> nums_as_type(new T[load_size * 2]);

			for (size_t i = 0; i < parts; i++) {
				
				file.read(reinterpret_cast<char*>(nums_as_type.get()), load_size * 2 * sizeof(T));

				if (!file.good()) {
					const auto read_done = static_cast<size_t>(file.tellg());
					auto err_msg = gps::cat("error reading: ", read_done, "/", byte_size, " bytes done");
					return err_msg.c_str();
				}

				size_t offset = i * load_size;
				for (size_t c = 0; c < load_size; c++) {
					_data[c*2 + offset]     = nums_as_type[c*2];
					_data[c*2 + offset + 1] = nums_as_type[c*2 + 1];
				}
				
				callback(load_size * (i + 1), elements);
			}

			if (leftover > 0) {
				
				file.read(reinterpret_cast<char*>(nums_as_type.get()), leftover * 2 * sizeof(T));

				if (!file.good()) {
					const auto read_done = static_cast<size_t>(file.tellg());
					auto err_msg = gps::cat("error reading: ", read_done, "/", byte_size, " bytes done");
					return err_msg.c_str();
				}

				size_t offset = parts * load_size;
				for (size_t c = 0; c < leftover; c++) {
					_data[c*2 + offset]     = nums_as_type[c * 2];
					_data[c*2 + offset + 1] = nums_as_type[c * 2 + 1];
				}

				callback(elements, elements);
			}
			
			return true;
		}
		size_t iqCount() const {
			return _data.size() / 2;
		}
		void fillData(size_t iqOffset, size_t iqCount, double* out_data) const {
			memcpy(out_data, _data.data() + iqOffset * sizeof(double) * 2, iqCount * sizeof(double) * 2);
		}
	};

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
		ptrdiff_t accum_count = -1;
		range<double> comp_freq = { -6000, 6000, 100 };
		range<int> checkCA = { 1, 10, 1 };
	};
	

	class detector {
		const system_params& params;
		std::unordered_set<ca_code, ca_code_hash> ca_code_cache;

		using store1 = std::vector<double>;
		using store2 = std::vector<store1>;
		using store3 = std::vector<store2>;
		
		store3 ca_corrs;
		store1 best_dfs;

		result<ca_code> GetCACode(unsigned int number);

	public:
		detector(const system_params& p);
		result<bool> CalcCACorrs(const data_proc& data);

	};

	
}


#endif // !GPS_H
