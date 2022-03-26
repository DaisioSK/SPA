#include "Stack.h"

//these functions cause error, but paste to other file works.
//should be this stupid IDE again???
template <class T>
void Stack<T>::push(T const& elem)
{
    elems.push_back(elem);
}

template <class T>
void Stack<T>::pop()
{
    if (elems.empty()) {
        throw out_of_range("Stack<>::pop(): empty stack");
    }
    elems.pop_back();
}

template <class T>
T Stack<T>::top() const
{
    if (elems.empty()) {
        throw out_of_range("Stack<>::top(): empty stack");
    }
    return elems.back();
}

