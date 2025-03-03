#pragma once

#include <iostream>
#include <vector>
#include <windows.h> // Для HeapAlloc и HeapFree (Windows-специфично)

#ifdef DEBUG_RC_MEM_MGR
#define MSG0(arg); printf(arg);
#define MSG1(arg1,arg2); printf(arg1,arg2);
#else
#define MSG0(arg);
#define MSG1(arg1,arg2);
#endif
#define U1 unsigned char
#define U4 unsigned long

/*
list element format
|0 3||4 7||8 11||12 15||16 .. n|
[PREV][NEXT][COUNT][ SIZE ][payload]
U4 U4 U4 U4 ?
byte allocated/freed is address of first byte of payload
header = 16 bytes
byte[0] is occupied by header data, so is always used, thus
first link has prev=0 ( 0 indicates not used )
last link has next=0
*/
#define PREV(i) (*((U4*)(&ram[i-16])))
#define NEXT(i) (*((U4*)(&ram[i-12])))
#define COUNT(i) (*((U1*)(&ram[i-8]))) /*# references*/
#define SIZE(i) (*((U4*)(&ram[i-4])))
#define FREE 0 /*free blocks have COUNT=0*/

#define START 16 /*address of first payload*/
#define SZ_HEADER 16
class RefCountMemoryManager
{
private:
	HANDLE handle;
	U1* ram; /*memory storage*/
	U4 size;
	int checkAddress(void* addr);
	int checkIndex(U4 free);
	void release(U4 free);
	void split(U4 addr, U4 nbytes);
	void merge(U4 prev, U4 current, U4 next);
public:
	RefCountMemoryManager(U4 nbytes);
	~RefCountMemoryManager();
	void* allocate(U4 nbytes);
	void inc(void* addr);
	void dec(void* addr);
	void printState();
};

RefCountMemoryManager::RefCountMemoryManager(U4 nbytes)
{
	handle = GetProcessHeap();
	if (handle == NULL)
	{
		printf("RefCountMemoryManager::");
		printf("RefCountMemoryManager():");
		printf("invalid handle\n");
		exit(1);
	}
	ram = (U1*)HeapAlloc(handle, HEAP_ZERO_MEMORY, nbytes);
	//for portability, you could use:
	//ram = (unsigned char*)malloc(nbytes);
	size = nbytes;
	if (size <= SZ_HEADER)
	{
		printf("RefCountMemoryManager::");
		printf("RefCountMemoryManager():");
		printf("not enough memory fed to constructor\n");
		exit(1);
	}
	PREV(START) = 0;
	NEXT(START) = 0;
	COUNT(START) = 0;
	SIZE(START) = size - SZ_HEADER;
	MSG0("RefCountMemoryManager::");
	MSG1("RefCountMemoryManager(%lu)\n", nbytes);
	return;
}/*end constructor--------------------------------------------*/

RefCountMemoryManager::~RefCountMemoryManager()
{
	if (HeapFree(handle, HEAP_NO_SERIALIZE, ram) == 0)
	{
		printf("RefCountMemoryManager::");
		printf("~RefCountMemoryManager():");
		printf("could not free heap storage\n");
		return;
	}
	//for portability, you could use:
	//free(ram);
	MSG0("RefCountMemoryManager::");
	MSG0("~RefCountMemoryManager()");
	MSG1("free ram[%lu]\n", size);
	return;
}/*end destructor---------------------------------------------*/

/*
U4 nbytes - number of bytes required
returns address of first byte of memory region allocated
( or NULL if cannot allocate a large enough block )
*/

void* RefCountMemoryManager::allocate(U4 nbytes)
{
	U4 current;
	MSG0("RefCountMemoryManager::");
	MSG1("allocate(%lu)\n", nbytes);
	if (nbytes == 0)
	{
		MSG0("RefCountMemoryManager::");
		MSG0("allocate(): zero bytes requested\n");
		return(NULL);
	}
	//traverse the linked list, starting with first element
	current = START;
	while (NEXT(current) != 0)
	{
		if ((SIZE(current) >= nbytes) && (COUNT(current) == FREE))
		{
			split(current, nbytes);
			return((void*)&ram[current]);
		}
		current = NEXT(current);
	}
	//handle the last block ( which has NEXT(current)=0 )
	if ((SIZE(current) >= nbytes) && (COUNT(current) == FREE))
	{
		split(current, nbytes);
		return((void*)&ram[current]);
	}
	return(NULL);
}/*end allocation---------------------------------------------*/

