#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define DEBUG_RC_MEM_MGR
#define DEBUG_MALLOCV4

#include "memmgr.h"
/*
wrapper functions
*/
RefCountMemoryManager* mmptr;
void initMemMgr(unsigned long totalbytes)
{
	mmptr = new RefCountMemoryManager(totalbytes);
}
void closeMemMgr()
{
	delete(mmptr);
}
void* newMalloc(unsigned long size)
{
	void* ptr = (*mmptr).allocate(size);
#ifdef DEBUG_MALLOCV4
	(*mmptr).printState();
#endif
	return(ptr);
}

void newFree(void* ptr)
{
	printf("newFree(): cannot free %p\n", ptr);
	printf("newFree(): not implemented, using garbage collector\n");
	return;
}