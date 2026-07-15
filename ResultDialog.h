#ifndef RESULTDIALOG_H
#define RESULTDIALOG_H

#include <QDialog>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <vector>
#include <string>


// 数据结构（根据你项目中的实际结构调整）
class ResultDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResultDialog(const std::vector<std::string> &authors, QWidget *parent = nullptr);
    ~ResultDialog();

private:
    void setupUI(const std::vector<std::string> &authors);

    QPlainTextEdit *m_textEdit;
};

#endif // RESULTDIALOG_H
