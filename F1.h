#ifndef F1_H
#define F1_H

#include "repo.h"
#include "BplusTree.h"
#include "BplusTree_author.h"
#include <vector>
#include <string>

std::vector<std::string> Author2Papers(const std::string& authorName);
std::vector<Paper> Title2Papers(const std::string& titleName);

#endif
