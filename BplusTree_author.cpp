#include "BplusTree_author.h"

const int STRING_BPLUS_ORDER = 64;

// ========== StringBPlusNode 实现 ==========

StringBPlusNode::StringBPlusNode(bool leaf) : isLeaf(leaf), next(nullptr) {}

// ========== StringBPlusTree 私有方法实现 ==========

pair<string, StringBPlusNode*> StringBPlusTree::splitLeaf(StringBPlusNode* node) {
    StringBPlusNode* newNode = new StringBPlusNode(true);
    int mid = order / 2;
    
    // 后半部分移到新节点
    newNode->keys.assign(node->keys.begin() + mid, node->keys.end());
    newNode->values.assign(node->values.begin() + mid, node->values.end());
    
    // 调整原节点
    node->keys.resize(mid);
    node->values.resize(mid);
    
    // 维护链表
    newNode->next = node->next;
    node->next = newNode;
    
    // 返回：提升的键（新节点的第一个键）+ 新节点
    return make_pair(newNode->keys[0], newNode);
}

pair<string, StringBPlusNode*> StringBPlusTree::splitInternal(StringBPlusNode* node) {
    StringBPlusNode* newNode = new StringBPlusNode(false);
    int mid = order / 2;
    
    string promotedKey = node->keys[mid];  // 中间键提升
    
    // 后半部分键和子节点移到新节点
    newNode->keys.assign(node->keys.begin() + mid + 1, node->keys.end());
    newNode->children.assign(node->children.begin() + mid + 1, node->children.end());
    
    // 调整原节点
    node->keys.resize(mid);
    node->children.resize(mid + 1);
    
    return make_pair(promotedKey, newNode);
}

void StringBPlusTree::insertNonFull(StringBPlusNode* node, const string& key, streampos value) {
    if (node->isLeaf) {
        // 叶子节点：找到插入位置（字符串二分查找）
        auto it = lower_bound(node->keys.begin(), node->keys.end(), key);
        int pos = it - node->keys.begin();
        node->keys.insert(it, key);
        node->values.insert(node->values.begin() + pos, value);
    } else {
        // 内部节点：找到合适的子节点
        int i = node->keys.size() - 1;
        while (i >= 0 && key < node->keys[i]) i--;
        i++;
        
        StringBPlusNode* child = node->children[i];
        if (child->keys.size() == order - 1) {
            // 子节点满，先分裂
            pair<string, StringBPlusNode*> splitResult;
            if (child->isLeaf) {
                splitResult = splitLeaf(child);
            } else {
                splitResult = splitInternal(child);
            }
            string promotedKey = splitResult.first;
            StringBPlusNode* newNode = splitResult.second;
            
            node->keys.insert(node->keys.begin() + i, promotedKey);
            node->children.insert(node->children.begin() + i + 1, newNode);
            
            // 决定插入哪个子节点
            if (key > promotedKey) i++;
        }
        insertNonFull(node->children[i], key, value);
    }
}

void StringBPlusTree::writeString(ofstream& out, const string& str) {
    int len = str.length();
    out.write(reinterpret_cast<const char*>(&len), sizeof(int));
    out.write(str.c_str(), len);
}

string StringBPlusTree::readString(ifstream& in) {
    int len;
    in.read(reinterpret_cast<char*>(&len), sizeof(int));
    string str(len, '\0');
    in.read(&str[0], len);
    return str;
}

streampos StringBPlusTree::findOffsetInTree(StringBPlusNode* node, const string& key) {
    if (!node->isLeaf) {
        // 内部节点：选择子节点
        int i = 0;
        while (i < node->keys.size() && key >= node->keys[i]) i++;
        if (i >= node->children.size()) return -1;
        return findOffsetInTree(node->children[i], key);
    } else {
        // 叶子节点：二分查找
        auto it = lower_bound(node->keys.begin(), node->keys.end(), key);
        if (it != node->keys.end() && *it == key) {
            int idx = it - node->keys.begin();
            return node->values[idx];
        }
        return -1;
    }
}

Author StringBPlusTree::readAuthorAtOffset(streampos offset,string lib) {
    ifstream csv(lib);
    csv.seekg(offset);
    
    string line;
    getline(csv, line);
    
    return parseAuthorLine(line);
}