/*
breaks [free] region into [alloc][free] pair, if possible
*/
void RefCountMemoryManager::split(U4 addr, U4 nbytes)
{
	/*
	want payload to have enough room for
	nbytes = size of request
	SZ_HEADER = header for new region
	SZ_HEADER = payload for new region (arbitrary 16 bytes)
	*/
	if (SIZE(addr) >= nbytes + SZ_HEADER + SZ_HEADER)
	{
		U4 oldnext;
		U4 oldprev;
		U4 oldsize;
		U4 newaddr;
		MSG0("RefCountMemoryManager::");
		MSG0("split(): split=YES\n");
		oldnext = NEXT(addr);
		oldprev = PREV(addr);
		oldsize = SIZE(addr);
		newaddr = addr + nbytes + SZ_HEADER;
		NEXT(addr) = newaddr;
		PREV(addr) = oldprev;
		COUNT(addr) = 1;
		SIZE(addr) = nbytes;
		NEXT(newaddr) = oldnext;
		PREV(newaddr) = addr;
		COUNT(newaddr) = FREE;
		SIZE(newaddr) = oldsize - nbytes - SZ_HEADER;
	}
	else
	{
		MSG0("RefCountMemoryManager::");
		MSG0("split(): split=NO\n");
		COUNT(addr) = 1;
	}
	return;
}/*end split--------------------------------------------------*/

int RefCountMemoryManager::checkAddress(void* addr)
{
	if (addr == NULL)
	{
		MSG0("RefCountMemoryManager::");
		MSG0("checkAddress(): cannot release NULL pointer\n");
		return(FALSE);
	}
	MSG0("RefCountMemoryManager::");
	MSG1("checkAddress(%lu)\n", addr);
	//perform sanity check to make sure address is kosher
	if ((addr >= (void*)&ram[size]) || (addr < (void*)&ram[0]))
	{
		MSG0("RefCountMemoryManager::");
		MSG0("checkAddress(): address out of bounds\n");
		return(FALSE);
	}
	return(TRUE);
}/*end checkAddress-------------------------------------------*/

int RefCountMemoryManager::checkIndex(U4 free)
{
	//a header always occupies first SZ_HEADER bytes of storage
	if (free < SZ_HEADER)
	{
		MSG0("RefCountMemoryManager::");
		MSG0("checkIndex(): address in first 16 bytes\n");
		return(FALSE);
	}
	//more sanity checks
	if ((COUNT(free) == FREE) || //region if free
		(PREV(free) >= free) || //previous element not previous
		(NEXT(free) >= size) || //next is beyond the end
		(SIZE(free) >= size) || //size greater than whole
		(SIZE(free) == 0)) //no size at all
	{
		MSG0("RefCountMemoryManager::");
		MSG0("checkIndex(): referencing invalid region\n");
		return(FALSE);
	}
	return(TRUE);
}/*end checkIndex---------------------------------------------*/

void RefCountMemoryManager::inc(void* addr)
{
	U4 free; //index into ram[]
	if (checkAddress(addr) == FALSE) { return; }
	//translate void* addr to index in ram[]
	free = (U4)(((U1*)addr) - &ram[0]);
	MSG0("RefCountMemoryManager::");
	MSG1("inc(): address resolves to index %lu\n", free);
	if (checkIndex(free) == FALSE) { return; }
	COUNT(free) = COUNT(free) + 1;
	MSG0("RefCountMemoryManager::");
	MSG1("inc(): incrementing ram[%lu] ", free);
	MSG1("to %lu\n", COUNT(free));
	return;
}/*end inc----------------------------------------------------*/

void RefCountMemoryManager::dec(void* addr)
{
	U4 free; //index into ram[]
	if (checkAddress(addr) == FALSE) { return; }
	//translate void* addr to index in ram[]
	free = (U4)(((U1*)addr) - &ram[0]);
	MSG0("RefCountMemoryManager::");
	MSG1("dec(): address resolves to index %lu\n", free);
	if (checkIndex(free) == FALSE) { return; }
	COUNT(free) = COUNT(free) - 1;
	MSG0("RefCountMemoryManager::");
	MSG1("dec(): decrementing ram[%lu] ", free);
	MSG1("to %lu\n", COUNT(free));
	if (COUNT(free) == FREE)
	{
		MSG0("RefCountMemoryManager::");
		MSG1("dec(): releasing ram[%lu]\n", free);
		release(free);
	}
	return;
}/*end dec----------------------------------------------------*/

