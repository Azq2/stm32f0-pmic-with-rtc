#pragma once

#include <cstdint>

class RTC {
	public:
		static void init();
		static uint32_t time();
		static void setDateTime(uint16_t y, uint8_t m, uint8_t d, uint8_t hh, uint8_t mm, uint8_t ss);
		
		static constexpr inline int decodeBCD(uint32_t v, uint32_t t_shift, uint32_t t_mask, uint32_t u_shift, uint32_t u_mask) {
			return ((((v >> t_shift) & t_mask) * 10) + ((v >> u_shift) & u_mask));
		}
		
		static uint32_t toUnixTime(int year, int month, int day, int hours, int minutes, int seconds);
};
