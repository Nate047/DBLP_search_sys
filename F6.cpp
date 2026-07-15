#include "F6.h"

// ============================================================
// 全局变量定义
// ============================================================
const int MAX_CLIQUE_SIZE_FOR_COUNTING = 500;   // 用于过滤极大团的大小上限

unordered_map<string, int> author_to_id;
vector<string> id_to_author;
int total_author = 0;

vector<vector<int>> coauthors;
vector<superNode> node;
unordered_set<string> seen_maximal;

// ============================================================
// nCr 计算
// ============================================================

long double nCr_ld(int n, int k) {
    if (k < 0 || k > n) return 0.0L;
    if (k > n - k) k = n - k;
    long double res = 1.0L;
    for (int i = 1; i <= k; i++) {
        res = res * (n - i + 1) / i;
    }
    return res;
}

bool fits_uint64(long double x) {
    return x >= 0 && x <= (long double)UINT64_MAX;
}

string ld_to_string(long double x) {
    if (x < 1e19 && fits_uint64(x)) {
        return to_string((uint64_t)x);
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "%.0Lf", x);
    return string(buf);
}

// ============================================================
// 工具函数
// ============================================================

int get_or_create_author_id(const string& name) {
    auto it = author_to_id.find(name);
    if (it != author_to_id.end()) return it->second;
    int new_id = ++total_author;
    author_to_id[name] = new_id;
    id_to_author.push_back(name);
    return new_id;
}

string canonical_key(const vector<int>& a) {
    string key;
    key.reserve(a.size() * 7);
    for (size_t i = 0; i < a.size(); i++) {
        key += to_string(a[i]) + ',';
    }
    return key;
}

void intersect_sorted(vector<int>& a, const vector<int>& b) {
    size_t i = 0, j = 0, w = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) i++;
        else if (a[i] > b[j]) j++;
        else { a[w++] = a[i]; i++; j++; }
    }
    a.resize(w);
}

void subtract_sorted(vector<int>& a, const vector<int>& b) {
    size_t i = 0, j = 0, w = 0;
    while (i < a.size()) {
        if (j >= b.size() || a[i] < b[j]) { a[w++] = a[i]; i++; }
        else if (a[i] > b[j]) { j++; }
        else { i++; }
    }
    a.resize(w);
}

int intersect_sorted_get(const vector<int>& a, const vector<int>& b,
                         vector<int>& out) {
    out.clear();
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) i++;
        else if (a[i] > b[j]) j++;
        else { out.push_back(a[i]); i++; j++; }
    }
    return (int)out.size();
}

int intersect3_size(const vector<int>& a, const vector<int>& b,
                    const vector<int>& c) {
    int i = 0, j = 0, t = 0, cnt = 0;
    while (i < (int)a.size() && j < (int)b.size() && t < (int)c.size()) {
        if (a[i] < b[j]) i++;
        else if (a[i] > b[j]) j++;
        else {
            int v = a[i];
            while (t < (int)c.size() && c[t] < v) t++;
            if (t < (int)c.size() && c[t] == v) cnt++;
            i++; j++;
        }
    }
    return cnt;
}

int intersect4_size(const vector<int>& a, const vector<int>& b,
                    const vector<int>& c, const vector<int>& d) {
    int i = 0, j = 0, t = 0, u = 0, cnt = 0;
    while (i < (int)a.size() && j < (int)b.size() &&
           t < (int)c.size() && u < (int)d.size()) {
        int min_val = a[i];
        if (b[j] < min_val) min_val = b[j];
        if (c[t] < min_val) min_val = c[t];
        if (d[u] < min_val) min_val = d[u];

        while (i < (int)a.size() && a[i] < min_val) i++;
        while (j < (int)b.size() && b[j] < min_val) j++;
        while (t < (int)c.size() && c[t] < min_val) t++;
        while (u < (int)d.size() && d[u] < min_val) u++;

        if (i < (int)a.size() && j < (int)b.size() &&
            t < (int)c.size() && u < (int)d.size() &&
            a[i] == b[j] && b[j] == c[t] && c[t] == d[u]) {
            cnt++;
            i++; j++; t++; u++;
        }
    }
    return cnt;
}

