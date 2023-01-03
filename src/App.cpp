#include "App.h"

#include <algorithm>
#include <climits>

#include "Loop.h"
#include "Task.h"
#include "Exti.h"
#include "Gpio.h"
#include "RTC.h"
#include "Button.h"
#include "Buzzer.h"
#include "Debug.h"
#include "utils.h"

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/iwdg.h>

bool App::setStateBit(uint32_t bit, bool value) {
	bool is_changed = (value != is(bit));
	if (value) {
		m_state |= bit;
	} else {
		m_state &= ~bit;
	}
	
	if (is_changed) {
		gpio_set(Pinout::I2C_IRQ.port, Pinout::I2C_IRQ.pin);
		m_task_irq_pulse.setTimeout(10);
	}
	
	return is_changed;
}

void App::initHw() {
	Gpio::setAllAnalog();
	
	rcc_periph_clock_enable(RCC_SYSCFG_COMP);
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOF);
	rcc_periph_clock_enable(RCC_RTC);
	rcc_periph_clock_enable(RCC_PWR);
	rcc_periph_clock_enable(RCC_I2C1);
	
	// Set clock to 4 MHz (low power)
	/*
	rcc_set_ppre(RCC_CFGR_PPRE_DIV4);
	rcc_set_hpre(RCC_CFGR_PPRE_NODIV);
	rcc_apb1_frequency = 8000000 / 2;
	rcc_ahb_frequency = 8000000 / 2;
	*/
	
	// VCC_EN
	gpio_mode_setup(Pinout::VCC_EN.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, Pinout::VCC_EN.pin);
	gpio_clear(Pinout::VCC_EN.port, Pinout::VCC_EN.pin);
	
	// BAT_TEMP_EN
	gpio_mode_setup(Pinout::BAT_TEMP_EN.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, Pinout::BAT_TEMP_EN.pin);
	gpio_clear(Pinout::BAT_TEMP_EN.port, Pinout::BAT_TEMP_EN.pin);
	
	// CHARGER_EN
	gpio_mode_setup(Pinout::CHARGER_EN.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, Pinout::CHARGER_EN.pin);
	gpio_clear(Pinout::CHARGER_EN.port, Pinout::CHARGER_EN.pin);
	
	// I2C_IRQ
    gpio_set_output_options(Pinout::I2C_IRQ.port, GPIO_OTYPE_OD, GPIO_OSPEED_LOW, Pinout::I2C_IRQ.pin);
    gpio_mode_setup(Pinout::I2C_IRQ.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, Pinout::I2C_IRQ.pin);
    gpio_set(Pinout::I2C_IRQ.port, Pinout::I2C_IRQ.pin);
	
	// PWR_KEY
	gpio_mode_setup(Pinout::PWR_KEY.port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, Pinout::PWR_KEY.pin);
	
	// CHARGER_STATUS
	gpio_mode_setup(Pinout::CHARGER_STATUS.port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, Pinout::CHARGER_STATUS.pin);
	
	// BUZZER_PWM
	gpio_set_output_options(Pinout::BUZZER_PWM.port, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, Pinout::BUZZER_PWM.pin);
	gpio_mode_setup(Pinout::BUZZER_PWM.port, GPIO_MODE_AF, GPIO_PUPD_NONE, Pinout::BUZZER_PWM.pin);
	gpio_set_af(Pinout::BUZZER_PWM.port, GPIO_AF4, Pinout::BUZZER_PWM.pin);
	
	// I2C
	gpio_set_output_options(Pinout::I2C_SCL.port, GPIO_OTYPE_OD, GPIO_OSPEED_LOW, Pinout::I2C_SCL.pin);
	gpio_set_output_options(Pinout::I2C_SDA.port, GPIO_OTYPE_OD, GPIO_OSPEED_LOW, Pinout::I2C_SDA.pin);
	gpio_mode_setup(Pinout::I2C_SCL.port, GPIO_MODE_AF, GPIO_PUPD_NONE, Pinout::I2C_SCL.pin);
	gpio_mode_setup(Pinout::I2C_SDA.port, GPIO_MODE_AF, GPIO_PUPD_NONE, Pinout::I2C_SDA.pin);
	gpio_set_af(Pinout::I2C_SCL.port, GPIO_AF4, Pinout::I2C_SCL.pin);
	gpio_set_af(Pinout::I2C_SDA.port, GPIO_AF4, Pinout::I2C_SDA.pin);
	
	#if DEBUG
	// USART for debug
	rcc_periph_clock_enable(RCC_USART1);
	gpio_mode_setup(Pinout::USART_TX.port, GPIO_MODE_AF, GPIO_PUPD_NONE, Pinout::USART_TX.pin);
	gpio_set_af(Pinout::USART_TX.port, GPIO_AF1, Pinout::USART_TX.pin);
	uart_simple_setup(USART1, 115200, 1);
	#endif
}

