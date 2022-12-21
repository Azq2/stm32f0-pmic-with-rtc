#include "Button.h"
#include "Debug.h"
#include "utils.h"
#include <cstdio>

void Button::init(Callback callback, void *user_data) {
	m_callback = callback;
	m_user_data = user_data;
	
	m_press_timer.init(Task::Callback::make<&Button::onPress>(*this));
	m_release_timer.init(Task::Callback::make<&Button::onRelease>(*this));
	m_longpress_timer.init(Task::Callback::make<&Button::onLongPress>(*this));
}

void Button::onPress(void *) {
	m_callback(m_user_data, m_state);
}

void Button::onLongPress(void *) {
	m_state = Event::EVT_LONGPRESS;
	m_callback(m_user_data, m_state);
}

void Button::onRelease(void *) {
	bool is_longpressed = isLongPressed();
	m_state = EVT_RELEASE;
	m_longpress_timer.cancel();
	m_callback(m_user_data, is_longpressed ? EVT_LONGRELEASE : EVT_RELEASE);
}

void Button::handleExti(void *, bool state) {
	update(state);
}

void Button::handleExtiInverted(void *, bool state) {
	update(!state);
}

void Button::update(bool new_state) {
	if (isPressed() && !new_state)
		m_release_timer.setTimeout(m_debounce);
	
	if (!isPressed() && new_state) {
		m_state = EVT_PRESS;
		m_release_timer.cancel();
		m_press_timer.setTimeout(0);
		m_longpress_timer.setTimeout(m_longpress_time);
	}
}