Author StringBPlusTree::parseAuthorLine(const string& line) {
    Author a;
    vector<string> f;
    string cur;
    bool inQ = false;
    
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == '"') {
            inQ = !inQ;
        } else if (c == ',' && !inQ) {
            f.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    f.push_back(cur);
    
    // 解析：f[0]=作者名, f[1]=数量, f[2]=id列表
    if (f.size() >= 3) {
        a.authorName = f[0];
        // 去除引号
        if (a.authorName.size() >= 2 && a.authorName.front() == '"' && a.authorName.back() == '"') {
            a.authorName = a.authorName.substr(1, a.authorName.size() - 2);
        }
        
        // 解析id列表 "1|2|3"
        string idsStr = f[2];
        if (idsStr.size() >= 2 && idsStr.front() == '"' && idsStr.back() == '"') {
            idsStr = idsStr.substr(1, idsStr.size() - 2);
        }
        
        stringstream ss(idsStr);
        string id;
        while (getline(ss, id, '|')) {
            if (!id.empty()) {
                a.paperIds.push_back(stoi(id));
            }
        }
    }
    return a;
}

// ========== StringBPlusTree 公有方法实现 ==========

StringBPlusTree::StringBPlusTree(int ord) : order(ord) {
    root = new StringBPlusNode(true);
}

void StringBPlusTree::insert(const string& key, streampos value) {
    if (root->keys.size() == order - 1) {
        // 根节点满，分裂
        StringBPlusNode* newRoot = new StringBPlusNode(false);
        newRoot->children.push_back(root);
        
        pair<string, StringBPlusNode*> splitResult;
        if (root->isLeaf) {
            splitResult = splitLeaf(root);
        } else {
            splitResult = splitInternal(root);
        }
        string promotedKey = splitResult.first;
        StringBPlusNode* newNode = splitResult.second;
        
        newRoot->keys.push_back(promotedKey);
        newRoot->children.push_back(newNode);
        root = newRoot;
    }
    insertNonFull(root, key, value);
}

Author StringBPlusTree::query(const string& author_Name,const string&lib) {
    streampos offset = findOffsetInTree(root, author_Name);
    if (offset == -1) {
        throw runtime_error("Author not found: " + author_Name);

    }
    
    return readAuthorAtOffset(offset,lib);
}

vector<pair<string, streampos>> StringBPlusTree::rangeSearch(const string& prefix) {
    vector<pair<string, streampos>> result;
    
    // 找到起始叶子节点
    StringBPlusNode* node = root;
    while (!node->isLeaf) {
        int i = 0;
        while (i < node->keys.size() && prefix > node->keys[i]) i++;
        node = node->children[i];
    }
    
    // 遍历链表收集匹配项
    while (node) {
        for (size_t i = 0; i < node->keys.size(); i++) {
            const string& key = node->keys[i];
            if (key.substr(0, prefix.length()) == prefix) {
                result.push_back(make_pair(key, node->values[i]));
            } else if (key > prefix) {
                return result;
            }
        }
        node = node->next;
    }
    return result;
}

void StringBPlusTree::clear(StringBPlusNode* node) {
    if (!node) node = root;
    if (!node->isLeaf) {
        for (auto* child : node->children) {
            clear(child);
        }
    }
    delete node;
}
bool StringBPlusTree::load(const string &path) {
    ifstream in(path);
    if (!in) {
        cerr << "Failed to open " << path << endl;
        return false;
    }
    //修改，原本的逐条插入太慢了，这里采用批量加载bulk load
    vector<pair<string, streampos>> entries;
    string line;
    streampos lineOffset = 0;   // 当前行在文件中的起始偏移

    // ---------- 第一阶段：收集所有 (key, offset) ----------
    while (getline(in, line)) {
        if (line.empty()) {
            lineOffset = in.tellg();   // 跳过空行，记录下一行偏移
            continue;
        }

        // 解析 CSV 中的引号字段（格式如: "author_name", ...）
        size_t firstQuote = line.find('"');
        size_t secondQuote = line.find('"', firstQuote + 1);
        if (firstQuote == string::npos || secondQuote == string::npos) {
            lineOffset = in.tellg();
            continue;   // 格式错误，跳过
        }

        string author = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
        entries.emplace_back(std::move(author), lineOffset);
        lineOffset = in.tellg();   // 记录下一行偏移
    }
    in.close();

    // ---------- 第二阶段：按键排序（string 字典序） ----------
    sort(entries.begin(), entries.end(),
         [](const auto &a, const auto &b) { return a.first < b.first; });

    // ---------- 第三阶段：清空旧树，顺序插入 ----------
    clear();                         // 递归释放所有节点（需先实现）
    root = new StringBPlusNode(true); // 创建新的空树

    int count = 0;
    for (auto &[key, offset] : entries) {
        insert(key, offset);         // 此时插入顺序已排序，性能最优
        ++count;
    }

    cout << "String B+tree loaded : " << count << " entries" << endl;
    return true;
}
StringBPlusTree::~StringBPlusTree() {
    clear();
}

StringBPlusNode* StringBPlusTree::getLeft(){
    StringBPlusNode* tem=root;
    while(!tem->isLeaf){
        tem=tem->children[0];
    }
    return tem;

}
