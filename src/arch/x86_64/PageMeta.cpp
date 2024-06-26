#include "arch/x86_64/PageMeta.h"
#include "arch/x86_64/PageTableWrapper.h"
#include "lib/printf.h"
#include "memory/memset.h"
#include "ThornUtil.h"
#include "Kernel.h"

extern bool abouttodie;

namespace x86_64 {
	PageMeta::PageMeta(void *physical_start):
		physicalStart(physical_start) {}

	uintptr_t PageMeta::allocateFreePhysicalAddress(size_t consecutive_count) {
		if (consecutive_count == 0)
			return 0;

		if (consecutive_count == 1) {
			int free_index = findFree();
			if (free_index == -1)
				return 0;
			mark(free_index, true);
			return (uintptr_t) physicalStart + free_index * pageSize();
		}

		int index = -1;
		for (;;) {
			index = findFree(index + 1);
			for (size_t i = 1; i < consecutive_count; ++i)
				if (!isFree(index + i))
					goto nope; // sorry
			mark(index, true);
			return (uintptr_t) physicalStart + index * pageSize();
			nope:
			continue;
		}
	}

	uintptr_t PageMeta::allocateFreePhysicalFrame(size_t consecutive_count) {
		return reinterpret_cast<uintptr_t>(allocateFreePhysicalAddress(consecutive_count)) / THORN_PAGE_SIZE;
	}

	bool PageMeta::assignAddress(PageTableWrapper &wrapper, uintptr_t virtual_address, uintptr_t physical_address, uint64_t extra_meta) {
		using PTW = PageTableWrapper;
		uintptr_t out;
		if (physicalMemoryMapReady) {
			if (physical_address == 0xfee00000)
				printf("Assigning after PMM (virtual 0x%lx -> physical 0x%lx).\n", virtual_address, physical_address);
			out = assign(wrapper, PTW::getPML4Index(virtual_address), PTW::getPDPTIndex(virtual_address),
			             PTW::getPDTIndex(virtual_address), PTW::getPTIndex(virtual_address),
			             physical_address, extra_meta);
		} else {
			if (physical_address == 0xfee00000)
				printf("Assigning before PMM (virtual 0x%lx -> physical 0x%lx).\n", virtual_address, physical_address);
			out = assignBeforePMM(wrapper, PTW::getPML4Index(virtual_address), PTW::getPDPTIndex(virtual_address), PTW::getPDTIndex(virtual_address),
			                      PTW::getPTIndex(virtual_address), physical_address, extra_meta);
		}

		if (physical_address == 0xfee00000)
			printf("assignAddress(0x%lx) -> 0x%lx\n", virtual_address, out);
		return out;
	}

	bool PageMeta::identityMap(PageTableWrapper &wrapper, uintptr_t address, uint64_t extra_meta) {
		return assignAddress(wrapper, address, address, extra_meta);
	}

	bool PageMeta::modifyEntry(PageTableWrapper &wrapper, uintptr_t virtual_address, std::function<uint64_t(uint64_t)> modifier) {
		Thorn::Kernel *kernel = Thorn::Kernel::instance;
		if (!kernel) {
			printf("Kernel instance is null!\n");
			for (;;) asm("hlt");
		}

		// printf("\e[31mModifying\e[0m 0x%lx\n", virtual_address);
		using PTW = PageTableWrapper;
		const uint16_t pml4_index = PTW::getPML4Index(virtual_address);
		const uint16_t pdpt_index = PTW::getPDPTIndex(virtual_address);
		const uint16_t pdt_index = PTW::getPDTIndex(virtual_address);
		const uint16_t pt_index = PTW::getPTIndex(virtual_address);
		if (!isPresent(wrapper.entries[pml4_index]))
			return false;

		uint64_t *pdpt = (uint64_t *) (wrapper.entries[pml4_index] & ~0xfff);
		if (!isPresent(pdpt[pdpt_index]))
			return false;

		uint64_t *pdt = (uint64_t *) (pdpt[pdpt_index] & ~0xfff);
		if (!isPresent(pdt[pdt_index]))
			return false;

		uint64_t *pt = (uint64_t *) (pdt[pdt_index] & ~0xfff);
		if (!isPresent(pt[pt_index]))
			return false;

		pt[pt_index] = modifier(pt[pt_index]);
		return true;
	}

