#include "F3.h"

std::vector<std::string> teamWith(const std::string& authorName) {
    try {
        std::vector<int> id = authorIndexTree->query(authorName, authorIndexPath).paperIds;
        std::unordered_set<std::string> mate;
        for (int i : id) {
            try {
                // 获取论文的合作者列表
                Paper paper = paperIndexTree->query(i);
                for (const std::string& j : paper.author) {
                    if (j != authorName)
                        mate.insert(j);
                }
            } catch (const std::exception& e) {
                // 单篇论文查询失败（如ID无效），跳过该条记录
                continue;
            }
        }
        vector<string>res;
        for(auto& i:mate){
            res.push_back(i);
            //这里是修改接口比较方便，影响不大
        }
        return res;
    } catch (const std::exception& e) {
        // 作者查询失败，返回空列表
        return {};
    }
}
