#include "AnalogMon.h"
#include "Exti.h"

#include <algorithm>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rcc.h>

void AnalogMon::init() {
	gpio_mode_setup(Pinout::BAT_TEMP.port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, Pinout::BAT_TEMP.pin);
	switchFormAdcToExti(true);
	
	rcc_periph_clock_enable(RCC_ADC);
	rcc_periph_clock_enable(RCC_DMA1);
	
	// ADC
	adc_power_off(ADC1);
	adc_set_clk_source(ADC1, ADC_CLKSOURCE_ADC);
	adc_calibrate(ADC1);
	adc_set_operation_mode(ADC1, ADC_MODE_SCAN);
	adc_disable_external_trigger_regular(ADC1);
	adc_set_right_aligned(ADC1);
	adc_enable_temperature_sensor();
	adc_enable_vrefint();
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPTIME_239DOT5);
	adc_set_regular_sequence(ADC1, COUNT_OF(m_adc_channels), (uint8_t *) m_adc_channels);
	adc_enable_dma(ADC1);
	adc_enable_dma_circular_mode(ADC1);
	adc_set_resolution(ADC1, ADC_RESOLUTION_12BIT);
	adc_disable_analog_watchdog(ADC1);
	
	// DMA
	dma_enable_mem2mem_mode(DMA1, DMA_CHANNEL1);
	dma_set_memory_size(DMA1, DMA_CHANNEL1, DMA_CCR_MSIZE_16BIT);
	dma_set_peripheral_size(DMA1, DMA_CHANNEL1, DMA_CCR_PSIZE_16BIT);
	dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL1);
	dma_set_read_from_peripheral(DMA1, DMA_CHANNEL1);
	dma_set_peripheral_address(DMA1, DMA_CHANNEL1, reinterpret_cast<uint32_t>(&ADC_DR(ADC1)));
	dma_set_memory_address(DMA1, DMA_CHANNEL1, reinterpret_cast<uint32_t>(&m_adc_result));
	dma_set_number_of_data(DMA1, DMA_CHANNEL1, COUNT_OF(m_adc_result));
	dma_enable_circular_mode(DMA1, DMA_CHANNEL1);
	dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL1);
}

void AnalogMon::switchFormAdcToExti(bool to_exti) {
	if (to_exti) {
		gpio_mode_setup(Pinout::DCIN_ADC.port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, Pinout::DCIN_ADC.pin);
		gpio_mode_setup(Pinout::VBAT_ADC.port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, Pinout::VBAT_ADC.pin);
		Exti::enable(Pinout::DCIN_ADC.port, Pinout::DCIN_ADC.pin);
		Exti::enable(Pinout::VBAT_ADC.port, Pinout::VBAT_ADC.pin);
	} else {
		Exti::disable(Pinout::DCIN_ADC.port, Pinout::DCIN_ADC.pin);
		Exti::disable(Pinout::VBAT_ADC.port, Pinout::VBAT_ADC.pin);
		gpio_mode_setup(Pinout::DCIN_ADC.port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, Pinout::DCIN_ADC.pin);
		gpio_mode_setup(Pinout::VBAT_ADC.port, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, Pinout::VBAT_ADC.pin);
	}
}

void AnalogMon::read() {
	switchFormAdcToExti(false);
	
	adc_power_on(ADC1);
	dma_enable_channel(DMA1, DMA_CHANNEL1);
	
	uint32_t result[COUNT_OF(m_adc_result)] = {};
	
	for (int i = 0; i < ADC_AVG_CNT; i++) {
		adc_start_conversion_regular(ADC1);
		while (!dma_get_interrupt_flag(DMA1, DMA_CHANNEL1, DMA_TCIF));
		dma_clear_interrupt_flags(DMA1, DMA_CHANNEL1, DMA_TCIF);
		
		for (size_t j = 0; j < COUNT_OF(m_adc_result); j++) {
			result[j] += m_adc_result[j];
			if (i == ADC_AVG_CNT - 1)
				result[j] = result[j] / ADC_AVG_CNT;
		}
	}
	
	dma_disable_channel(DMA1, DMA_CHANNEL1);
	adc_power_off(ADC1);
	
	switchFormAdcToExti(true);
	
	int vrefint = 3300 * VREFINT_CAL / result[VREF];
	m_vbat = toVoltage(result[VBAT], vrefint, Config::VBAT_RDIV);
	m_cpu_temp = toTemperature(result[CPU_TEMP], Config::CPU_TEMP);
	m_bat_temp = toTemperature(toVoltage(result[BAT_TEMP], vrefint, 1000), Config::BAT_TEMP);
	
	if (!m_ignore_dcin && Loop::ms() >= m_last_dcin_ignore)
		m_dcin = toVoltage(result[DCIN], vrefint, Config::DCIN_RDIV);
}

int AnalogMon::toVoltage(int raw_value, int vref, int rdiv) {
	return raw_value * vref / 4095 * rdiv / 1000;
}

int AnalogMon::toTemperature(int raw_value, const Config::Temp &calibration) {
	return calibration.T[0] - (calibration.value[0] - raw_value) * (calibration.T[1] - calibration.T[0]) / (calibration.value[1] - calibration.value[0]);
}

int AnalogMon::getBatPct() {
	return ((getVbat() - Config::BAT.v_min) * 100000) / (Config::BAT.v_max - Config::BAT.v_min);
}
