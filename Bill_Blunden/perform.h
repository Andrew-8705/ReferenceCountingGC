#pragma once

#pragma once

/* --perform.cpp-- */
/*holds setup data to pass to constructor*/

struct TestData
{
	double* dptr; // probability array
	unsigned long* lptr; // allocation sizes
	unsigned long samplesize; // # malloc() calls
	unsigned long length; // size of arrays
};

class PerformanceTest
{
public:
	PerformanceTest(struct TestData* tdptr);
	unsigned long runTest();
private:
	unsigned long nAllocations; // # of malloc() calls to make
	unsigned long arraySize; // size of P(x) and x arrays
	double* p; // P(x) = probability for X=x
	unsigned long* x; // X = # bytes allocated
	double getU();
	unsigned long getRandomVariate();
	void getAllocArray(unsigned long* lptr);
};

PerformanceTest::PerformanceTest(struct TestData* tdptr)
{
	p = (*tdptr).dptr;
	x = (*tdptr).lptr;
	nAllocations = (*tdptr).samplesize;
	arraySize = (*tdptr).length;
	return;
}/*end constructor--------------------------------------------*/

double PerformanceTest::getU()
{
	return(((double)rand()) / ((double)RAND_MAX));
}/*end getU---------------------------------------------------*/

void PerformanceTestDriver()
{
	double p[8] = { .15, .20, .35, .20, .02, .04, .02, .02 };
	unsigned long x[8] = { 16,32,64,128,256,512,1024,4096 };
	struct TestData td;
	td.dptr = p;
	td.lptr = x;
	td.samplesize = 1024;
	td.length = 8;
	PerformanceTest pt = PerformanceTest(&td);
	printf("msecs=%lu\n", pt.runTest());
	return;
}/*end PerformanceTestDriver----------------------------------*/

unsigned long PerformanceTest::getRandomVariate()
{
	double U;
	unsigned long i;
	double total;
	U = getU();
	for (i = 0, total = p[0]; i <= arraySize - 2; i++)
	{
		if (U < total) { return(x[i]); }
		total = total + p[i + 1];
	}
	return(x[arraySize - 1]);
	/*
	the above is a cleaner/slower way of doing something like:
	if(U < p[0]){return(x[0]);}
	else if(U <(p[0]+p[1])){return(x[1]);}
	else if(U <(p[0]+p[1]+p[2])){return(x[2]);}
	else if(U <(p[0]+p[1]+p[2]+p[3])){return(x[3]);}
	else if(U <(p[0]+p[1]+p[2]+p[3]+p[4])){return(x[4]);}
	else if(U <(p[0]+p[1]+p[2]+p[3]+p[4]+p[5])){return(x[5]);}
	else if(U <(p[0]+p[1]+p[2]+p[3]+p[4]+p[5]+p[6]))
	{return(x[6]);}
	else{ return(x[7]);}
	*/
}/*end getRandomVariate---------------------------------------*/

void PerformanceTest::getAllocArray(unsigned long* lptr)
{
	unsigned long i;
	for (i = 0; i < nAllocations; i++)
	{
		lptr[i] = getRandomVariate();
	}
	return;
}/*end getAllocationArray-------------------------------------*/

unsigned long PerformanceTest::runTest()
{
	unsigned long* allocs;
	unsigned long i;
	unsigned long ticks1, ticks2;
	char** addr; /*pointer to an array of pointers*/
	/*create array of address holders to stockpile malloc()
	returns*/
	addr = (char**)malloc(sizeof(char*) * nAllocations);
	if (addr == NULL)
	{
		printf("could not allocate address repository\n");
		exit(1);
	}
	/*create stream of allocation values*/
	allocs = (unsigned long*)malloc(sizeof(long) * nAllocations);
	if (allocs == NULL)
	{
		printf("could not allocate malloc() request stream\n");
		exit(1);
	}
	getAllocArray(allocs);
	/*start timer and do some work*/
	initMemMgr(1024 * 1024);
	printf("PerformanceTest::runTest(): time whistle blown\n");
	ticks1 = GetTickCount();
	for (i = 0; i < nAllocations; i++)
	{
		//printf("%lu\n",allocs[i]);
		addr[i] = (char*)newMalloc(allocs[i]);
		if (addr[i] == NULL)
		{
			printf("mallco()=addr[%lu]=%lu failed\n", i, addr[i]);
			exit(1);
		}
	}
	//array goes out of scope
	for (i = 0; i < nAllocations; i++)
	{
		(*mmptr).dec(addr[i]);
	}
	ticks2 = GetTickCount();
	printf("PerformanceTest::runTest(): race has ended\n");
	closeMemMgr();
	free(addr);
	free(allocs);
	return(ticks2 - ticks1);
}/*end runTest------------------------------------------------*/