// ============================================================
// 初始化
// ============================================================

void init() {
    id_to_author.push_back("");

    ifstream csv(repoPath, ios::binary);

    vector<vector<int>> papers;
    papers.reserve(5000000);

    //直接遍历库比遍历tree要快
    std::string line;
    std::getline(csv, line);
    while (std::getline(csv, line)) {
        vector<string> ath = paperIndexTree->parseLine(line).author;
        if (ath.empty()) continue;

        vector<int> ids;
        ids.reserve(ath.size());
        for (const string& e : ath)
            ids.push_back(get_or_create_author_id(e));
        sort(ids.begin(), ids.end());
        ids.erase(unique(ids.begin(), ids.end()), ids.end());
        if (ids.size() >= 2)
            papers.push_back(std::move(ids));

    }

    cout << "[INFO] Papers with >=2 authors: " << papers.size() << endl;
    cout << "[INFO] Total authors: " << total_author << endl;

    coauthors.resize(total_author + 1);
    node.reserve(papers.size());

    for (const auto& p : papers) {
        const auto& a = p;
        for (size_t i = 0; i < a.size(); i++)
            for (size_t j = i + 1; j < a.size(); j++) {
                coauthors[a[i]].push_back(a[j]);
                coauthors[a[j]].push_back(a[i]);
            }
        superNode sn;
        sn.authorId = a;
        sn.drop = false;
        node.push_back(std::move(sn));
    }

    uint64_t edges = 0;
    for (int id = 1; id <= total_author; id++) {
        auto& lst = coauthors[id];
        sort(lst.begin(), lst.end());
        lst.erase(unique(lst.begin(), lst.end()), lst.end());
        edges += lst.size();
    }
    cout << "[INFO] Undirected edges: " << (edges / 2) << endl;

    {
        unordered_set<string> seed_seen;
        vector<superNode> uniq;
        uniq.reserve(node.size());
        for (auto& sn : node) {
            string key = canonical_key(sn.authorId);
            if (seed_seen.insert(key).second)
                uniq.push_back(std::move(sn));
        }
        cout << "[INFO] Seeds after dedup: " << uniq.size()
             << " (from " << node.size() << ")" << endl;
        node = std::move(uniq);
    }
    seen_maximal.clear();
}

// ============================================================
// process() 相关
// ============================================================

void add_maximal_clique(const vector<int>& authors,
                        const vector<int>& core) {
    string key = canonical_key(authors);
    if (!seen_maximal.insert(key).second) return;
    superNode sn;
    sn.authorId = authors;
    sn.drop = false;
    if (!core.empty() && core.size() < authors.size())
        sn.core = core;
    node.push_back(sn);
}