void RefCountMemoryManager::release(U4 free)
{
	merge(PREV(free), free, NEXT(free));
#ifdef DEBUG_RC_MEM_MGR
	printState();
#endif
	return;
}/*end release------------------------------------------------*/

/*
4 cases ( F=free O=occupied )
FOF -> [F]
OOF -> O[F]
FOO -> [F]O
OOO -> OFO
*/

void RefCountMemoryManager::merge(U4 prev, U4 current, U4 next)
{
	/*
	first handle special cases of region at end(s)
	prev=0 low end
	next=0 high end
	prev=0 and next=0 only 1 list element
	*/
	if (prev == 0)
	{
		if (next == 0)
		{
			COUNT(current) = FREE;
		}
		else if (COUNT(next) != FREE)
		{
			COUNT(current) = FREE;
		}
		else if (COUNT(next) == FREE)
		{
			U4 temp;
			MSG0("RefCountMemoryManager::merge():");
			MSG0("merging to NEXT\n");
			COUNT(current) = FREE;
			SIZE(current) = SIZE(current) + SIZE(next) + SZ_HEADER;
			NEXT(current) = NEXT(next);
			temp = NEXT(next);
			if (temp != 0) { PREV(temp) = current; }
		}
	}
	else if (next == 0)
	{
		if (COUNT(prev) != FREE)
		{
			COUNT(current) = FREE;
		}
		else if (COUNT(prev) == FREE)
		{
			MSG0("RefCountMemoryManager::merge():");
			MSG0("merging to PREV\n");
			SIZE(prev) = SIZE(prev) + SIZE(current) + SZ_HEADER;
			NEXT(prev) = NEXT(current);
		}
	}
	/* now we handle 4 cases */
	else if ((COUNT(prev) != FREE) && (COUNT(next) != FREE))
	{
		COUNT(current) = FREE;
	}
	else if ((COUNT(prev) != FREE) && (COUNT(next) == FREE))
	{
		U4 temp;
		MSG0("RefCountMemoryManager::merge():");
		MSG0("merging to NEXT\n");
		COUNT(current) = FREE;
		SIZE(current) = SIZE(current) + SIZE(next) + SZ_HEADER;
		NEXT(current) = NEXT(next);
		temp = NEXT(next);
		if (temp != 0) { PREV(temp) = current; }
	}
	else if ((COUNT(prev) == FREE) && (COUNT(next) != FREE))
	{
		MSG0("RefCountMemoryManager::merge():");
		MSG0("merging to PREV\n");
		SIZE(prev) = SIZE(prev) + SIZE(current) + SZ_HEADER;
		NEXT(prev) = NEXT(current);
		PREV(next) = prev;
	}
	else if ((COUNT(prev) == FREE) && (COUNT(next) == FREE))
	{
		U4 temp;
		MSG0("RefCountMemoryManager::merge():");
		MSG0("merging with both sides\n");
		SIZE(prev) = SIZE(prev) +
			SIZE(current) + SZ_HEADER +
			SIZE(next) + SZ_HEADER;
		NEXT(prev) = NEXT(next);
		temp = NEXT(next);
		if (temp != 0) { PREV(temp) = prev; }
	}
	return;
}/*end merge--------------------------------------------------*/

void RefCountMemoryManager::printState()
{
	U4 i;
	U4 current;
	i = 0;
	current = START;
	while (NEXT(current) != 0)
	{
		printf("%lu) [P=%lu]", i, PREV(current));
		printf("[addr=%lu]", current);
		if (COUNT(current) == FREE) { printf("[FREE]"); }
		else { printf("[Ct=%lu]", COUNT(current)); }
		printf("[Sz=%lu]", SIZE(current));
		printf("[N=%lu]\n", NEXT(current));
		current = NEXT(current);
		i++;
	}
	//print the last list element
	printf("%lu) [P=%lu]", i, PREV(current));
	printf("[addr=%lu]", current);
	if (COUNT(current) == FREE) { printf("[FREE]"); }
	else { printf("[Ct=%lu]", COUNT(current)); }
	printf("[Sz=%lu]", SIZE(current));
	printf("[N=%lu]\n", NEXT(current));
	return;
}/*end printState---------------------------------------------*/