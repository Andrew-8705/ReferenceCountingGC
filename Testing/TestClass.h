#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <memory>

using namespace std;
using namespace std::chrono;

class TestClass {
public: 
	int value;
	TestClass(int val = 0) : value(val) {}
};

template<typename T, typename Ptr>
void test_performance(const string& title, int cnt_iter) {
	// время создания
	auto start1 = high_resolution_clock::now();
	vector<Ptr> pointers;
	for (int i = 0; i < cnt_iter; i++) {
		pointers.push_back(Ptr(new T(i)));
	}
	auto end1 = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(end1 - start1).count();
	cout << title << " Creation time for " << cnt_iter << " iterations: "
		<< duration << " microseconds. Average creation time: " << (double) duration / cnt_iter << '\n';

	// время копирования 
	auto start2 = high_resolution_clock::now();
	vector<Ptr> copied_pointers = pointers;
	auto end2 = high_resolution_clock::now();
	duration = duration_cast<microseconds>(end2 - start2).count();
	cout << title << " Copy time for " << cnt_iter << " iterations: " 
		<< duration << " microseconds. Average copy time: " << (double) duration / cnt_iter << '\n';

	// время удаления 
	auto start3 = high_resolution_clock::now();
	pointers.clear();
	auto end3 = high_resolution_clock::now();
	duration = duration_cast<microseconds>(end3 - start3).count();
	cout << title << " Deletion time for " << cnt_iter << " iterations: "
		<< duration << " microseconds. Average delete time: " << (double) duration / cnt_iter << '\n';
}