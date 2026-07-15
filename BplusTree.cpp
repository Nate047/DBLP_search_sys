#include "BplusTree.h"

const int BPLUS_ORDER = 128;

// ========== BPlusNode 实现 ==========

BPlusNode::BPlusNode(bool leaf) : isLeaf(leaf), next(nullptr) {}

// ========== BPlusTree 私有方法实现 ==========

pair<int, BPlusNode*> BPlusTree::splitLeaf(BPlusNode* node) {
    BPlusNode* newNode = new BPlusNode(true);
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
    
    return make_pair(newNode->keys[0], newNode);
}

pair<int, BPlusNode*> BPlusTree::splitInternal(BPlusNode* node) {
    BPlusNode* newNode = new BPlusNode(false);
    int mid = order / 2;
    
    int promotedKey = node->keys[mid];
    
    // 后半部分键和子节点移到新节点
    newNode->keys.assign(node->keys.begin() + mid + 1, node->keys.end());
    newNode->children.assign(node->children.begin() + mid + 1, node->children.end());
    
    // 调整原节点
    node->keys.resize(mid);
    node->children.resize(mid + 1);
    
    return make_pair(promotedKey, newNode);
}

void BPlusTree::insertNonFull(BPlusNode* node, int key, streampos value) {
    if (node->isLeaf) {
        // 叶子节点：找到插入位置
        auto it = lower_bound(node->keys.begin(), node->keys.end(), key);
        int pos = it - node->keys.begin();
        node->keys.insert(it, key);
        node->values.insert(node->values.begin() + pos, value);
    } else {
        // 内部节点：找到合适的子节点
        int i = node->keys.size() - 1;
        while (i >= 0 && key < node->keys[i]) i--;
        i++;
        
        BPlusNode* child = node->children[i];
        if (child->keys.size() == order - 1) {
            // 子节点满，先分裂
            pair<int, BPlusNode*> splitResult;
            if (child->isLeaf) {
                splitResult = splitLeaf(child);
            } else {
                splitResult = splitInternal(child);
            }
            int promotedKey = splitResult.first;
            BPlusNode* newNode = splitResult.second;
            
            node->keys.insert(node->keys.begin() + i, promotedKey);
            node->children.insert(node->children.begin() + i + 1, newNode);
            
            // 决定插入哪个子节点
            if (child->isLeaf) {
                if (key >= promotedKey) i++;
            } else {
                if (key > promotedKey) i++;
            }
        }
        insertNonFull(node->children[i], key, value);
    }
}

streampos BPlusTree::findOffsetInTree(BPlusNode* node, int key) {
    // 内部节点：递归到子节点
    if (!node->isLeaf) {
        int i = 0;
        while (i < node->keys.size() && key >= node->keys[i]) i++;
        if (i >= node->children.size()) return -1;
        return findOffsetInTree(node->children[i], key);
    }
    // 叶子节点：直接查找
    else {
        auto it = lower_bound(node->keys.begin(), node->keys.end(), key);
        if (it != node->keys.end() && *it == key) {
            int idx = it - node->keys.begin();
            return node->values[idx];
        }
        return -1;
    }
}

Paper BPlusTree::readPaperAtOffset(streampos offset) {
    ifstream csv(repoPath,ios::binary);
    csv.seekg(offset);
    string line;
    getline(csv, line);
    return parseLine(line);
}

Paper BPlusTree::parseLine(const string& line) {
    Paper p;
    vector<string> f;
    string cur;
    bool inQ = false;
    
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == '"') {
            inQ = !inQ;
        } else if (c == ',' && !inQ) {
            f.push_back(cur); cur.clear();
        } else {
            cur += c;
        }
    }
    f.push_back(cur);
    
    p.id = stoi(f[0]);
    string authors_str = f[1];
    // 去除首尾的引号（如果有）
    if (authors_str.size() >= 2 && authors_str.front() == '"' && authors_str.back() == '"') {
        authors_str = authors_str.substr(1, authors_str.size() - 2);
    }
    
    // 按 | 分割
    stringstream ss(authors_str);
    string author;
    while (getline(ss, author, '|')) {
        if (!author.empty()) {
            p.author.push_back(author);
        }
    }
    
    p.title = f[2];
    p.journal = f[3];
    p.volume = f[4];
    p.month = f[5];
    p.year = f[6];
    p.cdrom = f[7];
    p.ee = f[8];
    return p;
}

// ========== BPlusTree 公有方法实现 ==========

BPlusTree::BPlusTree(int ord) : order(ord) {
    root = new BPlusNode(true);
}

void BPlusTree::insert(int key, streampos value) {
    if (root->keys.size() == order - 1) {
        // 根节点满，分裂
        BPlusNode* newRoot = new BPlusNode(false);
        newRoot->children.push_back(root);
        
        pair<int, BPlusNode*> splitResult;
        if (root->isLeaf) {
            splitResult = splitLeaf(root);
        } else {
            splitResult = splitInternal(root);
        }
        int promotedKey = splitResult.first;
        BPlusNode* newNode = splitResult.second;
        
        newRoot->keys.push_back(promotedKey);
        newRoot->children.push_back(newNode);
        root = newRoot;
    }
    insertNonFull(root, key, value);
}


Paper BPlusTree::query(int index) {
    
    // 从root遍历查找
    streampos offset = findOffsetInTree(root, index);
    if (offset == -1) {
        throw runtime_error("Paper ID not found: " + to_string(index));
    }
    
    // 读取CSV
    return readPaperAtOffset(offset);
}

void BPlusTree::clear(BPlusNode* node) {
    if (!node) node = root;
    if (!node->isLeaf) {
        for (auto* child : node->children) {
            clear(child);
        }
    }
    delete node;
}
bool BPlusTree::load() {
    ifstream idx(indexPath);
    if (!idx) {
        cerr << "Failed to open index file: " << indexPath << endl;
        return false;
    }
    
    string line;
    int count = 0;
    
    // 逐行读取并插入B+树
    while (getline(idx, line)) {
        if (line.empty()) continue;
        
        // 解析 id,offset
        size_t commaPos = line.find(',');
        if (commaPos == string::npos) continue;
        
        int id = stoi(line.substr(0, commaPos));
        long long offset = stoll(line.substr(commaPos + 1));
        
        // 插入B+树
        insert(id, static_cast<streampos>(offset));
        count++;
    }
    
    idx.close();
    cout << "B+tree loaded: " << count << " entries" << endl;
    return true;
}

BPlusTree::~BPlusTree() {
    clear();
}
