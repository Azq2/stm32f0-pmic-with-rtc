#include "RTC.h"

#include "Debug.h"

#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/rcc.h>

constexpr uint32_t RTC_INIT_MAGIC = 0x32717a41;

void RTC::init() {
	pwr_disable_backup_domain_write_protect();
	
	rcc_osc_on(RCC_LSI);
	rcc_wait_for_osc_ready(RCC_LSI);
	rcc_set_rtc_clock_source(RCC_LSI);
	rcc_enable_rtc_clock();
	
	rtc_unlock();
	rtc_set_init_flag();
	rtc_wait_for_init_ready();
	rtc_set_prescaler(20000, 1);
	rtc_set_am_format();
	rtc_clear_init_flag();
	rtc_lock();
	rtc_wait_for_synchro();
	
	if (RTC_BKPXR(0) != RTC_INIT_MAGIC) {
		setDateTime(2021, 12, 19, 2, 36, 5);
		RTC_BKPXR(0) = RTC_INIT_MAGIC;
	}
	
	pwr_enable_backup_domain_write_protect();
}

void RTC::setDateTime(uint16_t y, uint8_t m, uint8_t d, uint8_t hh, uint8_t mm, uint8_t ss) {
	pwr_disable_backup_domain_write_protect();
	
	rtc_unlock();
	rtc_set_init_flag();
	rtc_wait_for_init_ready();
	
	rtc_enable_bypass_shadow_register();
	
	rtc_calendar_set_year(y - 2000);
	rtc_calendar_set_month(m);
	rtc_calendar_set_day(d);
	rtc_time_set_time(hh, mm, ss, true);
	
	rtc_disable_bypass_shadow_register();
	
	rtc_clear_init_flag();
	rtc_lock();
	
	pwr_enable_backup_domain_write_protect();
}

// https://blog.reverberate.org/2020/05/12/optimizing-date-algorithms.html
uint32_t RTC::toUnixTime(int year, int month, int day, int hours, int minutes, int seconds) {
	static const uint16_t month_yday[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
	uint32_t year_adj = year + 4800;  /* Ensure positive year, multiple of 400. */
	uint32_t febs = year_adj - (month <= 2 ? 1 : 0);  /* Februaries since base. */
	uint32_t leap_days = 1 + (febs / 4) - (febs / 100) + (febs / 400);
	uint32_t days = 365 * year_adj + leap_days + month_yday[month - 1] + day - 1;
	days -= 2472692; /* Adjust to Unix epoch. */
	return days * 3600 * 24 + (hours * 3600) + (minutes * 60) + seconds;
}

uint32_t RTC::time() {
	int year = 2000 + decodeBCD(RTC_DR, RTC_DR_YT_SHIFT, RTC_DR_YT_MASK, RTC_DR_YU_SHIFT, RTC_DR_YU_MASK);
	int month = decodeBCD(RTC_DR, RTC_DR_MT_SHIFT, RTC_DR_MT_MASK, RTC_DR_MU_SHIFT, RTC_DR_MU_MASK);
	int day = decodeBCD(RTC_DR, RTC_DR_DT_SHIFT, RTC_DR_DT_MASK, RTC_DR_DU_SHIFT, RTC_DR_DU_MASK);
	int hours = decodeBCD(RTC_TR, RTC_TR_HT_SHIFT, RTC_TR_HT_MASK, RTC_TR_HU_SHIFT, RTC_TR_HU_MASK);
	int minutes = decodeBCD(RTC_TR, RTC_TR_MNT_SHIFT, RTC_TR_MNT_MASK, RTC_TR_MNU_SHIFT, RTC_TR_MNU_MASK);
	int seconds = decodeBCD(RTC_TR, RTC_TR_ST_SHIFT, RTC_TR_ST_MASK, RTC_TR_SU_SHIFT, RTC_TR_SU_MASK);
	return toUnixTime(year, month, day, hours, minutes, seconds);
}
