#include "authorgraph.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDir>
#include <QApplication>
#include <QTimer>
#include <cmath>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
#include <climits>

// ═══════════════════════════════════════════
// 路径工具（基于 exe 自身位置，不受工作目录影响）
// ═══════════════════════════════════════════
static QString csvPath() {
    return QCoreApplication::applicationDirPath() + "./paperData.csv";
}
static QString coordsPath() {
    return QCoreApplication::applicationDirPath() + "./coords.json";
}
static QString adjacencyPath() {
    return QCoreApplication::applicationDirPath() + "./cache/adjacency.json";
}

// ═══════════════════════════════════════════
// 网络构建（igraph 0.10+ C API）
// ═══════════════════════════════════════════
static bool buildNetworkFromCSV(std::vector<GraphNode>& nodes,
                                AdjacencyMap& adj) {
    std::unordered_map<std::string, std::set<int>> authorPapers;
    std::map<std::pair<std::string,std::string>, int> coauthorWeights;

    std::ifstream f(csvPath().toStdString());
    if (!f.is_open()) return false;

    std::string line;
    std::getline(f, line);          // skip header

    int paperIdx = 0;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        size_t comma = line.find(',');
        if (comma == std::string::npos) continue;

        std::string rawAuth = line.substr(comma + 1);
        while (!rawAuth.empty() && std::isspace(rawAuth.front())) rawAuth.erase(0,1);
        while (!rawAuth.empty() && std::isspace(rawAuth.back())) rawAuth.pop_back();

        std::vector<std::string> authors;
        std::istringstream iss(rawAuth);
        std::string token;
        while (std::getline(iss, token, '|')) {
            while (!token.empty() && std::isspace(token.front())) token.erase(0,1);
            while (!token.empty() && std::isspace(token.back())) token.pop_back();
            if (!token.empty()) authors.push_back(token);
        }
        if (authors.empty()) continue;

        for (const auto& a : authors)
            authorPapers[a].insert(paperIdx);

        for (size_t i = 0; i < authors.size(); ++i)
            for (size_t j = i+1; j < authors.size(); ++j) {
                auto a1 = authors[i], a2 = authors[j];
                if (a1 > a2) std::swap(a1, a2);
                coauthorWeights[{a1, a2}]++;
            }
        ++paperIdx;
    }

    if (authorPapers.empty()) return false;

    // 按论文数降序，分配 ID
    std::vector<std::pair<std::string, std::set<int>>> sorted(
        authorPapers.begin(), authorPapers.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) {
                  return a.second.size() > b.second.size();
              });

    std::unordered_map<std::string, int> nameToId;
    int N = static_cast<int>(sorted.size());
    for (int i = 0; i < N; ++i) nameToId[sorted[i].first] = i+1;

    igraph_t g;
    igraph_vector_int_t edges;
    igraph_vector_t weights;
    igraph_vector_int_init(&edges, 0);
    igraph_vector_init(&weights, 0);

    for (const auto& kv : coauthorWeights) {
        int u = nameToId[kv.first.first] - 1;
        int v = nameToId[kv.first.second] - 1;
        igraph_vector_int_push_back(&edges, u);
        igraph_vector_int_push_back(&edges, v);
        igraph_vector_push_back(&weights, kv.second);
    }

    igraph_create(&g, &edges, N, IGRAPH_UNDIRECTED);

    igraph_matrix_t layout;
    igraph_matrix_init(&layout, N, 2);

    // ═══════════════════════════════════════════
    // igraph 0.10+ 布局 API 修正
    // ═══════════════════════════════════════════
    if (N <= 500) {
        igraph_rng_seed(igraph_rng_default(), 42);
        igraph_layout_fruchterman_reingold(
            &g, &layout,
            false,                                    // use_seed
            500,                                      // niter
            static_cast<igraph_real_t>(N) * 1.5,      // start_temp
            static_cast<igraph_layout_grid_t>(0),    // ← 0 = AUTO，不依赖宏
            &weights,
            nullptr, nullptr, nullptr, nullptr
            );
    } else if (N <= 5000) {
        igraph_rng_seed(igraph_rng_default(), 42);
        igraph_layout_fruchterman_reingold(
            &g, &layout,
            false,
            300,
            static_cast<igraph_real_t>(N) * 1.5,
            static_cast<igraph_layout_grid_t>(0),
            &weights,
            nullptr, nullptr, nullptr, nullptr
            );
    } else {
        igraph_layout_lgl(
            &g, &layout,
            150,
            static_cast<igraph_real_t>(N),
            static_cast<igraph_real_t>(N * N),
            1.5,
            static_cast<igraph_real_t>(N * N * N),
            std::max(1.0, static_cast<double>(N * N) / 1000000.0),
            -1
            );
    }
    // 提取坐标并缩放
    double xmin = DBL_MAX, xmax = -DBL_MAX, ymin = DBL_MAX, ymax = -DBL_MAX;
    for (int i = 0; i < N; ++i) {
        double x = MATRIX(layout, i, 0);
        double y = MATRIX(layout, i, 1);
        if (x < xmin) xmin = x;
        if (x > xmax) xmax = x;
        if (y < ymin) ymin = y;
        if (y > ymax) ymax = y;
    }

    double scaleX = xmax - xmin; if (scaleX == 0) scaleX = 1;
    double scaleY = ymax - ymin; if (scaleY == 0) scaleY = 1;
    double factor = std::sqrt(static_cast<double>(N)) * 1.2;

    nodes.resize(N);
    for (int i = 0; i < N; ++i) {
        nodes[i].id = i+1;
        nodes[i].x = (MATRIX(layout, i, 0) - xmin) / scaleX * factor;
        nodes[i].y = (MATRIX(layout, i, 1) - ymin) / scaleY * factor;
        nodes[i].name = sorted[i].first;
        std::ostringstream oss;
        bool first = true;
        for (int pid : sorted[i].second) {
            if (!first) oss << "|";
            oss << pid;
            first = false;
        }
        nodes[i].paperIds = oss.str();
        nodes[i].paperCount = static_cast<int>(sorted[i].second.size());
    }

    adj.clear();
    for (const auto& kv : coauthorWeights) {
        int id1 = nameToId[kv.first.first];
        int id2 = nameToId[kv.first.second];
        adj[id1].push_back({id2, kv.second});
        adj[id2].push_back({id1, kv.second});
    }

    // 保存 JSON 缓存
    QJsonArray coordsArr;
    for (const auto& nd : nodes) {
        QJsonObject o;
        o["x"] = nd.x;
        o["y"] = nd.y;
        o["name"] = QString::fromStdString(nd.name);
        o["paper_ids"] = QString::fromStdString(nd.paperIds);
        o["paper_count"] = nd.paperCount;
        coordsArr.append(o);
    }
    QFile fcc(coordsPath());
    if (fcc.open(QIODevice::WriteOnly)) {
        fcc.write(QJsonDocument(coordsArr).toJson());
        fcc.close();
    }

    QDir().mkpath(QCoreApplication::applicationDirPath() + "/../cache");
    QJsonObject adjObj;
    for (const auto& kv : adj) {
        QJsonArray neighArr;
        for (const auto& nb : kv.second) {
            QJsonArray pair;
            pair.append(nb.id);
            pair.append(nb.weight);
            neighArr.append(pair);
        }
        adjObj[QString::number(kv.first)] = neighArr;
    }
    QFile fac(adjacencyPath());
    if (fac.open(QIODevice::WriteOnly)) {
        fac.write(QJsonDocument(adjObj).toJson());
        fac.close();
    }

    igraph_matrix_destroy(&layout);
    igraph_vector_destroy(&weights);
    igraph_vector_int_destroy(&edges);
    igraph_destroy(&g);
    return true;
}

