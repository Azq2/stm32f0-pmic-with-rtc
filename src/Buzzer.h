#pragma once

#include <cstdint>

class Buzzer {
	protected:
		static bool m_playing;
	
	public:
		static void init();
		static void play(uint32_t freq, uint32_t duty_pct);
		static void stop();
};
