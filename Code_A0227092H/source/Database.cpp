#include "Database.h"
#include "fstream"
#include "iostream"
#include "set"
#include "regex"
using namespace std;

sqlite3* Database::dbConnection;
vector<vector<string>> Database::dbResults;
char* Database::errorMessage;
const string dbInitFileLocation = "../db.txt";

void Database::getNewID(string tblName, int& newID) {
	// clear the existing results
	dbResults.clear();
	
	//results are loaded to ‘dbResults'
	string sql = "SELECT _id FROM "+tblName+" ORDER BY _id DESC LIMIT 1;";
	sqlite3_exec(dbConnection, sql.c_str(), callback, 0, &errorMessage);

	//get the id from ‘dbResults'
	newID = 0;
	for (vector<string> dbRow : dbResults) {
		newID = stoi(dbRow.at(0));
	}
}

// method to connect to the database and initialize tables in the database
void Database::initialize() {
	// open a database connection and store the pointer into dbConnection
	sqlite3_open("database.db", &dbConnection);

	//read the sql init file (.txt) to get all DDL, make it clean here
	//open the txt file in pro IDE (e.g. VSC) for quick access/modify
	string line, sql;
	ifstream myfile(dbInitFileLocation);
	if (myfile.is_open()){
		while (getline(myfile, line)){
			//cout << line << '\n';
			sql += line;
		}
		myfile.close();
		sqlite3_exec(dbConnection, sql.c_str(), NULL, 0, &errorMessage);
	}else 
		cout << "Unable to create database";
	
	// initialize the result vector
	dbResults = vector<vector<string>>();
}

// method to close the database connection
void Database::close() {
	sqlite3_close(dbConnection);
}

// method to insert a procedure into the database
void Database::insertProcedure(string name, int line_sno, int& newID) {

	vector<string> procedures;
	getProcedures(procedures, name);

	if (procedures.size() > 0)
	{
		string updateProcedureSQL = "UPDATE pcd SET line_sno = '" + to_string(line_sno) + "' WHERE name = '" + name + "';";
		sqlite3_exec(dbConnection, updateProcedureSQL.c_str(), NULL, 0, &errorMessage);
		newID = stoi(procedures[0]);
	}
	else {
		string insertProcedureSQL = "INSERT INTO pcd (name, line_sno) VALUES ('" + name + "', '" + to_string(line_sno) + "');";
		sqlite3_exec(dbConnection, insertProcedureSQL.c_str(), NULL, 0, &errorMessage);
		getNewID("pcd", newID);
	}

	
}

// method to get all the procedures from the database
void Database::getProcedures(vector<string>& results){
	// clear the existing results
	dbResults.clear();

	// retrieve the procedures from the procedure table
	// The callback method is only used when there are results to be returned.
	string getProceduresSQL = "SELECT * FROM pcd;";
	sqlite3_exec(dbConnection, getProceduresSQL.c_str(), callback, 0, &errorMessage);

	// postprocess the results from the database so that the output is just a vector of procedure names
	for (vector<string> dbRow : dbResults) {
		string result;
		result = dbRow.at(0);
		results.push_back(result);
	}
}

// method to get procedure from the database by name
void Database::getProcedures(vector<string>& results, string name) {
	// clear the existing results
	dbResults.clear();

	// retrieve the procedures from the procedure table
	// The callback method is only used when there are results to be returned.
	string getProceduresSQL = "SELECT _id FROM pcd WHERE name = '" + name + "'";
	sqlite3_exec(dbConnection, getProceduresSQL.c_str(), callback, 0, &errorMessage);

	// postprocess the results from the database so that the output is just a vector of procedure names
	for (vector<string> dbRow : dbResults) {
		string result;
		result = dbRow.at(0);
		results.push_back(result);
	}
}

void Database::updateProcedure(int id, int line_eno) {
	string updateProcedureSQL = "UPDATE pcd SET line_eno = '" + to_string(line_eno) + "' WHERE _id = '" + to_string(id) + "';";
	sqlite3_exec(dbConnection, updateProcedureSQL.c_str(), NULL, 0, &errorMessage);
}

void Database::insertStatement(string type, int line_sno, int line_eno, int pcd__id, int& newID) {
	string sql = "INSERT INTO stmt (type, line_sno, line_sno, pcd__id) VALUES (";
	sql.append("'" + type + "', ");
	sql.append("'" + to_string(line_sno) + "', ");
	sql.append("'" + to_string(line_eno) + "', ");
	sql.append("'" + to_string(pcd__id) + "');");

	sqlite3_exec(dbConnection, sql.c_str(), NULL, 0, &errorMessage);
	getNewID("stmt", newID);
}

void Database::insertInstance(string type, string name, int pcd__id, int& newID) {
	string sql = "INSERT INTO instance (type, name, pcd__id) VALUES (";
	sql.append("'" + type + "', ");
	sql.append("'" + name + "', ");
	sql.append("'" + to_string(pcd__id) + "');");

	sqlite3_exec(dbConnection, sql.c_str(), NULL, 0, &errorMessage);
	getNewID("instance", newID);
}

void Database::updateStmt(int id, int line_eno) {
	string updateStmtSQL = "UPDATE stmt SET line_eno = '" + to_string(line_eno) + "' WHERE _id = '" + to_string(id) + "';";
	sqlite3_exec(dbConnection, updateStmtSQL.c_str(), NULL, 0, &errorMessage);
}

void Database::updateCallRelnTbls(string caller_name, string callee_name, int stmt_id) {
	int caller_id, callee_id;
	vector<string> results;

	getProcedures(results, caller_name);
	if (results.size() > 0)
	{
		caller_id = stoi(results[0]);
	}
	else {
		return;
	}

	results.clear();
	getProcedures(results, callee_name);
	if (results.size() > 0)
	{
		callee_id = stoi(results[0]);
	}
	else {
		return;
	}

	/*UPDATE RELN_USE_PCD TABLE*/
	//get reln_use_pcd by pcd callee_id
	dbResults.clear();

	string getUsePcdSQL = "SELECT * FROM reln_use_pcd WHERE pcd__id = '" + to_string(callee_id) + "'";
	sqlite3_exec(dbConnection, getUsePcdSQL.c_str(), callback, 0, &errorMessage);
	
	//loop through and duplicate records with caller_id as pcd_id
	for (vector<string> dbRow : dbResults) {		
		if (dbRow.size() == 2)
		{
			dbRow[0] = to_string(caller_id);
			insertPcdUseReln(stoi(dbRow[0]), stoi(dbRow[1]));
		}
	}

	/*UPDATE RELN_USE TABLE*/
	//get reln_use by callee_id
	dbResults.clear();
	string getUseSQL = "SELECT reln_use.stmt__id, reln_use.instance__id\
							FROM reln_use, pcd\
							WHERE reln_use.stmt__id >= pcd.line_sno\
							AND reln_use.stmt__id <= pcd.line_eno\
							AND pcd._id = '" + to_string(callee_id) + "'";
	sqlite3_exec(dbConnection, getUseSQL.c_str(), callback, 0, &errorMessage);

	//loop through and duplicate records with stmt_id as call stmt_id
	for (vector<string> dbRow : dbResults) {
		if (dbRow.size() == 2)
		{
			dbRow[0] = to_string(stmt_id);
			insertUseReln(stoi(dbRow[0]), stoi(dbRow[1]));
		}
	}

	/*UPDATE RELN_MODIFY_PCD TABLE*/
	//get reln_modify_pcd by pcd callee_id
	dbResults.clear();

	string getModifyPcdSQL = "SELECT * FROM reln_modify_pcd WHERE pcd__id = '" + to_string(callee_id) + "'";
	sqlite3_exec(dbConnection, getModifyPcdSQL.c_str(), callback, 0, &errorMessage);

	//loop through and duplicate records with caller_id as pcd_id
	for (vector<string> dbRow : dbResults) {
		if (dbRow.size() == 3)
		{
			dbRow[0] = to_string(caller_id);
			insertPcdModifyReln(stoi(dbRow[0]), stoi(dbRow[1]), dbRow[2]);
		}
	}

	/*UPDATE RELN_MODIFY TABLE*/
	dbResults.clear();
	string getModifySQL = "SELECT reln_modify.stmt__id, reln_modify.instance__id, reln_modify.expr\
							FROM reln_modify, pcd\
							WHERE reln_modify.stmt__id >= pcd.line_sno\
							AND reln_modify.stmt__id <= pcd.line_eno\
							AND pcd._id = '" + to_string(callee_id) + "'";
	sqlite3_exec(dbConnection, getModifySQL.c_str(), callback, 0, &errorMessage);

	//loop through and duplicate records with stmt_id as call stmt_id
	for (vector<string> dbRow : dbResults) {
		if (dbRow.size() == 3)
		{
			dbRow[0] = to_string(stmt_id);
			insertModifyReln(stoi(dbRow[0]), stoi(dbRow[1]), dbRow[2]);
		}
	}
}

