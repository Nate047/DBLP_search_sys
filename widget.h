#ifndef WIDGET_H
#define WIDGET_H

#include"repo.h"
#include"scan.h"
#include"BplusTree.h"
#include"BplusTree_author.h"

#include"F1.h"
#include"F2.h"
#include"F3.h"
#include"F4.h"
#include"F5.h"
#include"F6.h"
// #include"authorgraph.h"

#include <QWidget>
#include <QProcess>

#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QFile>
#include<string>



QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    // void launchPythonGraph();
private slots:                          // ★ slots 必须在这里
    // void onPythonOutput();
    // void handlePaperIds(const QString& paperIds);
    // void onPythonStdoutAvailable();

    void on_pushButton_4_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_gotoSearch_clicked();


private:
    Ui::Widget *ui;
    QProcess*   pythonProcess = nullptr;
    QTextEdit*  logWidget     = nullptr;

};
#endif // WIDGET_H
