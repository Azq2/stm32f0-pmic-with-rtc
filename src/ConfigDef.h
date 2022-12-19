#pragma once

#define VREFINT_CAL (*((uint16_t *) 0x1FFFF7BA))
#define TS_CAL1 (*((uint16_t *) 0x1FFFF7B8))
#define TS_CAL2 (*((uint16_t *) 0x1FFFF7C2))

namespace Config {
	struct Battery {
		int v_min;
		int v_max;
		int v_shutdown;
		int v_presence;
		int t_max;
		int t_min;
		int t_chrg_max;
		int t_chrg_min;
		int t_hysteresis;
	};
	
	constexpr int d2int(double d) {
		return static_cast<int>(d * 1000);
	}
	
	struct Temp {
		int T[2];
		int value[2];
	};
};
