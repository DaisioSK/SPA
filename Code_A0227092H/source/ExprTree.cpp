#include "ExprTree.h"
#include "regex"

static vector<string> split(string str, string token) {
	vector<string>result;
	while (str.size()) {
		int index = str.find(token);
		if (index != string::npos) {
			if (index != 0) result.push_back(str.substr(0, index));
			str = str.substr(index + token.size());
			if (str.size() == 0)result.push_back(str);
		}
		else {
			result.push_back(str);
			str = "";
		}
	}
	return result;

}

static vector<string> split_multi(string str, vector<string> tokens) {
	vector<string>result;
	int index = -1, temp = -1;
	string key_token = "";

	while (str.size()) {

		for (string token : tokens) {
			temp = str.find(token);
			if (temp != string::npos) {
				if (index < 0) {
					key_token = token;
					index = temp;
				}
				else {
					index = (index < temp) ? index : temp;
					key_token = (index < temp) ? key_token : token;
				}
			}
		}

		if (index != string::npos) {
			if (index != 0) result.push_back(str.substr(0, index));
			str = str.substr(index + key_token.size());
			if (str.size() == 0)result.push_back(str);
		}
		else {
			result.push_back(str);
			str = "";
		}
	}
	return result;

}

//TODO: helper func - find all matched regex and return a vector of string
static vector<string> getInstancesFromString(string str) {

	vector<string>result;
	std::smatch m;
	std::regex expr_inst("\^\[a-zA-z\]\[a-zA-z0-9\]\*\$");

	while (std::regex_search(str, m, expr_inst)) {
		//for (auto x : m) std::cout << x << " ";
		result.push_back(m[0]);
		str = m.suffix().str();
	}

	return result;
}

//to delete
ExprTree::ExprTree(string expr) {
	std::regex expr_first_operand("\^\\s\*\.\*");
	std::regex expr_cal_unit("\[a-zA-z0-9\]\+\\s\*\[\+\-\\/\\*\%\]");
}

bool optr_compare(string a, string b) {
	int rankA, rankB;

	if (a == "+" || a == "-") rankA = 1;
	else if (a == "*" || a == "/" || a == "%") rankA = 2;
	else rankA = 0;

	if (b == "+" || b == "-") rankB = 1;
	else if (b == "*" || b == "/" || b == "%") rankB = 2;
	else rankB = 0;

	//true: left >= right
	return rankA > rankB;
}

void ExprTree::inOrder(Node* n) {
	if (n==NULL) return;
	inOrder(n->_left);
	cout << n->item << " ";
	inOrder(n->_right);
}

void mergeNode(Stack<string>& operators, Stack<Node*>& operands) {
	struct Node* pNode = new Node;
	pNode->is_operand = 0;

	pNode->item = operators.top();
	operators.pop();
	pNode->_right = operands.top();
	operands.pop();
	pNode->_left = operands.top();
	operands.pop();
	operands.push(pNode);
}

bool ExprTree::HasSubtree(Node* root1, Node* root2) {
	if (root1 == NULL || root2 == NULL) return false;
	return judge(root1, root2);
}

bool ExprTree::judge(Node* root1, Node* root2) {
	if (root1 == NULL) return false;
	if (root1->item == root2->item && isSame(root1, root2)) return true;
	return judge(root1->_left, root2) || judge(root1->_right, root2);
}

bool ExprTree::isSame(Node* root1, Node* root2) {
	if (root2 == NULL) return true;
	if (root1 == NULL) return false;
	if (root1->item != root2->item) return false;
	return isSame(root1->_left, root2->_left) && isSame(root1->_right, root2->_right);
}


void ExprTree::exprStrToNode(string expr, Node& n) {
	
	Tokenizer tk;
	vector<string> tokens;
	tk.tokenize(expr, tokens);

	Stack<string> operators;
	Stack<Node*> operands;
	std::regex expr_operand("\^\[\+\-\\/\\*%\\(\\)\]\$");

	for (string token : tokens) {

		if (regex_match(token, expr_operand)) {
			//operator
			if (token == "(") {
				operators.push(token);
			}
			else if (token == ")") {
				while (operators.top() != "(") {
					mergeNode(operators, operands);
				}
				operators.pop();
			}
			else {
				while (!operators.empty() && !optr_compare(token, operators.top())) {
					mergeNode(operators, operands);
				}
				operators.push(token);
			}
		}else{
			//operand
			struct Node* pNode = new Node;
			pNode->item = token;
			pNode->is_operand = 1;
			pNode->_left = NULL;
			pNode->_right = NULL;
			operands.push(pNode);
		}
	}

	while (!operators.empty()) {
		mergeNode(operators, operands);
	}

	//inOrder(operands.top());
	n = *operands.top();


	//std::regex expr_sub("\\(\.\+\\)");
	//std::regex expr_first_operand("\^\\s\*\.\*");
	//std::regex expr_cal_unit("\[a-zA-z0-9\]\+\\s\*\[\+\-\\/\\*\%\]");
	//std::smatch m;

	//while (std::regex_search(expr, m, expr_sub)) {
	//	//for (auto x : m) std::cout << x << " ";
	//	//n = exprStrToNode(m[0]);
	//	m[0]; //matched part
	//	m.position(); //matched index
	//	expr = m.suffix().str();
	//}

}

void ExprTree::strInOrder(Node* n, string& str) {
	if (n == NULL) return;
	strInOrder(n->_left, str);
	str += n->item;
	strInOrder(n->_right, str);
}

bool ExprTree::testLeft(Node* parent, Node* child) {
	string str_parent, str_child;
	strInOrder(parent, str_parent);
	strInOrder(child, str_child);
	bool result = str_parent.substr(0, str_child.size()) == str_child;
	return result;
}

bool ExprTree::testRight(Node* parent, Node* child) {
	string str_parent, str_child;
	strInOrder(parent, str_parent);
	strInOrder(child, str_child);
	int index = str_parent.size() - str_child.size() - 1;
	bool result = index>=0 && str_parent.substr(index, str_child.size()) == str_child;
	return result;
}


template <class T>
void Stack<T>::push(T const& elem)
{
	elems.push_back(elem);
}

template <class T>
void Stack<T>::pop()
{
	//if (elems.empty()) {
	//	throw out_of_range("Stack<>::pop(): empty stack");
	//}
	//elems.pop_back();

	if(!elems.empty()) elems.pop_back();
}

template <class T>
T Stack<T>::top() const
{
	if (elems.empty()) {
		throw out_of_range("Stack<>::top(): empty stack");
	}
	return elems.back();
}




