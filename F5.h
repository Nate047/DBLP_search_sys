#ifndef F5_H
#define F5_H

#include "repo.h"
#include "BplusTree_author.h"
#include "BplusTree.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;

// 常量声明
extern const string output;

// 函数声明
string cleanString(const string& s);
void tran(string title, string id);
vector<string> singleLine(string& line);
void build_keyTree();
vector<Paper> key_query(vector<string> q);

// 全局指针（被外部使用，声明为 extern）
extern StringBPlusTree* key_search_tree;

#endif