// ═══════════════════════════════════════════
// GraphGLWidget 实现
// ═══════════════════════════════════════════
GraphGLWidget::GraphGLWidget(QWidget* parent)
    : QOpenGLWidget(parent),
    m_program(nullptr), m_vao(nullptr),
    m_nodeVBO(nullptr), m_edgeVBO(nullptr),
    m_nodeCount(0), m_edgeCount(0),
    m_selectedIdx(-1),
    m_viewScale(1.0),
    m_worldExtent(100.0) {
    setMouseTracking(true);
}

GraphGLWidget::~GraphGLWidget() {
    makeCurrent();
    delete m_nodeVBO; m_nodeVBO = nullptr;
    delete m_edgeVBO; m_edgeVBO = nullptr;
    delete m_vao; m_vao = nullptr;
    delete m_program; m_program = nullptr;
    doneCurrent();
}

void GraphGLWidget::setGraph(const std::vector<GraphNode>* nodes,
                             const AdjacencyMap* adj,
                             QueryFunc queryFn) {
    m_nodes = nodes;
    m_adj = adj;
    m_queryFn = queryFn;
    prepareGeometry();
    update();
}

void GraphGLWidget::initializeGL() {
    initializeOpenGLFunctions();

    // 启用混合与点精灵
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_PROGRAM_POINT_SIZE);   // 允许着色器控制点大小

    // 编译着色器（增加 uPointSize uniform）
    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex,
                                       R"glsl(#version 150
        uniform mat4 uProj;
        uniform mat4 uView;
        uniform float uPointSize;   // 点大小参数
        in vec2 aPos;
        in vec3 aColor;
        out vec3 vColor;
        void main() {
            gl_Position = uProj * uView * vec4(aPos, 0.0, 1.0);
            gl_PointSize = uPointSize;   // 必须赋值，否则未定义
            vColor = aColor;
        })glsl");
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                       R"glsl(#version 150
        in vec3 vColor;
        out vec4 fragColor;
        void main() {
            fragColor = vec4(vColor, 1.0);
        })glsl");
    m_program->bindAttributeLocation("aPos", 0);
    m_program->bindAttributeLocation("aColor", 1);
    m_program->link();

    // 创建 VAO 与 VBO
    m_vao = new QOpenGLVertexArrayObject(this);
    m_vao->create();
    m_nodeVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_nodeVBO->create();
    m_edgeVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_edgeVBO->create();

    // 默认投影矩阵（单位阵，后续在 resizeGL 中设置正交投影）
    m_proj.setToIdentity();
}

void GraphGLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    m_proj.setToIdentity();
    m_proj.ortho(-w/2.0, w/2.0, -h/2.0, h/2.0, -1, 1);
}

void GraphGLWidget::paintGL() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!m_nodes || m_nodes->empty()) return;

    // 视图矩阵：平移 + 缩放
    QMatrix4x4 view;
    view.translate(m_viewOffset.x(), m_viewOffset.y());
    view.scale(m_viewScale, m_viewScale);

    m_program->bind();
    m_vao->bind();
    m_program->setUniformValue("uProj", m_proj);
    m_program->setUniformValue("uView", view);

    // 1. 基础边（灰色）
    if (m_edgeCount > 0) {
        m_edgeVBO->bind();
        m_program->enableAttributeArray(0);
        m_program->enableAttributeArray(1);
        m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 5*sizeof(float));
        m_program->setAttributeBuffer(1, GL_FLOAT, 2*sizeof(float), 3, 5*sizeof(float));
        glDrawArrays(GL_LINES, 0, m_edgeCount*2);
    }

    // 2. 高亮边（橙色）—— 临时 VBO
    if (!m_highlightEdgesVertices.empty()) {
        QOpenGLBuffer tmp(QOpenGLBuffer::VertexBuffer);
        tmp.create();
        tmp.bind();
        tmp.allocate(m_highlightEdgesVertices.data(),
                     static_cast<int>(m_highlightEdgesVertices.size() * sizeof(float)));
        m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 5*sizeof(float));
        m_program->setAttributeBuffer(1, GL_FLOAT, 2*sizeof(float), 3, 5*sizeof(float));
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_highlightEdgesVertices.size()/5));
        tmp.destroy();
    }

    // 3. 基础节点（青色）
    {
        m_nodeVBO->bind();
        m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 5*sizeof(float));
        m_program->setAttributeBuffer(1, GL_FLOAT, 2*sizeof(float), 3, 5*sizeof(float));
        // ★ 用 uniform 代替 glPointSize
        m_program->setUniformValue("uPointSize", 5.0f * static_cast<float>(m_viewScale));
        glDrawArrays(GL_POINTS, 0, m_nodeCount);
    }

    // 4. 高亮节点（邻居黄色 + 选中红色）
    if (m_selectedIdx >= 0) {
        std::vector<float> highlightVerts;

        // 邻居节点（黄色）
        for (int nIdx : m_highlightNeighbors) {
            const auto& nnd = (*m_nodes)[nIdx];
            highlightVerts.push_back(static_cast<float>(nnd.x));
            highlightVerts.push_back(static_cast<float>(nnd.y));
            highlightVerts.push_back(1.0f);
            highlightVerts.push_back(0.84f);
            highlightVerts.push_back(0.0f);
        }

        // 选中节点（红色）
        const auto& snd = (*m_nodes)[m_selectedIdx];
        highlightVerts.push_back(static_cast<float>(snd.x));
        highlightVerts.push_back(static_cast<float>(snd.y));
        highlightVerts.push_back(1.0f);
        highlightVerts.push_back(0.18f);
        highlightVerts.push_back(0.18f);

        if (!highlightVerts.empty()) {
            QOpenGLBuffer tmpN(QOpenGLBuffer::VertexBuffer);
            tmpN.create();
            tmpN.bind();
            tmpN.allocate(highlightVerts.data(),
                          static_cast<int>(highlightVerts.size() * sizeof(float)));
            m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 5*sizeof(float));
            m_program->setAttributeBuffer(1, GL_FLOAT, 2*sizeof(float), 3, 5*sizeof(float));
            // ★ 用 uniform 设置高亮点大小
            m_program->setUniformValue("uPointSize", 10.0f * static_cast<float>(m_viewScale));
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(highlightVerts.size()/5));
            tmpN.destroy();
        }
    }

    m_vao->release();
    m_program->release();
}

