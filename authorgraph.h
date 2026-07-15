#ifndef AUTHORGRAPH_H
#define AUTHORGRAPH_H

#include <QMainWindow>




// Qt6 用法；若用 Qt5，改为 #include <QOpenGLWidget>（去掉 s）和 #include <QOpenGLFunctions>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>

#include <QMatrix4x4>
#include <QVector2D>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QTextEdit>

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

extern "C" {
#include <igraph.h>
}

// ---- 节点信息 ----
struct GraphNode {
    int id;               // 1‑based
    double x, y;
    std::string name;
    std::string paperIds; // 论文ID字符串
    int paperCount;
};

struct NeighborInfo {
    int id;
    int weight;           // 合著次数
};

using AdjacencyMap = std::unordered_map<int, std::vector<NeighborInfo>>;
using QueryFunc = std::function<std::vector<std::string>(const std::string&)>;

// ---- OpenGL 渲染部件 ----
class GraphGLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit GraphGLWidget(QWidget* parent = nullptr);
    ~GraphGLWidget() override;

    void setGraph(const std::vector<GraphNode>* nodes,
                  const AdjacencyMap* adj,
                  QueryFunc queryFn);

signals:
    void nodeClicked(const std::string& authorName,
                     const std::vector<std::pair<std::string,int>>& neighbors,
                     const std::vector<std::string>& paperTitles);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    void prepareGeometry();
    QPointF toWorldCoords(const QPoint& screenPos);
    int findNearestNode(const QPointF& worldPos);

    const std::vector<GraphNode>* m_nodes = nullptr;
    const AdjacencyMap* m_adj = nullptr;
    QueryFunc m_queryFn;

    // 着色器与 VBO
    QOpenGLShaderProgram* m_program = nullptr;
    QOpenGLVertexArrayObject* m_vao = nullptr;
    QOpenGLBuffer *m_nodeVBO = nullptr, *m_edgeVBO = nullptr;

    int m_nodeCount = 0;
    int m_edgeCount = 0;

    // 视图变换
    QVector2D m_viewOffset{0,0};
    double m_viewScale = 1.0;
    QMatrix4x4 m_proj;
    double m_worldExtent = 100.0;

    // 高亮状态
    int m_selectedIdx = -1;
    std::vector<int> m_highlightNeighbors;
    std::vector<float> m_highlightNodeVertices;
    std::vector<float> m_highlightEdgesVertices;

    QPoint m_lastMousePos;   // ← 修正：原为 QPointF，与 e->pos() 的 QPoint 不匹配
};

// ---- 主窗口 ----
class AuthorGraphWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit AuthorGraphWindow(QueryFunc queryFn, QWidget* parent = nullptr);

private:
    void loadData();
    void buildSidePanel();

    GraphGLWidget* m_graphCanvas = nullptr;
    QWidget* sidePanelWidget = nullptr;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_statsLabel = nullptr;
    QTextEdit* m_infoView = nullptr;

    std::vector<GraphNode> m_nodes;
    AdjacencyMap m_adj;
    QueryFunc m_queryFn;
};

#endif // AUTHORGRAPH_H
