#ifndef GPS_DATA_PROC_H
#define GPS_DATA_PROC_H

#include "gps_utils.h"
#include <fstream>


namespace gps
{
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
		result<bool> fillData(size_t iqOffset, size_t iqCount, double* out_data) const {
			if(iqOffset + iqCount > _data.size())
				return cat("requested to much: ", iqOffset, "+", iqCount, " of ", _data.size()).c_str();
			memcpy(out_data, _data.data() + iqOffset * sizeof(double) * 2, iqCount * sizeof(double) * 2);
			
			return true;
		}
	};

} // namespace gps

#endif //GPS_DATA_PROC_H