// 屏幕坐标 → 世界坐标（考虑 viewOffset 和 viewScale）
QPointF GraphGLWidget::toWorldCoords(const QPoint& screen) {
    double cx = width() / 2.0;
    double cy = height() / 2.0;
    double x = (screen.x() - cx - m_viewOffset.x()) / m_viewScale;
    double y = (cy - screen.y() - m_viewOffset.y()) / m_viewScale;
    return QPointF(x, y);
}

int GraphGLWidget::findNearestNode(const QPointF& world) {
    if (!m_nodes) return -1;
    double best = DBL_MAX;
    int bestIdx = -1;
    for (int i = 0; i < static_cast<int>(m_nodes->size()); ++i) {
        double dx = (*m_nodes)[i].x - world.x();
        double dy = (*m_nodes)[i].y - world.y();
        double sq = dx*dx + dy*dy;
        if (sq < best) { best = sq; bestIdx = i; }
    }
    return bestIdx;
}

void GraphGLWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        QPointF world = toWorldCoords(e->pos());
        int nearest = findNearestNode(world);

        // 15 像素容差 → 世界距离
        double pixelWorldSize = 1.0 / m_viewScale;
        double tol = std::pow(15.0 * pixelWorldSize, 2);

        if (nearest >= 0 && m_nodes && nearest < static_cast<int>(m_nodes->size())) {
            double dx = (*m_nodes)[nearest].x - world.x();
            double dy = (*m_nodes)[nearest].y - world.y();
            if (dx*dx + dy*dy <= tol) {
                m_selectedIdx = nearest;

                // 构建邻居信息
                int nodeId = nearest + 1;
                auto it = m_adj->find(nodeId);
                m_highlightNeighbors.clear();
                m_highlightEdgesVertices.clear();

                std::vector<std::pair<std::string,int>> neighList;
                if (it != m_adj->end()) {
                    for (const auto& nb : it->second) {
                        int nIdx = nb.id - 1;
                        if (nIdx >= 0 && nIdx < static_cast<int>(m_nodes->size())) {
                            m_highlightNeighbors.push_back(nIdx);
                            const auto& nnd = (*m_nodes)[nIdx];

                            // 高亮边：选中点 → 邻居
                            m_highlightEdgesVertices.push_back(static_cast<float>((*m_nodes)[m_selectedIdx].x));
                            m_highlightEdgesVertices.push_back(static_cast<float>((*m_nodes)[m_selectedIdx].y));
                            m_highlightEdgesVertices.push_back(1.0f);
                            m_highlightEdgesVertices.push_back(0.65f);
                            m_highlightEdgesVertices.push_back(0.0f);

                            m_highlightEdgesVertices.push_back(static_cast<float>(nnd.x));
                            m_highlightEdgesVertices.push_back(static_cast<float>(nnd.y));
                            m_highlightEdgesVertices.push_back(1.0f);
                            m_highlightEdgesVertices.push_back(0.65f);
                            m_highlightEdgesVertices.push_back(0.0f);

                            neighList.emplace_back(nnd.name, nb.weight);
                        }
                    }
                }

                // 查询论文
                auto papers = m_queryFn ? m_queryFn((*m_nodes)[nearest].name)
                                        : std::vector<std::string>{};
                update();
                emit nodeClicked((*m_nodes)[nearest].name, neighList, papers);
                return;
            }
        }

        // 清除选择
        m_selectedIdx = -1;
        m_highlightNeighbors.clear();
        m_highlightEdgesVertices.clear();
        update();
    }
    m_lastMousePos = e->pos();
}