	bool PageMeta::andMeta(PageTableWrapper &wrapper, uintptr_t virtual_address, uint64_t meta) {
		return modifyEntry(wrapper, virtual_address, [meta](uint64_t entry) {
			return entry & meta;
		});
	}

	bool PageMeta::orMeta(PageTableWrapper &wrapper, uintptr_t virtual_address, uint64_t meta) {
		return modifyEntry(wrapper, virtual_address, [meta](uint64_t entry) {
			return entry | meta;
		});
	}

	bool PageMeta::freeEntry(PageTableWrapper &wrapper, uintptr_t virtual_address) {
		// printf("\e[31mfreeEntry\e[0m 0x%lx\n", virtual_address);
		return modifyEntry(wrapper, virtual_address, [](uint64_t) { return 0; });
	}

	uint64_t PageMeta::addressToEntry(volatile void *address) const {
		return addressToEntry(reinterpret_cast<uintptr_t>(address));
	}

	uint64_t PageMeta::addressToEntry(uintptr_t address) const {
		return (((uint64_t) address) & ~0xfff) | MMU_PRESENT | MMU_WRITABLE;
	}

	PageMeta4K::PageMeta4K():
		PageMeta(nullptr), pages(-1) {}

	PageMeta4K::PageMeta4K(void *physical_start, void *bitmap_address, int pages_):
		PageMeta(physical_start), pages(pages_), bitmap((Bitmap *) bitmap_address) {}

	size_t PageMeta4K::bitmapSize() const {
		if (pages == -1 || !bitmap)
			return 0;
		return Thorn::Util::updiv((size_t) pages, 8 * sizeof(Bitmap)) * sizeof(Bitmap);
	}

	size_t PageMeta4K::pageCount() const {
		return pages;
	}

	size_t PageMeta4K::pageSize() const {
		return 4 << 10;
	}

	void PageMeta4K::clear() {
		if (pages == -1)
			return;
		memset(bitmap, 0, Thorn::Util::updiv(pages, 8 * (int) sizeof(Bitmap)) * sizeof(Bitmap));
	}

	int PageMeta4K::findFree(size_t start) const {
		if (pages != -1)
			for (size_t i = start; i < pages / (8 * sizeof(Bitmap)); ++i)
				if (bitmap[i] != -1ul)
					for (uint8_t j = 0; j < 8 * sizeof(Bitmap); ++j)
						if ((bitmap[i] & (1ul << j)) == 0)
							return i * 8 * sizeof(Bitmap) + j;
		return -1;
	}

	void PageMeta4K::mark(int index, bool used) {
		if (pages == -1) {
			printf("[PageMeta4K::mark] pages == -1\n");
			return;
		}

		if (used) {
			bitmap[index / (8 * sizeof(Bitmap))] |=   1L << (index % (8 * sizeof(Bitmap)));
		} else {
			if (index == 2) {
				printf("\e[31mMarking 2 (0xe77000) as free\e[39m\n");
				Thorn::Kernel::backtrace();
			}
			bitmap[index / (8 * sizeof(Bitmap))] &= ~(1L << (index % (8 * sizeof(Bitmap))));
		}
	}

