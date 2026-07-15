// repo.cpp
#include "repo.h"
#include "BplusTree_author.h"  // 提供 StringBPlusTree 和 STRING_BPLUS_ORDER
#include "BplusTree.h"
#include <iostream>

using namespace std;
// ========== 全局常量定义 ==========
const string repoPath = "./paperData.csv";
const string indexPath = "./paperData_index.idx";
// const string B_paper = "./paper_index.btree";
const string authorIndexPath = "./author_index.csv";
// const string B_author = "author_index.btree";
const string titleIndexPath="./title_index.csv";

// ========== 全局变量定义 ==========
ofstream csvFile;
ofstream idxFile;

BPlusTree* paperIndexTree = nullptr;
StringBPlusTree* authorIndexTree = nullptr;
StringBPlusTree* titleIndexTree = nullptr;

unordered_map<string, Author*> authorIndexMap;
unordered_map<string, Author*> titleIndexMap;
vector<Author*> authorIndexList;
vector<Author*> titleIndexList;

bool fileINIT = false;
int total = 0;
int threshold=500;

const char* path = "./dblp.xml";

const set<string> fakeWords = {
    "a", "an", "the", "in", "on", "at", "to", "of", "and", "or", 
    "for", "with", "by", "from", "as", "is", "are", "was", "were", 
    "be", "been", "being", "have", "has", "had", "do", "does", "did",
    "will", "would", "could", "should", "may", "might", "can", "shall",
    "not", "it", "its", "this", "that", "these", "those", "we", "our",
    "their", "there", "they", "them", "than", "more", "such", "used",
    "using", "based", "new", "approach", "algorithm", "algorithms",
    "method", "methods", "system", "systems", "model", "models",
    "data", "analysis", "paper", "problem", "proposed", "results",
    "study", "technique", "techniques", "application", "applications","via",
    "you","we're","just","prefrence","s","-","r","some","i","theory","der","d","problems","problem",
    "des","de","la","et","l","dans","en","les","du"
};


// ========== 函数实现 ==========

void initFiles() {
    if (fileINIT) return;
    
    csvFile.open(repoPath, ios::app | ios::binary);
    idxFile.open(indexPath, ios::app | ios::binary);

    // 新文件写入表头
    if (csvFile.tellp() == 0) {
        csvFile << "index,author,title,journal,volume,month,year,cdrom,ee\n";
    }
    
    fileINIT = true;
}

void updateAuthorIndex(const vector<string>& authorNames, int paperId) {
    for(string authorName:authorNames){
        auto it = authorIndexMap.find(authorName);
        
        if (it == authorIndexMap.end()) {
            // 新作者
            Author* entry = new Author();
            entry->authorName = authorName;
            entry->paperIds.push_back(paperId);
            
            authorIndexMap[authorName] = entry;
            authorIndexList.push_back(entry);
        } else {
            // 已有作者，追加论文ID
           Author* entry = it->second;
            auto& ids = entry->paperIds;
            
            // 使用find检查paperId是否已存在
            if (std::find(ids.begin(), ids.end(), paperId) == ids.end()) {
                // 不存在才添加
                ids.push_back(paperId);
            }
            //重复id
        }
    }
}
void updateTitleIndex(const string& titleName, int id) {
    auto it = titleIndexMap.find(titleName);
    
    if (it == titleIndexMap.end()) {
        // 新作者
        Author* entry = new Author();
        entry->authorName = titleName;
        entry->paperIds.push_back(id);
        
        titleIndexMap[titleName] = entry;
        titleIndexList.push_back(entry);
    } else {
        // 已有作者，追加论文ID
        it->second->paperIds.push_back(id);
    }
}
void serializeAuthorIndex(const string& csvFilename) {
    ofstream csvOut(csvFilename);
    if (!csvOut) {
        cerr << "Failed to open " << csvFilename << endl;
        return;
    }
    
    csvOut << "author_name,paper_count,paper_ids\n";
    
    // 初始化 B+ 树（确保 STRING_BPLUS_ORDER 已定义）
    authorIndexTree = new StringBPlusTree(STRING_BPLUS_ORDER);
    
    for (const auto* entry : authorIndexList) {
        streampos offset = csvOut.tellp();
        
        // 写入 CSV 行（注意转义引号）
        csvOut << "\"" << entry->authorName << "\",";
        csvOut << entry->paperIds.size() << ",\"";
        for (size_t i = 0; i < entry->paperIds.size(); i++) {
            if (i > 0) csvOut << "|";
            csvOut << entry->paperIds[i];
        }
        csvOut << "\"\n";
        
        // 插入 B+ 树索引
        authorIndexTree->insert(entry->authorName, offset);

    }
    
    csvOut.close();
    
    // // 序列化 B+ 树到文件
    // cout << "Serializing author B+tree..." << endl;
    // authorIndexTree->serialize(btreeFilename);
    // cout << "Author index: " << authorIndexList.size() << " authors" << endl;
}
void serializeTitleIndex(const string& csvFilename) {
    ofstream csvOut(csvFilename);
    if (!csvOut) {
        cerr << "Failed to open " << csvFilename << endl;
        return;
    }
    
    csvOut << "title,paper_count,paper_ids\n";
    
    // 初始化 B+ 树（确保 STRING_BPLUS_ORDER 已定义）
    titleIndexTree = new StringBPlusTree(STRING_BPLUS_ORDER);
    
    for (const auto* entry : titleIndexList) {
        streampos offset = csvOut.tellp();
        
        // 写入 CSV 行（注意转义引号）
        csvOut << "\"" << entry->authorName << "\",";
        csvOut << entry->paperIds.size() << ",\"";  // 添加count
        for (size_t i = 0; i < entry->paperIds.size(); i++) {
            if (i > 0) csvOut << "|";
            csvOut << entry->paperIds[i];
        }
        csvOut << "\"\n";
        
        // 插入 B+ 树索引
        titleIndexTree->insert(entry->authorName, offset);
    }
    
    csvOut.close();
    
    // // 序列化 B+ 树到文件
    // cout << "Serializing author B+tree..." << endl;
    // authorIndexTree->serialize(btreeFilename);
    // cout << "Author index: " << authorIndexList.size() << " authors" << endl;
}

void cleanupAuthorIndex() {
    // 释放 Author 对象内存
    for (auto* entry : authorIndexList) {
        delete entry;
    }
    authorIndexList.clear();
    authorIndexMap.clear();
    
    // 释放 B+ 树
    if (authorIndexTree) {
        delete authorIndexTree;
        authorIndexTree = nullptr;
    }
}

void closeFiles() {
    if (csvFile.is_open()) csvFile.close();
    if (idxFile.is_open()) idxFile.close();
    fileINIT = false;
}

bool existRepo(){
    if( filesystem::exists(repoPath)
        && filesystem::exists(indexPath)
        && filesystem::exists(authorIndexPath)
        &&filesystem::exists(titleIndexPath)
    )
        return true;
    return false;
}
void reset(){
    if(filesystem::exists(repoPath))
        filesystem::remove(repoPath);
    if(filesystem::exists(indexPath))
        filesystem::remove(indexPath);
    if(filesystem::exists(authorIndexPath))
        filesystem::remove(authorIndexPath);
    if(filesystem::exists(titleIndexPath))
        filesystem::remove(titleIndexPath);

}
