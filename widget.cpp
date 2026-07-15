#include "widget.h"
#include "./ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);


    if (!existRepo()) {
        cout << "BUILDING_REPO" << endl;
        reset();
        scan(path);
    } else {
        cout << "LOADING" << endl;
        paperIndexTree = new BPlusTree;
        paperIndexTree->load();
        authorIndexTree = new StringBPlusTree;
        authorIndexTree->load(authorIndexPath);
        titleIndexTree = new StringBPlusTree;
        titleIndexTree->load(titleIndexPath);
    }
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pushButton_4_clicked()
{
    if (pythonProcess && pythonProcess->state() != QProcess::NotRunning) {
        logWidget->append("[提示] Python 进程已在运行中");
        return;
    }
    pythonProcess = new QProcess(this);

    pythonProcess->execute("./vis/AuthorGraph.exe");
}



void Widget::on_pushButton_clicked()
{
    vector<uint64_t> tem;
    if (QFile::exists("./maximal_cliques_ids.csv"))
        tem=load_and_compute("./maximal_cliques_ids.csv");
    else
        tem=makeCliques("./");
    vector<string>res;
    for(auto& t:tem){
        res.push_back(std::to_string(t));
    }
    ResultDialog *dialog = new ResultDialog(res);
    dialog->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动释放内存
    dialog->show();
}


void Widget::on_pushButton_2_clicked()
{
    getTop();
}


void Widget::on_pushButton_3_clicked()
{
    map<string, vector<string>> ans = parseTop10();
    vector<string>tem;
    for (const auto&  it : ans) {
        // cout << it.first << ": "<<endl;
        tem.push_back("The year of: "+it.first);
        string t;
        for (const string& kw : it.second)
            // cout << kw << ", ";
            t=t+kw+" ";
        tem.push_back(t);
    }
    ResultDialog *dialog = new ResultDialog(tem);
    dialog->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动释放内存
    dialog->show();
}


void Widget::on_gotoSearch_clicked()
{
    int curState=ui->comboBox->currentIndex();
    //0123
    //标题，作者到paper，作者到team，关键字
    string line=ui->inputLine->text().toStdString();
    while(line[line.size()-1]==' '||line[line.size()-1]=='\n')
        line.pop_back();
    // cout<<line<<endl;
    vector<string>tem;
    switch (curState){
    case 0:{
        vector<Paper>noe=Title2Papers(line);
        for(const Paper& e:noe){
            tem.push_back("Title: "+e.title);
            string ath;
            for(const string &ea:e.author)
                ath=ath+ea+", ";
            tem.push_back("Authors: "+ath);
            tem.push_back("from jouranl: "+e.journal+"in: "+e.volume);
            tem.push_back("Time: "+e.year+"/"+e.month);
            tem.push_back("ee: "+e.ee+" and cdrom: "+e.cdrom);
        }

        break;
    }
    case 1:{
        tem=Author2Papers(line);
        break;
    }
    case 2:{
        tem=teamWith(line);
        break;
    }
    case 3:{
        build_keyTree();
        vector<string> keys;
        stringstream ss(line);
        string token;
        while (ss >> token) keys.push_back(token);
        vector<Paper> t = key_query(keys);
        tem.push_back("Here are the matched paper titles.");
        for(const Paper& ea:t){
            tem.push_back(ea.title);
        }
        break;
    }

    }
    if(tem.empty()){
        tem.push_back("No results...");
        std::cout<<"error no data found."<<endl;
    }
    ResultDialog *dialog = new ResultDialog(tem);
    dialog->setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动释放内存
    dialog->show();
}



