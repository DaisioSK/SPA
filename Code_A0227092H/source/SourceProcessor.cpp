#include "SourceProcessor.h"
#include "iostream"
#include "regex"

static vector<string> split(string str, string token) {
	vector<string>result;
	while (str.size()) {
		int index = str.find(token);
		if (index != string::npos) {
			if(index != 0) result.push_back(str.substr(0, index));
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

static std::string trim(std::string s) {
	s = std::regex_replace(s, std::regex("^\\s+"), std::string(""));
	s = std::regex_replace(s, std::regex("\\s+$"), std::string(""));
	return s;
}

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

static bool isNumber(const string& str) {
	istringstream sin(str);
	double test;
	return sin >> test && sin.eof();
}


// method for processing the source program
// This method currently only inserts the procedure name into the database
// using some highly simplified logic.
// You should modify this method to complete the logic for handling all the required syntax.
void SourceProcessor::process(string program) {
	// initialize the database
	Database::initialize();

	

	//define regex for different patterns
	//c++ regex: escape is needed for: ^ $ \ . * + ? ( ) [ ] { } |
	//double escape is needed for literal { and }, 1 for regex and 1 for c++ string
	std::regex expr_pcd ("\^\\s\*procedure\\s\*\[a-zA-z0-9\]\+\\s\*\\{\\s\*\$");
	std::regex expr_read ("\^\\s\*read\\s\*\[a-zA-z0-9\]\+;\\s\*\$");
	std::regex expr_print("\^\\s\*print\\s\*\[a-zA-z0-9\]\+;\\s\*\$");
	std::regex expr_assgin("\^\\s\*\[a-zA-z0-9\]\+\\s\*=\\s\*\[a-zA-z0-9\\s\+\-\\/\\(\\)\\*\]\+\\s\*;\\s\*\$");
	std::regex expr_close("\^\\s\*\\}\\s\*\$");
	
	std::regex expr_inst("\^\[a-zA-z\]\[a-zA-z0-9\]\*\$");

	//std::regex expr_assgin("\^\[a-zA-z\]\[a-zA-z0-9\]\*\$");

	////iteration 2
	std::regex expr_while("\^\\s\*while\\s\*\\(\.\*\\)\\s\*\\{\\s\*\$");
	std::regex expr_if("\^\\s\*if\\s\*\\(\.\*\\)\\s\*then\\s\*\\{\\s\*\$");
	std::regex expr_else("\^\\s\*\\}\\s\*else\\s\*\\{\\s\*\$");
	
	////iteration 3
	//std::regex expr_call("procedure\\s\*\[a-zA-z\]\+\\s\*\\{\\s\*\$");


	//SIMPLE assume that 1 line is 1 stmt, so split into lines.
	vector<string> lines = split(program, "\n");
	vector<string> tokens;
	vector<bracketInfo> brackets;
	vector<int> ifBranchEndIDs;
	string clean_line;
	int line_no = 1;
	int newID = 0;
	int pcdID = 0;
	bool flagIfEnd = 0;

	for (auto& line : lines) {
		//cout << line << endl;
		trim(line);
		if (line == "") continue;
			
		//init
		clean_line = split(line, ";").at(0);
		tokens = split(clean_line, " ");

		if (regex_match(line, expr_pcd)) {
			Database::insertProcedure(tokens[1], line_no+1, pcdID);
			cout << "find a procedure here: " << line << ", id is " << pcdID << endl;

			//TODO: add the pcdID to a stack to trace the start&end of a procudure
			brackets.push_back({ "pcd", pcdID });
		}

		//read statement
		else if (regex_match(line, expr_read)) {
			readStmtHandler(tokens[1], pcdID, line_no, newID);
			cout << "line " << line_no -1  << " is a read statement: " << line << ", id is " << newID << endl;
			parentRelnHander(brackets, newID);

			//insert next reln for 1st stmt after an "if" stmt
			if (flagIfEnd && ifBranchEndIDs.size()) {
				Database::insertNextReln(ifBranchEndIDs.back(), newID);
				ifBranchEndIDs.pop_back();
				flagIfEnd = 0;
			}
		}

		//print statement
		else if (regex_match(line, expr_print)) {
			printStmtHandler(tokens[1], pcdID, line_no, newID);
			cout << "line " << line_no - 1 << " is a print statement: " << line << ", id is " << newID << endl;
			parentRelnHander(brackets, newID);

			//insert next reln for 1st stmt after an "if" stmt
			if (flagIfEnd && ifBranchEndIDs.size()) {
				Database::insertNextReln(ifBranchEndIDs.back(), newID);
				ifBranchEndIDs.pop_back();
				flagIfEnd = 0;
			}
		}

		//assign statement
		else if (regex_match(line, expr_assgin)) {
			assignStmtHandler(tokens, pcdID, line_no, newID);
			cout << "line " << line_no -1  << " is a assign statement: " << line << ", id is " << newID << endl;
			parentRelnHander(brackets, newID);

			//insert next reln for 1st stmt after an "if" stmt
			if (flagIfEnd && ifBranchEndIDs.size()) {
				Database::insertNextReln(ifBranchEndIDs.back(), newID);
				ifBranchEndIDs.pop_back();
				flagIfEnd = 0;
			}
		}

		// while statement
		else if (regex_match(line, expr_while)) {
			//TODO: add while stmt record
			whileStmtHandler(tokens[1], pcdID, line_no, newID);
			cout << "line " << line_no -1  << " is a while statement: " << line << ", id is " << newID << endl;

			brackets.push_back(bracketInfo{ "while", newID });

			//insert next reln for 1st stmt after an "if" stmt
			if (flagIfEnd && ifBranchEndIDs.size()) {
				Database::insertNextReln(ifBranchEndIDs.back(), newID);
				ifBranchEndIDs.pop_back();
				flagIfEnd = 0;
			}
		}

		// if statement
		else if (regex_match(line, expr_if)) {
			//TODO: add if stmt record
			ifStmtHandler(tokens[1], pcdID, line_no, newID);
			cout << "line " << line_no - 1 << " is a if statement: " << line << ", id is " << newID << endl;

			brackets.push_back(bracketInfo{ "if", newID });

			//insert next reln for 1st stmt after an "if" stmt
			if (flagIfEnd && ifBranchEndIDs.size()) {
				Database::insertNextReln(ifBranchEndIDs.back(), newID);
				ifBranchEndIDs.pop_back();
				flagIfEnd = 0;
			}
		}

		// else statement
		else if (regex_match(line, expr_else)) {
			//TODO: add else stmt record
			
			cout << "line " << line_no - 1 << " is a else statement: " << line << ", id is " << newID << endl;

			//brackets.push_back(bracketInfo{ "else", newID });

			ifBranchEndIDs.push_back(newID);

			//repalce the last stmt to the if stmt
			bracketInfo currentBracket = brackets.back();
			newID = currentBracket.db_id;

		}

		else if (regex_match(line, expr_close)) {
			//TODO: update pcd table to close it
			cout << "close here: " << line << endl;

			if (!brackets.empty()) {
				bracketInfo currentBracket = brackets.back();

				if (currentBracket.type == "pcd")
				{
					Database::updateProcedure(pcdID, line_no);
				}
				else if (currentBracket.type == "while") {
					//TODO: update stmt table - while
					Database::updateStmt(currentBracket.db_id, line_no);
					Database::insertNextReln(newID, currentBracket.db_id);
				}
				else if (currentBracket.type == "if") {
					//TODO: update stmt table - if
					Database::updateStmt(currentBracket.db_id, line_no);
					flagIfEnd = 1;
				}

				brackets.pop_back();
			}
		}
	}

	cout << endl << endl;


	//old way not good :(
	// 
	//// tokenize the program
	//Tokenizer tk;
	//vector<string> tokens;
	//tk.tokenize(program, tokens);

	//// This logic is highly simplified based on iteration 1 requirements and 
	//// the assumption that the programs are valid.
	//string procedureName = tokens.at(1);

	//// insert the procedure into the database
	//Database::insertProcedure(procedureName);

	//vector<string> lines = split(program, "\n");


}

void SourceProcessor::printStmtHandler(string instName, int pcdID, int& line_no, int& newID) {

	//insert statement and get id
	int oldID = newID;
	Database::insertStatement("print", line_no, line_no, pcdID, newID);

	//get variable id being print
	int instID = getInstID(instName, pcdID, "variable");

	//add use reln record
	Database::insertUseReln(newID, instID);

	//insert next reln record
	if(oldID>0) Database::insertNextReln(oldID, newID);

	line_no++;
}

void SourceProcessor::readStmtHandler(string instName, int pcdID, int& line_no, int& newID) {

	//insert statement and get id
	int oldID = newID;
	Database::insertStatement("read", line_no, line_no, pcdID, newID);

	//get variable id being print
	int instID = getInstID(instName, pcdID, "variable");

	//add use reln record
	Database::insertModifyReln(newID, instID, "");

	//insert next reln record
	if (oldID > 0) Database::insertNextReln(oldID, newID);

	line_no++;
}

void SourceProcessor::assignStmtHandler(vector<string> tokens, int pcdID, int& line_no, int& newID) {
	
	std::regex expr_inst("\[a-zA-z0-9\]\*\$");
	std::regex expr_var("\[a-zA-z\]\[a-zA-z0-9\]\*\$");
	
	bool metEqualSign = 0;
	int modInstID = 0;
	int instID = 0;
	string expression = "";

	//insert statement and get id
	int oldID = newID;
	Database::insertStatement("assign", line_no, line_no, pcdID, newID);
	
	for (int i = 0; i < tokens.size(); i++) {
		//split by equal sigh
		//seems == is alr defined in string class, but good to know the compare()
		if (tokens[i].compare("=") == 0) {
			metEqualSign = 1;
			continue;
		}

		if (regex_match(tokens[i], expr_inst)) {
			if (metEqualSign) {
				expression.append(tokens[i]);
				if (regex_match(tokens[i], expr_var)) {
					if (isNumber(tokens[2]))
					{
						instID = getInstID(tokens[i], pcdID, "constant");
					}
					else {
						instID = getInstID(tokens[i], pcdID, "variable");
					}
					Database::insertUseReln(newID, instID);
				}
			}
			else {
				if (isNumber(tokens[2]))
				{
					modInstID = getInstID(tokens[i], pcdID, "constant");
				}
				else {
					modInstID = getInstID(tokens[i], pcdID, "variable");
				}
			}
		}
		else {
			if (metEqualSign)
			{
				expression.append(tokens[i]);
			}
			else {
				if (isNumber(tokens[2]))
				{
					modInstID = getInstID(tokens[i], pcdID, "constant");
				}
				else {
					modInstID = getInstID(tokens[i], pcdID, "variable");
				}
			}
		}
		
	}
	Database::insertModifyReln(newID, modInstID, expression);

	//insert next reln record
	if (oldID > 0) Database::insertNextReln(oldID, newID);

	line_no++;
}

void SourceProcessor::whileStmtHandler(string instName, int pcdID, int& line_no, int& newID) {

	//insert statement and get id
	int oldID = newID;
	Database::insertStatement("while", line_no, line_no, pcdID, newID);

	//insert next reln record
	if (oldID > 0) Database::insertNextReln(oldID, newID);
	line_no++;
}

void SourceProcessor::ifStmtHandler(string instName, int pcdID, int& line_no, int& newID) {

	//insert statement and get id
	int oldID = newID;
	Database::insertStatement("if", line_no, line_no, pcdID, newID);

	//insert next reln record
	if (oldID > 0) Database::insertNextReln(oldID, newID);

	line_no++;
}

void SourceProcessor::parentRelnHander(vector<bracketInfo> brackets, int newID) {
	if (!brackets.empty())
	{
		bracketInfo currentBracket = brackets.back();
		if (currentBracket.type == "while" || currentBracket.type == "if")
		{
			Database::insertParentReln(currentBracket.db_id, newID);
		}
	}
}

//provide instance name and pcd__id, return instance id
//if instance not found, create a new instance and return the id
int SourceProcessor::getInstID(string name, int pcdID, string type) {

	vector<queryCond> queryConds;
	vector<string> results;
	int instID = 0;

	queryConds.push_back(queryCond{ "AND","pcd__id",to_string(pcdID),false });
	queryConds.push_back(queryCond{ "AND","name",name,false });
	Database::getInstance(results, queryConds, vector<int>{0});

	if (results.size()) 
		instID = stoi(results[0]);
	else 
		Database::insertInstance(type, name, pcdID, instID);

	return instID;
}


