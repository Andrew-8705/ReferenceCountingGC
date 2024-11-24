#include <iostream>
#include "gc_ptr.h"
#include "TestClass.h"
#include <memory>

using namespace std;

// попытка сравнить класс gc_ptr с shared_ptr

int main()
{
	int cnt_iter = 1000000;
	cout << "Comparative test: " << '\n';
	cout << "Testing gc_ptr: " << '\n';
	test_performance<TestClass, gc_ptr<TestClass>>("gc_ptr", cnt_iter);
	cout << "=============================================================\n";
	test_performance<TestClass, shared_ptr<TestClass>>("shared_ptr", cnt_iter);

	return 0;
}
