#include "arch/x86_64/CPU.h"
#include <string.h>

namespace x86_64 {
	void cpuid(unsigned value, unsigned leaf, unsigned &eax, unsigned &ebx, unsigned &ecx, unsigned &edx) {
		asm volatile("cpuid" : "=a" (eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a" (value), "c" (leaf));
	}

	void getModel(char *out) {
		unsigned int eax, ebx, ecx, edx;
		cpuid(0, 0, eax, ebx, ecx, edx);
		unsigned char i = 0;
		while (ebx)
			out[i++] = ebx & 0xff, ebx >>= 8;
		while (edx)
			out[i++] = edx & 0xff, edx >>= 8;
		while (ecx)
			out[i++] = ecx & 0xff, ecx >>= 8;
		out[i] = '\0';
	}

	bool checkAPIC() {
		unsigned int eax, ebx, ecx, edx;
		cpuid(1, 0, eax, ebx, ecx, edx);
		return !!(edx & (1 << 9));
	}

	int coreCount() {
		char model[13];
		getModel(model);
		unsigned eax, ebx, ecx, edx;
		if (strcmp(model, "GenuineIntel") == 0) {
			cpuid(4, 0, eax, ebx, ecx, edx);
			return ((eax >> 26) & 0x3f) + 1;
		} else {
			cpuid(0x80000008, 0, eax, ebx, ecx, edx);
			return (ecx & 0xff) + 1;
		}
	}

	bool arat() {
		unsigned int eax, ebx, ecx, edx;
		cpuid(6, 0, eax, ebx, ecx, edx);
		return (eax & 0b100) != 0;
	}
}
