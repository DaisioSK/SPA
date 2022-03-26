#pragma once

#include <vector>
using namespace std;

template <class T>
class Stack {
private:
	vector<T> elems;

public:
	void push(T const& elem);
	void pop();
	T top() const;
	bool empty() const {
		return elems.empty();
	}
};