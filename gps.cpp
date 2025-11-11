#include "gps.h"
#include <algorithm>


using namespace gps;

result<ca_code> gps::GetCACodeFast(unsigned int number, unsigned int cycles) {
	
	if (number > 37)
		return "invalid ca code number. Must be in range [1..37]";

	static constexpr size_t N = (1 << 10) - 1;
	static constexpr int CP1[] = { 2,3,4,5,1,2,1,2,3,2,3,5,6,7,8,9,1,2,3,4,5,6,1,4,5,
		6,7,8,1,2,3,4,5,4,1,2,4};
	static constexpr int CP2[] = { 6,7,8,9,9,10,8,9,10,3,4,6,7,8,9,10,4,5,6,7,8,9,3,6,7,8,9,
		10,6,7,8,9,10,10,7,8,10};
	constexpr uint16_t all_ones = std::numeric_limits<uint16_t>::max();
	//constexpr uint16_t all_ones = (1 << 15) - 1 + (1 << 15);
	
	const size_t CCP1 = CP1[number - 1];
	const size_t CCP2 = CP2[number - 1];

	std::vector<char> CACode(N * cycles, (char)0);
	uint16_t G1{ all_ones }, G2{ all_ones };
	
#define GET_BIT(A, N) ((A >> (10 - N)) & 1)
#define G1B(N) GET_BIT(G1, N)
#define G2B(N) GET_BIT(G2, N)

	char fb_G1{0}, fb_G2{0};
		
	for (size_t i = 0; i < N; i++) {
		CACode[i] = G1B(10) ^ (G2B(CCP1) ^ G2B(CCP2));
		fb_G1 = G1B(3) ^ G1B(10);
		fb_G2 =
			(G2B(2) ^ G2B(3)) +
			(G2B(6) ^ G2B(8)) +
			(G2B(9) ^ G2B(10));
		fb_G2 = fb_G2 & 1;

		G1 = G1 >> 1;
		G2 = G2 >> 1;
		G1 = (G1 & ((1 << 9) - 1)) | (fb_G1 << 9);
		G2 = (G2 & ((1 << 9) - 1)) | (fb_G2 << 9);
	}

	for (size_t c = 1; c < cycles; c++) {
		memcpy(CACode.data() + N * c, CACode.data(), N);
	}
	
#undef GET_BIT
#undef G1B
#undef G2B

	return ca_code{ CACode, number};
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
			ca_code_iqs[cc * 2] = ca_code_bit_vec[cc];
			ca_code_iqs[cc * 2 + 1] = 0.0;
		}

		for (double df = start_df; df < end_df; df += step_df) {
			const double df_e_step = 2 * M_2PI * df / params.Fs;
			
			for (size_t i = 0; i < params.accum_length; i++) {
				const double dfc = cos(df_e_step * i);
				const double dfs = sin(df_e_step * i);
				shifted_iqs[i * 2]     = ca_code_iqs[i * 2]     * dfc - ca_code_iqs[i * 2 + 1] * dfs;
				shifted_iqs[i * 2 + 1] = ca_code_iqs[i * 2 + 1] * dfc + ca_code_iqs[i * 2]     * dfs;
			}

			for (size_t k = 0; k < accum_count; k++) {

				data.fillData(k * params.accum_length, params.accum_length, cur_iqs.data());
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
		}

	}

}

