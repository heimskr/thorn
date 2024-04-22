#include "arch/x86_64/kernel.h"
#include "arch/x86_64/mmu.h"
#include "kernel_core.h"


using Bitmap = uint32_t;

extern volatile char _kernel_physical_start[];
extern volatile char _kernel_physical_end;
extern volatile char _boot_end;
extern volatile Bitmap _bitmap_start[];
extern volatile Bitmap _bitmap_end;
extern volatile uint64_t pml4[];

namespace Boot {
	// constexpr static uint64_t pageSize = 2 << 20;
	constexpr static uint64_t pageSize = THORN_PAGE_SIZE;

	static inline int getPages() {
		return 8 * (&_bitmap_end - _bitmap_start);
	}

	static inline bool isPresent(uint64_t entry) {
		return entry & 1;
	}

	static inline uint16_t getPML4Index(uint64_t addr) { return (addr >> 39) & 0x1ff; }
	static inline uint16_t getPDPTIndex(uint64_t addr) { return (addr >> 30) & 0x1ff; }
	static inline uint16_t getPDTIndex (uint64_t addr) { return (addr >> 21) & 0x1ff; }
	static inline uint16_t getPTIndex  (uint64_t addr) { return (addr >> 12) & 0x1ff; }
	static inline uint16_t getOffset(uint64_t addr) { return addr & 0xfff; }

	static inline Bitmap * getBitmap() {
		return (Bitmap *) _bitmap_start;
	}

	int findFree(int start) {
		Bitmap *bitmap = getBitmap();
		const int pages = getPages();
		for (int i = start; i < pages / (8 * sizeof(Bitmap)); ++i)
			if (bitmap[i] != -1l)
				for (uint8_t j = 0; j < 8 * sizeof(Bitmap); ++j)
					if ((bitmap[i] & (1l << j)) == 0)
						return i * 8 * sizeof(Bitmap) + j;
		return -1;
	}

	static inline uint64_t addressToEntry(uint64_t address) {
		return (address & ~0xfff) | MMU_PRESENT | MMU_WRITABLE;
	}

	void markUsed(uint32_t index) {
		getBitmap()[index / (8 * sizeof(Bitmap))] |=   1l << (index % (8 * sizeof(Bitmap)));
	}

	void markFree(uint32_t index) {
		getBitmap()[index / (8 * sizeof(Bitmap))] &= ~(1l << (index % (8 * sizeof(Bitmap))));
	}

	bool isFree(uint32_t index) {
		// NB: Change the math here if Bitmap changes in size.
		static_assert(sizeof(Bitmap) == 4);
		return (getBitmap()[index >> 5] & (1 << (index & 31))) != 0;
	}

	uint64_t allocateFreePhysicalAddress(uint32_t consecutive_count) {
		if (consecutive_count == 0)
			return 0;

		uint64_t free_start = (((uint64_t) &_kernel_physical_end) + 0xfff) & ~0xfff;

		if (consecutive_count == 1) {
			int free_index = findFree(1);
			if (free_index == -1)
				return 0;
			markUsed(free_index);
			return free_start + free_index * pageSize;
		}

		int index = -1;
		for (;;) {
			index = findFree(index + 1);
			for (uint32_t i = 1; i < consecutive_count; ++i)
				if (!isFree(index + i))
					goto nope; // sorry
			markUsed(index);
			return free_start + index * pageSize;
			nope:
			continue;
		}
	}