void App::checkBatteryTemp(const char *name, int min, int max, Flags flag_lo, Flags flag_hi) {
	auto temp = m_mon.getBatTemp();
	
	if (is(flag_lo) && temp > min + Config::BAT.t_hysteresis) {
		LOGD("Battery %s now is OK (%d.%d °C)\r\n", name, idec(temp), iexp(temp));
		setStateBit(flag_lo, false);
	}
	
	if (is(flag_hi) && temp < max - Config::BAT.t_hysteresis) {
		LOGD("Battery %s now is OK (%d.%d °C)\r\n", name, idec(temp), iexp(temp));
		setStateBit(flag_hi, false);
	}
	
	if (!is(flag_lo) && temp <= min) {
		LOGD("Battery %s is too LOW! (%d.%d °C)\r\n", name, idec(temp), iexp(temp));
		setStateBit(flag_lo, true);
	}
	
	if (!is(flag_hi) && temp >= max) {
		LOGD("Battery %s is to HIGH!!! (%d.%d °C)\r\n", name, idec(temp), iexp(temp));
		setStateBit(flag_hi, true);
	}
}

void App::buzzerTask(void *) {
	if (m_buzzer_freq) {
		Buzzer::play(m_buzzer_freq, m_buzzer_vol);
	} else {
		Buzzer::stop();
	}
}

void App::watchdogTask(void *) {
	iwdg_reset();
	m_task_watchdog.setTimeout(Config::WATCHDOG_TIMEOUT - 5000);
}

void App::irqPulseTask(void *) {
	gpio_clear(Pinout::I2C_IRQ.port, Pinout::I2C_IRQ.pin);
}

