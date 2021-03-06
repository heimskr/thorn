#ifndef KERNEL_H_
#define KERNEL_H_

#include <stddef.h>

#include "kernel_core.h"
#include "arch/x86_64/PageMeta.h"
#include "arch/x86_64/PageTableWrapper.h"
#include "hardware/Keyboard.h"
#include "memory/Memory.h"

namespace Thorn {
	class Kernel {
		private:
			uintptr_t memoryLow = 0;
			uintptr_t memoryHigh = 0;
			Memory memory;

			/** The area where page descriptors are stored. */
			char *pageDescriptors = nullptr;

			/** The length of the pageDescriptors area in bytes. */
			size_t pageDescriptorsLength = 0;

			/** The area where actual pages are stored. */
			void *pagesStart = nullptr;

			/** The size of the area where actual pages are stored in bytes. */
			size_t pagesLength = 0;

			/** Uses data left behind by multiboot to determine the boundaries of physical memory. */
			void detectMemory();

			/** Carves the usable region of physical memory into a section for page descriptors and a section for
			 *  pages. */
			void arrangeMemory();

			void initPhysicalMemoryMap();

			/** Sets all page descriptors to zero. */
			void initPageDescriptors();

		public:
			x86_64::PageTableWrapper kernelPML4;
			x86_64::PageMeta4K pager;

			/** A region at the very top of virtual memory is mapped to all physical memory. This address stores the
			 *  start of that region. */
			void *physicalMemoryMap = nullptr;

			static Kernel *instance;
			static Kernel & getInstance();

			Kernel() = delete;
			Kernel(const x86_64::PageTableWrapper &pml4_);

			void main();

			static void wait(size_t num_ticks, uint32_t frequency = 1);
			static void perish();

			void schedule();

			void onKey(Keyboard::InputKey, bool down);

			static void backtrace();
			static void backtrace(uintptr_t *);
			static x86_64::PageMeta4K & getPager();

			friend void runTests();
	};
}

#endif
