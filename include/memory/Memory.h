#pragma once

#include "Defs.h"
#include "lib/printf.h"

#define MEMORY_ALIGN 32
// #define DECLARE_MEMORY_OPERATORS
// #define SKIP_MEMORY_DECLARATIONS

void spin(size_t time = 3);

namespace Thorn {
	class Memory {
		public:
			struct BlockMeta {
				size_t size;
				BlockMeta *next;
				bool free;
			};

		private:
			static constexpr size_t PAGE_LENGTH = 4096;

			// size_t align;
			size_t allocated = 0;
			char *start, *high, *end;
			BlockMeta *base = nullptr;
			uintptr_t highestAllocated = 0;

			uintptr_t realign(uintptr_t);
			BlockMeta * findFreeBlock(BlockMeta * &last, size_t);
			BlockMeta * requestSpace(BlockMeta *last, size_t);
			void split(BlockMeta &, size_t);
			int merge();

		public:
			Memory(const Memory &) = delete;
			Memory(Memory &&) = delete;

			Memory(char *start_, char *high_);
			Memory();

			Memory & operator=(const Memory &) = delete;
			Memory & operator=(Memory &&) = delete;

			void * allocate(size_t size, size_t alignment = 0);
			void free(void *);
			void setBounds(char *new_start, char *new_high);
			BlockMeta * getBlock(void *);
			size_t getAllocated() const;
			size_t getUnallocated() const;
	};
}

extern "C" {
	void * malloc(size_t);
	void * calloc(size_t, size_t);
	void free(void *);
	int posix_memalign(void **memptr, size_t alignment, size_t size);
}

void * malloc(size_t size, size_t alignment);

extern Thorn::Memory *global_memory;

#define MEMORY_OPERATORS_SET

namespace std {
	struct nothrow_t;
}

#ifndef SKIP_MEMORY_DECLARATIONS
	#if defined(__clang__) || defined(DECLARE_MEMORY_OPERATORS)
		void * operator new(size_t size);
		void * operator new[](size_t size);
		void * operator new(size_t, void *ptr);
		void * operator new[](size_t, void *ptr);
		void operator delete(void *ptr)   noexcept;
		void operator delete[](void *ptr) noexcept;
		void operator delete(void *, void *)   noexcept;
		void operator delete[](void *, void *) noexcept;
		void operator delete(void *, unsigned long)   noexcept;
		void operator delete[](void *, unsigned long) noexcept;
	#else
		#ifndef __cpp_exceptions
			inline void * operator new(size_t size)   throw() { return malloc(size); }
			inline void * operator new(size_t size, const std::nothrow_t &)   throw() { return malloc(size); }
			inline void * operator new[](size_t size) throw() { return malloc(size); }
			inline void * operator new(size_t size, std::align_val_t align) throw() { return malloc(size, size_t(align)); }
			inline void * operator new(size_t size, std::align_val_t &align, const std::nothrow_t &) noexcept { return malloc(size, size_t(align)); }
			inline void * operator new(size_t, void *ptr)   throw() { return ptr; }
			inline void * operator new[](size_t, void *ptr) throw() { return ptr; }
			inline void operator delete(void *ptr)   throw() { free(ptr); }
			inline void operator delete[](void *ptr) throw() { free(ptr); }
			inline void operator delete(void *, void *)   throw() {}
			inline void operator delete[](void *, void *) throw() {}
			inline void operator delete(void *, unsigned long)   throw() {}
			inline void operator delete[](void *, unsigned long) throw() {}
			inline void operator delete(void *, unsigned long, std::align_val_t) noexcept {}
		#else
			inline void * operator new(size_t size)   { return malloc(size); }
			inline void * operator new[](size_t size) { return malloc(size); }
			inline void * operator new(size_t size, std::align_val_t &align, const std::nothrow_t &) noexcept { return malloc(size, size_t(align)); }
			inline void * operator new(size_t, void *ptr)   { return ptr; }
			inline void * operator new[](size_t, void *ptr) { return ptr; }
			inline void operator delete(void *ptr)   noexcept { free(ptr); }
			inline void operator delete[](void *ptr) noexcept { free(ptr); }
			inline void operator delete(void *, void *)   noexcept {}
			inline void operator delete[](void *, void *) noexcept {}
			inline void operator delete(void *, unsigned long)   noexcept {}
			inline void operator delete[](void *, unsigned long) noexcept {}
			inline void operator delete(void *, unsigned long, std::align_val_t) noexcept {}
		#endif
	#endif
#endif