void App::monitorTask(void *) {
	m_mon.read();
	
	if (setStateBit(DCIN_PRESENT, m_mon.isDcinPresent())) {
		LOGD("DCIN %s!\r\n", is(DCIN_PRESENT) ? "connected" : "disconnected");
		
		if (is(DCIN_PRESENT)) {
			m_dcin_connected = Loop::ms();
			
			// New power source plugged?
			if (m_last_chrg_failure == CHRG_FAIL_BAD_DCIN) {
				m_last_chrg_failure = CHRG_FAIL_NO_DCIN;
				m_last_chrg_failure_cnt = 0;
				m_last_chrg_failure_time = Loop::ms();
				m_dcin_bad_cnt = 0;
			}
		}
	}
	
	if (setStateBit(DCIN_GOOD, is(DCIN_PRESENT) && m_mon.isDcinGood())) {
		if (is(DCIN_GOOD) || is(DCIN_PRESENT))
			LOGD("DCIN voltage is %s [%d mV]\r\n", is(DCIN_GOOD) ? "OK" : "BAD", m_mon.getDcin());
	}
	
	if (setStateBit(BAT_PRESENT, m_mon.isBatPresent()))
		LOGD("Battery %s!\r\n", is(BAT_PRESENT) ? "connected" : "disconnected");
	
	checkBatteryTemp("temperature for discharging", Config::BAT.t_min, Config::BAT.t_max, BAT_LOW_TEMP, BAT_HIGH_TEMP);
	checkBatteryTemp("temperature for charging", Config::BAT.t_chrg_min, Config::BAT.t_chrg_max, BAT_CHARGE_LOW_TEMP, BAT_CHARGE_HIGH_TEMP);
	
	auto chrg_fail = checkChargingAllowed();
	if (!is(BAT_CHARGE_EN) && chrg_fail == CHRG_FAIL_NONE && !isChargingDisabled()) {
		LOGD("Charging allowed\r\n");
		m_last_charging = Loop::ms();
		setStateBit(BAT_CHARGE_EN, true);
	}
	
	if (is(BAT_CHARGE_EN) && chrg_fail != CHRG_FAIL_NONE) {
		LOGD("Charging is NOT allowed, reason=%s\r\n", getEnumName(chrg_fail));
		m_last_chrg_failure = chrg_fail;
		m_last_chrg_failure_time = Loop::ms();
		if (Loop::ms() - m_last_charging < Config::MIN_CHARGE_TIME) {
			m_last_chrg_failure_cnt++;
			
			if (chrg_fail == CHRG_FAIL_NO_DCIN)
				m_dcin_bad_cnt++;
		} else {
			m_last_chrg_failure_cnt = 0;
			m_dcin_bad_cnt = 0;
		}
		
		setStateBit(BAT_CHARGE_EN, false);
	}
	
	if (is(BAT_CHARGE_EN)) {
		gpio_set(Pinout::CHARGER_EN.port, Pinout::CHARGER_EN.pin);
	} else {
		gpio_clear(Pinout::CHARGER_EN.port, Pinout::CHARGER_EN.pin);
	}
	
	if (setStateBit(BAT_CHARGING, m_charger_status.isPressed() && is(BAT_CHARGE_EN)))
		LOGD("Battery %s\r\n", is(BAT_CHARGING) ? "is charging..." : "is stop charging!");
	
	if (!is(POWER_ON) && !is(USER_POWER_OFF) && !isAutoPowerOnDisabled())
		powerOn();
	
	if (is(POWER_ON)) {
		auto pwr_fail = checkPowerOnAllowed();
		if (pwr_fail != PWR_FAIL_NONE) {
			LOGD("Power-on not allowed, reason=%s\r\n", getEnumName(pwr_fail));
			LOGD("Force power-off system power!!!\r\n");
			powerOff(false);
		}
	}
	
	uint32_t info_print_freq = is(BAT_CHARGING) ? 5000 : 30000;
	if (!m_last_info_print || Loop::ms() - m_last_info_print >= info_print_freq) {
		LOGD(
			"BAT: %d mV / %d.%d%% / %d.%d °C | DCIN: %d mV | CPU: %d.%d °C\r\n",
			m_mon.getVbat(), idec(m_mon.getBatPct()), iexp(m_mon.getBatPct()), idec(m_mon.getBatTemp()), iexp(m_mon.getBatTemp()),
			m_mon.getDcin(),
			idec(m_mon.getCpuTemp()), iexp(m_mon.getCpuTemp())
		);
		m_last_info_print = Loop::ms();
	}
	
	uint32_t next_timeout = 30000;
	if (is(BAT_CHARGING)) {
		next_timeout = 200;
	} else if (is(BAT_CHARGE_EN)) {
		next_timeout = 500;
	} else if (is(DCIN_GOOD)) {
		next_timeout = 1000;
	} else if (is(DCIN_PRESENT) && Loop::ms() - m_dcin_connected <= 5000) {
		next_timeout = 1000;
	} else if (!is(POWER_ON) && !is(DCIN_PRESENT)) {
		bool allow_sleep = (
			!gpio_get(Pinout::DCIN_ADC.port, Pinout::DCIN_ADC.pin) &&
			!gpio_get(Pinout::PWR_KEY.port, Pinout::PWR_KEY.pin)
		);
		if (allow_sleep) {
			allowDeepSleep(true);
			return;
		}
	}
	
	m_task_analog_mon.setTimeout(next_timeout);
	iwdg_reset();
}

void App::allowDeepSleep(bool flag) {
	if (flag) {
		m_task_analog_mon.cancel();
		m_task_watchdog.cancel();
	} else {
		m_task_analog_mon.setTimeout(0);
		m_task_watchdog.setTimeout(0);
	}
}

