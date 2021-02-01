#pragma once

#include <cstdint>

namespace Thorn::Util {
	template <typename T>
	inline T upalign(T number, int alignment) {
		return number + ((alignment - (number % alignment)) % alignment);
	}

	template <typename T>
	inline T downalign(T number, int alignment) {
		return number - (number % alignment);
	}

	template <typename T>
	inline T updiv(T n, T d) {
		return n / d + (n % d? 1 : 0);
	}

	inline uintptr_t __attribute__((always_inline)) getReturnAddress() {
		uintptr_t return_address;
		asm volatile("mov 8(%%rbp), %0" : "=r"(return_address));
		return return_address;
	}

	inline bool isCanonical(uintptr_t address) {
		return address < 0x0000800000000000 || 0xffff800000000000 <= address;
	}

	inline bool isCanonical(void *address) {
		return isCanonical((uintptr_t) address);
	}
}