void Database::updateParentRelnTbls(int parent_id, int child_id) {
	

	/*UPDATE RELN_MODIFY TABLE*/
	dbResults.clear();
	string getModifySQL = "SELECT * FROM reln_modify WHERE reln_modify.stmt__id = '" + to_string(child_id) + "'";
	sqlite3_exec(dbConnection, getModifySQL.c_str(), callback, 0, &errorMessage);

	//loop through and duplicate records with stmt_id as call stmt_id
	for (vector<string> dbRow : dbResults) {
		if (dbRow.size() == 3)
		{
			dbRow[0] = to_string(parent_id);
			insertModifyReln(stoi(dbRow[0]), stoi(dbRow[1]), dbRow[2]);
		}
	}

	/*UPDATE RELN_USE TABLE*/
	dbResults.clear();
	string getUseSQL = "SELECT * FROM reln_use WHERE reln_uses.stmt__id = '" + to_string(child_id) + "'";
	sqlite3_exec(dbConnection, getUseSQL.c_str(), callback, 0, &errorMessage);

	//loop through and duplicate records with stmt_id as call stmt_id
	for (vector<string> dbRow : dbResults) {
		if (dbRow.size() == 2)
		{
			dbRow[0] = to_string(parent_id);
			insertUseReln(stoi(dbRow[0]), stoi(dbRow[1]));
		}
	}
}

void Database::getInstance(vector<string>& results, vector<queryCond> queryConds, vector<int> cols) {
	// clear the existing results
	dbResults.clear();

	string sql = "SELECT * FROM instance WHERE 1=1 ";
	appendQueryCond(sql, queryConds);
	sql += ";";

	sqlite3_exec(dbConnection, sql.c_str(), callback, 0, &errorMessage);
	for (vector<string> dbRow : dbResults) {
		for (int col_no : cols) {
			results.push_back(dbRow.at(col_no));
		}
	}
}

void Database::appendQueryCond(string& queryString, vector<queryCond> queryConds) {
	for (queryCond q : queryConds) {
		if (q.customText)
			queryString.append(q.value + "\n");
		else
			queryString.append(q.joinType + " " + q.key + " = '" + q.value + "'\n");
	}
}

//method to insert a parent relationship
void Database::insertParentReln(int parent__id, int child__id, int level) {
	string sql = "INSERT INTO reln_parent (parent__id, child__id, level) VALUES (";
	sql.append("'" + to_string(parent__id) + "', ");
	sql.append("'" + to_string(child__id) + "', ");
	sql.append("'" + to_string(level) + "');");

	sqlite3_exec(dbConnection, sql.c_str(), NULL, 0, &errorMessage);
}

//method to insert a modify relationship
void Database::insertModifyReln(int stmt__id, int instance__id, string expression) {
	string sql = "INSERT INTO reln_modify (stmt__id, instance__id, expr) VALUES (";
	sql.append("'" + to_string(stmt__id) + "', ");
	sql.append("'" + to_string(instance__id) + "', ");
	sql.append("'" + expression + "');");

	sqlite3_exec(dbConnection, sql.c_str(), NULL, 0, &errorMessage);
}

void Database::insertPcdModifyReln(int pcd__id, int instance__id, string expression) {
	string sql = "INSERT INTO reln_modify_pcd (pcd__id, instance__id, expr) VALUES (";
	sql.append("'" + to_string(pcd__id) + "', ");
	sql.append("'" + to_string(instance__id) + "', ");
	sql.append("'" + expression + "');");

	sqlite3_exec(dbConnection, sql.c_str(), NULL, 0, &errorMessage);
}

//method to insert a parent relationship
void Database::insertUseReln(int stmt__id, int instance__id) {
	string sql = "INSERT INTO reln_use (stmt__id, instance__id) VALUES (";
	sql.append("'" + to_string(stmt__id) + "', ");
	sql.append("'" + to_string(instance__id) + "');");

	sqlite3_exec(dbConnection, sql.c_str(), NULL, 0, &errorMessage);
}

void Database::insertPcdUseReln(int pcd__id, int instance__id) {
	string sql = "INSERT INTO reln_use_pcd (pcd__id, instance__id) VALUES (";
	sql.append("'" + to_string(pcd__id) + "', ");
	sql.append("'" + to_string(instance__id) + "');");

	sqlite3_exec(dbConnection, sql.c_str(), NULL, 0, &errorMessage);
}


//method to insert a call relationship
void Database::insertCallReln(int caller__id, string callee, int stmt__id) {

	int callee_id = -1;
	vector<string> procedures;
	getProcedures(procedures, callee);

	if (procedures.size() > 0)
	{
		callee_id = stoi(procedures[0]);
	}
	else {
		insertProcedure(callee, NULL, callee_id);
	}

	string sql = "INSERT INTO reln_call (caller__id, callee__id, stmt__id) VALUES (";
	sql.append("'" + to_string(caller__id) + "', ");
	sql.append("'" + to_string(callee_id) + "', ");
	sql.append("'" + to_string(stmt__id) + "');");

	sqlite3_exec(dbConnection, sql.c_str(), NULL, 0, &errorMessage);
}

void Database::insertNextReln(int cur_stmt__id, int next_stmt__id) {
	string sql = "INSERT INTO reln_next (stmt__id, next_stmt__id) VALUES (";
	sql.append("'" + to_string(cur_stmt__id) + "', ");
	sql.append("'" + to_string(next_stmt__id) + "');");

	sqlite3_exec(dbConnection, sql.c_str(), NULL, 0, &errorMessage);
}


