#ifndef F4_H
#define F4_H

#include "repo.h"
#include "BplusTree.h"
#include "BplusTree_author.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <algorithm>


void recordWords(std::unordered_map<std::string, int>& yearMap, const std::string& title);
std::vector<std::string> computeTop10(const std::unordered_map<std::string, int>& freqMap);
std::map<std::string, std::vector<std::string>> parseTop10();

#endif
