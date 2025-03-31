#ifndef HASH_OPEN_ADDR_TABLE_H
#define HASH_OPEN_ADDR_TABLE_H

#include <string>
#include <bitset>
#include "base_table.h"

template<class T, class B>
class Hash_open_addr_table : public Base_table<T, B>
{
	using Base_table<T, B>::record;
	using Base_table<T, B>::Fill;
	enum class Condition { empty, busy, deleted };
	struct HashRecord
	{
		record rc;
		Condition cond = Condition::empty;
	};
	vector<HashRecord> table;
	size_t step;
	int gcd(int a, size_t b)//алгоритм евклида
	{
		while (b != 0)
		{
			int temp = b;
			b = a % b;
			a = temp;
		}
		return a;
	}
public:
	Hash_open_addr_table()
	{
		Fill = 0;
		table.resize(max_fill);
		step = 2;
		while (gcd(max_fill, step) != 1)
			step++;
	}
	int hash_function(const T& name) const
	{
		const unsigned char* data = reinterpret_cast<const unsigned char*>(&name);
		int len = sizeof(name);
		int hashcode = 0;
		const unsigned char last_byte = data[len - 1];
		bitset<8> last_bites(last_byte);
		for (int i = 0; i < last_bites.size(); i++)
			hashcode += last_bites[i] * (i + 1);
		return hashcode % max_fill;
	}
	const B& find(const T& name) const override
	{
		if (isEmpty())
			throw runtime_error("Table is empty!");
		int hashcode = hash_function(name);
		for (int i = 0; i < max_fill; i++)//max_fill строчек посмотрим в худшем случае
		{
			if (table[hashcode].cond == Condition::empty)//пустая - значит цепочка с таким хэшкодом пуста(в нее просто никто не вставлял)
				throw runtime_error("Key not found!");
			else if (table[hashcode].cond == Condition::busy && table[hashcode].rc.key == name)//это наша запись
				return table[hashcode].rc.data;
			else if (table[hashcode].cond == Condition::busy && table[hashcode].rc.key != name || table[hashcode].cond == Condition::deleted)//занята и не та или удалена
				hashcode = (hashcode + step) % max_fill;
		}
		throw runtime_error("Key not found!");
	}
	bool insert(const T& name, const B& obj) override
	{
		if (Fill == max_fill)
			return false;
		int hashcode = hash_function(name);
		for (int i = 0; i < max_fill; i++)//max_fill строчек посмотрим в худшем случае
		{
			if (table[hashcode].cond == Condition::empty || table[hashcode].cond == Condition::deleted)//пустая запись или удаленная запись
			{
				table[hashcode].cond = Condition::busy;
				table[hashcode].rc.key = name;
				table[hashcode].rc.data = obj;
				Fill++;
				return true;
			}
			else if (table[hashcode].cond == Condition::busy && table[hashcode].rc.key == name)//уже есть запись
				return false;
			else if (table[hashcode].cond == Condition::busy && table[hashcode].rc.key != name)//не та запись
				hashcode = (hashcode + step) % max_fill;
		}
		return false;
	}
	bool delete_rec(const T& name) override
	{
		if (isEmpty())
			return false;
		int hashcode = hash_function(name);
		for (int i = 0; i < max_fill; i++)//max_fill строчек посмотрим в худшем случае
		{
			if (table[hashcode].cond == Condition::empty)//пустая - значит цепочка с таким хэшкодом пуста(в нее просто никто не вставлял)
				return false;
			else if (table[hashcode].cond == Condition::busy && table[hashcode].rc.key == name)//это наша запись
			{
				table[hashcode].cond = Condition::deleted;
				Fill--;
				return true;
			}
			else if (table[hashcode].cond == Condition::busy && table[hashcode].rc.key != name || table[hashcode].cond == Condition::deleted)//занята и не та или удалена
				hashcode = (hashcode + step) % max_fill;
		}
		return false;
	}
	bool isEmpty() const override
	{
		return Fill == 0;
	}
	~Hash_open_addr_table() override = default;
};

#endif