void GraphGLWidget::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) {
        QPoint delta = e->pos() - m_lastMousePos;
        m_viewOffset.setX(m_viewOffset.x() + delta.x());
        m_viewOffset.setY(m_viewOffset.y() - delta.y());
        m_lastMousePos = e->pos();
        update();
    }
}

void GraphGLWidget::wheelEvent(QWheelEvent* e) {
    double factor = std::pow(1.001, e->angleDelta().y());
    double newScale = m_viewScale * factor;
    if (newScale < 0.01) newScale = 0.01;
    if (newScale > 20) newScale = 20;

    // 以鼠标位置为中心缩放
    QPointF mousePos = e->position();
    double cx = width() / 2.0;
    double cy = height() / 2.0;

    m_viewOffset.setX(m_viewOffset.x() * factor + (mousePos.x() - cx) * (1.0 - factor));
    m_viewOffset.setY(m_viewOffset.y() * factor + (cy - mousePos.y()) * (1.0 - factor));
    m_viewScale = newScale;

    update();
}

void GraphGLWidget::prepareGeometry() {
    if (!m_nodes || m_nodes->empty()) return;
    m_vao->bind();

    m_nodeCount = static_cast<int>(m_nodes->size());
    std::vector<float> nodeData(m_nodeCount * 5);
    for (int i = 0; i < m_nodeCount; ++i) {
        nodeData[i*5+0] = static_cast<float>((*m_nodes)[i].x);
        nodeData[i*5+1] = static_cast<float>((*m_nodes)[i].y);
        nodeData[i*5+2] = 0.0f; nodeData[i*5+3] = 0.75f; nodeData[i*5+4] = 0.85f;
    }
    m_nodeVBO->bind();
    m_nodeVBO->allocate(nodeData.data(), static_cast<int>(nodeData.size()*sizeof(float)));

    std::set<std::pair<int,int>> edgeSet;
    for (const auto& kv : *m_adj)
        for (const auto& nb : kv.second) {
            int u = kv.first-1, v = nb.id-1;
            if (u < v) edgeSet.insert({u, v});
        }
    m_edgeCount = static_cast<int>(edgeSet.size());
    std::vector<float> edgeData(m_edgeCount*2*5);
    int idx = 0;
    for (const auto& e : edgeSet) {
        const auto& a = (*m_nodes)[e.first];
        const auto& b = (*m_nodes)[e.second];
        edgeData[idx++] = static_cast<float>(a.x); edgeData[idx++] = static_cast<float>(a.y);
        edgeData[idx++] = 0.78f; edgeData[idx++] = 0.78f; edgeData[idx++] = 0.78f;
        edgeData[idx++] = static_cast<float>(b.x); edgeData[idx++] = static_cast<float>(b.y);
        edgeData[idx++] = 0.78f; edgeData[idx++] = 0.78f; edgeData[idx++] = 0.78f;
    }
    m_edgeVBO->bind();
    m_edgeVBO->allocate(edgeData.data(), static_cast<int>(edgeData.size()*sizeof(float)));

    m_vao->release();

    // 更新世界范围（用于初始化视图）
    if (m_nodeCount > 0) {
        double xmin = DBL_MAX, xmax = -DBL_MAX, ymin = DBL_MAX, ymax = -DBL_MAX;
        for (const auto& nd : *m_nodes) {
            if (nd.x < xmin) xmin = nd.x;
            if (nd.x > xmax) xmax = nd.x;
            if (nd.y < ymin) ymin = nd.y;
            if (nd.y > ymax) ymax = nd.y;
        }
        double xrange = xmax - xmin;
        double yrange = ymax - ymin;
        m_worldExtent = std::max({xrange, yrange, 1.0}) * 0.55;

        // 首次加载时自动居中并适配画布
        if (m_viewScale == 1.0 && m_viewOffset == QVector2D(0, 0) &&
            xrange > 0 && yrange > 0) {
            double margin = 40.0;
            double scaleX = (width() - margin) / xrange;
            double scaleY = (height() - margin) / yrange;
            m_viewScale = std::min(scaleX, scaleY);
            m_viewOffset = QVector2D(
                -(xmin + xmax) * 0.5 * m_viewScale,
                -(ymin + ymax) * 0.5 * m_viewScale
                );
        }
    }
}