void bk_on_subgraph(
    const vector<int>& cn,
    const function<void(const vector<int>&)>& output)
{
    int m = (int)cn.size();
    if (m == 0) return;
    if (m == 1) { output(cn); return; }

    auto global_to_local = [&](int gv) -> int {
        auto it = lower_bound(cn.begin(), cn.end(), gv);
        if (it != cn.end() && *it == gv) return (int)(it - cn.begin());
        return -1;
    };

    vector<vector<int>> sub_adj(m);
    for (int i = 0; i < m; i++) {
        int gu = cn[i];
        for (int gv : coauthors[gu]) {
            int lv = global_to_local(gv);
            if (lv != -1 && lv != i) sub_adj[i].push_back(lv);
        }
        sort(sub_adj[i].begin(), sub_adj[i].end());
    }

    vector<int> R, P(m), X;
    iota(P.begin(), P.end(), 0);

    function<void()> bk = [&]() {
        if (P.empty() && X.empty()) {
            vector<int> clique(R.size());
            for (size_t i = 0; i < R.size(); i++) clique[i] = cn[R[i]];
            output(clique);
            return;
        }
        int pivot = -1, best = -1;
        auto deg_in_P = [&](int u) {
            int cnt = 0;
            for (int v : P)
                if (binary_search(sub_adj[u].begin(), sub_adj[u].end(), v)) cnt++;
            return cnt;
        };
        for (int u : P) { int d = deg_in_P(u); if (d > best) { best = d; pivot = u; } }
        for (int u : X) { int d = deg_in_P(u); if (d > best) { best = d; pivot = u; } }

        vector<int> candidates;
        if (pivot == -1) candidates = P;
        else for (int v : P)
                if (!binary_search(sub_adj[pivot].begin(), sub_adj[pivot].end(), v))
                    candidates.push_back(v);

        for (int v : candidates) {
            R.push_back(v);
            vector<int> new_P, new_X;
            for (int w : P)
                if (binary_search(sub_adj[v].begin(), sub_adj[v].end(), w))
                    new_P.push_back(w);
            for (int w : X)
                if (binary_search(sub_adj[v].begin(), sub_adj[v].end(), w))
                    new_X.push_back(w);
            swap(P, new_P); swap(X, new_X);
            bk();
            swap(X, new_X); swap(P, new_P);
            R.pop_back();
            auto it = find(P.begin(), P.end(), v);
            if (it != P.end()) P.erase(it);
            X.push_back(v);
        }
    };
    bk();
}

vector<int> compute_CN(const vector<int>& C) {
    if (C.empty()) return {};
    int best = 0;
    for (size_t i = 1; i < C.size(); i++)
        if (coauthors[C[i]].size() < coauthors[C[best]].size())
            best = (int)i;
    const vector<int>* result = &coauthors[C[best]];
    vector<int> tmp;
    for (size_t i = 0; i < C.size(); i++) {
        if ((int)i == best) continue;
        if (result == &coauthors[C[best]]) {
            tmp = *result;
            intersect_sorted(tmp, coauthors[C[i]]);
            result = &tmp;
        } else {
            intersect_sorted(tmp, coauthors[C[i]]);
        }
        if (result->empty()) return {};
    }
    vector<int> cn = *result;
    subtract_sorted(cn, C);
    return cn;
}

void process() {
    queue<int> q;
    for (int i = 0; i < (int)node.size(); i++) q.push(i);
    int processed = 0, total = (int)node.size();

    while (!q.empty()) {
        int idx = q.front(); q.pop();
        if (node[idx].drop) continue;

        if (++processed % 5000 == 0)
            cout << "\r[INFO] Processed: " << processed << "/" << total << flush;

        vector<int> cn = compute_CN(node[idx].authorId);

        if (cn.empty()) {
            add_maximal_clique(node[idx].authorId);
            node[idx].drop = true;
            continue;
        }

        vector<vector<int>> maximal_Ks;
        bk_on_subgraph(cn, [&](const vector<int>& K) {
            maximal_Ks.push_back(K);
        });

        if (maximal_Ks.empty()) {
            add_maximal_clique(node[idx].authorId);
        } else {
            vector<int> I = maximal_Ks[0];
            for (size_t j = 1; j < maximal_Ks.size(); j++)
                intersect_sorted(I, maximal_Ks[j]);

            if (!I.empty()) {
                vector<int> core = node[idx].authorId;
                core.insert(core.end(), I.begin(), I.end());
                sort(core.begin(), core.end());
                add_maximal_clique(core);
            }

            for (const auto& K : maximal_Ks) {
                vector<int> M = node[idx].authorId;
                M.insert(M.end(), K.begin(), K.end());
                sort(M.begin(), M.end());
                vector<int> core_full;
                if (!I.empty()) {
                    core_full = node[idx].authorId;
                    core_full.insert(core_full.end(), I.begin(), I.end());
                    sort(core_full.begin(), core_full.end());
                }
                add_maximal_clique(M, core_full);
            }
        }
        node[idx].drop = true;
    }
    cout << "\r[INFO] Processed: " << processed << "/" << total << endl;

    int mc = 0;
    for (const auto& sn : node)
        if (!sn.drop) mc++;
    cout << "[INFO] Maximal cliques found: " << mc << endl;
}