	uintptr_t PageMeta4K::assign(PageTableWrapper &wrapper, uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index,
	                             uintptr_t physical_address, uint64_t extra_meta) {
		// serprintf("\e[32massign\e[0m %u, %u, %u, %u, 0x%lx, 0x%lx\n", pml4_index, pdpt_index, pdt_index, pt_index, physical_address, extra_meta);

		if (pages == -1) {
			printf("[PageMeta4K::assign] pages == -1\n");
			return 0;
		}

		auto access = [&](uint64_t *ptr) -> uint64_t * {
			return (uint64_t *) ((uintptr_t) physicalMemoryMap + (uintptr_t) ptr);
		};

		if (!Thorn::Util::isCanonical(wrapper.entries)) {
			printf("PML4 (0x%lx) isn't canonical!\n", wrapper.entries);
			for (;;) asm("hlt");
		}
		if (!isPresent(wrapper.entries[pml4_index])) {
			// Allocate a page for a new PDPT if the PML4E is empty.
			if (auto free_addr = allocateFreePhysicalAddress()) {
				wrapper.entries[pml4_index] = addressToEntry(free_addr);
				memset((char *) physicalMemoryMap + (uintptr_t) free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
		}

		uint64_t *pdpt = (uint64_t *) (wrapper.entries[pml4_index] & ~0xfff);
		if (!Thorn::Util::isCanonical(pdpt)) {
			printf("PDPT (0x%lx) isn't canonical!\n", pdpt);
			wrapper.print(false);
			for (;;) asm("hlt");
		}
		pdpt = access(pdpt);
		if (!isPresent(pdpt[pdpt_index])) {
			// Allocate a page for a new PDT if the PDPE is empty.
			if (uintptr_t free_addr = allocateFreePhysicalAddress()) {
				pdpt[pdpt_index] = addressToEntry(free_addr);
				memset((char *) physicalMemoryMap + (uintptr_t) free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
		}

		uint64_t *pdt = (uint64_t *) (pdpt[pdpt_index] & ~0xfff);
		if (!Thorn::Util::isCanonical(pdt)) {
			printf("PDT (0x%lx) isn't canonical!\n", pdt);
			wrapper.print(false);
			for (;;) asm("hlt");
		}
		pdt = access(pdt);
		if (!isPresent(pdt[pdt_index])) {
			// Allocate a page for a new PT if the PDE is empty.
			if (uintptr_t free_addr = allocateFreePhysicalAddress()) {
				pdt[pdt_index] = addressToEntry(free_addr);
				memset((char *) physicalMemoryMap + (uintptr_t) free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
		}

		uint64_t *pt = (uint64_t *) (pdt[pdt_index] & ~0xfff);
		uintptr_t assigned = 0;
		if (!Thorn::Util::isCanonical(pt)) {
			printf("PT (0x%lx) isn't canonical!\n", pt);
			wrapper.print(false);
			for (;;) asm("hlt");
		}
		pt = access(pt);
		if (!isPresent(pt[pt_index])) {
			// Allocate a new page if the PTE is empty (or, optionally, use a provided physical address).
			if (physical_address) {
				pt[pt_index] = addressToEntry(physical_address) | extra_meta;
			} else if (uintptr_t free_addr = allocateFreePhysicalAddress()) {
				pt[pt_index] = addressToEntry(free_addr) | extra_meta;
				memset((char *) physicalMemoryMap + (uintptr_t) free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
			assigned = pt[pt_index];
		} else {
			// Nothing really needed to be done anyway...
		}

		return assigned;
	}

	uintptr_t PageMeta4K::assignBeforePMM(PageTableWrapper &wrapper, uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index,
	                                      uint16_t pt_index, uintptr_t physical_address, uint64_t extra_meta) {
		// serprintf("\e[32massignBeforePMM\e[0m %u, %u, %u, %u, 0x%lx, 0x%lx\n", pml4_index, pdpt_index, pdt_index, pt_index, physical_address, extra_meta);

		if (pages == -1) {
			printf("[PageMeta4K::assign] pages == -1\n");
			return 0;
		}

		constexpr uintptr_t magic = 0x2000000;

		auto access = [&](volatile uint64_t *ptr) -> volatile uint64_t * {
			if ((uintptr_t) ptr < magic)
				return ptr;
			return (volatile uint64_t *) ((uintptr_t) physicalMemoryMap + (uintptr_t) ptr);
		};

		if (!Thorn::Util::isCanonical(wrapper.entries)) {
			printf("PML4 (0x%lx) isn't canonical!\n", wrapper.entries);
			for (;;) asm("hlt");
		}
		if (!isPresent(wrapper.entries[pml4_index])) {
			// Allocate a page for a new PDPT if the PML4E is empty.
			if (uintptr_t free_addr = allocateFreePhysicalAddress()) {
				wrapper.entries[pml4_index] = addressToEntry(free_addr);
				if (!disableMemset)
					memset((void *) free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pdpt = (volatile uint64_t *) (wrapper.entries[pml4_index] & ~0xfff);
		if (!Thorn::Util::isCanonical(pdpt)) {
			printf("PDPT (0x%lx) isn't canonical!\n", pdpt);
			wrapper.print(false);
			for (;;) asm("hlt");
		}
		pdpt = access(pdpt);
		if (!isPresent(access(pdpt)[pdpt_index])) {
			// Allocate a page for a new PDT if the PDPE is empty.
			if (uintptr_t free_addr = allocateFreePhysicalAddress()) {
				pdpt[pdpt_index] = addressToEntry(free_addr);
				if (!disableMemset)
					memset((void *) free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pdt = (volatile uint64_t *) (pdpt[pdpt_index] & ~0xfff);
		if (!Thorn::Util::isCanonical(pdt)) {
			printf("PDT (0x%lx) isn't canonical!\n", pdt);
			wrapper.print(false);
			for (;;) asm("hlt");
		}

		const volatile uint64_t *old_pdt = pdt;
		pdt = access(pdt);
		if (!isPresent(pdt[pdt_index])) {
			// Allocate a page for a new PT if the PDE is empty.
			if (uintptr_t free_addr = allocateFreePhysicalAddress()) {
				pdt[pdt_index] = addressToEntry(free_addr);
				if (!disableMemset)
					memset((void *) free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
		}

		volatile uint64_t *pt = (volatile uint64_t *) (pdt[pdt_index] & ~0xfff);
		// printf("pdt = 0x%lx, index = %u, pt = 0x%lx\n", pdt, pdt_index, pt);
		uintptr_t assigned = 0;
		if (!Thorn::Util::isCanonical(pt)) {
			printf("PT (0x%lx) isn't canonical!\n", pt);
			printf("PDT: 0x%lx -> 0x%lx\n", old_pdt, pdt);
			printf("pdt_index: 0x%x\n", pdt_index);
			printf("PML4 %u -> PDPT %u -> PDT %u -> PT %u\n", pml4_index, pdpt_index, pdt_index, pt_index);
			wrapper.print(false);
			for (;;) asm("hlt");
		}
		pt = access(pt);
		if (!isPresent(pt[pt_index])) {
			// Allocate a new page if the PTE is empty (or, optionally, use a provided physical address).
			if (physical_address) {
				pt[pt_index] = addressToEntry(physical_address);
			} else if (uintptr_t free_addr = allocateFreePhysicalAddress()) {
				pt[pt_index] = addressToEntry(free_addr) | extra_meta;
				if (!disableMemset)
					memset((void *) free_addr, 0, 4096);
			} else {
				printf("No free pages!\n");
				for (;;) asm("hlt");
			}
			assigned = pt[pt_index];
		} else {
			// Nothing really needed to be done anyway...
		}

		return assigned;
	}

	void PageMeta4K::assignSelf(PageTableWrapper &wrapper) {
		if (pages == -1) {
			printf("PageMeta4K::assignSelf failed: pages == -1\n");
			return;
		}

		uint16_t pml4i, pdpti, pdti, pti;
		const size_t bsize = bitmapSize(), psize = pageSize();
		for (size_t i = 0; i < bsize; i += psize) {
			void *address = bitmap + i;
			pml4i = PageTableWrapper::getPML4Index(address);
			pdpti = PageTableWrapper::getPDPTIndex(address);
			 pdti = PageTableWrapper:: getPDTIndex(address);
			  pti = PageTableWrapper::  getPTIndex(address);
			assign(wrapper, pml4i, pdpti, pdti, pti);
		}
	}

	PageMeta4K::operator bool() const {
		return pages != -1;
	}

	size_t PageMeta4K::pagesUsed() const {
		size_t out = 0;
		size_t popcnt = 0;
		for (size_t i = 0; i < bitmapSize() / sizeof(Bitmap); ++i) {
			asm("popcnt %1, %0" : "=r"(popcnt) : "r"(bitmap[i]));
			out += popcnt;
		}
		return out;
	}

	bool PageMeta4K::isFree(size_t index) const {
		// NB: Change the math here if Bitmap changes in size.
		static_assert(sizeof(Bitmap) == 8);
		return (bitmap[index >> 6] & (1 << (index & 63))) != 0;
	}
}