	void allocate2MiB(uint64_t virt, uint64_t phys) {
		const auto pml4_index = getPML4Index(virt);
		const auto pdpt_index = getPDPTIndex(virt);
		const auto pdt_index  = getPDTIndex(virt);

		if (!isPresent(pml4[pml4_index])) {
			if (uint64_t free_addr = allocateFreePhysicalAddress(1)) {
				pml4[pml4_index] = addressToEntry(free_addr);
				for (int i = 0; i < 512; ++i) ((uint64_t *) free_addr)[i] = 0;
			} else {
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pdpt = (volatile uint64_t *) (pml4[pml4_index] & ~0xfff);

		if (!isPresent(pdpt[pdpt_index])) {
			if (uint64_t free_addr = allocateFreePhysicalAddress(1)) {
				pdpt[pdpt_index] = addressToEntry(free_addr);
				for (int i = 0; i < 512; ++i) ((uint64_t *) free_addr)[i] = 0;
			} else {
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pdt = (volatile uint64_t *) (pdpt[pdpt_index] & ~0xfff);

		pdt[pdt_index] = addressToEntry(phys) | MMU_PDE_TWO_MB;

		if (pdt[pdt_index] & 0b111101100000) {
			// asm("movl %0, %%eax; movl %1, %%ebx; movl %2, %%ecx; movl %3, %%edx; 1: hlt; jmp 1b" :: "r"((uint32_t) phys), "r"((uint32_t) (phys >> 32)), "r"((uint32_t) pdt[pdt_index]), "r"((uint32_t) (pdt[pdt_index] >> 32)));
		}
	}

	void allocate4KiB(uint64_t virt, uint64_t phys) {
		const auto pml4_index = getPML4Index(virt);
		const auto pdpt_index = getPDPTIndex(virt);
		const auto pdt_index  = getPDTIndex(virt);
		const auto pt_index   = getPTIndex(virt);

		if (!isPresent(pml4[pml4_index])) {
			if (uint64_t free_addr = allocateFreePhysicalAddress(1)) {
				pml4[pml4_index] = addressToEntry(free_addr);
				for (int i = 0; i < 512; ++i) ((uint64_t *) free_addr)[i] = 0;
			} else {
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pdpt = (volatile uint64_t *) (pml4[pml4_index] & ~0xfff);

		if (!isPresent(pdpt[pdpt_index])) {
			if (uint64_t free_addr = allocateFreePhysicalAddress(1)) {
				pdpt[pdpt_index] = addressToEntry(free_addr);
				for (int i = 0; i < 512; ++i) ((uint64_t *) free_addr)[i] = 0;
			} else {
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pdt = (volatile uint64_t *) (pdpt[pdpt_index] & ~0xfff);

		if (!isPresent(pdt[pdt_index])) {
			if (uint64_t free_addr = allocateFreePhysicalAddress(1)) {
				pdt[pdt_index] = addressToEntry(free_addr);
				for (int i = 0; i < 512; ++i) ((uint64_t *) free_addr)[i] = 0;
			} else {
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pt = (volatile uint64_t *) (pdt[pdt_index] & ~0xfff);
		pt[pt_index] = addressToEntry(phys);

		if (pt[pt_index] & 0b111101100000) {
			for (;;) asm("hlt");
		}

		// asm("movq %0, %rax; movq %1, %rbx; 1: hlt; jmp 1b" :: "r"(phys), "r"(pt[pt_index]));
	}

	static inline void allocate(uint64_t virt, uint64_t phys) {
		static_assert(pageSize == 4096 || pageSize == 2 << 20);

		if constexpr (pageSize == 4096) {
			allocate4KiB(virt, phys);
		} else {
			allocate2MiB(virt, phys);
		}
	}

	static inline void allocate(uint64_t virt) {
		allocate(virt, virt);
	}

	extern "C" void setup_paging() {
		for (int i = 0; i < PML4_SIZE / PML4_ENTRY_SIZE; ++i)
			pml4[i] = 0;

		// for (volatile uint64_t i = 0; i < 10'000'000'000; ++i); // Wait for GDB to attach

		for (uint64_t page = 0; page < 512 * (pageSize == 4096? 512 : 1); ++page) {
			allocate(page * pageSize);
			// allocate(((uint64_t) &_kernel_physical_start & ~(pageSize - 1)) + page * pageSize);
			// allocate((uint64_t) (0x7ffffffff000 & ~(pageSize - 1)) - page * pageSize);
			// allocate((uint64_t) KERNEL_VIRTUAL_START + page * pageSize);
			// allocate((uint64_t) 0xffffffff80000000 + page * pageSize, (((uint64_t) &_kernel_physical_end + pageSize - 1) & ~(pageSize - 1)) + page * pageSize);
			// allocate((uint64_t) 0xffffffff80000000 + page * pageSize, (uint64_t) _kernel_physical_start + page * pageSize);
			// allocate((uint64_t) 0xffffffff80000000 + page * pageSize, (uint64_t) &_boot_end + page * pageSize);
		}
	}
}
