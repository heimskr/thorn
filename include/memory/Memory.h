#ifndef MEMORY_MEMORY_H_
#define MEMORY_MEMORY_H_

#include "Defs.h"
#include "lib/printf.h"

#define MEMORY_ALIGN 32

void spin(size_t time = 3);

namespace DsOS {
	class Memory {
		public:
			struct BlockMeta {
				size_t size;
				BlockMeta *next;
				bool free;
			};

		private:
			size_t align;
			char *start, *high, *end;
			BlockMeta *base = nullptr;

			char * realign(char * &);
			BlockMeta * findFreeBlock(BlockMeta **last, size_t);
			BlockMeta * requestSpace(BlockMeta *last, size_t);
			void split(BlockMeta &, size_t);
			BlockMeta * getBlock(void *);
			int merge();

		public:
			Memory(const Memory &) = delete;
			Memory(Memory &&) = delete;

			Memory(char *start_, char *high_);
			Memory();

			Memory & operator=(const Memory &) = delete;
			Memory & operator=(Memory &&) = delete;

			void * allocate(size_t);
			void free(void *);
			void setBounds(char *new_start, char *new_high);
	};
}

void * malloc(size_t);
void * calloc(size_t, size_t);
void free(void *);

extern DsOS::Memory *global_memory;

#define MEMORY_OPERATORS_SET

inline void * operator new(size_t size)   throw() { return malloc(size); }
inline void * operator new[](size_t size) throw() { return malloc(size); }
inline void * operator new(size_t, void *ptr)   throw() { return ptr; }
inline void * operator new[](size_t, void *ptr) throw() { return ptr; }
inline void operator delete(void *ptr)   throw() { free(ptr); }
inline void operator delete[](void *ptr) throw() { free(ptr); }
inline void operator delete(void *, void *)   throw() {}
inline void operator delete[](void *, void *) throw() {}
inline void operator delete(void *, unsigned long)   throw() {}
inline void operator delete[](void *, unsigned long) throw() {}

#endif