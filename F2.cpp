#include "F2.h"

std::priority_queue<Elem> top100;

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;

    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

void getTop() {
    std::ifstream csv(authorIndexPath);
    //直接遍历库比遍历tree要快
    std::string line;
    std::getline(csv, line);
    while (std::getline(csv, line)) {
        auto fields = parseCSVLine(line);//得到数据行
        if (fields.size() >= 3) {
            Elem elem;
            elem.name = fields[0];
            elem.num = std::stoi(fields[1]);
            //得到堆元素
            top100.push(elem);
        }
    }
    // std::cout << "Here are the top 100 authors:" << std::endl;
    vector<string>tem;//转为<string>方便接口
    for (int i = 0; i < 100; i++) {
        // std::cout << top100.top().name << '	' << top100.top().num << std::endl;
        tem.push_back(top100.top().name + " with paper num: " + std::to_string(top100.top().num));
        top100.pop();
    }
    ResultDialog *dialog = new ResultDialog(tem);
    dialog->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动释放内存
    dialog->show();
}
