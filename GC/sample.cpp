#include "gc.h"

void foo()
{
	int* ptr = new int(1);
}

int main()
{
	GarbageCollector::init();

	foo();
	GarbageCollector::getInstance().collect();
}