// ============================================================
// 退化序直接计数（k≤4）
// ============================================================

pair<vector<int>, vector<int>> degeneracy_ordering() {
    int n = total_author;
    if (n == 0) return {{}, {}};

    vector<int> deg(n + 1, 0);
    int max_deg = 0;
    for (int v = 1; v <= n; v++) {
        deg[v] = (int)coauthors[v].size();
        if (deg[v] > max_deg) max_deg = deg[v];
    }

    vector<vector<int>> buckets(max_deg + 1);
    for (int v = 1; v <= n; v++) buckets[deg[v]].push_back(v);

    vector<int> rank(n + 1, -1), order(n);
    vector<bool> removed(n + 1, false);
    int cur_rank = 0, cur_min = 0;

    while (cur_rank < n) {
        while (cur_min <= max_deg && buckets[cur_min].empty()) cur_min++;
        if (cur_min > max_deg) break;
        int v = buckets[cur_min].back();
        buckets[cur_min].pop_back();
        if (removed[v]) continue;
        if (deg[v] != cur_min) {
            if (deg[v] >= 0 && deg[v] <= max_deg)
                buckets[deg[v]].push_back(v);
            continue;
        }
        rank[v] = cur_rank;
        order[cur_rank] = v;
        removed[v] = true;
        cur_rank++;
        for (int u : coauthors[v]) {
            if (!removed[u]) {
                deg[u]--;
                if (deg[u] < cur_min) cur_min = deg[u];
                if (deg[u] >= 0 && deg[u] <= max_deg)
                    buckets[deg[u]].push_back(u);
            }
        }
    }

    int degen = 0;
    for (int v = 1; v <= n; v++) {
        int c = 0;
        for (int u : coauthors[v])
            if (rank[u] > rank[v]) c++;
        if (c > degen) degen = c;
    }
    cout << "[INFO] Degeneracy = " << degen << endl;
    return {rank, order};
}

void build_forward_adj(
    const vector<int>& rank,
    vector<vector<int>>& fwd,
    vector<unordered_set<int>>& fwd_set)
{
    int n = total_author;
    fwd.assign(n + 1, vector<int>());
    fwd_set.assign(n + 1, unordered_set<int>());
    uint64_t tf = 0;
    for (int v = 1; v <= n; v++) {
        vector<int> fv;
        for (int u : coauthors[v])
            if (rank[u] > rank[v]) fv.push_back(u);
        sort(fv.begin(), fv.end(),
             [&](int a, int b) { return rank[a] < rank[b]; });
        fwd_set[v].insert(fv.begin(), fv.end());
        fwd[v] = std::move(fv);
        tf += fwd[v].size();
    }
    cout << "[INFO] Forward edges: " << tf << endl;
}

uint64_t count_k2(const vector<vector<int>>& fwd) {
    uint64_t cnt = 0;
    for (int v = 1; v <= total_author; v++) cnt += fwd[v].size();
    return cnt;
}

uint64_t count_k3(
    const vector<vector<int>>& fwd,
    const vector<unordered_set<int>>& fwd_set)
{
    uint64_t cnt = 0;
    int n = total_author;
    for (int v = 1; v <= n; v++) {
        const auto& fv = fwd[v];
        int d = (int)fv.size();
        if (d < 2) continue;
        for (int i = 0; i < d; i++) {
            int u = fv[i];
            const auto& su = fwd_set[u];
            for (int j = i + 1; j < d; j++)
                if (su.count(fv[j])) cnt++;
        }
    }
    return cnt;
}

