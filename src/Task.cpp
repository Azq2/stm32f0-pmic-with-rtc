#include "Loop.h"
#include "Task.h"
#include "utils.h"

void Task::exec() {
	if (!m_loop)
		cancel();
	
	if (m_callback)
		m_callback(m_user_data);
	
	if (m_loop)
		run(m_interval, true);
}

void Task::run(uint32_t ms, bool loop) {
	ENTER_CRITICAL();
	
	if (m_enabled)
		cancel();
	
	m_interval = ms;
	m_next_run = Loop::ms() + ms;
	m_loop = loop;
	
	if (!Loop::last()) {
		Loop::first(this);
		Loop::last(this);
	} else {
		Loop::last()->m_next = this;
		m_prev = Loop::last();
		m_next = nullptr;
		Loop::last(this);
	}
	
	m_enabled = true;
	
	Loop::onChange();
	
	EXIT_CRITICAL();
}

void Task::cancel() {
	ENTER_CRITICAL();
	
	if (!m_enabled) {
		EXIT_CRITICAL();
		return;
	}
	
	if (this == Loop::first())
		Loop::first(m_next);
	
	if (this == Loop::last())
		Loop::last(m_prev);
	
	if (m_prev)
		m_prev->m_next = m_next;
	
	if (m_next)
		m_next->m_prev = m_prev;
	
	m_prev = nullptr;
	m_next = nullptr;
	
	m_enabled = false;
	
	Loop::onChange();
	
	EXIT_CRITICAL();
}
