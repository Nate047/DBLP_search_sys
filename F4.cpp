#include "F4.h"

void recordWords(std::unordered_map<std::string, int>& yearMap, const std::string& title) {
    if (title.empty()) return;

    std::string cur = "";
    for (unsigned char c : title) {
        if (isalpha(c) || c == '-') { //保留字段中间的连字符，防止deep-learning这样的词语被拆分
            cur += tolower(c);
        } else if (!cur.empty()) {
            if (fakeWords.find(cur) == fakeWords.end()) { //过滤虚词
                yearMap[cur]++;
            }
            cur.clear();
        }
    }

    if (!cur.empty() && fakeWords.find(cur) == fakeWords.end()) {
        yearMap[cur]++;
    }
}

std::vector<std::string> computeTop10(const std::unordered_map<std::string, int>& freqMap) {
    if (freqMap.empty()) return {};
    //对词频表排序得到结果
    std::vector<std::pair<std::string, int>> vec(freqMap.begin(), freqMap.end());

    std::sort(vec.begin(), vec.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });
    //sort内置快速排序

    std::vector<std::string> result;
    for (size_t i = 0; i < 10; i++) {
        result.push_back(vec[i].first);
    }
    return result;
}

std::map<std::string, std::vector<std::string>> parseTop10() {
    std::ifstream csv(repoPath, std::ios::binary);
    if (!csv.is_open()) {
        std::cerr << "Error: Cannot open " << repoPath << std::endl;
        return {};
    }
    //直接遍历库比遍历tree要快
    std::string line;
    std::getline(csv, line);

    std::map<std::string, std::unordered_map<std::string, int>> wordFreq;

    std::string year, title;

    while (std::getline(csv, line)) {
        auto tem_paper = paperIndexTree->parseLine(line);
        year = tem_paper.year;
        title = tem_paper.title;
        recordWords(wordFreq[year], title);
    }
    csv.close();

    std::map<std::string, std::vector<std::string>> result; //<year,<top10 words>>
    for (auto& entry : wordFreq) {
        result[entry.first] = computeTop10(entry.second);
    }

    return result;
}
