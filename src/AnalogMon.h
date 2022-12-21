#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstdio>

#include <libopencm3/stm32/adc.h>

#include "Loop.h"
#include "Pinout.h"
#include "Config.h"
#include "Debug.h"
#include "utils.h"

#define VREFINT_CAL (*((uint16_t *) 0x1FFFF7BA))
#define TS_CAL1 (*((uint16_t *) 0x1FFFF7B8))
#define TS_CAL2 (*((uint16_t *) 0x1FFFF7C2))

class AnalogMon {
	public:
		enum Value : int {
			DCIN = 0,
			VBAT,
			BAT_TEMP,
			CPU_TEMP,
			VREF
		};
	
	protected:
		constexpr static int ADC_AVG_CNT = 10;
		constexpr static uint8_t m_adc_channels[] = {
			Pinout::ADC_CH_DCIN,
			Pinout::ADC_CH_VBAT,
			Pinout::ADC_CH_TEMP,
			ADC_CHANNEL_TEMP,
			ADC_CHANNEL_VREF,
		};
		uint16_t m_adc_result[COUNT_OF(m_adc_channels)] = {};
		bool m_ignore_dcin = false;
		int64_t m_last_dcin_ignore = 0;
		
		bool m_dma_work_done = false;
		
		int m_vbat = 0;
		int m_dcin = 0;
		int m_cpu_temp = 0;
		int m_bat_temp = 0;
		bool m_dcin_present = false;
	public:
		AnalogMon();
		~AnalogMon();
		
		void init();
		
		void read();
		void switchFormAdcToExti(bool to_exti);
		
		int toVoltage(int raw_value, int vref, int rdiv);
		int toTemperature(int raw_value, const Config::Temp &calibration);
		
		inline int isBatDischarged() {
			return m_vbat <= Config::BAT.v_shutdown;
		}
		
		inline int isBatPresent() {
			return m_vbat >= Config::BAT.v_presence;
		}
		
		inline int isDcinPresent() {
			return m_dcin_present;
		}
		
		inline int isDcinGood() {
			return isDcinPresent() && m_dcin >= Config::DCIN_MIN_VOLTAGE;
		}
		
		inline int getVbat() {
			return m_vbat;
		}
		
		inline int getDcin() {
			return isDcinPresent() ? m_dcin : 0;
		}
		
		inline int getCpuTemp() {
			return m_cpu_temp;
		}
		
		inline void ignoreDcinVoltage(bool state) {
			m_ignore_dcin = state;
			m_last_dcin_ignore = Loop::ms() + 1000;
		}
		
		int getBatPct();
		
		inline int getBatTemp() {
			return m_bat_temp;
		}
		
		void dmaIrqHandler();
};
