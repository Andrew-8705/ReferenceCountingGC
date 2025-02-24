#include "mallocV4.h"
#include "perform.h"
/*
not using a modified compiler, so will need to insert
reference counting code manually
*/

void debugTest()
{
	void* ptr[6];
	void* ptr1;
	void* ptr2;
	void* ptr3;
	unsigned long allocs[6] = { 8,12,33,1,122,50 };
	int i;
	initMemMgr(270);
	for (i = 0; i < 6; i++)
	{
		ptr[i] = newMalloc(allocs[i]);
		if (ptr[i] == NULL) { printf("ptr[%lu]==NULL!\n", i); }
	}
	//copying addresses
	printf("copying ptr[0]\n");
	ptr1 = ptr[0];
	(*mmptr).inc(ptr[0]); //compiler insert
	printf("copying ptr[1]\n");
	ptr3 = ptr[1];
	(*mmptr).inc(ptr[1]); //compiler insert
	printf("copying ptr1\n");
	ptr2 = ptr1;
	(*mmptr).inc(ptr1); //compiler insert
	//overwritting
	printf("overwriting ptr1 with ptr3\n");
	(*mmptr).dec(ptr2); //compiler insert
	ptr2 = ptr3;
	(*mmptr).inc(ptr3); //compiler insert
	//locals going out of scope, need to decrement
	printf("leaving scope\n");
	for (i = 0; i < 6; i++)
	{
		(*mmptr).dec(ptr[i]); //compiler insert
	}
	(*mmptr).dec(ptr1); //compiler insert
	(*mmptr).dec(ptr2); //compiler insert
	(*mmptr).dec(ptr3); //compiler insert
	closeMemMgr();
	return;
}/*end debugTest----------------------------------------------*/

void main()
{
	//for the debug test, should activate debug macros in
	//mallocVx.cpp
	debugTest();
	//for the performance test, should comment out debug macros
	//PerformanceTestDriver();
	return;
}/*end main---------------------------------------------------*/