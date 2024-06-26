#pragma once

// Based on code from RTEMS (https://www.rtems.org/), licensed under the GPL.

#include <cstddef>
#include <cstdint>

extern void (*timer_addr)();
extern uint64_t timer_max;
extern uint64_t ticks;

namespace Thorn {
	class Kernel;
}

namespace x86_64::APIC {
	extern uint32_t lastTPS;

	constexpr uint32_t BSP_VECTOR_SPURIOUS = 0xff;
	constexpr uint32_t BSP_VECTOR_APIC_TIMER = 32;
	constexpr uint32_t DISABLE = 0x10000;
	constexpr uint32_t ENABLE = 0x800;
	constexpr uint32_t EOI_ACK = 0;
	constexpr uint32_t MSR = 0x1b;
	constexpr uint32_t PIT_CALIBRATE_DIVIDER = 20;
	constexpr uint32_t PIT_CHAN2_TIMER_BIT = 1;
	constexpr uint32_t PIT_CHAN2_SPEAKER_BIT = 2;
	constexpr uint32_t PIT_FREQUENCY = 1193180;
	constexpr uint32_t PIT_CALIBRATE_TICKS = PIT_FREQUENCY / PIT_CALIBRATE_DIVIDER;
	constexpr uint32_t PIT_PORT_CHAN0 = 0x40;
	constexpr uint32_t PIT_PORT_CHAN1 = 0x41;
	constexpr uint32_t PIT_PORT_CHAN2 = 0x42;
	constexpr uint32_t PIT_PORT_CHAN2_GATE = 0x61;
	constexpr uint32_t PIT_PORT_MCR = 0x43;
	constexpr uint32_t PIT_SELECT_CHAN0 = 0b00000000;
	constexpr uint32_t PIT_SELECT_CHAN1 = 0b01000000;
	constexpr uint32_t PIT_SELECT_CHAN2 = 0b10000000;
	constexpr uint32_t PIT_SELECT_ACCESS_LOHI = 0b00110000;
	constexpr uint32_t PIT_SELECT_ONE_SHOT_MODE = 0b00000010;
	constexpr uint32_t PIT_SELECT_BINARY_MODE = 0;
	constexpr uint32_t REGISTER_APICID = 0x20 >> 2;
	constexpr uint32_t REGISTER_EOI = 0xb0 >> 2;
	constexpr uint32_t REGISTER_LVT_TIMER = 0x320 >> 2;
	constexpr uint32_t REGISTER_SPURIOUS = 0xf0 >> 2;
	constexpr uint32_t REGISTER_TIMER_CURRCNT = 0x390 >> 2;
	constexpr uint32_t REGISTER_TIMER_DIV = 0x3e0 >> 2;
	constexpr uint32_t REGISTER_TIMER_INITCNT = 0x380 >> 2;
	constexpr uint32_t SELECT_TMR_PERIODIC = 0x20000;
	constexpr uint32_t SPURIOUS_ENABLE = 0x100;
	constexpr uint32_t TIMER_DIVIDE_VALUE = 16;
	constexpr uint32_t TIMER_NUM_CALIBRATIONS = 5;
	constexpr uint32_t TIMER_SELECT_DIVIDER = 3;

	constexpr uint16_t ICR_MESSAGE_TYPE_FIXED = 0;
	constexpr uint16_t ICR_MESSAGE_TYPE_LOW_PRIORITY = 1 << 8;
	constexpr uint16_t ICR_MESSAGE_TYPE_SMI = 2 << 8;
	constexpr uint16_t ICR_MESSAGE_TYPE_REMOTE_READ = 3 << 8;
	constexpr uint16_t ICR_MESSAGE_TYPE_NMI = 4 << 8;
	constexpr uint16_t ICR_MESSAGE_TYPE_INIT = 5 << 8;
	constexpr uint16_t ICR_MESSAGE_TYPE_STARTUP = 6 << 8;
	constexpr uint16_t ICR_MESSAGE_TYPE_EXTERNAL = 7 << 8;

	void init(Thorn::Kernel &);
	void initTimer(uint32_t frequency);
	void reloadTimer(uint32_t initcnt);
	uint32_t calibrateTimer();
	void disableTimer();

	void wait(size_t num_ticks, uint32_t frequency);
}

extern volatile uint32_t *apic_base;