//void Database::buildQueryItem(string objType, string alias, queryItem& queryItem) {
//	queryItem.alias = alias;
//	queryItem.objType = objType;
//	
//	if (objType == "procedure") {
//		queryItem.table = "pcd";
//		queryItem.displayFields = "name";
//	}
//	else if (objType == "variable" || objType == "constant") {
//		queryItem.table = "instance";
//		queryItem.displayFields = "name";
//		queryItem.cond.push_back(queryCond{ "AND", "type", objType, false });
//	}
//	else {
//		queryItem.table = "stmt";
//		queryItem.displayFields = "line_sno";
//		if (objType != "stmt") {
//			queryItem.cond.push_back(queryCond{ "AND", "type", objType, false });
//		}
//	}
//}

static std::string trim(std::string s) {
	s = std::regex_replace(s, std::regex("^\\s+"), std::string(""));
	s = std::regex_replace(s, std::regex("\\s+$"), std::string(""));
	return s;
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


static void lowercase(string& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
		return std::tolower(c);
	});
}

static bool is_number(const std::string& s){
	std::string::const_iterator it = s.begin();
	while (it != s.end() && std::isdigit(*it)) ++it;
	return !s.empty() && it == s.end();
}

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
		index = -1;

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


void Database::selectHandler(string str, queryCmd& queryCmd){

	//TODO - iteration 3: grouping select

	vector<string> selections = split(str,",");
	string attrName = "";

	for (string selection : selections) {
		for (queryTable tbl : queryCmd.tables) {
			if (tbl.alias == selection) {
				attrName = tbl.tblName == "stmt" ? "line_sno" : "name";
				queryCmd.selections.push_back(queryItem{ tbl.tblAlias, attrName });
			}
		}
	}
	
}

//queryItem Database::getSelectionByAlias(queryCmd queryCmd, string alias) {
//	for (queryItem item : queryCmd.selections) {
//		if (item.alias == alias) {
//			return item;
//		}
//	}
//	return {};
//}


