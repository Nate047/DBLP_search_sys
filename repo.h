// repo.h
#ifndef REPO_H
#define REPO_H

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include<set>
#include<filesystem>
#include "ResultDialog.h"

using namespace std;

// 前向声明（减少编译依赖）
class BPlusTree;
class StringBPlusTree;

// ========== 数据结构定义 ==========
struct Author {
    string authorName;
    vector<int> paperIds;
};

struct Paper {
    int id = 0;
    vector<string> author;
    string title;
    string journal;
    string volume;
    string month;
    string year;
    string cdrom;
    string ee;
};

// ========== 全局常量声明 ==========
extern const string repoPath;
extern const string indexPath;
// extern const string B_paper;
extern const string authorIndexPath;
// extern const string B_author;
extern const string titleIndexPath;
// ========== 全局变量声明 ==========
extern ofstream csvFile;
extern ofstream idxFile;

extern BPlusTree* paperIndexTree;
extern StringBPlusTree* authorIndexTree;
extern StringBPlusTree* titleIndexTree;

extern unordered_map<string, Author*> authorIndexMap;
extern vector<Author*> authorIndexList;
extern unordered_map<string, Author*> titleIndexMap;
extern vector<Author*> titleIndexList;

extern bool fileINIT;
extern int total;  // 移除 static，确保跨文件可见
extern int threshold;

extern const char* path;
extern const set<string> fakeWords;


// ========== 函数声明 ==========
void initFiles();
void updateAuthorIndex(const vector<string>& authorNames, int paperId);
void serializeAuthorIndex(const string& csvFilename);
void updateTitleIndex(const string& authorName, int paperId);
void serializeTitleIndex(const string& csvFilename);
void cleanupAuthorIndex();
void closeFiles();
bool existRepo();
void reset();

#endif // REPO_H
