#include "arch/x86_64/PageMeta.h"
#include "lib/printf.h"
#include "memory/memset.h"
#include "Kernel.h"

namespace x86_64 {
	PageMeta::PageMeta(void *physical_start, void *virtual_start):
		physicalStart(physical_start), virtualStart(virtual_start) {}

	void * PageMeta::allocateFreePhysicalAddress() {
		int free_index = findFree();
		if (free_index == -1)
			return nullptr;
		mark(free_index, true);
		return (void *) ((uintptr_t) physicalStart + free_index * pageSize());
	}

	uint64_t PageMeta::addressToEntry(void *address) const {
		return (((uint64_t) address) & ~0xfff) | MMU_PRESENT | MMU_WRITABLE;
	}
	
	PageMeta2M::PageMeta2M(void *physical_start, void *virtual_start, int max_):
		PageMeta(physical_start, virtual_start), max(max_) {}

	size_t PageMeta2M::pageCount() const {
		return sizeof(bitmap);
	}

	size_t PageMeta2M::pageSize() const {
		return 2 << 20;
	}

	void PageMeta2M::clear() {
		memset(bitmap, 0, sizeof(bitmap));
	}

	int PageMeta2M::findFree() const {
		for (unsigned int i = 0; i < max / sizeof(bitmap_t); ++i)
			if (bitmap[i] != -1L)
				for (unsigned int j = 0; j < sizeof(bitmap_t); ++j)
					if ((bitmap[i] & (1 << j)) == 0)
						return (i * sizeof(bitmap_t)) + j;
		return -1;
	}

	void PageMeta2M::mark(int index, bool used) {
		if (used)
			bitmap[index / sizeof(bitmap_t)] |= 1 << (index % sizeof(bitmap_t));
		else
			bitmap[index / sizeof(bitmap_t)] &= ~(1 << (index % sizeof(bitmap_t)));
	}

	bool PageMeta2M::assign(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index) {
		DsOS::Kernel *kernel = DsOS::Kernel::instance;
		if (!kernel) {
			printf("Kernel instance is null!\n");
			for (;;);
		}

		PageTableWrapper &wrapper = kernel->kernelPML4;
		if (wrapper.entries[pml4_index] == 0) {
			// Allocate a page for a new PDPT if the PML4E is empty.
			if (void *free_addr = allocateFreePhysicalAddress()) {
				wrapper.entries[pml4_index] = addressToEntry(free_addr);
			} else {
				printf("No free pages!\n");
				for (;;);
			}
		}

		uint64_t *pdpt = (uint64_t *) (wrapper.entries[pml4_index] & ~0xfff);
		if (pdpt[pdpt_index] == 0) {
			// Allocate a page for a new PDT if the PDPE is empty.
			if (void *free_addr = allocateFreePhysicalAddress()) {
				pdpt[pdpt_index] = addressToEntry(free_addr);
			} else {
				printf("No free pages!\n");
				for (;;);
			}
		}

		uint64_t *pdt = (uint64_t *) (pdpt[pdpt_index] & ~0xfff);
		if (pdt[pdt_index] == 0) {
			// Allocate a page for a new PT if the PDE is empty.
			if (void *free_addr = allocateFreePhysicalAddress()) {
				pdt[pdt_index] = addressToEntry(free_addr);
			} else {
				printf("No free pages!\n");
				for (;;);
			}
		}

		uint64_t *pt = (uint64_t *) (pdt[pdt_index] & ~0xfff);
		if (pt[pt_index] == 0) {
			// Allocate a new page if the PTE is empty.
			if (void *free_addr = allocateFreePhysicalAddress()) {
				pt[pt_index] = addressToEntry(free_addr);
			} else {
				printf("No free pages!\n");
				for (;;);
			}
		} else {
			// Nothing really needed to be done anyway...
		}

		return true;
	}
}