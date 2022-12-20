#pragma once

#include <cstdio>
#include <cstdlib>

#include <libopencm3/stm32/adc.h>
#include "Pinout.h"
#include "Task.h"
#include "Button.h"
#include "AnalogMon.h"
#include "utils.h"

class App {
	protected:
		enum Flags {
			DCIN_GOOD				= 1 << 0,
			DCIN_PRESENT			= 1 << 1,
			BAT_PRESENT				= 1 << 2,
			POWER_ON				= 1 << 3,
			USER_POWER_OFF			= 1 << 4,
			
			// Charging
			BAT_CHARGING			= 1 << 5,
			BAT_CHARGE_EN			= 1 << 6,
			
			// BAT temperature
			BAT_LOW_TEMP			= 1 << 7,
			BAT_HIGH_TEMP			= 1 << 8,
			BAT_CHARGE_LOW_TEMP		= 1 << 9,
			BAT_CHARGE_HIGH_TEMP	= 1 << 10,
			
			// Key
			PWR_KEY_PRESSED			= 1 << 11
		};
		
		enum ChrgFailureReason {
			CHRG_FAIL_NONE,
			CHRG_FAIL_LOW_TEMP,
			CHRG_FAIL_HIGH_TEMP,
			CHRG_FAIL_NO_DCIN,
			CHRG_FAIL_BAD_DCIN,
			CHRG_FAIL_NO_BAT
		};
		
		enum PwrOnFailureReason {
			PWR_FAIL_NONE,
			PWR_FAIL_BAT_IS_LOW,
			PWR_FAIL_BAT_TEMP_IS_LOW,
			PWR_FAIL_BAT_TEMP_IS_HIGH,
		};
		
		ChrgFailureReason m_last_chrg_failure = CHRG_FAIL_NONE;
		int64_t m_last_chrg_failure_time = 0;
		int m_last_chrg_failure_cnt = 0;
		int m_dcin_bad_cnt = 0;
		int64_t m_last_charging = 0;
		int64_t m_dcin_connected = 0;
		
		PwrOnFailureReason m_last_pwron_fail = PWR_FAIL_NONE;
		
		Task m_task_analog_mon;
		Task m_task_print_info;
		
		uint32_t m_state = 0;
		Button m_pwr_key = {};
		Button m_charger_status = {};
		AnalogMon m_mon;
		
		void initHw();
		void check();
		
		inline bool is(uint32_t bit) {
			return (m_state & bit) != 0;
		}
		
		bool setStateBit(uint32_t bit, bool value);
		
		void checkBatteryTemp(const char *name, int min, int max, Flags flag_lo, Flags flag_hi);
		ChrgFailureReason checkChargingAllowed();
		PwrOnFailureReason checkPowerOnAllowed();
		bool isChargingDisabled();
		bool isAutoPowerOnDisabled();
		uint32_t getTimeoutForChrgFail();
		const char *getEnumName(ChrgFailureReason reason);
		const char *getEnumName(PwrOnFailureReason reason);
	public:
		int run();
		void monitorTask(void *);
		void printInfoTask(void *);
		void onDcinChange(void *, bool state);
		void onBatChange(void *, bool state);
		void onChargerStatus(void *, Button::Event evt);
		void onPwrKey(void *, Button::Event evt);
		bool idleHook(void *);
		
		void allowDeepSleep(bool flag);
		
		bool isPowerOnAllowed();
		void powerOn();
		void powerOff(bool user);
};