const char *App::getEnumName(ChrgFailureReason reason) {
	switch (reason) {
		case CHRG_FAIL_NONE:		return "NONE";
		case CHRG_FAIL_LOW_TEMP:	return "LOW_TEMP";
		case CHRG_FAIL_HIGH_TEMP:	return "HIGH_TEMP";
		case CHRG_FAIL_NO_DCIN:		return "NO_DCIN";
		case CHRG_FAIL_BAD_DCIN:	return "BAD_DCIN";
		case CHRG_FAIL_NO_BAT:		return "NO_BAT";
	}
	return "???";
}

const char *App::getEnumName(PwrOnFailureReason reason) {
	switch (reason) {
		case PWR_FAIL_NONE:					return "NONE";
		case PWR_FAIL_BAT_IS_LOW:			return "BAT_IS_LOW";
		case PWR_FAIL_BAT_TEMP_IS_LOW:		return "BAT_TEMP_IS_LOW";
		case PWR_FAIL_BAT_TEMP_IS_HIGH:		return "BAT_TEMP_IS_HIGH";
	}
	return "???";
}

uint32_t App::getTimeoutForChrgFail() {
	switch (m_last_chrg_failure) {
		case CHRG_FAIL_LOW_TEMP:	return Config::CHARGING_BAD_TEMP_TIMEOUT;
		case CHRG_FAIL_HIGH_TEMP:	return Config::CHARGING_BAD_TEMP_TIMEOUT;
		case CHRG_FAIL_NO_DCIN:		return Config::CHARGING_LOST_DCIN_TIMEOUT;
		case CHRG_FAIL_BAD_DCIN:	return Config::CHARGING_BAD_DCIN_TIMEOUT;
		case CHRG_FAIL_NO_BAT:		return 0;
		case CHRG_FAIL_NONE:		return 0;
	}
	return 0;
}

bool App::isChargingDisabled() {
	if (!is(DCIN_GOOD))
		return true;
	return m_last_chrg_failure_time && (Loop::ms() - m_last_chrg_failure_time < getTimeoutForChrgFail());
}

bool App::isAutoPowerOnDisabled() {
	if (m_last_pwron_fail == PWR_FAIL_BAT_IS_LOW) {
		if (!is(BAT_CHARGE_EN))
			return true;
	}
	
	if (m_last_pwron_fail == PWR_FAIL_BAT_TEMP_IS_HIGH)
		return true;
	
	return false;
}

App::ChrgFailureReason App::checkChargingAllowed() {
	if (!is(BAT_PRESENT))
		return CHRG_FAIL_NO_BAT;
	if (!is(DCIN_PRESENT))
		return CHRG_FAIL_NO_DCIN;
	if (!is(DCIN_GOOD)) {
		if (m_dcin_bad_cnt >= 5)
			return CHRG_FAIL_BAD_DCIN;
		return CHRG_FAIL_NO_DCIN;
	}
	if (is(BAT_CHARGE_LOW_TEMP))
		return CHRG_FAIL_LOW_TEMP;
	if (is(BAT_CHARGE_HIGH_TEMP))
		return CHRG_FAIL_HIGH_TEMP;
	return CHRG_FAIL_NONE;
}

App::PwrOnFailureReason App::checkPowerOnAllowed() {
	if (!is(DCIN_GOOD)) {
		if (m_mon.isBatDischarged())
			return PWR_FAIL_BAT_IS_LOW;
		if (is(BAT_LOW_TEMP))
			return PWR_FAIL_BAT_TEMP_IS_LOW;
		if (is(BAT_HIGH_TEMP))
			return PWR_FAIL_BAT_TEMP_IS_HIGH;
	}
	return PWR_FAIL_NONE;
}

void App::powerOn() {
	auto pwr_fail = checkPowerOnAllowed();
	if (pwr_fail == PWR_FAIL_NONE) {
		LOGD("System power is ON\r\n");
		m_last_pwron = Loop::ms();
		setStateBit(POWER_ON, true);
		setStateBit(USER_POWER_OFF, false);
		gpio_set(Pinout::VCC_EN.port, Pinout::VCC_EN.pin);
	} else {
		LOGD("Power-on not allowed, reason=%s\r\n", getEnumName(pwr_fail));
	}
	m_task_analog_mon.setTimeout(0);
}

