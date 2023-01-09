#pragma once

#include <cstdio>
#include <cstdlib>

#include <libopencm3/stm32/adc.h>
#include "Pinout.h"
#include "Task.h"
#include "Button.h"
#include "I2CSlave.h"
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
			PWR_KEY_PRESSED			= 1 << 11,
			
			// Deep sleep
			ALLOW_DEEP_SLEEP		= 1 << 12
		};
		
		enum Regs {
			// Read
			I2C_REG_STATUS,
			I2C_REG_IRQ_STATUS,
			I2C_REG_BAT_VOLTAGE,
			I2C_REG_BAT_TEMP,
			I2C_REG_BAT_MIN_TEMP,
			I2C_REG_BAT_MAX_TEMP,
			I2C_REG_BAT_PCT,
			I2C_REG_DCIN_VOLTAGE,
			I2C_REG_CPU_TEMP,
			I2C_REG_GET_MAX_BAT_VOLTAGE,
			I2C_REG_GET_MIN_BAT_VOLTAGE,
			I2C_REG_POWER_OFF,
			I2C_REG_RTC_TIME,
			I2C_REG_PLAY_BUZZER,
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
		int64_t m_last_pwron = 0;
		int64_t m_last_info_print = 0;
		uint32_t m_buzzer_freq = 0;
		uint32_t m_buzzer_vol = 0;
		
		PwrOnFailureReason m_last_pwron_fail = PWR_FAIL_NONE;
		
		Task m_task_analog_mon;
		Task m_task_watchdog;
		Task m_task_irq_pulse;
		
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
		void watchdogTask(void *);
		void irqPulseTask(void *);
		
		void onDcinChange(void *, bool state);
		void onBatChange(void *, bool state);
		void onChargerStatus(void *, Button::Event evt);
		void onPwrKey(void *, Button::Event evt);
		
		void onI2C(void *, I2CSlave::Event ev, uint8_t *byte);
		uint32_t readReg(void *, uint8_t reg);
		void writeReg(void *, uint8_t reg, uint32_t value);
		
		bool idleHook(void *);
		
		void allowDeepSleep(bool flag);
		
		bool isPowerOnAllowed();
		void powerOn();
		void powerOff(bool user);
};
