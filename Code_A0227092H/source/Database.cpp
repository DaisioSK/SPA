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
			queryString.append(q.value + " \n");
		else
			queryString.append(q.joinType + " " + q.key + " = '" + q.value + "' \n");
	}
}

//method to insert a parent relationship
void Database::insertParentReln(int parent__id, int child__id) {
	string sql = "INSERT INTO reln_parent (parent__id, child__id) VALUES (";
	sql.append("'" + to_string(parent__id) + "', ");
	sql.append("'" + to_string(child__id) + "');");

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


//method to insert a parent relationship
void Database::insertUseReln(int stmt__id, int instance__id) {
	string sql = "INSERT INTO reln_use (stmt__id, instance__id) VALUES (";
	sql.append("'" + to_string(stmt__id) + "', ");
	sql.append("'" + to_string(instance__id) + "');");

	sqlite3_exec(dbConnection, sql.c_str(), NULL, 0, &errorMessage);
}


//method to insert a call relationship
void Database::insertCallReln(int caller__id, string callee) {

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

	string sql = "INSERT INTO reln_call (caller__id, callee__id) VALUES (";
	sql.append("'" + to_string(caller__id) + "', ");
	sql.append("'" + to_string(callee_id) + "');");

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


void Database::suchThatHandler(string str, queryCmd& queryCmd){
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
		* follows* (3, s)
		select stmt.line_sno
		from stmt
		where stmt.line_sno > 3 and stmt.pcd__id = (select pcd__id from stmt where line_sno = 2)
		and not exist
		(	
			select *
			from reln_parent 
			where child__id = stmt._id and parent__id not in (select parent__id from reln_parent where child__id = 2)
		);


		from stmt t1, stmt t2
		where t1.pcd__id = t2.pcd__id AND t1.line_sno > t2.line_sno
		AND NOT EXIST
		(
			select parent__id from reln_parent
			where child__id in (t1._id, t2._id)
			group_by parent__id 
			having count(distinct child__id) = 1;
		)


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
			idRight= "(SELECT _id from stmt where line_sno = " + itemRight + ")";
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
		string condition = " AND " + condLine + " AND " + pcdLeft + " = " + pcdRight;
		condition += "\nAND NOT EXISTS (\n\
			SELECT parent__id FROM reln_parent\n\
			WHERE child__id = "+ idLeft +" OR child__id = "+idRight+"\n\
			GROUP BY parent__id HAVING COUNT(DISTINCT child__id) = 1\n\
		)\n";
		queryCmd.conditions.push_back({ "","",condition ,1 });

	}

	else if (relnType == "next") {
		//left = stmt (line no / alias)
		//right = stmt (line no / alias)

		/*
		* next* (3, s)
		select stmt.line_sno
		from stmt
		where stmt.line_sno > 3 and stmt.pcd__id = (select pcd__id from stmt where line_sno = 2)
		and not exist
		(
			select *
			from reln_parent
			where child__id = stmt._id and parent__id not in (select parent__id from reln_parent where child__id = 2)
		);


		from stmt t1, stmt t2
		where t1.pcd__id = t2.pcd__id AND t1.line_sno > t2.line_sno
		AND NOT EXIST
		(
			select parent__id from reln_parent
			where child__id in (t1._id, t2._id)
			group_by parent__id
			having count(distinct child__id) = 1;
		)


		1. same pcd__id
		2. while and its children

		*/
	}
	
	else if (relnType == "parent") {
		//left = stmt (line no / alias)
		//right = stmt (line no / alias)

		/*
		* parent* (2, s)
		
		* parent* (s, 2)

		from stmt a, stmt b, reln_parent
		where reln_parent.parent__id = a._id and reln_parent.child__id = b._id
		
		parent* (a,b)
		select ...
		from stmt a, stmt b, reln_parent
		where reln_parent.parent__id = a._id and reln_parent.child__id = b._id

		parent (a,b)
		select ...
		from stmt a, stmt b, reln_parent
		where reln_parent.parent__id = a._id and reln_parent.child__id = b._id
		and not exist (
			select * from reln_parent where parent__id = a
			and child__id in (select parent__id from reln_parent where child__id = b._id)
		)
		*/

		//queryCmd.tables.push_back({table, how_to_join, (alias)});

		//left side: stmt (line no / alias)
		queryTable leftStmtTable;
		if (is_number(itemLeft)) {
			leftStmtTable = { "stmt", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(leftStmtTable);
			queryCmd.conditions.push_back({ "AND",leftStmtTable.tblAlias + ".line_sno",itemLeft,0 });
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
		else {
			rightStmtTable = findTable("alias", itemRight, queryCmd);
		}

		//modify relations
		queryTable parentTable = queryTable{ "reln_parent", "t" + to_string(++tmpSize), "" };
		queryCmd.tables.push_back(parentTable);
		queryCmd.connects.push_back(tblConnector{ leftStmtTable.tblAlias, "_id", parentTable.tblAlias, "parent__id" });
		queryCmd.connects.push_back(tblConnector{ parentTable.tblAlias, "child__id", rightStmtTable.tblAlias, "_id" });

		//recursive condition
		if (!is_recursive_reln) {
			queryCmd.conditions.push_back({ "","","AND NOT EXISTS(\
			SELECT * FROM reln_parent WHERE parent__id = " + leftStmtTable.tblAlias + "._id\
			AND child__id IN (SELECT parent__id FROM reln_parent WHERE child__id = " + rightStmtTable.tblAlias + "._id)\
			)",1 });
		}
	}

	else if (relnType == "uses") {

		int tmpSize = findTableNum(queryCmd);

		/*
		Uses(stmt, variable) ==> stmt <+> reln_use <+> instance
		Uses(pcd, variable) ==> pcd <+> instance <+> reln_use
		*/

		//left side: stmt (line no / alias)
		queryTable leftTable;
		if (is_number(itemLeft)) {
			//Uses(x, y), x can be stmt no. value
			leftTable = { "stmt", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(leftTable);
			queryCmd.conditions.push_back({ "AND",leftTable.tblAlias + ".line_sno",itemLeft,0 });
		}
		else {
			// Uses(x, y) x can be stmt alias/ pcd alias/ pcd name value
			leftTable = findTable("alias", itemLeft, queryCmd); // can be stmt/ procedure tbl
		}

		//right side: instance (name / alias)
		queryTable rightTable;
		if (itemRight.find("\"") != string::npos) {
			//Uses(x, y), y can be inst name value
			rightTable = { "instance","t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(rightTable);
			queryCmd.conditions.push_back({ "AND",rightTable.tblAlias + ".name",itemRight.substr(1, itemRight.size() - 2),0 });
		}
		else {
			// Uses(x, y), y can be inst alias
			rightTable = findTable("alias", itemRight, queryCmd);// can be instance table
		}

		//modify relations
		queryTable useTable = queryTable{ "reln_use", "t" + to_string(++tmpSize), "" };
		queryCmd.tables.push_back(useTable);
		if (leftTable.tblName == "pcd")
		{
			queryCmd.connects.push_back(tblConnector{ leftTable.tblAlias, "_id", rightTable.tblAlias, "pcd__id" });
		}
		else if (leftTable.tblName == "stmt")
		{
			queryCmd.connects.push_back(tblConnector{ leftTable.tblAlias, "_id", useTable.tblAlias, "stmt__id" });
		}
		
		if (!rightTable.tblAlias.empty())
		{
			queryCmd.connects.push_back(tblConnector{ useTable.tblAlias, "instance__id", rightTable.tblAlias, "_id" });
		}
	}

	else if (relnType == "modifies") {

		int tmpSize = 1;

		//TODO: temp solution - to delete after updateing appendEntityTable method
		for (int i = 0; i < queryCmd.conditions.size(); i++)
		{
			if (queryCmd.conditions.at(i).value == " AND t1.type = 'assign'")
			{
				queryCmd.conditions.at(i).value = " AND (t1.type = 'assign' OR t1.type = 'read')";
			}
		}
		
		//left side: stmt (line no / alias)
		queryTable stmtTable;
		if (is_number(itemLeft)) {
			stmtTable = { "stmt", "t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(stmtTable);
			queryCmd.conditions.push_back({ "AND",stmtTable.tblAlias + ".line_sno",itemLeft,0 });
		}
		else {
			stmtTable = findTable("alias", itemLeft, queryCmd);
		}

		//right side: instance (name / alias)
		queryTable instanceTable;
		if (itemRight.find("\"") != string::npos) {
			instanceTable = { "instance","t" + to_string(++tmpSize), "" };
			queryCmd.tables.push_back(instanceTable);
			queryCmd.conditions.push_back({ "AND",instanceTable.tblAlias + ".name",itemRight.substr(1, itemRight.size() - 2),0 });
		}
		else {
			instanceTable = findTable("alias", itemRight, queryCmd);
		}

		//modify relations
		queryTable modifyTable = queryTable{ "reln_modify", "t" + to_string(++tmpSize), "" };
		queryCmd.tables.push_back(modifyTable);
		queryCmd.connects.push_back(tblConnector{ stmtTable.tblAlias, "_id", modifyTable.tblAlias, "stmt__id" });
		queryCmd.connects.push_back(tblConnector{ modifyTable.tblAlias, "instance__id", instanceTable.tblAlias, "_id" });

		//iteration 3: left can be procedures
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
		appendEntityTable("instance", "inst" + to_string(tmpSize), queryCmd);
		queryCmd.conditions.push_back({ "",""," AND inst" + to_string(tmpSize) + ".name = " + clean_pattern_left , true });

		//join reln_use table
		relnTblAlias = "t" + to_string(tmpSize);
		queryCmd.tables.push_back(queryTable{ "reln_use", relnTblAlias, "" });
		queryCmd.connects.push_back({ "inst" + to_string(tmpSize), "_id", relnTblAlias, "instance__id" });

	}

	//register the right pattern
	string pattern_right = getPatternString(sub_str[1]);
	bool openLeft = regex_match(pattern_right, regex("\^_\.\*\$"));
	bool openRight = regex_match(pattern_right, regex("\^\.\*_\$"));
	string clean_pattern_right = regex_replace(pattern_right, std::regex("\[_\\s\"\]"), std::string(""));
	if (clean_pattern_right != "") {
		ExprTree::exprStrToNode(clean_pattern_right, n);
		patterns.push_back(queryPattern{ ptn_index, n, openLeft, openRight });
	}

	//after query the results, need to loop the result and do tree mapping
	//end of function. now test algorithm


	//string test_pattern = "d+e";
	//string test2_pattern = "b*c";

	//Node n1, n2;
	//ExprTree::exprStrToNode(clean_pattern, n1);
	//ExprTree::exprStrToNode(test2_pattern, n2);

	//bool test = ExprTree::HasSubtree(&n1, &n2);

}

void Database::queryCmdToResult(vector<vector<string>>& results, queryCmd queryCmd) {

	dbResults.clear();
	string queryStr = "\nSELECT DISTINCT ";

	//build select clause
	for (queryItem item : queryCmd.selections) {
		queryStr += item.tableALias + "." + item.attrName + ", ";
	}
	queryStr = regex_replace(queryStr, std::regex("\,\\s\$"), "");

	//build from clause
	queryStr += " \nFROM ";
	for (queryTable tbl : queryCmd.tables) {
		queryStr += tbl.tblName + " " + tbl.tblAlias + ", ";
	}
	queryStr = regex_replace(queryStr, std::regex("\,\\s\$"), "");

	//build join conditions
	queryStr += " \nWHERE 1=1";
	for (tblConnector connector : queryCmd.connects) {
		queryStr += " AND " + connector.aliasLeft + "." + connector.keyLeft + " = " + connector.aliasRight + "." + connector.keyRight + "\n";
	}
	queryStr = regex_replace(queryStr, std::regex("1=1 AND "), "");

	//build where clause
	appendQueryCond(queryStr, queryCmd.conditions);

	//finish and return
	queryStr = regex_replace(queryStr, std::regex("\\s\*\$"), "");
	queryStr += ";";

	//test purpose
	//queryStr = "SELECT stmt.line_sno AS a, reln_modify.expr \
	//	FROM reln_modify , stmt\
	//	WHERE 1=1  AND reln_modify.stmt__id = stmt._id AND stmt.type = 'assign';";

	if (!queryCmd.recursivePrefix.empty()) {
		queryStr = queryCmd.recursivePrefix + queryStr;
	}

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

//TODO: maybe vector<string> types
//
void Database::appendEntityTable(string type, string alias, queryCmd& queryCmd) {
	// type =  procedure/ stmt/ variable/ assign/ while/ if/ print/ read/ constant
	
	string tblAlias = "t" + to_string(queryCmd.tables.size() + 1);
	string tblName = "";

	if (type == "procedure") {
		tblName = "pcd";
	}
	else if (type == "variable" || type == "constant" || type == "instance") {
		tblName = "instance";
		if (type != "instance") {
			queryCmd.conditions.push_back(queryCond{ "", "", " AND " + tblAlias + ".type = '" + type + "'", true});
		}
	}
	else {
		tblName = "stmt";
		if (type != "stmt") {
			queryCmd.conditions.push_back(queryCond{ "", "", " AND " + tblAlias + ".type = '" + type + "'", true });
		}
	}
	
	queryCmd.tables.push_back(queryTable{ tblName, tblAlias , alias });
}
