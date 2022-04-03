#pragma once
#include <sstream>
#include <fstream>
#include <vector>
#include "Database.h"
#include "Tokenizer.h"

using namespace std;

struct bracketInfo
{
	string type;
	int db_id;
};

struct callReln
{
	string caller_name;
	string callee_name;
	int stmt_id;
};

struct parentReln
{
	int parent_id;
	int child_id;
};

class SourceProcessor {
public:
	// method for processing the source program
	void process(string program);

	//method to handle print statement
	static void printStmtHandler(string instName, vector<bracketInfo> brackets, int pcdID, int& line_no, int& newID);

	static void readStmtHandler(string instName, vector<bracketInfo> brackets, int pcdID, int& line_no, int& newID);

	static void assignStmtHandler(vector<string> tokens, vector<bracketInfo> brackets, int pcdID, int& line_no, int& newID);
	
	static void whileStmtHandler(vector<string> tokens, int pcdID, int& line_no, int& newID);

	static void ifStmtHandler(vector<string> tokens, int pcdID, int& line_no, int& newID);

	static void parentRelnHander(vector<bracketInfo> brackets, int newID);

	static void callStmtHandler(string instName, int pcdID, int& line_no, int& newID);

	static int getInstID(string name, int pcdID, string type);

	static void lowercase(string& str);

	static void updateCallRelnTables(vector<callReln> callRelnArr, vector<parentReln> parentRelnArr);

};