uint64_t count_k4(
    const vector<vector<int>>& fwd,
    const vector<unordered_set<int>>& fwd_set)
{
    uint64_t cnt = 0;
    int n = total_author;
    int progress = 0;
    auto t0 = high_resolution_clock::now();

    for (int v = 1; v <= n; v++) {
        const auto& fv = fwd[v];
        int d = (int)fv.size();
        if (d < 3) continue;
        for (int i = 0; i < d; i++) {
            int u = fv[i];
            const auto& su = fwd_set[u];
            if (fwd[u].size() < 2) continue;
            for (int j = i + 1; j < d; j++) {
                int w = fv[j];
                if (!su.count(w)) continue;
                const auto& sw = fwd_set[w];
                for (int t = j + 1; t < d; t++) {
                    int x = fv[t];
                    if (su.count(x) && sw.count(x)) cnt++;
                }
            }
        }
        if (++progress % 100000 == 0) {
            auto ela = duration_cast<seconds>(
                           high_resolution_clock::now() - t0).count();
            cout << "\r  k=4: " << progress << "/" << n
                 << "  found=" << cnt << "  elapsed=" << ela << "s" << flush;
        }
    }
    cout << "\r  k=4: " << cnt << "                    " << endl;
    return cnt;
}

// ============================================================
// 极大团公式法（k≥5）
// ============================================================

uint64_t count_k_from_maximal_cliques(int k) {
    // 过滤极大团：仅使用 size ≤ MAX_CLIQUE_SIZE_FOR_COUNTING 且 size ≥ k 的团
    vector<const vector<int>*> cliques;
    cliques.reserve(node.size());
    int filtered = 0;
    for (const auto& sn : node) {
        if (!sn.drop && (int)sn.authorId.size() >= k) {
            if ((int)sn.authorId.size() <= MAX_CLIQUE_SIZE_FOR_COUNTING) {
                cliques.push_back(&sn.authorId);
            } else {
                filtered++;
            }
        }
    }

    int m = (int)cliques.size();
    cout << "  Cliques with size >= " << k
         << " (size <= " << MAX_CLIQUE_SIZE_FOR_COUNTING << "): " << m
         << " (filtered out " << filtered << " with size > "
         << MAX_CLIQUE_SIZE_FOR_COUNTING << ")" << endl;

    if (m == 0) return 0;

    // ----------- 倒排索引 -----------
    unordered_map<int, vector<int>> inv;
    for (int i = 0; i < m; i++)
        for (int v : *cliques[i])
            inv[v].push_back(i);

    // ----------- 第一项：各极大团内 k‑子集之和 -----------
    long double total = 0.0L;
    for (auto* M : cliques)
        total += nCr_ld((int)M->size(), k);

    // ----------- 第二项：两两重叠的 k‑子集之和（二阶容斥） -----------
    long double subtract2 = 0.0L;
    int pair_checked = 0, pair_overlap = 0;
    vector<int> inter;
    inter.reserve(500);

    for (int i = 0; i < m; i++) {
        const auto& Mi = *cliques[i];

        // 统计每个 j>i 与 i 的公共顶点数（通过倒排索引）
        unordered_map<int, int> overlap_cnt;
        for (int v : Mi) {
            auto it = inv.find(v);
            if (it == inv.end()) continue;
            for (int j : it->second)
                if (j > i) overlap_cnt[j]++;
        }

        for (auto& [j, cnt] : overlap_cnt) {
            pair_checked++;
            if (cnt < k) continue;   // 公共顶点数少于 k，不可能有公共 k‑子集

            int isz = intersect_sorted_get(Mi, *cliques[j], inter);
            if (isz >= k) {
                subtract2 += nCr_ld(isz, k);
                pair_overlap++;
            }
        }
    }

    cout << "    Pairs checked: " << pair_checked
         << "  overlaps(>=k): " << pair_overlap << endl;
    cout << "    Subtract(2nd): " << ld_to_string(subtract2) << endl;

    total = total - subtract2;

    uint64_t result = (uint64_t)total;
    cout << "    result = " << result << endl;   // 输出最终值
    return result;
}
// ============================================================
// 混合计数入口
// ============================================================