void App::powerOff(bool user) {
	LOGD("System power is OFF\r\n");
	setStateBit(POWER_ON, false);
	setStateBit(USER_POWER_OFF, user);
	gpio_clear(Pinout::VCC_EN.port, Pinout::VCC_EN.pin);
	m_buzzer_freq = 0;
	m_task_analog_mon.setTimeout(0);
	m_task_buzzer.setTimeout(0);
}

void App::onDcinChange(void *, bool) {
	m_task_analog_mon.setTimeout(0);
}

void App::onBatChange(void *, bool) {
	m_task_analog_mon.setTimeout(0);
}

void App::onChargerStatus(void *, Button::Event) {
	m_task_analog_mon.setTimeout(0);
}

void App::onPwrKey(void *, Button::Event evt) {
	if (evt == Button::EVT_PRESS) 
		m_mon.ignoreDcinVoltage(true);
	
	if (evt == Button::EVT_RELEASE || evt == Button::EVT_LONGRELEASE)
		m_mon.ignoreDcinVoltage(false);
	
	if (evt == Button::EVT_RELEASE) {
		m_last_pwron_fail = PWR_FAIL_NONE;
		setStateBit(USER_POWER_OFF, false);
	}
	
	if (evt == Button::EVT_LONGPRESS)
		powerOff(true);
	
	setStateBit(PWR_KEY_PRESSED, evt == Button::EVT_PRESS || evt == Button::EVT_LONGPRESS);
	
	m_task_analog_mon.setTimeout(0);
}

bool App::idleHook(void *) {
	LOGD("No tasks, going to deep sleep...\r\n\r\n\r\n");
	
	pwr_disable_backup_domain_write_protect();
	RTC_BKPXR(1) = (m_state & USER_POWER_OFF);
	pwr_enable_backup_domain_write_protect();
	
	Loop::suspend(true);
	allowDeepSleep(false);
	return true;
}

uint32_t App::readReg(void *, uint8_t reg) {
	if (reg == I2C_REG_IRQ_STATUS) {
		gpio_set(Pinout::I2C_IRQ.port, Pinout::I2C_IRQ.pin);
		m_task_irq_pulse.cancel();
	}
	
	switch (reg) {
		case I2C_REG_STATUS:				return m_state;
		case I2C_REG_IRQ_STATUS:			return m_state;
		case I2C_REG_BAT_VOLTAGE:			return m_mon.getVbat();
		case I2C_REG_BAT_TEMP:				return m_mon.getBatTemp();
		case I2C_REG_BAT_MIN_TEMP:			return Config::BAT.t_min;
		case I2C_REG_BAT_MAX_TEMP:			return Config::BAT.t_max;
		case I2C_REG_BAT_PCT:				return (m_state & BAT_CHARGING) ? std::min(99 * 1000, m_mon.getBatPct()) : m_mon.getBatPct();
		case I2C_REG_DCIN_VOLTAGE:			return m_mon.getDcin();
		case I2C_REG_CPU_TEMP:				return m_mon.getCpuTemp();
		case I2C_REG_GET_MIN_BAT_VOLTAGE:	return Config::BAT.v_min;
		case I2C_REG_GET_MAX_BAT_VOLTAGE:	return Config::BAT.v_max;
		case I2C_REG_RTC_TIME:				return RTC::time();
	}
	return 0xFFFFFFFF;
}

void App::writeReg(void *, uint8_t reg, uint32_t value) {
	switch (reg) {
		case I2C_REG_POWER_OFF:
			// Poweron
			if (value == 0)
				powerOn();
			
			// Shutdown
			if (value == 1)
				powerOff(true);
			
			// Reboot
			if (value == 2) {
				powerOff(false);
				m_task_analog_mon.setTimeout(2000);
			}
		break;
		
		case I2C_REG_RTC_TIME:
			RTC::tm new_tm;
			RTC::fromUnixTime(value, &new_tm);
			RTC::setDateTime(new_tm.year, new_tm.month, new_tm.day, new_tm.hours, new_tm.minutes, new_tm.seconds);
		break;
		
		case I2C_REG_PLAY_BUZZER:
			m_buzzer_freq = (value >> 8) & 0xFFFF;
			m_buzzer_vol = value & 0xFF;
			m_task_buzzer.setTimeout(0);
		break;
	}
}

