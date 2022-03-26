#pragma once

#include <string>
#include <vector>
#include "sqlite3.h"
#include "Tokenizer.h"
#include "ExprTree.h"

using namespace std;

/*struct queryItem {
	string alias;
	string objType;
	string table;
	string displayFields;
	vector<queryCond> cond;
};
struct queryCmd {
	vector<queryItem> selections;
	vector<string> tables;
	vector<queryCond> conditions;
};*/

struct queryCond {
	string joinType; //and
	string key; //name
	string value; //sk or "and name = 'sk'"
	bool customText; //0 or 1
};

struct queryItem {
	string tableALias;
	string attrName;
};

struct tblConnector {
	string aliasLeft;
	string keyLeft;
	string aliasRight;
	string keyRight;
};

struct queryTable {
	string tblName;
	string tblAlias;
	string alias;
};

struct queryCmd {
	vector<queryItem> selections;
	vector<queryTable> tables;
	vector<tblConnector> connects;
	vector<queryCond> conditions;
	string recursivePrefix;
};

struct tableEdge {
	int destId;
	string joinCond;
};

struct queryPattern {
	int attrIndex;
	Node patternNode;
	bool openLeft;
	bool openRight;
};

struct queryNextCond {
	int prevStmtIndex;
	int nextStmtIndex;
};


// The Database has to be a static class due to various constraints.
// It is advisable to just add the insert / get functions based on the given examples.
class Database {
public:

	// method to connect to the database and initialize tables in the database
	static void initialize();

	// method to close the database connection
	static void close();


	/**
		inserter
	*/
	// method to insert a procedure into the database
	static void insertProcedure(string name, int line_sno, int& newID);

	//method to insert a statement into the database
	static void insertStatement(string type, int line_sno, int line_eno, int pcd__id, int& newID);

	//method to insert a instance into the database
	static void insertInstance(string type, string name, int pcd__id, int& newID);

	//method to insert a parent relationship
	static void insertParentReln(int parent__id, int child__id);

	//method to insert a modify relationship
	static void insertModifyReln(int stmt__id, int instance__id, string expression);

	//method to insert a use relationship
	static void insertUseReln(int stmt__id, int instance__id);

	//method to insert a call relationship
	static void insertCallReln(int caller__id, string callee);

	//method to insert a next relationship
	static void insertNextReln(int cur_stmt__id, int next_stmt__id);

	/**
		getter
	*/
	// method to get all the procedures from the database
	static void getProcedures(vector<string>& results);

	// method to get procedure from the database by name
	static void getProcedures(vector<string>& results, string name);

	//method to get the latest insert id
	static void getNewID(string tblName, int& newID);

	// method to get instances
	static void getInstance(vector<string>& results, vector<queryCond> queryConds, vector<int> cols);


	/**
		updater
	*/
	// method to insert a procedure into the database
	static void updateProcedure(int id, int line_eno);

	// method to update a procedure into the database
	static void updateStmt(int id, int line_eno);


	/**
		query parser
	*/
	//append conditions to query
	static void appendQueryCond(string& queryString, vector<queryCond> queryConds);

	//append conditions to query
	static void appendTables(string& queryString, vector<string> tables);

	//build a query item 
	//static void buildQueryItem(string objType, string alias, queryItem& queryItem);
	static void prepareTableGraph(vector<tableEdge> graph[]);
	static void addEdge(vector<tableEdge> graph[], int src, int desc, string joinQuery);
	static void BFSJoinTable(string& queryString, vector<string> tables);

	static void selectHandler(string str, queryCmd& queryCmd);
	static void suchThatHandler(string str, queryCmd& queryCmd, vector<queryNextCond>& nextConds);
	static void patternHandler(string str, queryCmd& queryCmd, vector<queryPattern>& patterns);

	static void queryCmdToResult(vector<vector<string>>& result, queryCmd queryCmd);
	//static queryItem getSelectionByAlias(queryCmd queryCmd, string alias);

	/**
		new query parser
	*/
	static void appendEntityTable(string type, string alias, queryCmd& queryCmd);
	static queryTable findTable(string type, string key, queryCmd const queryCmd);
	static int findTableNum(queryCmd const queryCmd);


	//find pattern string from stmt
	static string getPatternString(string str);

	//test the recursive next reln between 2 stmt ids
	static bool findNextReln(int prevID, int nextID);
	static void initRoutes();

private:
	// the connection pointer to the database
	static sqlite3* dbConnection; 
	// a vector containing the results from the database
	static vector<vector<string>> dbResults; 
	// the error message from the database
	static char* errorMessage;
	// callback method to put one row of results from the database into the dbResults vector
	// This method is called each time a row of results is returned from the database
	static int callback(void* NotUsed, int argc, char** argv, char** azColName);

};

