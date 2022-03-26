#pragma once

#include <string>
#include <vector>
#include <iostream>
#include "Tokenizer.h"
#include "Stack.h"

using namespace std;

struct Node {
	string item;
	bool is_operand;
	Node* _left;
	Node* _right;
};

//a list node
struct CalUnit {
	char symbol;
	Node* _operand;
	CalUnit* _next;
};

class ExprTree {
private:
	static Node* _root;

public:
	ExprTree(string expr);
	static void exprStrToNode(string expr, Node& n);
	static void inOrder(Node* n);
	static void strInOrder(Node* n, string& str);

	static bool HasSubtree(Node* parent, Node* child);
	static bool judge(Node* n1, Node* n2);
	static bool isSame(Node* n1, Node* n2);

	static bool testLeft(Node* parent, Node* child);
	static bool testRight(Node* parent, Node* child);
};






