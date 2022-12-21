#pragma once
#include <libopencm3/stm32/gpio.h>

namespace Pinout {
	struct Pin {
		uint32_t port;
		uint16_t pin;
	};
	
	constexpr uint32_t ADC_CH_DCIN		= 0;
	constexpr uint32_t ADC_CH_VBAT		= 1;
	constexpr uint32_t ADC_CH_TEMP		= 4;
	
	constexpr const Pin DCIN_ADC		= {GPIOA, GPIO0};
	constexpr const Pin VBAT_ADC		= {GPIOA, GPIO1};
	
	constexpr const Pin USART_TX		= {GPIOA, GPIO2};
	constexpr const Pin PWR_KEY			= {GPIOA, GPIO3};
	constexpr const Pin BAT_TEMP		= {GPIOA, GPIO4};
	constexpr const Pin BAT_TEMP_EN		= {GPIOA, GPIO5};
	
	constexpr const Pin I2C_IRQ			= {GPIOF, GPIO0};
	constexpr const Pin I2C_SDA			= {GPIOA, GPIO10};
	constexpr const Pin I2C_SCL			= {GPIOA, GPIO9};
	
	constexpr const Pin CHARGER_EN		= {GPIOB, GPIO1};
	constexpr const Pin CHARGER_STATUS	= {GPIOA, GPIO7};
	constexpr const Pin VCC_EN			= {GPIOA, GPIO6};
};
