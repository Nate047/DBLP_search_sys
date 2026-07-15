#include "scan.h"              // 访问全局变量：csvFile, paperIndexTree, total 等


// ========== 内部命名空间：隐藏实现细节 ==========
using namespace std;

// 解析状态结构体（仅本文件可见）
struct State {
    string currentTag;   // 当前 XML 标签名
    string buffer;       // 累积的文本内容（expat 可能分多次回调）
    Paper tem;                // 临时存储当前解析的 Paper 对象
};

// 辅助函数：检查文件是否为空
bool isFileEmpty(const string& filename) {
    ifstream f(filename, ios::ate);
    return f.tellg() == 0;
}

// 辅助函数：转义字段为 CSV 格式（处理引号和逗号）
string joinAuthors(const vector<string>& authors) {
    string result;
    for (size_t i = 0; i < authors.size(); ++i) {
        if (i > 0) result += "|";
        result += authors[i];
    }
    return result;
}

string wrapCSV(const string& field) {
    string escaped;
    escaped.reserve(field.size() + 2);
    
    for (char c : field) {
        if (c == '"') {
            escaped += "\"\"";
        } else {
            escaped += c;
        }
    }
    return '"' + escaped + '"';
}

// 辅助函数：将 Paper 写入 CSV 并更新索引
void toCSV(const Paper& x) {
    // 记录当前文件偏移（用于 B+Tree 索引）
    streampos offset = csvFile.tellp();
    string authors_str = joinAuthors(x.author);
    // 写入 CSV 数据行
    csvFile << x.id << "," 
            << wrapCSV(authors_str) << "," 
            << wrapCSV(x.title) << ","
            << wrapCSV(x.journal) << ","   
            << wrapCSV(x.volume) << "," 
            << wrapCSV(x.month) << ","
            << wrapCSV(x.year) << ","
            << wrapCSV(x.cdrom) << "," 
            << wrapCSV(x.ee) << "\n";
    
    // 写入偏移索引文件（可选，用于调试）
    idxFile << x.id << "," << offset << "\n";
    
    // 更新 Paper B+Tree 索引：id → 文件偏移
    if (paperIndexTree) {
        paperIndexTree->insert(x.id, offset);
    }

    //title倒排index，也是string存vector，结构与author一样直接复用
    if (!x.title.empty()) {
        updateTitleIndex(x.title, x.id);
    }
    // 更新作者倒排索引
    if (!x.author.empty()) {
            updateAuthorIndex(x.author, x.id);
        }
}


// ========== Expat 回调函数 ==========

void startElement(void *userData, const char *name, const char **atts) {
    // 只处理 article 内部的子标签
    State *state = static_cast<State*>(userData);
    state->currentTag = name;
    state->buffer.clear();  // 新标签开始，清空缓冲区
}

void charDataHandler(void *userData, const XML_Char *s, int len) {
    State* state = static_cast<State*>(userData);
    state->buffer.append(s, len);  // 累积文本（可能分多次回调）
}

void endElement(void *userData, const char *name) {
    State* state = static_cast<State*>(userData);
    
    // 处理 article 内部的字段结束
    if (strcmp(name, "article") != 0) {
        // 根据标签名将缓冲内容赋值给 Paper 对象
        if (strcmp(name, "author") == 0) {
            // 改为 push_back，收集所有作者
            state->tem.author.push_back(state->buffer);
        }
        else if (strcmp(name, "title") == 0) 
            state->tem.title = state->buffer;
        else if (strcmp(name, "journal") == 0) 
            state->tem.journal = state->buffer;
        else if (strcmp(name, "volume") == 0) 
            state->tem.volume = state->buffer;
        else if (strcmp(name, "month") == 0) 
            state->tem.month = state->buffer;
        else if (strcmp(name, "year") == 0) 
            state->tem.year = state->buffer;
        else if (strcmp(name, "cdrom") == 0) 
            state->tem.cdrom = state->buffer;
        else if (strcmp(name, "ee") == 0) 
            state->tem.ee = state->buffer;
        
        state->buffer.clear();
        state->currentTag.clear();
    } 
    // 处理 article 标签结束：一条记录解析完成
    else {
        int size=state->tem.author.size();
        if(size<threshold&&size!=0){
            state->tem.id = total++;  // 使用 repo.h 中的全局计数器
            toCSV(state->tem);         // 写入文件并更新索引
        }
        // 重置 Paper 对象（为下一条记录准备）
        state->tem.author.clear();
        state->tem.title.clear();
        state->tem.journal.clear();
        state->tem.volume.clear();
        state->tem.month.clear();
        state->tem.year.clear();
        state->tem.cdrom.clear();
        state->tem.ee.clear();
        state->tem.id = 0;
    }
}

// end anonymous namespace


// ========== 公共接口实现 ==========

void scan(const char* filePath) {
    // 1. 初始化全局索引结构
    paperIndexTree = new BPlusTree(BPLUS_ORDER);
    authorIndexMap.reserve(4500000);  // 预分配哈希表，减少 rehash
    initFiles();  // 初始化 CSV/IDX 文件流（来自 repo.cpp）
    
    // 2. 打开 XML 文件
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        perror("Unable to open file");
        return;
    }
    
    // 3. 创建 Expat 解析器（指定编码避免 UTF-8 问题）
    XML_Parser parser = XML_ParserCreate("ISO-8859-1");
    if (!parser) {
        cerr << "Failed to create parser\n";
        fclose(file);
        return;
    }
    
    // 4. 设置解析状态和回调
    State state;
    XML_SetUserData(parser, &state);
    XML_SetElementHandler(parser, startElement, endElement);
    XML_SetCharacterDataHandler(parser, charDataHandler);
    
    // 5. 流式解析（避免大文件一次性加载）
    char buffer[8192 * 4];  // 32KB 缓冲区
    while (!feof(file)) {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
        int is_final = feof(file) ? 1 : 0;
        
        if (XML_Parse(parser, buffer, static_cast<int>(bytes_read), is_final) == XML_STATUS_ERROR) {
            fprintf(stderr, "Parse error at line %lu: %s\n", 
                    XML_GetCurrentLineNumber(parser),
                    XML_ErrorString(XML_GetErrorCode(parser)));
            break;
        }
        
        // 实时进度反馈（\r 覆盖当前行）
        cout << "processing... " << total << " records\r";
        cout.flush();
    }

    // 6. 清理资源
    XML_ParserFree(parser);
    fclose(file);
    closeFiles();  // 关闭文件流（来自 repo.cpp）
    
    // 7. 序列化索引到磁盘
    // cout << "\nfinish recording files: " << total << endl;
    
    // cout << "Serializing paper B+tree to disk..." << endl;
    // paperIndexTree->serialize("paper_index.btree");
    cout<<endl;
    cout << "Serializing index file..." << endl;
    serializeAuthorIndex("author_index.csv");
    serializeTitleIndex("title_index.csv");
    
    cout << "All done!" << endl;
}