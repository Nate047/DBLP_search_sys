#include "F1.h"

std::vector<std::string> Author2Papers(const std::string& authorName) {
    try {
        // 查询作者对应的论文ID列表
        std::vector<int> id = authorIndexTree->query(authorName, authorIndexPath).paperIds;
        std::vector<std::string> res;
        for (int each : id) {
            try {
                // 通过ID获取论文标题
                res.push_back(paperIndexTree->query(each).title);
            } catch (const std::exception& e) {
                // 忽略无效的论文ID，可在此记录日志
                continue;
            }
        }
        return res;
    } catch (const std::exception& e) {
        // 作者不存在或索引异常，返回空列表
        return {};
    }
}

std::vector<Paper> Title2Papers(const std::string& titleName) {
    try {
        // 查询标题对应的论文ID列表
        std::vector<int> id = titleIndexTree->query(titleName, titleIndexPath).paperIds;
        std::vector<Paper> ans;
        for (int each : id) {
            try {
                // 通过ID获取完整Paper对象
                ans.push_back(paperIndexTree->query(each));
            } catch (const std::exception& e) {
                // 忽略无效ID
                continue;
            }
        }
        return ans;
    } catch (const std::exception& e) {
        // 标题不存在或索引异常，返回空列表
        return {};
    }
}
