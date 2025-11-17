#ifndef GPS_DETECTOR_H
#define GPS_DETECTOR_H

#include "data_proc.h"

#include <unordered_map>
#include <unordered_set>

namespace gps
{
    class detector {

		using store1 = std::vector<double>;
		using store2 = std::vector<store1>;
		using store3 = std::vector<store2>;

		const system_params& params;
		std::unordered_set<ca_code, ca_code_hash> ca_code_cache;
		std::unordered_map<size_t, store1> code_corr_map_cache;
		
		bool map_cached = false;
		store1 corr_map_cache;
		
		
		store3 ca_corrs;
		std::vector<size_t> best_corrs;
		std::vector<size_t> best_dfs;

		result<ca_code> GetCACode(unsigned int number);

	public:
		detector(const system_params& p);
		result<bool> CalcCACorrs(const data_proc& data);
		const store1& getCorrFunc(size_t ca_code);
		const store1& getCAMap();
		const std::vector<double>& getCorrFunc(size_t ca_code, size_t freq_shift);
		

	};
}

#endif //GPS_DETECTOR_H