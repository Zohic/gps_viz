#include "gps/gps.h"
#include <algorithm>
#include <limits>

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

