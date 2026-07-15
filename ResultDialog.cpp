#include "ResultDialog.h"
#include <QHeaderView>

ResultDialog::ResultDialog(const std::vector<std::string> &tem, QWidget *parent)
    : QDialog(parent)
{
    setupUI(tem);
}

ResultDialog::~ResultDialog() {}

void ResultDialog::setupUI(const std::vector<std::string> &tem)
{
    setWindowTitle("Results");
    resize(500, 400);
    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setReadOnly(true);                    // 只读
    m_textEdit->setFont(QFont("Consolas", 10));       // 等宽字体，方便对齐
    // 拼接所有字符串，每行一个
    QString allText;
    for (const auto &line : tem) {
        allText += QString::fromStdString(line) + "\n";
    }
    m_textEdit->setPlainText(allText);
    // 布局
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_textEdit);
    setLayout(layout);



}
