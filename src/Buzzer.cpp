#include "Buzzer.h"
#include "Debug.h"

#include <algorithm>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

bool Buzzer::m_playing = false;

void Buzzer::init() {
	rcc_periph_clock_enable(RCC_TIM14);
	rcc_periph_reset_pulse(RST_TIM14);
	timer_set_mode(TIM14, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	
	timer_continuous_mode(TIM14);
	timer_enable_break_main_output(TIM14);
	timer_set_prescaler(TIM14, 0);
	timer_set_oc_mode(TIM14, TIM_OC1, TIM_OCM_PWM1);
	timer_enable_oc_preload(TIM14, TIM_OC1);
	timer_set_oc_polarity_high(TIM14, TIM_OC1);
	timer_enable_oc_output(TIM14, TIM_OC1);
	
	rcc_periph_clock_disable(RCC_TIM14);
}

void Buzzer::play(uint32_t freq, uint32_t duty_pct) {
	rcc_periph_clock_enable(RCC_TIM14);
	uint32_t apr = 0;
	uint32_t psc = 0;
	
	freq = std::max(20UL, std::min(20000UL, freq));
	duty_pct = std::min(100UL, duty_pct);
	
	do {
		psc++;
		apr = rcc_apb1_frequency / (psc * freq);
	} while (apr >= 0xFFFF);
	
	uint32_t duty_period = duty_pct > 0 ? std::max(1UL, ((apr / 2) * duty_pct / 100)) : 1;
	
	timer_set_prescaler(TIM14, psc - 1);
	timer_set_period(TIM14, apr - 1);
	timer_set_oc_value(TIM14, TIM_OC1, duty_period);
	
	if (!m_playing) {
		timer_enable_preload(TIM14);
		timer_enable_counter(TIM14);
	}
	
	m_playing = true;
}

void Buzzer::stop() {
	if (m_playing) {
		timer_disable_preload(TIM14);
		timer_disable_counter(TIM14);
		rcc_periph_clock_disable(RCC_TIM14);
		m_playing = false;
	}
}
