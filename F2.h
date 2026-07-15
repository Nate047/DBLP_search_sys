#ifndef F2_H
#define F2_H

#include "repo.h"
#include <queue>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

struct Elem {
    std::string name;
    int num;
    bool operator<(const Elem& other) const {
        return num < other.num;
    }
};
//方便建立小根堆
extern std::priority_queue<Elem> top100;

std::vector<std::string> split(const std::string& s, char delimiter);
std::vector<std::string> parseCSVLine(const std::string& line);
void getTop();

#endif