// ═══════════════════════════════════════════
// AuthorGraphWindow 实现
// ═══════════════════════════════════════════
AuthorGraphWindow::AuthorGraphWindow(QueryFunc queryFn, QWidget* parent)
    : QMainWindow(parent), m_queryFn(std::move(queryFn)) {
    setWindowTitle("作者合作网络");
    resize(1450, 880);

    auto* central = new QWidget;
    setCentralWidget(central);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    m_graphCanvas = new GraphGLWidget(this);
    root->addWidget(m_graphCanvas, 1);

    buildSidePanel();
    root->addWidget(sidePanelWidget, 0);

    // 连接节点点击信号
    connect(m_graphCanvas, &GraphGLWidget::nodeClicked,
            [this](const std::string& name,
                   const std::vector<std::pair<std::string,int>>& neighbors,
                   const std::vector<std::string>& papers) {
                m_nameLabel->setText(QString::fromStdString("👤 " + name));
                m_statsLabel->setText(QString("🔗 合作者 %1 人 | 论文 %2 篇")
                                          .arg(neighbors.size()).arg(papers.size()));

                QString info;
                info += "【合作者】\n";
                int show = std::min(static_cast<int>(neighbors.size()), 150);
                for (int i = 0; i < show; ++i)
                    info += QString::fromStdString(neighbors[i].first) +
                            "  (合著 " + QString::number(neighbors[i].second) + " 篇)\n";
                if (static_cast<int>(neighbors.size()) > 150)
                    info += "\n... 共 " + QString::number(neighbors.size()) + " 人\n";

                info += "\n【论文】\n";
                for (const auto& p : papers)
                    info += QString::fromStdString(p) + "\n";

                m_infoView->setText(info);
            });

    // 加载数据
    QTimer::singleShot(10, this, &AuthorGraphWindow::loadData);
}

