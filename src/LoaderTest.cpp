#include "progs.h"
#include "Kernel.h"
#include "lib/printf.h"

namespace Thorn {
	void loaderTest() {
		if (!Kernel::instance) {
			printf("No kernel instance!");
			Kernel::perish();
		}

		// Lock<Mutex> pager_lock;
		// x86_64::PageMeta4K &pager = Kernel::getPager(pager_lock);
		// Kernel &kernel = *Kernel::instance;

		// void *page = pager.allocateFreePhysicalAddress();
		// memset(page, 0, pager.pageSize());

		// kernel.processes

	}
}