vector<uint64_t> compute_cliques(int max_k) {

    cout << "k-Clique Counting..." << endl;
    auto total_t0 = high_resolution_clock::now();

    cout << "[1/4] Degeneracy ordering ..." << endl;
    auto t0 = high_resolution_clock::now();
    auto [rank, order] = degeneracy_ordering();
    cout << "  done in " << duration_cast<seconds>(
                                high_resolution_clock::now() - t0).count() << "s" << endl;

    cout << "[2/4] Building forward adjacency ..." << endl;
    t0 = high_resolution_clock::now();
    vector<vector<int>> fwd;
    vector<unordered_set<int>> fwd_set;
    build_forward_adj(rank, fwd, fwd_set);
    cout << "  done in " << duration_cast<seconds>(
                                high_resolution_clock::now() - t0).count() << "s" << endl;
    rank.clear(); rank.shrink_to_fit();
    order.clear(); order.shrink_to_fit();

    cout << "[3/4] Maximal cliques: ";
    int mc = 0, large = 0;
    for (const auto& sn : node) {
        if (!sn.drop) {
            mc++;
            if ((int)sn.authorId.size() > MAX_CLIQUE_SIZE_FOR_COUNTING) large++;
        }
    }
    cout << mc << " (size > " << MAX_CLIQUE_SIZE_FOR_COUNTING << ": " << large << ")" << endl;

    cout << "[4/4] Counting ..." << endl;

    vector<uint64_t> result(max_k + 1);

    result[1] = (uint64_t)total_author;
    cout << "  k=1: " << result[1] << endl;

    result[2] = count_k2(fwd);
    cout << "  k=2: " << result[2] << endl;

    if (max_k >= 3) {
        t0 = high_resolution_clock::now();
        result[3] = count_k3(fwd, fwd_set);
        cout << "  k=3: " << result[3] << "  ("
             << duration_cast<seconds>(
                    high_resolution_clock::now() - t0).count() << "s)" << endl;
    }

    if (max_k >= 4) {
        t0 = high_resolution_clock::now();
        result[4] = count_k4(fwd, fwd_set);
        cout << "  k=4: " << result[4] << "  ("
             << duration_cast<seconds>(
                    high_resolution_clock::now() - t0).count() << "s)" << endl;
    }
    vector<string> msg;
    msg.push_back("k<=4: ");
    for(int i=1;i<=4;i++){
        msg.push_back(std::to_string(result[i])+", ");
    }
    // ResultDialog *dialog = new ResultDialog(msg);
    // dialog->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动释放内存
    // dialog->show();


    for (int k = 5; k <= max_k; k++) {
        t0 = high_resolution_clock::now();
        result[k] = count_k_from_maximal_cliques(k);
        cout << "  k=" << k << ": " << result[k] << "  ("
             << duration_cast<seconds>(
                    high_resolution_clock::now() - t0).count() << "s)" << endl;
        if (result[k] == 0) {
            cout << "  (k > " << k << " are 0, stopping)" << endl;
            break;
        }
    }

    auto tel = duration_cast<seconds>(
                   high_resolution_clock::now() - total_t0).count();
    cout << "=== Total: " << tel << "s ===" << endl;
    return result;
}

// ============================================================
// 保存 / 加载
// ============================================================

void save_maximal_cliques_ids(const string& filepath) {
    ofstream out(filepath);
    if (!out) { cerr << "[ERROR] Cannot open: " << filepath << endl; return; }
    int count = 0;
    for (const auto& sn : node) {
        if (sn.drop) continue;
        out << sn.authorId.size();
        for (int aid : sn.authorId) out << "," << aid;
        out << "\n";
        count++;
    }
    out.close();
    cout << "[INFO] Saved " << count << " maximal cliques to: " << filepath << endl;
}

