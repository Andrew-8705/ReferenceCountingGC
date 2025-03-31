#ifndef BASE_TABLE_H
#define BASE_TABLE_H

#include <string>
#include <stdexcept>
#include <vector>

using namespace std;

const size_t max_fill = 1000;

template<class T, class B>
class Base_table
{
protected:
	struct record
	{
		T key;
		B data;
	};
	size_t Fill;
public:
	virtual const B& find(const T& name) const = 0;
	virtual bool insert(const T& name, const B& obj) = 0;
	virtual bool delete_rec(const T& name) = 0;
	virtual bool isEmpty() const = 0;
	virtual ~Base_table() = default;
};

#endif