void Database::suchThatHandler(string str, queryCmd& queryCmd, vector<queryNextCond>& nextConds){
	str; //"follows* (2, s)"
	queryCmd;

	vector<string> tokens = split_multi(str, { "(", ")", "," });
	string relnType = trim(tokens[0]);
	string itemLeft = trim(tokens[1]);
	string itemRight = trim(tokens[2]);
	bool is_recursive_reln = relnType.find("*") != string::npos ? 1 : 0;
	if(is_recursive_reln) relnType = relnType.substr(0, relnType.size() - 1);
	lowercase(relnType);
	int tmpSize = queryCmd.tables.size();

	if (relnType == "follows") {
		//left = stmt (line no / alias)
		//right = stmt (line no / alias)

		/*
		1. same pcd__id
		2. right appear after left
		3. parents must be the same (null-null),(8,14-8,14), not exist A is left' parent, but not right's parent.
		*/

		string lineLeft, lineRight, pcdLeft, pcdRight, idLeft, idRight;

		if (is_number(itemLeft)) {
			//lineno
			lineLeft = itemLeft;
			pcdLeft = "(SELECT pcd__id from stmt where line_sno = " + itemLeft + ")";
			idLeft = "(SELECT _id from stmt where line_sno = " + itemLeft + ")";
		}
		else {
			//alias
			queryTable tLeft = findTable("alias", itemLeft, queryCmd);
			lineLeft = tLeft.tblAlias + ".line_sno";
			pcdLeft = tLeft.tblAlias + ".pcd__id";
			idLeft = tLeft.tblAlias + "._id";
		}

		if (is_number(itemRight)) {
			//lineno
			lineRight = itemRight;
			pcdRight = "(SELECT pcd__id from stmt where line_sno = " + itemRight + ")";
			idRight = "(SELECT _id from stmt where line_sno = " + itemRight + ")";
		}
		else {
			//alias
			queryTable tRight = findTable("alias", itemRight, queryCmd);
			lineRight = tRight.tblAlias + ".line_sno";
			pcdRight = tRight.tblAlias + ".pcd__id";
			idRight = tRight.tblAlias + "._id";
		}

		//TODO: they have to be under same parent as well. parent must be shared by both sides
		string condLine = is_recursive_reln ? lineLeft + " < " + lineRight : lineLeft + "+1 = " + lineRight;
		string condition = "AND " + condLine + " AND " + pcdLeft + " = " + pcdRight;
		condition += "\nAND NOT EXISTS (\n\
			SELECT parent__id FROM reln_parent\n\
			WHERE child__id = " + idLeft + " OR child__id = " + idRight + "\n\
			GROUP BY parent__id HAVING COUNT(DISTINCT child__id) = 1\n\
		)\n";
		queryCmd.conditions.push_back({ "","",condition ,1 });

	}

	else if (relnType == "next") {

		if (!is_recursive_reln) {

			//left side: stmt (line no / alias)
			queryTable leftStmtTable;
			if (is_number(itemLeft)) {
				leftStmtTable = { "stmt", "t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(leftStmtTable);
				queryCmd.conditions.push_back({ "AND",leftStmtTable.tblAlias + ".line_sno",itemLeft,0 });
			}
			else if (itemLeft == "_") {
				leftStmtTable = { "stmt", "t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(leftStmtTable);
			}
			else {
				leftStmtTable = findTable("alias", itemLeft, queryCmd);
			}

			//right side: stmt (line no / alias)
			queryTable rightStmtTable;
			if (is_number(itemRight)) {
				rightStmtTable = { "stmt","t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(rightStmtTable);
				queryCmd.conditions.push_back({ "AND",rightStmtTable.tblAlias + ".line_sno",itemRight,0 });
			}
			else if (itemRight == "_") {
				rightStmtTable = { "stmt","t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(rightStmtTable);
			}
			else {
				rightStmtTable = findTable("alias", itemRight, queryCmd);
			}

			//modify relations
			queryTable nextTable = queryTable{ "reln_next", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(nextTable);
			queryCmd.connects.push_back(tblConnector{ leftStmtTable.tblAlias, "_id", nextTable.tblAlias, "stmt__id" });
			queryCmd.connects.push_back(tblConnector{ nextTable.tblAlias, "next_stmt__id", rightStmtTable.tblAlias, "_id" });

		}
		else {

			//indicate the index of attr to be accessed
			int beginIndex = queryCmd.selections.size();
			nextConds.push_back({ beginIndex, beginIndex + 1 });

			//left side: stmt (line no / alias)
			queryTable leftStmtTable;
			if (is_number(itemLeft)) {
				leftStmtTable = { "stmt", "t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(leftStmtTable);
				queryCmd.conditions.push_back({ "AND",leftStmtTable.tblAlias + ".line_sno",itemLeft,0 });
			}
			else if (itemLeft == "_") {
				leftStmtTable = { "stmt", "t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(leftStmtTable);
			}
			else {
				leftStmtTable = findTable("alias", itemLeft, queryCmd);
			}
			queryCmd.selections.push_back({ leftStmtTable.tblAlias, "_id" });

			//right side: stmt (line no / alias)
			queryTable rightStmtTable;
			if (is_number(itemRight)) {
				rightStmtTable = { "stmt","t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(rightStmtTable);
				queryCmd.conditions.push_back({ "AND",rightStmtTable.tblAlias + ".line_sno",itemRight,0 });
			}
			else if (itemRight == "_") {
				rightStmtTable = { "stmt","t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(rightStmtTable);
			}
			else {
				rightStmtTable = findTable("alias", itemRight, queryCmd);
			}
			queryCmd.selections.push_back({ rightStmtTable.tblAlias, "_id" });
		}
	}
	else if (relnType == "parent") {

		//left side: stmt (line no / alias / _)
		queryTable leftStmtTable;
		if (is_number(itemLeft)) {
			leftStmtTable = { "stmt", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(leftStmtTable);
			queryCmd.conditions.push_back({ "AND",leftStmtTable.tblAlias + ".line_sno",itemLeft,0 });
		}
		else if (itemLeft == "_") {
			leftStmtTable = { "stmt", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(leftStmtTable);
		}
		else {
			leftStmtTable = findTable("alias", itemLeft, queryCmd);
		}

		//right side: stmt (line no / alias / _)
		queryTable rightStmtTable;
		if (is_number(itemRight)) {
			rightStmtTable = { "stmt","t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(rightStmtTable);
			queryCmd.conditions.push_back({ "AND",rightStmtTable.tblAlias + ".line_sno",itemRight,0 });
		}
		else if (itemRight == "_") {
			rightStmtTable = { "stmt","t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(rightStmtTable);
		}
		else {
			rightStmtTable = findTable("alias", itemRight, queryCmd);
		}

		//modify relations
		queryTable parentTable = queryTable{ "reln_parent", "t" + to_string(++tmpSize), "" };
		queryCmd.tables.push_back(parentTable);
		queryCmd.connects.push_back(tblConnector{ leftStmtTable.tblAlias, "_id", parentTable.tblAlias, "parent__id" });

		if (!rightStmtTable.tblAlias.empty())
		{
			queryCmd.connects.push_back(tblConnector{ parentTable.tblAlias, "child__id", rightStmtTable.tblAlias, "_id" });
		}

		//recursive condition
		if (!is_recursive_reln) {

			queryCmd.conditions.push_back({ "AND",parentTable.tblAlias + ".level","1",0});

			//if (!rightStmtTable.tblAlias.empty())
			//{
			//	queryCmd.conditions.push_back({ "","","AND NOT EXISTS(\
			//	SELECT * FROM reln_parent WHERE parent__id = " + leftStmtTable.tblAlias + "._id\
			//	AND child__id IN (SELECT parent__id FROM reln_parent WHERE child__id = " + rightStmtTable.tblAlias + "._id)\
			//	)",1 });
			//}
			//else {
			//	queryCmd.conditions.push_back({ "","","AND NOT EXISTS(\
			//	SELECT * FROM reln_parent WHERE parent__id = " + leftStmtTable.tblAlias + "._id\
			//	AND child__id IN (SELECT parent__id FROM reln_parent)\
			//	)",1 });
			//}

		}
	}

	else if (relnType == "uses") {

		//left side: stmt (line no / alias)
		queryTable leftTable;
		if (is_number(itemLeft)) {
			//Uses(x, y), x can be stmt no. value
			leftTable = { "stmt", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(leftTable);
			queryCmd.conditions.push_back({ "AND",leftTable.tblAlias + ".line_sno",itemLeft,0 });
			//queryCmd.recursivePrefix = "WITH RECURSIVE GETSTMT(N) as\
										(\
										VALUES(3)\
										UNION\
										SELECT stmt._id\
										FROM stmt, pcd caller, reln_call, pcd callee, GETSTMT\
										WHERE reln_call.caller__id = caller._id\
										AND reln_call.callee__id = callee._id\
										AND stmt.pcd__id = callee._id\
										AND reln_call.stmt__id = GETSTMT.N\
										)";
			//queryCmd.conditions.push_back(queryCond{"", "", " AND " + leftTable.tblAlias + "IN GETSTMT"});
		}
		else {
			vector<string> results;
			getProcedures(results, trim(itemLeft, '\"'));
			if (!results.empty()) {
				//pcd name
				leftTable = { "pcd", "t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(leftTable);
				queryCmd.conditions.push_back({ "AND",leftTable.tblAlias + ".name",trim(itemLeft, '\"'),0 });
			}
			else {
				leftTable = findTable("alias", itemLeft, queryCmd); //pcd or stmt table
			}
		}

		//right side: instance (name / alias)
		queryTable rightTable;
		if (itemRight.find("\"") != string::npos) {
			rightTable = { "instance","t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(rightTable);
			queryCmd.conditions.push_back({ "AND",rightTable.tblAlias + ".name",itemRight.substr(1, itemRight.size() - 2),0 });
		}
		else {
			// Uses(x, y), y can be inst alias
			rightTable = findTable("alias", itemRight, queryCmd);// can be instance table
		}

		//modify relations
		queryTable useTable;
		if (leftTable.tblName == "pcd")
		{
			useTable = queryTable{ "reln_use_pcd", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(useTable);
			queryCmd.connects.push_back(tblConnector{ leftTable.tblAlias, "_id", useTable.tblAlias, "pcd__id" });
		}
		else if (leftTable.tblName == "stmt")
		{
			useTable = queryTable{ "reln_use", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(useTable);
			queryCmd.connects.push_back(tblConnector{ leftTable.tblAlias, "_id", useTable.tblAlias, "stmt__id" });
		}
		
		if (!rightTable.tblAlias.empty())
		{
			queryCmd.connects.push_back(tblConnector{ useTable.tblAlias, "instance__id", rightTable.tblAlias, "_id" });
		}

		
		/*bool returnStmt = false;
		bool returnPcd = false;
		bool returnVar = false;
		vector<queryItem> selections = queryCmd.selections;
		vector<queryTable> tables = queryCmd.tables;

		for (int i = 0; i < selections.size(); i++)
		{
			queryItem item = selections.at(i);
			for (int j = 0; j < tables.size(); j++)
			{
				queryTable tbl = tables.at(j);
				if (tbl.tblAlias == item.tableALias)
				{
					if (tbl.tblName == "stmt")
					{
						returnStmt = true;
					}
					else if (tbl.tblName == "pcd")
					{
						returnPcd = true;
					}
					else if (tbl.tblName == "instance")
					{
						returnVar = true;
					}
					
				}
			}
		}

		string postfix;
		if (returnStmt)
		{
			postfix = " UNION\
							SELECT reln_call.stmt__id\
							FROM reln_call, instance, stmt, reln_use_pcd \
							WHERE reln_call.callee__id = instance.pcd__id\
							AND reln_call.stmt__id = stmt._id\
							AND reln_call.callee__id = reln_use_pcd.pcd__id";
			}
			else {
				postfix = " UNION\
							SELECT pcd.name\
							FROM reln_call, instance, stmt, pcd\
							WHERE reln_call.stmt__id = stmt._id\
							AND reln_call.callee__id = instance.pcd__id\
							AND reln_call.caller__id = pcd._id";
			}

			if (is_number(itemLeft))
			{
				postfix += " AND stmt._id = " + itemLeft;
			}

			if (itemRight.find("\"") != string::npos) {
				postfix += " AND instance.name = '" + itemRight.substr(1, itemRight.size() - 2) + "'";
			}

			for (int i = 0; i < queryCmd.conditions.size(); i++)
			{
				queryCond cond = queryCmd.conditions.at(i);
				if (cond.key.find("type") != string::npos)
				{
					if (cond.value == "read" || cond.value == "print" || cond.value == "assign"
						|| cond.value == "while" || cond.value == "if" || cond.value == "call")
					{
						postfix += " AND stmt.type = '" + cond.value + "'";
					}
					else if (cond.value == "variable" || cond.value == "constant")
					{
						postfix += " AND instance.type = '" + cond.value + "'";
					}
				}
			}

			queryCmd.postfix = postfix;
		}*/
	}

	else if (relnType == "modifies") {
		
		//left side: stmt (line no / alias)
		queryTable leftTable;
		string colLeft;
		if (is_number(itemLeft)) {
			leftTable = { "stmt", "t" + to_string(++tmpSize), "" };
			colLeft = "stmt__id";
			queryCmd.tables.push_back(leftTable);
			queryCmd.conditions.push_back({ "AND",leftTable.tblAlias + ".line_sno",itemLeft,0 });
		}
		else {
			vector<string> results;
			getProcedures(results, trim(itemLeft, '\"'));
			if (!results.empty()) {
				//pcd name
				leftTable = { "pcd", "t" + to_string(++tmpSize), "" };
				colLeft = "pcd__id";
				queryCmd.tables.push_back(leftTable);
				queryCmd.conditions.push_back({ "AND",leftTable.tblAlias + ".name", trim(itemLeft, '\"'), 0 });
			}
			else {
				leftTable = findTable("alias", itemLeft, queryCmd); //pcd or stmt table
				colLeft = leftTable.tblName == "pcd" ? "pcd__id" : "stmt__id";
			}
		}

		//right side: instance (name / alias)
		queryTable rightTable;
		if (itemRight.find("\"") != string::npos) {
			rightTable = { "instance","t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(rightTable);
			queryCmd.conditions.push_back({ "AND", rightTable.tblAlias + ".name",itemRight.substr(1, itemRight.size() - 2),0 });
		}
		else {
			rightTable = findTable("alias", itemRight, queryCmd);
		}

		//modify relations
		queryTable modifyTable;
		if (leftTable.tblName == "pcd")
		{
			modifyTable = queryTable{ "reln_modify_pcd", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(modifyTable);
			queryCmd.connects.push_back(tblConnector{ leftTable.tblAlias, "_id", modifyTable.tblAlias, colLeft });
		}
		else if (leftTable.tblName == "stmt")
		{
			modifyTable = queryTable{ "reln_modify", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(modifyTable);
			queryCmd.connects.push_back(tblConnector{ leftTable.tblAlias, "_id", modifyTable.tblAlias, colLeft });
		}

		if (!rightTable.tblAlias.empty())
		{
			queryCmd.connects.push_back(tblConnector{ modifyTable.tblAlias, "instance__id", rightTable.tblAlias, "_id" });
		}

	}

	else if (relnType == "calls") {
		//iteration 3

		int tmpSize = 1;

		/* Case 1
		
		(1)
		procedure p;
		Select p such that Calls(p, "second")
		                    caller                callee
		SELECT t1.name FROM pcd t1, reln_call t2, pcd t3
		WHERE t2.caller__id = t1._id
		AND t2.callee__id = t3._id
		AND t3.name = 'second'

		(2)
		procedure p;
		Select p such that Calls*(p, "second")

		WITH RECURSIVE GETID(N) AS (
		VALUES('second')
		UNION
		SELECT caller.name FROM reln_call, GETID, pcd caller, pcd callee
		WHERE caller._id = reln_call.caller__id AND callee._id = reln_call.callee__id AND callee.name=GETID.N)
									caller				 callee
		SELECT t1.name FROM pcd t1, reln_call t2, pcd t3
		WHERE t1._id = t2.caller__id AND t3._id = t2.callee__id AND t3.name IN GETID

		(3)
		procedure p;
		Select p such that Calls*(p, _)

		SELECT t1.name FROM pcd t1, reln_call t2
		WHERE t2.caller__id = t1._id

		*/ 

		if (itemLeft.find("\"") == string::npos //left item is alias
			&& (itemRight.find("\"") != string::npos || itemRight.find("_") != string::npos)) // right item is value/ _
		{

			queryTable pcdCallerTable = findTable("alias", itemLeft, queryCmd);
			queryTable relnCallTable = queryTable{ "reln_call", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(relnCallTable);
			queryCmd.connects.push_back(tblConnector{ relnCallTable.tblAlias, "caller__id", pcdCallerTable.tblAlias, "_id" });
			
			if (itemRight != "_") {

				queryTable pcdCalleeTable = queryTable{ "pcd", "t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(pcdCalleeTable);
				queryCmd.connects.push_back(tblConnector{ relnCallTable.tblAlias, "callee__id", pcdCalleeTable.tblAlias, "_id" });
			
				if (is_recursive_reln) {
					queryCmd.conditions.push_back(queryCond{ "", "","AND t3.name IN GETID", 1 });
					queryCmd.recursivePrefix = "WITH RECURSIVE GETID(N) AS (\
												VALUES('"+ itemRight.substr(1, itemRight.size() - 2) +"')\
												UNION\
												SELECT caller.name FROM reln_call, GETID, pcd caller, pcd callee\
												WHERE caller._id = reln_call.caller__id AND callee._id = reln_call.callee__id AND callee.name = GETID.N)";
				}
				else {
					queryCmd.conditions.push_back(queryCond{ "AND", pcdCalleeTable.tblAlias + ".name",itemRight.substr(1, itemRight.size() - 2), 0 });
				}
			}
		}

		/* Case 2
		
		(1)
		procedure p;
		Select p such that Calls("second", p)
						    callee                caller
		SELECT t1.name FROM pcd t1, reln_call t2, pcd t3
		WHERE t2.callee__id = t1._id
		AND t2.caller__id = t3._id
		AND t3.name = 'second'

		(2)
		procedure p;
		Select p such that Calls*("first", p)

		WITH RECURSIVE GETID(N) AS (
		VALUES('first')
		UNION
		SELECT callee.name FROM reln_call, GETID, pcd caller, pcd callee
		WHERE caller._id = reln_call.caller__id AND callee._id = reln_call.callee__id AND caller.name=GETID.N)
								callee				
		SELECT t1.name FROM pcd t1, reln_call t2, pcd t3
		WHERE t3._id = t2.caller__id AND t1._id = t2.callee__id AND t3.name IN GETID

		(3)
		procedure p;
		Select p such that Calls*(_, p)

		SELECT t1.name FROM pcd t1, reln_call t2
		WHERE t2.callee__id = t1._id

		*/

		else if ((itemLeft.find("\"") != string::npos || itemLeft.find("_") != string::npos) // left item is value/ _
			&& itemRight.find("\"") == string::npos) // right item is alias
		{

			if (itemLeft == "_")
			{
				queryTable pcdCalleeTable = findTable("alias", itemRight, queryCmd);
				queryTable relnCallTable = queryTable{ "reln_call", "t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(relnCallTable);
				queryCmd.connects.push_back(tblConnector{ relnCallTable.tblAlias, "callee__id", pcdCalleeTable.tblAlias, "_id" });

			}
			else {
				queryTable pcdCalleeTable = findTable("alias", itemRight, queryCmd);
				queryTable relnCallTable = queryTable{ "reln_call", "t" + to_string(++tmpSize), "" };
				queryTable pcdCallerTable = queryTable{ "pcd", "t" + to_string(++tmpSize), "" };
				queryCmd.tables.push_back(relnCallTable);
				queryCmd.tables.push_back(pcdCallerTable);
				queryCmd.connects.push_back(tblConnector{ relnCallTable.tblAlias, "callee__id", pcdCalleeTable.tblAlias, "_id" });
				queryCmd.connects.push_back(tblConnector{ relnCallTable.tblAlias, "caller__id", pcdCallerTable.tblAlias, "_id" });
			
				if (is_recursive_reln) {
					queryCmd.conditions.push_back(queryCond{ "", "", "AND t3.name IN GETID", 1 });
					queryCmd.recursivePrefix = "WITH RECURSIVE GETID(N) AS (\
												VALUES('"+ itemLeft.substr(1, itemLeft.size() - 2) +"')\
												UNION\
												SELECT callee.name FROM reln_call, GETID, pcd caller, pcd callee\
												WHERE caller._id = reln_call.caller__id AND callee._id = reln_call.callee__id AND caller.name = GETID.N)";
				}
				else {
					queryCmd.conditions.push_back(queryCond{ "AND", pcdCallerTable.tblAlias + ".name",itemLeft.substr(1, itemLeft.size() - 2), 0 });
				}
			}
		}

		/* Case 3
		
		(1)
		procedure p, q;
		Select p such that Calls(p, q)
					       			caller                 callee
		SELECT t1.name, t3.name FROM pcd t1, reln_call t2, pcd t3
		WHERE t2.caller__id = t1._id
		AND t2.callee__id = t3._id

		(2) //TODO: need to fix
		procedure p, q;
		Select p such that Calls*(p, q)

		with RECURSIVE pair as
		(
		SELECT t1.name as name1, t3.name as name2 FROM pcd t1, reln_call t2, pcd t3
		WHERE t2.caller__id = t1._id
		AND t2.callee__id = t3._id
		)
		select t1.name1, t1.name2 from pair t1

		*/

		else if (itemLeft.find("\"") == string::npos && itemRight.find("\"") == string::npos)
		{

			if (is_recursive_reln) {

				queryTable pairTable = queryTable{ "pair", "t" + to_string(tmpSize++), "" };
				queryItem callerItem = queryItem{ pairTable.tblAlias, "name1" };
				queryItem calleeItem = queryItem{ pairTable.tblAlias, "name2" };
				queryCmd.selections.push_back(callerItem);
				queryCmd.selections.push_back(calleeItem);
				queryCmd.tables.push_back(pairTable);

				queryCmd.recursivePrefix = "with RECURSIVE pair as(\
						SELECT t1.name as name1, t3.name as name2 FROM pcd t1, reln_call t2, pcd t3\
						WHERE t2.caller__id = t1._id\
						AND t2.callee__id = t3._id )";
			}
			else {
				queryTable pcdCallerTable = queryTable{ "pcd", "t" + to_string(tmpSize++), "p" };
				queryTable relnCallTable = queryTable{ "reln_call", "t" + to_string(tmpSize++), "" };
				queryTable pcdCalleeTable = queryTable{ "pcd", "t" + to_string(tmpSize++), "q" };
				queryItem callerItem = queryItem{pcdCallerTable.tblAlias, "name"};
				queryItem calleeItem = queryItem{ pcdCalleeTable.tblAlias, "name" };

				queryCmd.selections.push_back(callerItem);
				queryCmd.selections.push_back(calleeItem);
				queryCmd.tables.push_back(pcdCallerTable);
				queryCmd.tables.push_back(relnCallTable);
				queryCmd.tables.push_back(pcdCalleeTable);
				queryCmd.connects.push_back(tblConnector{ relnCallTable.tblAlias, "caller__id", pcdCallerTable.tblAlias, "_id" });
				queryCmd.connects.push_back(tblConnector{ relnCallTable.tblAlias, "callee__id", pcdCalleeTable.tblAlias, "_id" });
			}
		}
	}
}

//not sure why cannot define under header file. strange
static vector<vector<int>> routes;

void Database::initRoutes() {
	//build the graph and store to routes
	dbResults.clear();

	int prevID, nextID;
	int maxID = 0;

	string sql = "SELECT stmt__id, next_stmt__id FROM reln_next;";
	sqlite3_exec(dbConnection, sql.c_str(), callback, 0, &errorMessage);
	for (vector<string> dbRow : dbResults) {
		prevID = stoi(dbRow[0]);
		if (prevID > maxID) maxID = prevID;

		nextID = stoi(dbRow[1]);
		if (nextID > maxID) maxID = nextID;

		while (routes.size() <= maxID) {
			routes.push_back({});
		}
		routes[prevID].push_back(nextID);
	}
}

static bool isSubArray(vector<int> parent, vector<int> child)
{
	// Two pointers to traverse the arrays
	int i = 0, j = 0;

	// Traverse both arrays simultaneously
	while (i < parent.size() && j < child.size()) {

		// If element matches
		// increment both pointers
		if (parent[i] == child[j]) {

			i++;
			j++;

			// If array B is completely
			// traversed
			if (j == child.size())
				return true;
		}
		// If not,
		// increment i and reset j
		else {
			i = i - j + 1;
			j = 0;
		}
	}

	return false;
}

bool Database::findNextReln(int prevID, int nextID) {
	//--next* (18, s)
	//	WITH RECURSIVE GETID(N) AS(
	//		VALUES('18')
	//		UNION
	//		SELECT bottom._id
	//		FROM reln_next, GETID, stmt top, stmt bottom
	//		WHERE top._id = reln_next.stmt__id AND bottom._id = reln_next.next_stmt__id AND top._id = GETID.N)
	//	SELECT* FROM GETID

	if (prevID == nextID) return false;
	dbResults.clear();

	string sql = "WITH RECURSIVE GETID(N) AS(\
		VALUES('"+ to_string(prevID) +"')\
		UNION\
		SELECT bottom._id\
		FROM reln_next, GETID, stmt top, stmt bottom\
		WHERE top._id = reln_next.stmt__id AND bottom._id = reln_next.next_stmt__id AND top._id = GETID.N)\
		SELECT* FROM GETID";
	sqlite3_exec(dbConnection, sql.c_str(), callback, 0, &errorMessage);

	for (vector<string> row : dbResults) {
		if (std::find(row.begin(), row.end(), to_string(nextID)) != row.end())
			return true;
	}
	return false;

}


//bool Database::findNextReln(int prevID, int nextID) {
//	if (!routes.size()) initRoutes();
//	vector<int> queue;
//	vector<int> visited;
//	vector<int>::iterator ip;
//	
//	if (routes[prevID].empty()) return false;
//	for (int stmtID : routes[prevID]) {
//		if (stmtID == nextID) return true;
//		else {
//			queue.push_back(stmtID);
//		}
//	}
//
//	int index = 0;
//	while (!isSubArray(visited, queue)) {
//		if (std::find(visited.begin(), visited.end(), queue[index]) == visited.end()) goto nxt;
//		for (int stmtID : routes[queue[index]]) {
//			if (stmtID == nextID) return true;
//			else {
//				queue.push_back(stmtID);
//			}
//		}
//
//		//unique the queue
//		ip = std::unique(queue.begin(), queue.end());
//		queue.resize(std::distance(queue.begin(), ip));
//
//		nxt:
//		visited.push_back(queue[index]);
//		index++;
//	}
//
//	return false;
//}

string Database::getPatternString(string str) {
	std::regex expr_pattern("_\?\"\[a-zA-z0-9\\s\+\-\\/\\*%\\(\\)\]\+\"_\?");
	vector<string>results;
	std::smatch m;

	while (std::regex_search(str, m, expr_pattern)) {
		//for (auto x : m) std::cout << x << " ";
		results.push_back(m[0]);
		str = m.suffix().str();
	}

	return results.size() > 0 ? results[0] : "";
}

queryTable Database::findTable(string type, string key, queryCmd queryCmd) {
	for (queryTable table : queryCmd.tables) {
		if (type=="alias" && table.alias == trim(key)) {
			return table;
		}
		else if (type == "tblAlias" && table.tblAlias == trim(key)) {
			return table;
		}
	}
	return {};
}


int Database::findTableNum(queryCmd queryCmd) {

	int tempSize = 0;

	for (queryTable table : queryCmd.tables) {
		string numInStr = trim(table.tblAlias, 't');

		if (is_number(numInStr))
		{
			tempSize = stoi(numInStr);
		}

	}
	return tempSize;
}


void Database::patternHandler(string str, queryCmd& queryCmd, vector<queryPattern>& patterns){
	//a(_, _"x * y + z * t"_)
	/*
	LHS: _ or variable or "string"
	RHS: _ or "string"

	Case 1: pattern a(_, _)

		SELECT DISTINCT t1.line_sno, t2.expr
		FROM stmt t1, reln_modify t2
		WHERE t1._id = t2.stmt__id
		AND t1.type = 'assign';

	Case 2: pattern a(_, _"i"_)

		SELECT DISTINCT t1.line_sno, t2.expr FROM stmt t1, reln_modify t2
		WHERE t1._id = t2.stmt__id
		AND t1.type = 'assign';

	Case 3: pattern a(v, _)

		SELECT DISTINCT t1.line_sno, t3.expr
		FROM stmt t1, instance t2, reln_modify t3, instance t4, reln_use t5
		WHERE t1._id = t3.stmt__id
		AND t4._id = t5.instance__id AND t1.type = 'assign' AND t2.type = 'variable';

	Case 4: pattern a(v, _"i"_)

		
	Case 5: pattern a("i", _)

	Case 6: pattern a("x", _"i"_)

		SELECT DISTINCT t1.line_sno, t2.expr
		FROM stmt t1, reln_modify t2, instance t3
		WHERE t1._id = t2.stmt__id
		AND t2.instance__id = t3._id
		AND t1.type = 'assign'
		AND t3.name = 'x';

	*/
	Node n;
	int tmpSize = 0;
	vector<string> sub_str = split(str, ",");
	vector<string> tokens = split(sub_str[0], "(");

	//select the alias
	string alias = tokens[0];
	queryTable t1 = findTable("alias", alias, queryCmd);
	tmpSize = queryCmd.tables.size() + 1;
	string relnTblAlias = "t" + to_string(tmpSize);

	queryCmd.tables.push_back(queryTable{ "reln_modify", relnTblAlias, ""});
	queryCmd.connects.push_back({ t1.tblAlias, "_id", relnTblAlias, "stmt__id" });
	int ptn_index = queryCmd.selections.size();
	queryCmd.selections.push_back({ relnTblAlias , "expr" });

	//register the left pattern
	string pattern_left = tokens[1];
	string clean_pattern_left = regex_replace(pattern_left, std::regex("\[_\\s\"\]"), std::string(""));

	if (clean_pattern_left != "") {
		//FROM reln_use t1 JOIN instance i ON i._id = t1.instance__id AND i.name = ...

		//join instance table
		tmpSize = queryCmd.tables.size() + 1;
		//appendEntityTable("instance", {pattern_left}, queryCmd);

		queryTable instanceTable = findTable("alias", pattern_left, queryCmd);
		if (instanceTable.tblAlias.empty())
		{
			instanceTable = queryTable{ "instance", "t" + to_string(tmpSize), pattern_left };
			queryCmd.tables.push_back(instanceTable);
		}

		
		queryCmd.connects.push_back(tblConnector{relnTblAlias, "instance__id", instanceTable.tblAlias, "_id"});

		if (pattern_left.find("\"") != string::npos) // Left side is "string"
		{
			queryCmd.conditions.push_back({ "","","AND " + instanceTable.tblAlias + ".name = '" + clean_pattern_left + "'", true});
		}

		//join reln_use table
		/*relnTblAlias = "t" + to_string(++tmpSize);
		queryCmd.tables.push_back(queryTable{ "reln_use", relnTblAlias, "" });
		queryCmd.connects.push_back({ "t" + to_string(--tmpSize), "_id", relnTblAlias, "instance__id" });*/

	}

	//register the right pattern
	string pattern_right = getPatternString(sub_str[1]);
	bool openLeft = regex_match(pattern_right, regex("\^_\.\*\$"));
	bool openRight = regex_match(pattern_right, regex("\^\.\*_\$"));
	string clean_pattern_right = regex_replace(pattern_right, std::regex("\[_\\s\"\]"), std::string(""));
	if (clean_pattern_right != "") {
		ExprTree::exprStrToNode(clean_pattern_right, n);
	}
	else {
		n = Node{"", 0, NULL, NULL};
	}
	patterns.push_back(queryPattern{ ptn_index, n, openLeft, openRight });

	//after query the results, need to loop the result and do tree mapping
}

void Database::queryCmdToResult(vector<vector<string>>& results, queryCmd queryCmd) {

	dbResults.clear();
	string queryStr = "\nSELECT DISTINCT ";

	//build select clause
	for (queryItem item : queryCmd.selections) {
		if (item.tableALias != "") queryStr += item.tableALias + ".";
		queryStr += item.attrName + ", ";
	}
	queryStr = regex_replace(queryStr, std::regex("\,\\s\$"), "");

	//build from clause
	queryStr += " \nFROM ";
	for (queryTable tbl : queryCmd.tables) {
		queryStr += tbl.tblName + " " + tbl.tblAlias + ", ";
	}
	queryStr = regex_replace(queryStr, std::regex("\,\\s\$"), "");

	//build join conditions
	queryStr += " \nWHERE 1=1 ";
	for (tblConnector connector : queryCmd.connects) {
		queryStr += "AND " + connector.aliasLeft + "." + connector.keyLeft + " = " + connector.aliasRight + "." + connector.keyRight + "\n";
	}
	queryStr = regex_replace(queryStr, std::regex("1=1 AND "), "");

	//build where clause
	appendQueryCond(queryStr, queryCmd.conditions);

	//test purpose
	//queryStr = "SELECT stmt.line_sno AS a, reln_modify.expr \
	//	FROM reln_modify , stmt\
	//	WHERE 1=1  AND reln_modify.stmt__id = stmt._id AND stmt.type = 'assign';";

	if (!queryCmd.recursivePrefix.empty()) {
		queryStr = queryCmd.recursivePrefix + queryStr;
	}

	if (!queryCmd.postfix.empty())
	{
		queryStr += queryCmd.postfix;
	}

	//finish and return
	queryStr = regex_replace(queryStr, std::regex("\\s\*\$"), "");
	queryStr += ";";

	cout << "run query: " << queryStr << endl;
	sqlite3_exec(dbConnection, queryStr.c_str(), callback, 0, &errorMessage);
	for (vector<string> dbRow : dbResults) {
		results.push_back(dbRow);
	}

}

void Database::appendTables(string& queryString, vector<string> tables) {
	//stmt, pcd, instance, reln_parent, reln_modify, reln_use, reln_call

	//iteration 1 implementation: only 1 table is required
	//string table = tables[0];
	//queryString += table;

	//iteration 2: build a direct graph using adjacency list maybe?
	//build a graph links all tables(node), and store their join condition in edges.
	//found some example here: https://www.techiedelight.com/graph-implementation-using-stl/
	
	if (tables.size() <= 1)
	{
		string table = tables[0];
		queryString += table;
	}
	else {
		BFSJoinTable(queryString, tables);
	}

}

void Database::prepareTableGraph(vector<tableEdge> adjList[]) {
	// 0 - pcd
	// 1 - stmt
	// 2 - instance
	// 3 - reln_parent
	// 4 - reln_modify
	// 5 - reln_use
	// 6 - reln_call
	addEdge(adjList, 0, 1, "pcd._id = stmt.pcd__id");
	addEdge(adjList, 0, 2, "pcd._id = instance.pcd__id");
	addEdge(adjList, 0, 6, "pcd._id = reln_call.callee__id");

	addEdge(adjList, 1, 3, "stmt._id = reln_parent.parent__id");
	addEdge(adjList, 1, 4, "stmt._id = reln_modify.stmt__id");
	addEdge(adjList, 1, 5, "stmt._id = reln_use.parent__id");
	addEdge(adjList, 1, 6, "stmt._id = reln_call.caller__id");

	addEdge(adjList, 2, 4, "instance._id = reln_modify.instance__id");
	addEdge(adjList, 2, 5, "instance._id = reln_use.instance__id");

}

void Database::addEdge(vector<tableEdge> adjList[], int src, int desc, string joinQuery) {
	adjList[src].push_back(tableEdge{ desc , joinQuery });
	adjList[desc].push_back(tableEdge{ src , joinQuery });
}

void Database::BFSJoinTable(string& queryString, vector<string> tables) {

	vector<tableEdge> adjList[7];
	int pred[7];

	//step 1: create graph usiung adjacency list
	prepareTableGraph(adjList);

	vector<string> tableNames =
	{
		"pcd" ,
		"stmt",
		"instnace",
		"reln_parent",
		"reln_modify",
		"reln_use",
		"reln_call"
	};

	int num = tables.size();
	vector<int> tableIds;

	for (int i = 0; i < tables.size(); i++)
	{
		for (int j = 0; j < tableNames.size(); j++)
		{
			if (tables[i] == tableNames[j])
			{
				tableIds.push_back(j);
			}
		}
	}

	//step 2: start BFS
	bool* visited = new bool[7];
	for (int i = 0; i < 7; i++) {
		visited[i] = false;
		pred[i] = -1;
	}

	vector<int> queue;

	visited[tableIds[0]] = true;
	queue.push_back(tableIds[0]);

	int currentId = queue.front();
	vector<tableEdge>::iterator i;

	while (!queue.empty())
	{
		currentId = queue.front();
		//cout << currentId << " ";
		queue.erase(queue.begin());

		for (i = adjList[currentId].begin(); i != adjList[currentId].end(); ++i)
		{
			int destId = (*i).destId;
			if (!visited[destId])
			{
				visited[destId] = true;
				pred[destId] = currentId;
				queue.push_back(destId);

				if (destId == tableIds[1])
				{
					break;
				}
			}
		}
	}

	//step 3: get shortest path and prepare join cond 
	vector<int> path;
	string cond;
	int crawl = tableIds[1];
	path.push_back(crawl);
	while (pred[crawl] != -1) {
		path.push_back(pred[crawl]);

		vector<tableEdge> edges = adjList[crawl];
		for (int j = 0; j < edges.size(); j++)
		{
			if (edges.at(j).destId == pred[crawl])
			{
				if (cond.empty())
				{
					cond += " WHERE " + edges.at(j).joinCond;
				}
				else {
					cond += " AND " + edges.at(j).joinCond;
				}
			}
		}
		crawl = pred[crawl];
	}

	for (int i = path.size() - 1; i >= 0; i--) {
		queryString += tableNames[path[i]] + ", ";

	}
	queryString = queryString.substr(0, queryString.size() - 2);
	queryString += cond;
	//cout << queryString;
}

// callback method to put one row of results from the database into the dbResults vector
// This method is called each time a row of results is returned from the database
int Database::callback(void* NotUsed, int argc, char** argv, char** azColName) {
	NotUsed = 0;
	vector<string> dbRow;

	// argc is the number of columns for this row of results
	// argv contains the values for the columns
	// Each value is pushed into a vector.
	for (int i = 0; i < argc; i++) {
		dbRow.push_back(argv[i]);
	}

	// The row is pushed to the vector for storing all rows of results 
	dbResults.push_back(dbRow);

	return 0;
}

//void Database::buildQueryItem(string objType, string alias, queryItem& queryItem) {
//	queryItem.alias = alias;
//	queryItem.objType = objType;
//
//	if (objType == "procedure") {
//		queryItem.table = "pcd";
//		queryItem.displayFields = "name";
//	}
//	else if (objType == "variable" || objType == "constant") {
//		queryItem.table = "instance";
//		queryItem.displayFields = "name";
//		queryItem.cond.push_back(queryCond{ "AND", "type", objType, false });
//	}
//	else {
//		queryItem.table = "stmt";
//		queryItem.displayFields = "line_sno";
//		if (objType != "stmt") {
//			queryItem.cond.push_back(queryCond{ "AND", "type", objType, false });
//		}
//	}
//
//}


void Database::appendEntityTable(string type, vector<string> aliases, queryCmd& queryCmd) {
	// type =  procedure/ stmt/ variable/ assign/ while/ if/ print/ read/ constant

	for ( int i = 0; i < aliases.size(); i++)
	{
		
		string alias = trim(aliases[i], ',');
		string tblAlias = "t" + to_string(queryCmd.tables.size() + 1);
		string tblName = "";

		if (type == "procedure") {
			tblName = "pcd";
		}
		else if (type == "variable" || type == "constant" || type == "instance") {
			tblName = "instance";
			if (type != "instance") {
				//queryCmd.conditions.push_back(queryCond{ "", "", " AND " + tblAlias + ".type = '" + type + "'", true });
				queryCmd.conditions.push_back(queryCond{ " AND", tblAlias + ".type", type , false });
			}
		}
		else {
			tblName = "stmt";
			if (type != "stmt") {
				//queryCmd.conditions.push_back(queryCond{ "", "", " AND " + tblAlias + ".type = '" + type + "'", true });
				queryCmd.conditions.push_back(queryCond{ " AND", tblAlias + ".type", type , false });
			}
		}

		queryCmd.tables.push_back(queryTable{ tblName, tblAlias , alias });
	}
}