void save_maximal_cliques_names(const string& filepath) {
    ofstream out(filepath);
    if (!out) { cerr << "[ERROR] Cannot open: " << filepath << endl; return; }
    int count = 0;
    for (const auto& sn : node) {
        if (sn.drop) continue;
        out << sn.authorId.size();
        for (int aid : sn.authorId) {
            out << ",\"";
            const string& name = id_to_author[aid];
            for (char ch : name) {
                if (ch == '\"') out << "\"\"";
                else out << ch;
            }
            out << "\"";
        }
        out << "\n";
        count++;
    }
    out.close();
    cout << "[INFO] Saved " << count << " maximal cliques to: " << filepath << endl;
}

void load_maximal_cliques(const string& filepath) {
    ifstream in(filepath);
    if (!in) { cerr << "[ERROR] Cannot open: " << filepath << endl; return; }

    node.clear();
    seen_maximal.clear();
    int loaded = 0, max_id = 0;
    string line;
    string temline;
    while (getline(in, line)) {
        if (line.empty()) continue;
        stringstream p(line);
        vector<int> ids;
        ids.reserve(20);
        while (getline(p, temline, ',')) {
            int token = stoi(temline);
            ids.push_back(token);
            if (token > max_id) max_id = token;
        }
        if (ids.empty()) continue;
        int k = ids[0];
        if ((int)ids.size() < k + 1) continue;
        superNode sn;
        sn.authorId.assign(ids.begin() + 1, ids.begin() + 1 + k);
        sn.drop = false;
        node.push_back(std::move(sn));
        loaded++;
    }
    in.close();

    if (max_id > total_author) {
        total_author = max_id;
        for (int i = (int)id_to_author.size(); i <= max_id; i++)
            id_to_author.push_back("author_" + to_string(i));
        coauthors.resize(max_id + 1);
    }
    cout << "[INFO] Loaded " << loaded << " maximal cliques" << endl;
}

// ============================================================
// 顶层入口
// ============================================================

vector<uint64_t> makeCliques(const string& output_dir) {
    cout << "=== Initializing ===" << endl;
    init();
    cout << "  Total authors: " << total_author << endl;

    cout << "=== Processing (expanding to maximal cliques) ===" << endl;
    process();

    // 确定保存路径（默认当前目录）
    string save_path;
    if (output_dir.empty()) {
        save_path = "./maximal_cliques_ids.csv";
    } else {
        string dir = output_dir;
        if (dir.back() != '/' && dir.back() != '\\') dir += "/";
        save_path = dir + "maximal_cliques_ids.csv";
    }
    save_maximal_cliques_ids(save_path);
    cout << "[INFO] Maximal cliques saved to " << save_path << endl;

    return compute_cliques();
}
vector<uint64_t> load_and_compute(const string& csv_path) {
    load_maximal_cliques(csv_path);

    cout << "[INFO] Rebuilding coauthors ..." << endl;
    coauthors.assign(total_author + 1, vector<int>());
    for (const auto& sn : node) {
        if (sn.drop) continue;
        const auto& a = sn.authorId;
        for (size_t p = 0; p < a.size(); p++)
            for (size_t q = p + 1; q < a.size(); q++) {
                coauthors[a[p]].push_back(a[q]);
                coauthors[a[q]].push_back(a[p]);
            }
    }
    uint64_t edges = 0;
    for (int id = 1; id <= total_author; id++) {
        auto& lst = coauthors[id];
        sort(lst.begin(), lst.end());
        lst.erase(unique(lst.begin(), lst.end()), lst.end());
        edges += lst.size();
    }
    cout << "[INFO] Reconstructed " << (edges / 2) << " edges" << endl;

    return compute_cliques();
}
