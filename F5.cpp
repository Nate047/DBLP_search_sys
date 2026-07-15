#include "F5.h"
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

// 常量定义
const string output = "./key_search.csv";

// 全局变量定义（仅本文件内部使用）
unordered_map<string, string> mp;
unordered_set<string> processedInThisTitle;

// 全局指针定义
StringBPlusTree* key_search_tree = nullptr;

// ========== 函数实现 ==========

string cleanString(const string& s) {
    string res;
    res.reserve(s.size());
    for (char c : s) {
        if (c != '\n' && c != '\r' && c != '"') {
            res += c;
        }
    }
    return res;
}

void tran(string title, string id) {
    id = cleanString(id);
    if (id.empty()) return;

    string tem;

    for (char c : title) {
        char lower_c = tolower(c);
        if (lower_c == ' ' || lower_c == ',' || lower_c == '.' ||
            lower_c == ':' || lower_c == ';' || lower_c == '!' ||
            lower_c == '?' || lower_c == '(' || lower_c == ')' ||
            lower_c == '"') {
            if (!tem.empty() && fakeWords.find(tem) == fakeWords.end()) {
                if (processedInThisTitle.insert(tem).second) {
                    if (!mp[tem].empty()) mp[tem] += "|";
                    mp[tem] += id;
                }
            }
            tem.clear();
        } else {
            tem += lower_c;
        }
    }
    if (!tem.empty() && fakeWords.find(tem) == fakeWords.end()) {
        if (processedInThisTitle.insert(tem).second) {
            if (!mp[tem].empty()) mp[tem] += "|";
            mp[tem] += id;
        }
    }
    processedInThisTitle.clear();
}

vector<string> singleLine(string& line) {
    vector<string> f;
    size_t c1 = line.rfind(',');
    size_t c2 = line.rfind(',', c1 - 1);

    f.push_back(line.substr(0, c2));               // title
    f.push_back(line.substr(c2 + 1, c1 - c2 - 1));  // noUse
    f.push_back(line.substr(c1 + 1));               // id

    if (f[0].size() >= 2 && f[0].front() == '"' && f[0].back() == '"')
        f[0] = f[0].substr(1, f[0].size() - 2);
    if (f[2].size() >= 2 && f[2].front() == '"' && f[2].back() == '"')
        f[2] = f[2].substr(1, f[2].size() - 2);

    return f;
}

void build_keyTree() {
    if (key_search_tree != nullptr) return;

    key_search_tree = new StringBPlusTree();

    if (fs::exists(output)) {
        if (key_search_tree->load(output)) {
            cout << "load key tree success." << endl;
            return;
        } else {
            cerr << "Failed to load existing index, rebuilding..." << endl;
            fs::remove(output);
        }
    }

    if (!titleIndexTree) {
        cerr << "ERROR: titleIndexTree is null, cannot build keyword tree." << endl;
        return;
    }

    StringBPlusNode* left = titleIndexTree->getLeft();
    if (!left) {
        cerr << "ERROR: titleIndexTree is empty." << endl;
        return;
    }

    ifstream csv(titleIndexPath, ios::binary);
    if (!csv) {
        cerr << "ERROR: Cannot open title index file: " << titleIndexPath << endl;
        return;
    }

    cout << "BUILDING KEY_WORD_SEARCH_TREE..." << endl;
    ofstream out(output);
    out << "key_word,noUse,paper_id\n";

    while (left != nullptr) {
        for (streampos s : left->values) {
            csv.seekg(s);
            string line;
            getline(csv, line);

            if (line.empty() || line.find_first_not_of(" \t\r\n") == string::npos)
                continue;

            vector<string> fields = singleLine(line);
            if (fields.size() < 3) {
                cerr << "fail parsing line: " << line.substr(0, 200) << "..." << endl;
                continue;
            }

            string title = fields[0];
            string paper_ids = fields[2];

            stringstream ss(paper_ids);
            string pid;
            while (getline(ss, pid, '|')) {
                if (!pid.empty()) {
                    tran(title, pid);
                }
            }
        }
        left = left->next;
    }

    for (const auto& pair : mp) {
        streampos pos = out.tellp();
        out << "\"" << pair.first << "\",1,\"" << pair.second << "\"\n";
        key_search_tree->insert(pair.first, pos);
    }

    out.close();
    csv.close();
    mp.clear();
    cout << "done" << endl;
}

vector<Paper> key_query(vector<string> q) {
    if (q.empty()) return {};
    if (key_search_tree == nullptr) {
        build_keyTree();
        if (key_search_tree == nullptr) return {};
    }

    vector<vector<int>> all;
    for (string e : q) {
        for (char& c : e) c = tolower(c);
        if (fakeWords.find(e) != fakeWords.end()) continue;
        try {
            vector<int> ids = key_search_tree->query(e, output).paperIds;
            if (!ids.empty()) all.push_back(move(ids));
        } catch (const exception& ex) {
            continue;
        }
    }

    if (all.empty()) return {};

    vector<int> valid_ids = all[0];
    for (size_t i = 1; i < all.size(); ++i) {
        unordered_set<int> set2(all[i].begin(), all[i].end());
        vector<int> new_result;
        for (int id : valid_ids) {
            if (set2.count(id)) new_result.push_back(id);
        }
        valid_ids = move(new_result);
        if (valid_ids.empty()) break;
    }

    vector<Paper> res;
    for (int id : valid_ids) {
        try {
            Paper paper = paperIndexTree->query(id);
            res.push_back(move(paper));
        } catch (const exception& ex) {
            continue;
        }
    }
    return res;
}