int App::run() {
	initHw();
	
	iwdg_reset();
	iwdg_set_period_ms(Config::WATCHDOG_TIMEOUT);
	iwdg_start();
	
	RTC::init();
	Loop::init();
	Buzzer::init();
	m_mon.init();
	
	// Restore settings
	m_state = RTC_BKPXR(1);
	
	#if DEBUG_CALIBRATE_RTC
	gpio_mode_setup(Pinout::USART_TX.port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, Pinout::USART_TX.pin);
	uint32_t last = RTC_TR, now;
	while (true) {
		while ((now = RTC_TR) == last);
		last = now;
		gpio_toggle(Pinout::USART_TX.port, Pinout::USART_TX.pin);
	}
	#endif
	
	#if DEBUG_CALIBRATE_BAT_TEMP
	uint32_t last_time = 0;
	while (true) {
		if (!last_time || Loop::ms() - last_time > 1000) {
			m_mon.read();
			LOGD(
				"BAT TEMP: %d mV / %d.%d °C\r\n",
				m_mon.getBatTempRaw(), idec(m_mon.getBatTemp()), iexp(m_mon.getBatTemp())
			);
			last_time = Loop::ms();
		}
		__asm__ volatile("wfi");
	}
	#endif
	
	// I2C
	I2CSlave::init();
	I2CSlave::setCallback(
		I2CSlave::ReadCallback::make<&App::readReg>(*this),
		I2CSlave::WriteCallback::make<&App::writeReg>(*this)
	);
	
	// Idle hook
	Loop::setIdleCallback(Loop::IdleCallback::make<&App::idleHook>(*this));
	
	// IRQ pulse
	m_task_irq_pulse.init(Task::Callback::make<&App::irqPulseTask>(*this));
	
	// Buzzer task
	m_task_buzzer.init(Task::Callback::make<&App::buzzerTask>(*this));
	
	// Analog monitor task
	m_task_analog_mon.init(Task::Callback::make<&App::monitorTask>(*this));
	m_task_analog_mon.setTimeout(0);
	
	// Watchdog task
	m_task_watchdog.init(Task::Callback::make<&App::watchdogTask>(*this));
	m_task_watchdog.setTimeout(0);
	
	// Power key
	m_pwr_key.update(gpio_get(Pinout::PWR_KEY.port, Pinout::PWR_KEY.pin) != 0);
	m_pwr_key.init(Button::Callback::make<&App::onPwrKey>(*this));
	Exti::set(Pinout::PWR_KEY.port, Pinout::PWR_KEY.pin, Exti::BOTH, Exti::Callback::make<&Button::handleExti>(m_pwr_key));
	
	// Charger status
	m_charger_status.update(gpio_get(Pinout::CHARGER_STATUS.port, Pinout::CHARGER_STATUS.pin) == 0);
	m_charger_status.init(Button::Callback::make<&App::onChargerStatus>(*this));
	m_charger_status.setTimings(1000, 5000);
	Exti::set(Pinout::CHARGER_STATUS.port, Pinout::CHARGER_STATUS.pin, Exti::BOTH, Exti::Callback::make<&Button::handleExtiInverted>(m_charger_status));
	
	// Monitoring presence of DCIN/VBAT
	Exti::set(Pinout::DCIN_ADC.port, Pinout::DCIN_ADC.pin, Exti::BOTH, Exti::Callback::make<&App::onDcinChange>(*this));
	Exti::set(Pinout::VBAT_ADC.port, Pinout::VBAT_ADC.pin, Exti::BOTH, Exti::Callback::make<&App::onBatChange>(*this));
	
	LOGD("----------------------------------------------------------------\r\n");
	LOGD("PMIC started!\r\n");
	
	Loop::run();
	
	return 0;
}
