#ifndef F6_H
#define F6_H

#include "repo.h"
#include "BplusTree.h"
#include "BplusTree_author.h"
#include "F3.h"


#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <queue>
#include <chrono>
#include <numeric>
#include <functional>
#include <cmath>

#include<QToolTip>

using namespace std;
using namespace std::chrono;

// ============================================================
// 全局变量（extern 声明）
// ============================================================

extern unordered_map<string, int> author_to_id;
extern vector<string> id_to_author;
extern int total_author;

extern vector<vector<int>> coauthors;

struct superNode {
    bool drop = false;
    vector<int> authorId;
    vector<int> core;
};

extern vector<superNode> node;
extern unordered_set<string> seen_maximal;

// ============================================================
// 安全整数运算
// ============================================================

// 最大公约数
inline int gcd(int a, int b) {
    while (b) { int t = b; b = a % b; a = t; }
    return a;
}

inline bool safe_mul(uint64_t a, uint64_t b, uint64_t& out) {
    if (a == 0 || b == 0) { out = 0; return true; }
    if (a > UINT64_MAX / b) return false;
    out = a * b;
    return true;
}

inline bool safe_add(uint64_t a, uint64_t b, uint64_t& out) {
    if (a > UINT64_MAX - b) return false;
    out = a + b;
    return true;
}

// 累加器（GCC/Clang 128位，否则仅 UINT64_T 并可能溢出）
#ifdef __SIZEOF_INT128__
using acc_t = unsigned __int128;
#define ACC_MAX "2^128-1"
#else
using acc_t = uint64_t;
#define ACC_MAY_OVERFLOW  // 编译时警告
#define ACC_MAX "2^64-1"
#endif

// 安全组合数 C(n,k) -> uint64_t
bool nCr_uint64(int n, int k, uint64_t& result);

// 累加器转字符串（用于打印）
string acc_to_string(acc_t x);

// ============================================================
// nCr / 工具函数
// ============================================================

long double nCr_ld(int n, int k);   // 保留，但不再用于主计数
bool fits_uint64(long double x);
string ld_to_string(long double x);

int get_or_create_author_id(const string& name);
string canonical_key(const vector<int>& a);
void intersect_sorted(vector<int>& a, const vector<int>& b);
void subtract_sorted(vector<int>& a, const vector<int>& b);
int intersect_sorted_get(const vector<int>& a, const vector<int>& b, vector<int>& out);
int intersect3_size(const vector<int>& a, const vector<int>& b, const vector<int>& c);
int intersect4_size(const vector<int>& a, const vector<int>& b,
                    const vector<int>& c, const vector<int>& d);

// ============================================================
// 初始化 / 处理
// ============================================================

void init();
void add_maximal_clique(const vector<int>& authors, const vector<int>& core = {});
void bk_on_subgraph(const vector<int>& cn,
                    const function<void(const vector<int>&)>& output);
vector<int> compute_CN(const vector<int>& C);
void process();

// ============================================================
// 退化序 / 前向邻接 / 计数
// ============================================================

pair<vector<int>, vector<int>> degeneracy_ordering();
void build_forward_adj(const vector<int>& rank,
                       vector<vector<int>>& fwd,
                       vector<unordered_set<int>>& fwd_set);

uint64_t count_k2(const vector<vector<int>>& fwd);
uint64_t count_k3(const vector<vector<int>>& fwd,
                  const vector<unordered_set<int>>& fwd_set);
uint64_t count_k4(const vector<vector<int>>& fwd,
                  const vector<unordered_set<int>>& fwd_set);
uint64_t count_k_from_maximal_cliques(int k);

// ============================================================
// 对外入口
// ============================================================

vector<uint64_t> compute_cliques(int max_k = 20);
vector<uint64_t> makeCliques(const string& output_dir = "");
vector<uint64_t> load_and_compute(const string& csx_path);

// ============================================================
// 保存 / 加载
// ============================================================

void save_maximal_cliques_ids(const string& filepath);
void save_maximal_cliques_names(const string& filepath);
void load_maximal_cliques(const string& filepath);

#endif
