#ifndef BPLUSTREE_AUTHOR_H
#define BPLUSTREE_AUTHOR_H

#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <queue>
#include <utility>
#include <unordered_map>
#include <cstring>
#include "repo.h"

using namespace std;

extern const int STRING_BPLUS_ORDER;

// 字符串B+树节点
struct StringBPlusNode {
    bool isLeaf;
    vector<string> keys;            // 键：作者名
    vector<streampos> values;       // 值：文件偏移（仅叶子节点）
    vector<StringBPlusNode*> children; // 子节点（仅内部节点）
    StringBPlusNode* next;          // 叶子链表指针
    
    StringBPlusNode(bool leaf = true);
};

// 字符串B+树类
class StringBPlusTree {
private:
    StringBPlusNode* root;
    int order;
    
    // 分裂叶子节点
    pair<string, StringBPlusNode*> splitLeaf(StringBPlusNode* node);
    
    // 分裂内部节点
    pair<string, StringBPlusNode*> splitInternal(StringBPlusNode* node);
    
    // 插入到非满节点
    void insertNonFull(StringBPlusNode* node, const string& key, streampos value);
    
    // 序列化时写入字符串（长度前缀）
    void writeString(ofstream& out, const string& str);
    
    // 从文件读取字符串
    string readString(ifstream& in);

    // 在树中查找偏移
    streampos findOffsetInTree(StringBPlusNode* node, const string& key);
public:    
    // 从CSV偏移读取并解析Author
    Author readAuthorAtOffset(streampos offset,string lib);
    
    // 解析CSV行
    Author parseAuthorLine(const string& line);
    

    StringBPlusTree(int ord = STRING_BPLUS_ORDER);

    // 流式插入接口
    void insert(const string& key, streampos value);
    
    // 序列化
    // void serialize(const string& filename);
    
    // 查询
    Author query(const string& author_Name,const string&lib);
    
    // 范围查询：查找前缀匹配的所有作者（用于模糊搜索）
    vector<pair<string, streampos>> rangeSearch(const string& prefix);
    
    // 清理
    void clear(StringBPlusNode* node = nullptr);

    // 从文件加载
    bool load(const string& path);

    StringBPlusNode* getLeft();
    
    ~StringBPlusTree();
};

#endif