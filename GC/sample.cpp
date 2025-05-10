#define GC_LOG_LEVEL GC_LOG_LEVEL_DEBUG
#include "gc.h"

void foo()
{
	int* ptr1 = new int(1);
	int* ptr2 = new int(2);
	int* ptr3 = new int(3);
	GarbageCollector::getInstance().collect();
}

int main()
{
	GarbageCollector::init();
	int* ptr = new int(0);
	int* ptr1 = ptr;
	foo();
	GarbageCollector::getInstance().collect();
	cout << *ptr << ' ' << *ptr1 << '\n';
}