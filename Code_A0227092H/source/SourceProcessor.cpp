#include "SourceProcessor.h"
#include "iostream"
#include "regex"


static vector<string> split(string str, string token, bool keep) {
	vector<string>result;
	while (str.size()) {
		int index = str.find(token);
		if (index != string::npos) {
			if (index != 0) {
				result.push_back(str.substr(0, index));
				if (keep) result.push_back(token);
			}
			str = str.substr(index + token.size());
			if (str.size() == 0) result.push_back(str);
		}
		else {
			result.push_back(str);
			str = "";
		}
	}
	return result;
}


// static std::string trim(std::string s) {
// 	s = std::regex_replace(s, std::regex("^\\s+"), std::string(""));
// 	s = std::regex_replace(s, std::regex("\\s+$"), std::string(""));
// 	return s;
// }
 


static vector<string> splitByOperators(string str) {
	vector<string>result;
	int index = 0;
	bool endWithOps = false;

	for (int i = 0; i < str.size(); i++)
	{
		char c = str[i];
		if (c == ' ') continue;

		if (c == '=' || c == '>' || c == '<' || c == '+' || c == '-'|| c == '*' || c == '/' || c == '%' || c == '(' || c == ')')
		{
			if (i > index) result.push_back(str.substr(index, i - index));
			result.push_back(string(1, c));
			index = i + 1;
			endWithOps = true;
			continue;
		}
		endWithOps = false;
	}

	if(!endWithOps) result.push_back(str.substr(index, str.size() - index));

	return result;
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

static string trim(string s, char c) {
	if (!s.empty())
	{
		if (c == NULL)
		{
			s.erase(remove(s.begin(), s.end(), ' '), s.end());
			s.erase(remove(s.begin(), s.end(), ';'), s.end());
			s.erase(remove(s.begin(), s.end(), '\t'), s.end());
		}
		else {
			s.erase(remove(s.begin(), s.end(), c), s.end());
		}
	}

	return s;
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
	std::regex expr_assgin("\^\\s\*\[a-zA-z0-9\]\+\\s\*=\\s\*\[a-zA-z0-9\\s\+\-\\/\\(\\)\\%\\*\]\+\\s\*;\\s\*\$");
	std::regex expr_close("\^\\s\*\\}\\s\*\$");
	
	std::regex expr_inst("\^\[a-zA-z\]\[a-zA-z0-9\]\*\$");

	//std::regex expr_assgin("\^\[a-zA-z\]\[a-zA-z0-9\]\*\$");

	////iteration 2
	std::regex expr_while("\^\\s\*while\\s\*\\(\.\*\\)\\s\*\\{\\s\*\$");
	std::regex expr_if("\^\\s\*if\\s\*\\(\.\*\\)\\s\*then\\s\*\\{\\s\*\$");
	std::regex expr_else("\^\\s\*\\}\\s\*else\\s\*\\{\\s\*\$");
	
	////iteration 3
	std::regex expr_call("\^\\s\*call\\s\*\[a-zA-z0-9\]\+;\\s\*\$");


	//SIMPLE assume that 1 line is 1 stmt, so split into lines.
	vector<string> lines = split(program, "\n", false);
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
  
		if (line == "") continue;

		//init
		clean_line = trim(line, NULL);
		line = trim(line, '\t');
		//clean_line = split(line, ";").at(0);
		//tokens = split(line, " ");

		if (regex_match(line, expr_pcd)) {
			clean_line = trim(clean_line, '{');
			tokens = split(clean_line, "procedure", false); // {"proc_name"};

			Database::insertProcedure(tokens[0], line_no+1, newID);
			pcdID = newID;
			cout << "find a procedure here: " << line << ", id is " << newID << endl;

			brackets.push_back({ "pcd", pcdID });
		}

		//read statement
		else if (regex_match(line, expr_read)) {
			tokens = split(clean_line, "read", false); // {"var_name"};

			readStmtHandler(tokens[0], brackets, pcdID, line_no, newID);
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
			tokens = split(clean_line, "print", false); // {"var_name"};

			printStmtHandler(tokens[0], brackets, pcdID, line_no, newID);
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
			tokens = split(clean_line, "=", true); // {"var_name", "=", "assigned_val"};
			vector<string> rhs_tokens = splitByOperators(tokens[2]);
			tokens.pop_back();
			tokens.insert(tokens.end(), rhs_tokens.begin(), rhs_tokens.end());

			assignStmtHandler(tokens, brackets, pcdID, line_no, newID);
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

			tokens = splitByOperators(clean_line);

			whileStmtHandler(tokens, pcdID, line_no, newID);
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
			tokens = splitByOperators(clean_line);

			ifStmtHandler(tokens, pcdID, line_no, newID);
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
			
			cout << "line " << line_no - 1 << " is a else statement: " << line << ", id is " << newID << endl;

			ifBranchEndIDs.push_back(newID);

			//repalce the last stmt to the if stmt
			bracketInfo currentBracket = brackets.back();
			newID = currentBracket.db_id;

		}

		// call statement
		else if (regex_match(line, expr_call)) {
			cout << "line " << line_no - 1 << " is a call statement: " << line << ", id is " << newID << endl;

			tokens = split(clean_line, "call", false);

			callStmtHandler(tokens[0], pcdID, line_no, newID);
			parentRelnHander(brackets, newID);
		}

		else if (regex_match(line, expr_close)) {
			cout << "close here: " << line << endl;

			if (!brackets.empty()) {
				bracketInfo currentBracket = brackets.back();

				if (currentBracket.type == "pcd")
				{
					Database::updateProcedure(pcdID, line_no);
				}
				else if (currentBracket.type == "while") 
				{
					Database::updateStmt(currentBracket.db_id, line_no);
					Database::insertNextReln(newID, currentBracket.db_id);
				}
				else if (currentBracket.type == "if") 
				{
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

void SourceProcessor::printStmtHandler(string instName, vector<bracketInfo> brackets, int pcdID, int& line_no, int& newID) {

	//insert statement and get id
	int oldID = newID;
	Database::insertStatement("print", line_no, line_no, pcdID, newID);

	//get variable id being print
	int instID = getInstID(instName, pcdID, "variable");

	//add use reln record
	Database::insertUseReln(newID, instID);
	if (!brackets.empty()) {
		for (bracketInfo b : brackets) {
			if (b.type == "while" || b.type == "if")
				Database::insertUseReln(b.db_id, instID);
			else if (b.type == "pcd") {
				Database::insertPcdUseReln(b.db_id, instID);
			}
		}
	}

	//insert next reln record
	if(oldID>0) Database::insertNextReln(oldID, newID);

	line_no++;
}

void SourceProcessor::readStmtHandler(string instName, vector<bracketInfo> brackets, int pcdID, int& line_no, int& newID) {

	//insert statement and get id
	int oldID = newID;
	Database::insertStatement("read", line_no, line_no, pcdID, newID);

	//get variable id being print
	int instID = getInstID(instName, pcdID, "variable");

	//add use reln record
	Database::insertModifyReln(newID, instID, "");
	if (!brackets.empty()) {
		for (bracketInfo b : brackets) {
			if (b.type == "while" || b.type == "if")
				Database::insertModifyReln(b.db_id, instID, "");
			else if (b.type == "pcd") {
				Database::insertPcdModifyReln(b.db_id, instID, "");
			}
		}
	}

	//insert next reln record
	if (oldID > 0) Database::insertNextReln(oldID, newID);

	line_no++;
}

void SourceProcessor::assignStmtHandler(vector<string> tokens, vector<bracketInfo> brackets, int pcdID, int& line_no, int& newID) {
	
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

		string currentToken = tokens[i];
		bool isNum = isNumber(currentToken);

		//split by equal sigh
		//seems == is alr defined in string class, but good to know the compare()
		if (currentToken.compare("=") == 0) {
			metEqualSign = 1;
			continue;
		}

		//variable
		if (regex_match(currentToken, expr_inst)) {
			if (metEqualSign) {
				expression.append(currentToken);
				if (regex_match(currentToken, expr_var)) 
				{
					//current token is variable
					instID = getInstID(currentToken, pcdID, "variable");	
					Database::insertUseReln(newID, instID);
					if (!brackets.empty()) {
						for (bracketInfo b : brackets) {
							if (b.type == "while" || b.type == "if")
								Database::insertUseReln(b.db_id, instID);
							else if (b.type == "pcd") {
								Database::insertPcdUseReln(b.db_id, instID);
							}
						}
					}
				}
				else {
					//current token is constant
					instID = getInstID(currentToken, pcdID, "constant");
				}
			}
			else {
				if (regex_match(currentToken , expr_var)) 
				{
					modInstID = getInstID(currentToken, pcdID, "variable");
				}
				else {
					modInstID = getInstID(currentToken, pcdID, "constant");
				}
			}
		}

		//constant
		else {
			if (metEqualSign)
			{
				expression.append(currentToken);
			}
			else {
				if (regex_match(currentToken, expr_var))
				{
					getInstID(currentToken, pcdID, "variable");
				}
				else {
					getInstID(currentToken, pcdID, "constant");
				}
			}
		}
	}

	Database::insertModifyReln(newID, modInstID, expression);
	if (!brackets.empty()) {
		for (bracketInfo b : brackets) {
			if (b.type == "while" || b.type == "if")
				Database::insertModifyReln(b.db_id, modInstID, expression);
			else if (b.type == "pcd") {
				Database::insertPcdModifyReln(b.db_id, modInstID, expression);
			}
		}
	}

	//insert next reln record
	if (oldID > 0) Database::insertNextReln(oldID, newID);

	line_no++;
}

void SourceProcessor::callStmtHandler(string instName, int pcdID, int& line_no, int& newID) {

	//insert call reln and get id

	Database::insertStatement("call", line_no, line_no, pcdID, newID);
	Database::insertCallReln(pcdID, instName, line_no);
	line_no++;
}

void SourceProcessor::whileStmtHandler(vector<string> tokens, int pcdID, int& line_no, int& newID) {

	//insert statement and get id
	int oldID = newID;
	Database::insertStatement("while", line_no, line_no, pcdID, newID);

	//read condition and insert use reln
	std::regex expr_var("\[a-zA-z\]\[a-zA-z0-9\]\*\$");
	for (string token : tokens) {
		if (token != "while") {
			if (regex_match(token, expr_var))
			{
				Database::insertUseReln(newID, getInstID(token, pcdID, "variable"));
			}
			else if(isNumber(token)){
				getInstID(token, pcdID, "constant");
			}
		}
	}

	//insert next reln record
	if (oldID > 0) Database::insertNextReln(oldID, newID);
	line_no++;
}

void SourceProcessor::ifStmtHandler(vector<string> tokens, int pcdID, int& line_no, int& newID) {

	//insert statement and get id
	int oldID = newID;
	Database::insertStatement("if", line_no, line_no, pcdID, newID);

	//read condition and insert use reln
	std::regex expr_var("\[a-zA-z\]\[a-zA-z0-9\]\*\$");
	for (string token : tokens) {
		if (token != "if") {
			if (regex_match(token, expr_var))
			{
				Database::insertUseReln(newID, getInstID(token, pcdID, "variable"));
			}
			else if (isNumber(token)) {
				getInstID(token, pcdID, "constant");
			}
		}
	}

	//insert next reln record
	if (oldID > 0) Database::insertNextReln(oldID, newID);

	line_no++;
}

void SourceProcessor::parentRelnHander(vector<bracketInfo> brackets, int newID) {
	if (!brackets.empty())
	{
		for (bracketInfo currentBracket : brackets) {
			if (currentBracket.type == "while" || currentBracket.type == "if")
			{
				Database::insertParentReln(currentBracket.db_id, newID);
			}
		}
		//bracketInfo currentBracket = brackets.back();
		//if (currentBracket.type == "while" || currentBracket.type == "if")
		//{
		//	Database::insertParentReln(currentBracket.db_id, newID);
		//}
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

static void lowercase(string& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
		return std::tolower(c);
	});
}

