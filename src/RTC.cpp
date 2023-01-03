#include "RTC.h"

#include "Config.h"
#include "Debug.h"

#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>

/* 2000-03-01 (mod 400 year, immediately after feb29 */
#define LEAPOCH (946684800LL + 86400 * (31 + 29))

#define DAYS_PER_400Y (365 * 400 + 97)
#define DAYS_PER_100Y (365 * 100 + 24)
#define DAYS_PER_4Y   (365 * 4   + 1)

constexpr uint32_t RTC_INIT_MAGIC = 0x32717a41;

int RTC::decodeBCD(uint32_t v, uint32_t t_shift, uint32_t t_mask, uint32_t u_shift, uint32_t u_mask) {
	return ((((v >> t_shift) & t_mask) * 10) + ((v >> u_shift) & u_mask));
}

int RTC::encodeBCD(uint32_t v, uint32_t t_shift, uint32_t t_mask, uint32_t u_shift, uint32_t u_mask) {
	return (((v / 10) & t_mask) <<  t_shift) | (((v % 10) & u_mask) << u_shift);
}

void RTC::init() {
	pwr_disable_backup_domain_write_protect();
	
	rcc_osc_on(RCC_LSI);
	rcc_wait_for_osc_ready(RCC_LSI);
	rcc_set_rtc_clock_source(RCC_LSI);
	rcc_enable_rtc_clock();
	
	rtc_unlock();
	rtc_set_init_flag();
	rtc_wait_for_init_ready();
	rtc_set_prescaler(Config::RTC_PRESCALER_S, Config::RTC_PRESCALER_A);
	rtc_set_am_format();
	rtc_clear_init_flag();
	
	// RTC alarm exti data line
	exti_enable_request(EXTI17);
	exti_set_trigger(EXTI17, EXTI_TRIGGER_RISING);
	
	// RTC alarm irq
	RTC_CR |= RTC_CR_ALRAIE;
	nvic_enable_irq(NVIC_RTC_IRQ);
	nvic_set_priority(NVIC_RTC_IRQ, 1);
	
	// Disable previous alarm
	RTC_CR &= ~RTC_CR_ALRAE;
	
	rtc_lock();
	rtc_wait_for_synchro();
	pwr_enable_backup_domain_write_protect();
	
	if (RTC_BKPXR(0) != RTC_INIT_MAGIC) {
		setDateTime(2022, 12, 19, 2, 36, 5);
		
		pwr_disable_backup_domain_write_protect();
		RTC_BKPXR(0) = RTC_INIT_MAGIC;
		for (int i = 1; i < 42; i++)
			RTC_BKPXR(i) = 0;
		pwr_enable_backup_domain_write_protect();
	}
}

void RTC::setDateTime(int y, int m, int d, int hh, int mm, int ss) {
	unlock();
	rtc_set_init_flag();
	rtc_wait_for_init_ready();
	
	rtc_calendar_set_year(y - 2000);
	rtc_calendar_set_month(m);
	rtc_calendar_set_day(d);
	rtc_time_set_time(hh, mm, ss, true);
	
	rtc_clear_init_flag();
	lock();
}

void RTC::setAlarm(int hh, int mm, int ss, int wday) {
	unlock();
	
	RTC_CR &= ~RTC_CR_ALRAE;
	while (!(RTC_ISR & RTC_ISR_ALRAWF));
	
	uint32_t reg = 0;
	
	if (wday >= 0) {
		reg |= encodeBCD(wday, RTC_ALRMXR_DT_SHIFT, RTC_ALRMXR_DT_MASK, RTC_ALRMXR_DU_SHIFT, RTC_ALRMXR_DU_MASK);
	} else {
		reg |= RTC_ALRMXR_MSK4;
	}
	
	if (hh >= 0) {
		reg |= encodeBCD(hh, RTC_ALRMXR_HT_SHIFT, RTC_ALRMXR_HT_MASK, RTC_ALRMXR_HU_SHIFT, RTC_ALRMXR_HU_MASK);
	} else {
		reg |= RTC_ALRMXR_MSK3;
	}
	
	if (mm >= 0) {
		reg |= encodeBCD(mm, RTC_ALRMXR_MNT_SHIFT, RTC_ALRMXR_MNT_MASK, RTC_ALRMXR_MNU_SHIFT, RTC_ALRMXR_MNU_MASK);
	} else {
		reg |= RTC_ALRMXR_MSK2;
	}
	
	if (ss >= 0) {
		reg |= encodeBCD(ss, RTC_ALRMXR_MNT_SHIFT, RTC_ALRMXR_ST_SHIFT, RTC_ALRMXR_SU_SHIFT, RTC_ALRMXR_SU_MASK);
	} else {
		reg |= RTC_ALRMXR_MSK1;
	}
	
	RTC_ALRMAR = reg;
	
	RTC_CR |= RTC_CR_ALRAE;
	
	lock();
}

