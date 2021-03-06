#include "QueryProcessor.h"
#include "Tokenizer.h"
#include "regex"
#include "iostream"


// constructor
QueryProcessor::QueryProcessor() {}

// destructor
QueryProcessor::~QueryProcessor() {}

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
	
	string splitor;

	while (str.size()) {

		int index = str.find(tokens[0]);
		splitor = tokens[0];

		for (string token : tokens) {
			if (str.find(token) != string::npos) {
				index = (index != string::npos) ? (str.find(token) < index ? str.find(token) : index) : str.find(token);
				splitor = (index != string::npos) ? (str.find(token) < index ? token : splitor) : token;
			}
		}

		if (index != string::npos) {
			if (index != 0) result.push_back(str.substr(0, index));
			str = str.substr(index + splitor.size());
			if (str.size() == 0)result.push_back(str);
		}
		else {
			result.push_back(str);
			str = "";
		}
	}
	return result;
}

static void lowercase(string& str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
		return std::tolower(c);
	});
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


// method to evaluate a query
// This method currently only handles queries for getting all the procedure names,
// using some highly simplified logic.
// You should modify this method to complete the logic for handling all required queries.
void QueryProcessor::evaluate(string query, vector<string>& output) {
	// clear the output vector
	output.clear();

	vector<string> queries = split(query, ";");
	vector<string> tokens;
	string clean_query;
	queryCmd queryCmd{};
	vector<queryPattern> patterns;
	vector<queryNextCond> nextConds;

	//assign a1, a2; while w1, w2;
	//Select a1 pattern a1("x", _) pattern a2("x", _"x"_) such that Next* (a1, a2)
	//such that Parent* (w2, a2) such that Parent* (w1, w2)

	std::regex expr_select("\^\\s\*select\.\*\$", std::regex_constants::icase);
	std::regex expr_declare("\^\\s\*\(procedure\|variable\|constant\|stmt\|while\|assign\|read\|print\|if\|call\)\\s\*\[a-z0-9\\s\,]\+\$", std::regex_constants::icase);

	for (string query : queries) {

		//init
		//lowercase(query);
		string query_lower = query;
		transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
		if (query == "") continue;
		//TODO: x and X cannnot be differentiated. fix it later

		clean_query = split(query, ";").at(0);
		tokens = split(clean_query, " ");

		if (regex_match(query_lower, expr_declare)) {
			cout << "find a declare query: " << query << endl;
			
			string type = tokens[0];
			tokens.erase(tokens.begin());
			Database::appendEntityTable(type, tokens, queryCmd);
		}

		else if (regex_match(query_lower, expr_select)) {
			cout << "find a select query: " << query << endl;

			//parse the query into groups
			string select_query = query;
			select_query = regex_replace(select_query, std::regex("\\s\*Select\\s\*"), std::string("/pick/"));
			select_query = regex_replace(select_query, std::regex("\\s\*such that\\s\*"), std::string("/cond/"));
			select_query = regex_replace(select_query, std::regex("\\s\*pattern\\s\*"), std::string("/pattern/"));
			vector<string> parts = split(select_query, std::string("/"));

			//for (string test : parts) {
			//	cout << "found: " << test << endl;
			//}

			for (int i = 0; i < parts.size(); i += 2) {
				if (parts[i] == "pick") {
					Database::selectHandler(parts[i + 1], queryCmd);
				}
				else if (parts[i] == "cond") {
					Database::suchThatHandler(parts[i + 1], queryCmd, nextConds);
				}
				else if (parts[i] == "pattern") {
					Database::patternHandler(parts[i + 1], queryCmd, patterns);
				}
			}
		}
	}

	vector<vector<string>> databaseResults;
	Database::queryCmdToResult(databaseResults, queryCmd);

	// post process the results to fill in the output vector
	for (vector<string> row : databaseResults) {

		string attr;
    
    //check patterns
		if (!patterns.empty()) {
			Node attr_node;
			for (queryPattern p : patterns) {
				string stmt_pattern = row.at(p.attrIndex);
				if (stmt_pattern=="") goto nxt;

				ExprTree::exprStrToNode(stmt_pattern, attr_node);
				bool has_subtree = ExprTree::HasSubtree(&attr_node, &p.patternNode);
				bool is_left_matched = p.openLeft || ExprTree::testLeft(&attr_node, &p.patternNode);
				bool is_right_matched = p.openRight || ExprTree::testRight(&attr_node, &p.patternNode);
				bool is_pattern_matched = has_subtree && is_left_matched && is_right_matched;

				if (!is_pattern_matched) goto nxt;
				row.erase(row.begin() + p.attrIndex);
			}
		}

		//check next relations
		if (!nextConds.empty()) {
			for (queryNextCond n : nextConds) {
				int prevStmtID = stoi(row.at(n.prevStmtIndex));
				int nextStmtID = stoi(row.at(n.nextStmtIndex));
				bool hasNext = Database::findNextReln(prevStmtID, nextStmtID);

				if (!hasNext) goto nxt;
				row.erase(row.begin() + n.nextStmtIndex);
				row.erase(row.begin() + n.prevStmtIndex);
			}
		}
    

		for (string s : row) {
			attr += s + " ";
		}
		output.push_back(attr.substr(0, attr.size() -1));

		nxt:;
	}

	//if (output.empty()) output.push_back("0");

	//TODO: output now can only display results from 1 row. need to expand to multu rows: <1,2,3>, <4,5,6>

}
