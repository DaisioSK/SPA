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

class SourceProcessor {
public:
	// method for processing the source program
	void process(string program);

	//method to handle print statement
	static void printStmtHandler(string instName, int pcdID, int& line_no, int& newID);

	static void readStmtHandler(string instName, int pcdID, int& line_no, int& newID);

	static void assignStmtHandler(vector<string> tokens, int pcdID, int& line_no, int& newID);
	
	static void whileStmtHandler(string instName, int pcdID, int& line_no, int& newID);

	static void ifStmtHandler(string instName, int pcdID, int& line_no, int& newID);

	static void parentRelnHander(vector<bracketInfo> brackets, int newID);

	static void callStmtHandler(string instName, int pcdID, int& line_no, int& newID);

	static int getInstID(string name, int pcdID, string type);

	static void lowercase(string& str);

};