void AuthorGraphWindow::buildSidePanel() {
    sidePanelWidget = new QWidget;
    sidePanelWidget->setMinimumWidth(270);
    sidePanelWidget->setMaximumWidth(360);

    auto* lay = new QVBoxLayout(sidePanelWidget);
    lay->setContentsMargins(12,12,12,12);

    auto* titleLbl = new QLabel("📋 节点详情");
    titleLbl->setStyleSheet("font-size:15px; font-weight:bold; color:#2c3e50;");
    lay->addWidget(titleLbl);

    auto* h1 = new QFrame; h1->setFrameShape(QFrame::HLine); h1->setStyleSheet("color:#ddd;");
    lay->addWidget(h1);

    m_nameLabel = new QLabel("点击图中节点查看");
    m_nameLabel->setWordWrap(true);
    m_nameLabel->setStyleSheet("font-size:14px; font-weight:bold; color:#2980b9;");
    lay->addWidget(m_nameLabel);

    m_statsLabel = new QLabel;
    m_statsLabel->setStyleSheet("font-size:12px; color:#555;");
    lay->addWidget(m_statsLabel);

    auto* h2 = new QFrame; h2->setFrameShape(QFrame::HLine); h2->setStyleSheet("color:#ddd;");
    lay->addWidget(h2);

    auto* collabHdr = new QLabel("🔗 合作者 / 📄 论文:");
    collabHdr->setStyleSheet("font-size:12px; font-weight:bold; color:#444;");
    lay->addWidget(collabHdr);

    m_infoView = new QTextEdit;
    m_infoView->setReadOnly(true);
    m_infoView->setStyleSheet(
        "font-size:11px; font-family:Consolas,monospace;"
        "color:#1a1a2e; background:#f4f5f9; border:1px solid #ccd0da; border-radius:4px; padding:4px;");
    lay->addWidget(m_infoView, 1);
}

void AuthorGraphWindow::loadData() {
    if (!QFile::exists(coordsPath())) {
        if (!buildNetworkFromCSV(m_nodes, m_adj)) {
            qWarning() << "Failed to build network from CSV";
            return;
        }
    } else {
        // 从 JSON 加载 coords
        QFile fc(coordsPath());
        if (fc.open(QIODevice::ReadOnly)) {
            QJsonArray arr = QJsonDocument::fromJson(fc.readAll()).array();
            fc.close();
            m_nodes.resize(arr.size());
            for (int i = 0; i < arr.size(); ++i) {
                auto o = arr[i].toObject();
                m_nodes[i].id = i+1;
                m_nodes[i].x = o["x"].toDouble();
                m_nodes[i].y = o["y"].toDouble();
                m_nodes[i].name = o["name"].toString().toStdString();
                m_nodes[i].paperIds = o["paper_ids"].toString().toStdString();
                m_nodes[i].paperCount = o["paper_count"].toInt();
            }
        }
        QFile fa(adjacencyPath());
        if (fa.open(QIODevice::ReadOnly)) {
            QJsonObject adjObj = QJsonDocument::fromJson(fa.readAll()).object();
            fa.close();
            for (auto it = adjObj.begin(); it != adjObj.end(); ++it) {
                int nodeId = it.key().toInt();
                QJsonArray neigh = it.value().toArray();
                for (const auto& val : neigh) {
                    QJsonArray pair = val.toArray();
                    if (pair.size() >= 2)
                        m_adj[nodeId].push_back({pair[0].toInt(), pair[1].toInt()});
                }
            }
        }
    }

    m_graphCanvas->setGraph(&m_nodes, &m_adj, m_queryFn);
}
