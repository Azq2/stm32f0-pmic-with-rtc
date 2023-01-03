#pragma once

#include "ConfigDef.h"

#define DEBUG						1	// USART debug
#define DEBUG_CALIBRATE_RTC			0	// Output RTC freq to USART_TX pin
#define DEBUG_CALIBRATE_BAT_TEMP	0	// Output bat temp in voltage

namespace Config {
	constexpr uint32_t WATCHDOG_TIMEOUT				= 60000;
	
	constexpr uint32_t CHARGING_BAD_TEMP_TIMEOUT	= 1000 * 60 * 30;
	constexpr uint32_t CHARGING_LOST_DCIN_TIMEOUT	= 1000 * 5;
	constexpr uint32_t CHARGING_BAD_DCIN_TIMEOUT	= 1000 * 60 * 30;
	constexpr uint32_t MIN_CHARGE_TIME				= 1000 * 60;
	
	// RTC calibration
	constexpr uint32_t RTC_PRESCALER_S				= 19200;
	constexpr uint32_t RTC_PRESCALER_A				= 1;
	
	// Battery seettings
	constexpr Battery BAT = {
		.v_min			= d2int(3.4),
		.v_max			= d2int(4.15),
		.v_shutdown		= d2int(3.3),
		.v_presence		= d2int(2.5),
		.t_max			= d2int(45),
		.t_min			= d2int(-20),
		.t_chrg_max		= d2int(40),
		.t_chrg_min		= d2int(5),
		.t_hysteresis	= d2int(4)
	};
	
	// DCIN
	constexpr int DCIN_MIN_VOLTAGE		= d2int(4.5);
	
	// Voltage dividers ratio
	constexpr int DCIN_RDIV	= d2int(2);
	constexpr int VBAT_RDIV	= d2int(2);
	
	// Diode sensor for battery temperature
	const Temp BAT_TEMP = {
		{d2int(19), d2int(45)},
		{592, 536}
	};
	
	// Internal cpu temperature sensor
	const Temp CPU_TEMP = {
		{d2int(30), d2int(110)},
		{TS_CAL1, TS_CAL2}
	};
};