void RTC::clearAlarm() {
	unlock();
	RTC_CR &= ~RTC_CR_ALRAE;
	while (!(RTC_ISR & RTC_ISR_ALRAWF));
	lock();
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

// https://git.musl-libc.org/cgit/musl/tree/src/time/__secs_to_tm.c
bool RTC::fromUnixTime(uint32_t t, tm *result) {
	long long days, secs, years;
	int remdays, remsecs, remyears;
	int qc_cycles, c_cycles, q_cycles;
	int months;
	static const char days_in_month[] = {31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29};
	
	secs = t - LEAPOCH;
	days = secs / 86400;
	remsecs = secs % 86400;
	if (remsecs < 0) {
		remsecs += 86400;
		days--;
	}
	
	qc_cycles = days / DAYS_PER_400Y;
	remdays = days % DAYS_PER_400Y;
	if (remdays < 0) {
		remdays += DAYS_PER_400Y;
		qc_cycles--;
	}
	
	c_cycles = remdays / DAYS_PER_100Y;
	if (c_cycles == 4)
		c_cycles--;
	remdays -= c_cycles * DAYS_PER_100Y;

	q_cycles = remdays / DAYS_PER_4Y;
	if (q_cycles == 25)
		q_cycles--;
	remdays -= q_cycles * DAYS_PER_4Y;
	
	remyears = remdays / 365;
	if (remyears == 4)
		remyears--;
	remdays -= remyears * 365;
	
	years = remyears + 4 * q_cycles + 100 * c_cycles + 400LL * qc_cycles;
	
	for (months = 0; days_in_month[months] <= remdays; months++)
		remdays -= days_in_month[months];
	
	if (months >= 10) {
		months -= 12;
		years++;
	}
	
	result->year = years + 2000;
	result->month = months + 3;
	result->day = remdays + 1;
	result->hours = remsecs / 3600;
	result->minutes = remsecs / 60 % 60;
	result->seconds = remsecs % 60;
	
	return true;
}

void RTC::readTime(tm *result) {
	result->year = 2000 + decodeBCD(RTC_DR, RTC_DR_YT_SHIFT, RTC_DR_YT_MASK, RTC_DR_YU_SHIFT, RTC_DR_YU_MASK);
	result->month = decodeBCD(RTC_DR, RTC_DR_MT_SHIFT, RTC_DR_MT_MASK, RTC_DR_MU_SHIFT, RTC_DR_MU_MASK);
	result->day = decodeBCD(RTC_DR, RTC_DR_DT_SHIFT, RTC_DR_DT_MASK, RTC_DR_DU_SHIFT, RTC_DR_DU_MASK);
	result->hours = decodeBCD(RTC_TR, RTC_TR_HT_SHIFT, RTC_TR_HT_MASK, RTC_TR_HU_SHIFT, RTC_TR_HU_MASK);
	result->minutes = decodeBCD(RTC_TR, RTC_TR_MNT_SHIFT, RTC_TR_MNT_MASK, RTC_TR_MNU_SHIFT, RTC_TR_MNU_MASK);
	result->seconds = decodeBCD(RTC_TR, RTC_TR_ST_SHIFT, RTC_TR_ST_MASK, RTC_TR_SU_SHIFT, RTC_TR_SU_MASK);
}

uint32_t RTC::time() {
	tm now;
	readTime(&now);
	return toUnixTime(now.year, now.month, now.day, now.hours, now.minutes, now.seconds);
}

void RTC::unlock() {
	pwr_disable_backup_domain_write_protect();
	rtc_unlock();
	rtc_enable_bypass_shadow_register();
}

void RTC::lock() {
	rtc_disable_bypass_shadow_register();
	rtc_lock();
	pwr_enable_backup_domain_write_protect();
}

void rtc_isr(void) {
	RTC_ISR &= ~RTC_ISR_ALRAF;
	exti_reset_request(EXTI17);
}
