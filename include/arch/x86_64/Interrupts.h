#pragma once

#include <stdint.h>

namespace x86_64::IDT {
	constexpr int SIZE = 256;

	struct Header {
		uint16_t size;
		uint32_t start;
	} __attribute__((packed));
			
	struct Descriptor {
		uint16_t offset_1;
		uint16_t selector;
		uint8_t ist;
		uint8_t type_attr;
		uint16_t offset_2;
		uint32_t offset_3;
		uint32_t zero;
	} __attribute__((packed));


	void add(int index, void (*fn)());
	void init();
}

extern "C" {
	extern x86_64::IDT::Descriptor idt[x86_64::IDT::SIZE];
	extern x86_64::IDT::Header idt_header;
	extern void isr_0();
	extern void isr_14();

	void div0();
	void page_interrupt();
}