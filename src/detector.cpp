#include "gps/detector.h"
#include <cmath>

using namespace gps;

extern "C" {
#include "dspl.h"
}

detector::detector(const system_params& p) : params(p) {

}

result<ca_code> detector::GetCACode(unsigned int number) {
	
	decltype(ca_code_cache)::const_iterator found_ca = ca_code_cache.find(ca_code{ {}, number });
	if (found_ca != ca_code_cache.end()) {
		auto& found = *found_ca;
		return found;
	}
	else {
		auto c = GetCACodeFast(number, 1);
		if (!c.isOk())
			return std::move(c);

		ca_code_cache.insert(c.GetValue());
		
		return c.GetValue();
	}
}

result<bool> detector::CalcCACorrs(const data_proc& data) {
	const int& startCA = params.checkCA.min;
	const int& endCA   = params.checkCA.max;
	const int& stepCA  = params.checkCA.step;
	
	const double& start_df = params.comp_freq.min;
	const double& end_df   = params.comp_freq.max;
	const double& step_df  = params.comp_freq.step;

	size_t accum_count = params.accum_count;
	if (accum_count == -1)
		accum_count = data.iqCount() / params.accum_length;

	size_t search_iq_size = accum_count * params.accum_length;
	//double* search_data = new double[search_iq_size * 2];
	//data.fillData(0, search_iq_size, search_data);

	if (params.accum_length != ca_length)
		return "accumulation vector length must be divisible by ca-code length (1023)";

	ca_corrs.resize(params.checkCA.count());

	for (auto& ca_corr_i : ca_corrs) {
		ca_corr_i.resize(params.comp_freq.count());
		for (auto& freq_corr_i : ca_corr_i)
			freq_corr_i.resize(2 * params.accum_length + 1);
	}
	best_corrs.resize(params.checkCA.count());
	best_dfs.resize(params.checkCA.count());

	std::vector<double> cur_iqs(params.accum_length * 2);
	std::vector<double> ca_code_iqs(params.accum_length * 2);
	std::vector<double> shifted_iqs(params.accum_length * 2);
	std::vector<double> xcorr_result((2 * params.accum_length + 1) * 2);
	std::vector<double> xcorr_accum(2 * params.accum_length + 1, 0.0);

	for (int c = startCA; c <= endCA; c += stepCA) {
		auto ca = GetCACode(c);
		if (!ca.isOk())
			return ca.GetError();
		auto& ca_code_bit_vec = ca.GetValue().code;

		for (size_t cc = 0; cc < params.accum_length; cc++) {
			ca_code_iqs[cc * 2] = 2*ca_code_bit_vec[cc] - 1;
			ca_code_iqs[cc * 2 + 1] = 0.0;
		}
		size_t cur_df = 0;
		size_t max_df_ind = 0;
		size_t max_corr_ind = 0;

		for (double df = start_df; df < end_df; df += step_df) {
			const double df_e_step = 2 * M_2PI * df / params.Fs;
			
			for (size_t i = 0; i < params.accum_length; i++) {
				const double dfc = cos(df_e_step * i);
				const double dfs = sin(df_e_step * i);
				shifted_iqs[i * 2]     = ca_code_iqs[i * 2]     * dfc - ca_code_iqs[i * 2 + 1] * dfs;
				shifted_iqs[i * 2 + 1] = ca_code_iqs[i * 2 + 1] * dfc + ca_code_iqs[i * 2]     * dfs;
			}
			xcorr_accum.resize(params.accum_length * 2 + 1);
			size_t max_df_corr_ind = 0;
			
			for (size_t k = 0; k < accum_count; k++) {

				auto r = data.fillData(k * params.accum_length, params.accum_length, cur_iqs.data());
				
				if(!r.isOk())
					return cat("detection error: ", r.GetError()).c_str();

				xcorr_cmplx(
					(complex_t*)shifted_iqs.data(), params.accum_length,
					(complex_t*)cur_iqs.data(), params.accum_length,
					DSPL_XCORR_NOSCALE,
					params.accum_length,
					(complex_t*)xcorr_result.data(), NULL);

				for (size_t acc = 0; acc < xcorr_accum.size(); acc++) {
					const double re = xcorr_result[acc * 2];
					const double im = xcorr_result[acc * 2 + 1];
					xcorr_accum[acc] += re * re + im * im;
				}

			}

			for(size_t k = 0; k < accum_count; k++){
				if(xcorr_accum[k] > xcorr_accum[max_df_corr_ind])
					max_df_corr_ind = k;
			}

			if(xcorr_accum[max_df_corr_ind] > xcorr_accum[max_df_ind]){
				max_corr_ind = max_df_corr_ind;
				max_df_ind = cur_df;
			}
				

			ca_corrs[c-1][cur_df] = std::move(xcorr_accum);
			cur_df++;
		}

		best_dfs[c-1] = max_df_ind;
		best_corrs[c-1] = max_corr_ind;

	}
	
	return true;
}

const detector::store1& detector::getCorrFunc(size_t ca_code){
	if(ca_code == 0){
		throw std::invalid_argument("ca_codes are 1 to 37");
	}
	
	ca_code -=1;
	if(code_corr_map_cache.count(ca_code) == 1){
		return code_corr_map_cache[ca_code];
	}

	
	size_t freqs_num =  ca_corrs[ca_code].size();
	size_t corrs_num = ca_corrs[ca_code][0].size();

	store1 corr_map;
	corr_map.resize(freqs_num * corrs_num);
	
	for(size_t f = 0; f < freqs_num; f++){
		for(size_t c = 0; c < corrs_num; c++){
			corr_map[f*corrs_num + c] = ca_corrs[ca_code][f][c];
		}
	}

	code_corr_map_cache[ca_code] = std::move(corr_map);

	return code_corr_map_cache[ca_code];
}
const std::vector<double>& detector::getCorrFunc(size_t ca_code, size_t freq_shift){
	return {};
}

const detector::store1& detector::getCAMap(){	
	if(map_cached){
		return corr_map_cache;
	}

	size_t codes_num =  ca_corrs.size();
	size_t corrs_num = ca_corrs[0][0].size();

	corr_map_cache.resize(codes_num * corrs_num);

	for(size_t ca = 0; ca < codes_num; ca++){
		for(size_t c = 0; c < corrs_num; c++){
			corr_map_cache[ca*corrs_num + c] = ca_corrs[ca][best_dfs[ca]][c];
		}
	}

	return corr_map_cache;
}