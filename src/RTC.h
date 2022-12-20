#pragma once

#include <cstdint>

class RTC {
	public:
		struct tm {
			int year;
			int month;
			int day;
			int hours;
			int minutes;
			int seconds;
		};
		
		static constexpr int ALARM_ANY = -1;
	
	protected:
		static void lock();
		static void unlock();
		
	public:
		static void init();
		static uint32_t time();
		static void readTime(tm *result);
		static void setDateTime(int y, int m, int d, int hh, int mm, int ss);
		static inline void setDateTime(const tm *t) {
			setDateTime(t->year, t->month, t->day, t->hours, t->minutes, t->seconds);
		}
		
		static int decodeBCD(uint32_t v, uint32_t t_shift, uint32_t t_mask, uint32_t u_shift, uint32_t u_mask);
		static int encodeBCD(uint32_t v, uint32_t t_shift, uint32_t t_mask, uint32_t u_shift, uint32_t u_mask);
		
		static uint32_t toUnixTime(int year, int month, int day, int hours, int minutes, int seconds);
		static inline uint32_t toUnixTime(const tm *t) {
			return toUnixTime(t->year, t->month, t->day, t->hours, t->minutes, t->seconds);
		}
		
		static bool fromUnixTime(uint32_t t, tm *result);
		
		static void setAlarm(int hh = ALARM_ANY, int mm = ALARM_ANY, int ss = ALARM_ANY, int wday = ALARM_ANY);
		static void clearAlarm();
};
