#ifndef BPLUSTREE_H
#define BPLUSTREE_H

#include <vector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include<sstream>
#include <string>
#include <queue>
#include <utility>
#include <unordered_map>
#include <cstring>
#include "repo.h"

using namespace std;

extern const int BPLUS_ORDER;

struct BPlusNode {
    bool isLeaf;
    vector<int> keys;           // 键：id
    vector<streampos> values;   // 值：文件偏移（仅叶子节点有效）
    vector<BPlusNode*> children; // 子节点指针（仅内部节点）
    BPlusNode* next;            // 叶子节点链表指针
    
    BPlusNode(bool leaf = true);
};

class BPlusTree {
private:
    BPlusNode* root;
    int order;
    
    // 分裂叶子节点
    pair<int, BPlusNode*> splitLeaf(BPlusNode* node);
    
    // 分裂内部节点
    pair<int, BPlusNode*> splitInternal(BPlusNode* node);
    
    // 插入到非满节点
    void insertNonFull(BPlusNode* node, int key, streampos value);
    
    // 在树中查找偏移
    streampos findOffsetInTree(BPlusNode* node, int key);   
    
public:
     // 从偏移读取Paper
    Paper readPaperAtOffset(streampos offset);
    
    // 解析CSV行
    Paper parseLine(const string& line);

    BPlusTree(int ord = BPLUS_ORDER);
    
    // 流式插入接口
    void insert(int key, streampos value);
    
    // 查询
    Paper query(int index);
    
    // 清理内存
    void clear(BPlusNode* node = nullptr);
    
    // 从文件加载
    bool load();

    ~BPlusTree();
};

#endif
