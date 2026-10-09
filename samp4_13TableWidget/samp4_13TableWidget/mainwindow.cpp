#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "rosterdata.h"

#include    <QDate>
#include    <QTableWidgetItem>
#include    <QRandomGenerator>
#include    <QSplitter>


//为一行的单元格创建 Items
void MainWindow::createItemsARow(int rowNo,QString name,QString sex,QDate birth,QString nation,bool isPM,int score)
{
    uint studID=202105000;  //学号基数
    //姓名
    QTableWidgetItem *item=new  QTableWidgetItem(name, MainWindow::ctName);   //数据项类型为MainWindow::ctName
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    studID  +=rowNo;        //学号 =基数 + 行号
    item->setData(Qt::UserRole,QVariant(studID));           //设置studID为用户数据
    ui->tableInfo->setItem(rowNo,MainWindow::colName,item);

    //性别
    QIcon   icon;
    if (sex=="男")
        icon.addFile(":/images/icons/boy.ico");
    else
        icon.addFile(":/images/icons/girl.ico");

    item=new  QTableWidgetItem(sex,MainWindow::ctSex);      //type为MainWindow::ctSex
    item->setIcon(icon);
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    Qt::ItemFlags flags=Qt::ItemIsSelectable |Qt::ItemIsEnabled;    //不允许编辑
    item->setFlags(flags);
    ui->tableInfo->setItem(rowNo,MainWindow::colSex,item);  //为单元格设置Item

    //出生日期
    QString str=birth.toString("yyyy-MM-dd");   //日期转换为字符串
    item=new  QTableWidgetItem(str,MainWindow::ctBirth);        //type为MainWindow::ctBirth
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);   //文本对齐格式
    ui->tableInfo->setItem(rowNo,MainWindow::colBirth,item);

    //民族
    item=new  QTableWidgetItem(nation,MainWindow::ctNation);        //type为MainWindow::ctNation
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableInfo->setItem(rowNo,MainWindow::colNation,item);

    //是否党员
    item=new  QTableWidgetItem("党员",MainWindow::ctPartyM);      //type为 MainWindow::ctPartyM
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    flags= Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |Qt::ItemIsEnabled;   //不允许编辑，但可以更改复选状态
    item->setFlags(flags);
    if (isPM)
        item->setCheckState(Qt::Checked);
    else
        item->setCheckState(Qt::Unchecked);
    item->setBackground(QBrush(Qt::yellow));   //设置背景颜色
    ui->tableInfo->setItem(rowNo,MainWindow::colPartyM,item);

    //分数
    str.setNum(score);
    item=new  QTableWidgetItem(str,MainWindow::ctScore);    //type为MainWindow::ctPartyM
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ui->tableInfo->setItem(rowNo,MainWindow::colScore,item);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //    setCentralWidget(ui->splitterMain);

    //状态栏初始化创建
    labCellIndex = new QLabel("当前单元格坐标：",this);
    labCellIndex->setMinimumWidth(550);

    labCellType=new QLabel("当前单元格类型：",this);
    labCellType->setMinimumWidth(200);

    labStudID=new QLabel("学生ID：",this);
    labStudID->setMinimumWidth(200);

    //作业二：状态栏新增一个QLabel，用于显示选中行学生的籍贯
    labHometown=new QLabel("籍贯：-",this);
    labHometown->setMinimumWidth(200);

    ui->statusBar->addWidget(labCellIndex); //添加到状态栏
    ui->statusBar->addWidget(labCellType);
    ui->statusBar->addWidget(labStudID);
    ui->statusBar->addWidget(labHometown);

    //本人学号：用于在名单中定位“前两行 + 自己 + 后两行”
    m_selfId = QStringLiteral("2024414300107");

    //作业二：让右侧表格占据更多高度，保证 5 行名单全部可见
    ui->splitter->setSizes(QList<int>() << 250 << 90);
}

MainWindow::~MainWindow()
{
    delete ui;
}


//设置水平表头
void MainWindow::on_btnSetHeader_clicked()
{
    //作业二：学生名单模式下列结构固定，不允许被旧功能覆盖
    if (ui->tableInfo->columnCount()==RosterColumnCount)
    {
        ui->statusBar->showMessage(QStringLiteral("当前为学生名单模式，此操作不适用"),3000);
        return;
    }

    QStringList headerText;
    headerText<<"姓名"<<"性别"<<"出生日期"<<"民族"<<"分数"<<"是否党员";
    //    ui->tableInfo->setHorizontalHeaderLabels(headerText); //只设置标题
    ui->tableInfo->setColumnCount(headerText.size());      //设置表格列数
    for (int i=0;i<ui->tableInfo->columnCount();i++)
    {
        QTableWidgetItem *headerItem=new QTableWidgetItem(headerText.at(i));
        QFont font=headerItem->font();   //获取原有字体设置
        font.setBold(true);              //设置为粗体
        font.setPointSize(11);           //字体大小
        headerItem->setForeground(QBrush(Qt::red));  //设置文字颜色
        headerItem->setFont(font);       //设置字体
        ui->tableInfo->setHorizontalHeaderItem(i,headerItem);    //设置表头单元格的item
    }
}

//设置行数,设置的行数为数据区的行数，不含表头
void MainWindow::on_btnSetRows_clicked()
{
    if (ui->tableInfo->columnCount()==RosterColumnCount)
    {
        ui->statusBar->showMessage(QStringLiteral("当前为学生名单模式，此操作不适用"),3000);
        return;
    }

    ui->tableInfo->setRowCount(ui->spinRowCount->value());//设置数据区行数
    ui->tableInfo->setAlternatingRowColors(ui->chkBoxRowColor->isChecked()); //设置交替行背景颜色
}


//初始化表格数据
void MainWindow::on_btnIniData_clicked()
{
    if (ui->tableInfo->columnCount()==RosterColumnCount)
    {
        ui->statusBar->showMessage(QStringLiteral("当前为学生名单模式，此操作不适用"),3000);
        return;
    }

    QDate   birth(2001,4,6);        //初始化一个日期
    ui->tableInfo->clearContents(); //只清除工作区，不清除表头
    for (int i=0; i<ui->tableInfo->rowCount(); i++)
    {
        QString strName=QString("学生%1").arg(i);
        QString strSex= ((i % 2)==0)? "男":"女";
        bool isParty= ((i % 2)==0)? false:true;
        int score= QRandomGenerator::global()->bounded(60,100);   //随机数[60,100)
        createItemsARow(i, strName, strSex, birth,"汉族",isParty,score);  //为某一行创建items
        birth=birth.addDays(20);    //日期加20天
    }
}

void MainWindow::on_chkBoxTabEditable_clicked(bool checked)
{ //设置表格是否可编辑，以及进入编辑模式的方式
    if (checked)
        //双击或获取焦点后单击，进入编辑状态
        ui->tableInfo->setEditTriggers(QAbstractItemView::DoubleClicked
                                       | QAbstractItemView::SelectedClicked);
    else
        ui->tableInfo->setEditTriggers(QAbstractItemView::NoEditTriggers); //不允许编辑
}

void MainWindow::on_chkBoxHeaderH_clicked(bool checked)
{//是否显示水平表头
    ui->tableInfo->horizontalHeader()->setVisible(checked);
}

void MainWindow::on_chkBoxHeaderV_clicked(bool checked)
{//是否显示垂直表头
    ui->tableInfo->verticalHeader()->setVisible(checked);
}

void MainWindow::on_chkBoxRowColor_clicked(bool checked)
{ //行的底色交替采用不同颜色
    ui->tableInfo->setAlternatingRowColors(checked);
}

void MainWindow::on_rBtnSelectItem_clicked()
{//选择方式：单元格选择
    ui->tableInfo->setSelectionBehavior(QAbstractItemView::SelectItems);
}

void MainWindow::on_rBtnSelectRow_clicked()
{//选择方式：行选择
    ui->tableInfo->setSelectionBehavior(QAbstractItemView::SelectRows);
}


//将 QTableWidget的所有行的内容提取字符串，显示在QPlainTextEdit里
void MainWindow::on_btnReadToEdit_clicked()
{
    QTableWidgetItem    *item;

    ui->textEdit->clear();  //文本编辑器清空
    for (int i=0;i<ui->tableInfo->rowCount();i++)   //逐行处理
    {
        QString str=QString::asprintf("第 %d 行： ",i+1);

        if (ui->tableInfo->columnCount()==RosterColumnCount)
        {
            //作业二：名单模式，7列全部按文本输出
            for (int j=0;j<ui->tableInfo->columnCount();j++)
            {
                item=ui->tableInfo->item(i,j);
                str=str+(item?item->text():QString())+"   ";
            }
        }
        else
        {
            for (int j=0;j<ui->tableInfo->columnCount()-1;j++) //逐列处理，但最后一列是check型，单独处理
            {
                item=ui->tableInfo->item(i,j);      //获取单元格的item
                if (item)
                    str=str+item->text()+"   ";     //字符串连接
            }
            item=ui->tableInfo->item(i,colPartyM);  //最后一列，党员
            if (item && item->checkState()==Qt::Checked) //根据check状态显示文字
                str=str+"党员";
            else
                str=str+"群众";
        }
        ui->textEdit->appendPlainText(str);     //添加到编辑框作为一行
    }
}

//currentCellChanged()信号的槽函数，当前单元格发生变化时的响应
void MainWindow::on_tableInfo_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(previousRow);
    Q_UNUSED(previousColumn);

    QTableWidgetItem* item=ui->tableInfo->item(currentRow,currentColumn); //获取单元格的Item
    if  (item==nullptr)
        return;

    labCellIndex->setText(QString::asprintf("当前单元格坐标：%d 行，%d 列",currentRow,currentColumn));

    int cellType=item->type();      //获取单元格的类型
    labCellType->setText(QString::asprintf("当前单元格类型：%d",cellType));

    item=ui->tableInfo->item(currentRow,MainWindow::colName);   //取当前行第1列的单元格的item
    uint ID=item->data(Qt::UserRole).toUInt();        //读取用户数据
    labStudID->setText(QString::asprintf("学生ID：%d",ID));      //学生ID

    //作业二：显示当前行学生的籍贯。名单模式下籍贯存放在“姓名”item的UserRole里
    QString hometown;
    if (ui->tableInfo->columnCount()==RosterColumnCount)
    {
        QTableWidgetItem *nameItem=ui->tableInfo->item(currentRow,colRosterName);
        if (nameItem)
            hometown=nameItem->data(Qt::UserRole).toString();
    }
    labHometown->setText(QStringLiteral("籍贯：")+(hometown.isEmpty()?QStringLiteral("-"):hometown));
}

//插入一行
void MainWindow::on_btnInsertRow_clicked()
{
    int curRow=ui->tableInfo->currentRow();     //当前行号

    ui->tableInfo->insertRow(curRow);           //插入一行，但不会自动为单元格创建item
    createItemsARow(curRow, "新学生", "男",
                    QDate::fromString("2002-10-1","yyyy-M-d"),"苗族",true,80 ); //为某一行创建items
}

//添加一行
void MainWindow::on_btnAppendRow_clicked()
{
    int curRow=ui->tableInfo->rowCount();       //当前行号
    ui->tableInfo->insertRow(curRow);           //在表格尾部添加一行
    createItemsARow(curRow, "新生", "女",
                    QDate::fromString("2002-6-5","yyyy-M-d"),"满族",false,76 ); //为某一行创建items
}

//删除当前行及其items
void MainWindow::on_btnDelCurRow_clicked()
{
    int curRow=ui->tableInfo->currentRow();     //当前行号
    ui->tableInfo->removeRow(curRow);           //删除当前行及其items
}

void MainWindow::on_btnAutoHeght_clicked()
{
    ui->tableInfo->resizeRowsToContents();
}

void MainWindow::on_btnAutoWidth_clicked()
{
    ui->tableInfo->resizeColumnsToContents();
}

/**
 * 作业二：工具栏“设置学生名单”按钮的槽函数。
 * 按本人学号在周一/周四两份点名册里定位，取“前两行 + 自己 + 后两行”共 5 行，
 * 重新设置右侧 tableWidget 的内容（7 列），并把本人学号、姓名的单元格设为粗体红色。
 */
void MainWindow::on_actSetRoster_triggered()
{
    buildRosterTable();
}

void MainWindow::buildRosterTable()
{
    //表头文字（与作业要求一致）
    const char *headerText[RosterColumnCount] = {
        "学号", "姓名", "性别", "行政班级", "院(系)/部", "专业", "修读性质"
    };

    //1. 在周一、周四两份名单里查找本人学号
    const RosterRow *rows = nullptr;
    int rowCount = 0;
    int selfIndex = -1;

    for (int i = 0; i < kMondayRosterCount && selfIndex < 0; ++i)
        if (m_selfId == QString::fromUtf8(kMondayRoster[i].id))
            selfIndex = i;
    if (selfIndex >= 0)
    {
        rows = kMondayRoster;
        rowCount = kMondayRosterCount;
    }
    else
    {
        for (int i = 0; i < kThursdayRosterCount && selfIndex < 0; ++i)
            if (m_selfId == QString::fromUtf8(kThursdayRoster[i].id))
                selfIndex = i;
        if (selfIndex >= 0)
        {
            rows = kThursdayRoster;
            rowCount = kThursdayRosterCount;
        }
    }

    if (selfIndex < 0)
    {
        ui->statusBar->showMessage(
            QStringLiteral("在点名册中未找到学号 %1").arg(m_selfId), 5000);
        return;
    }

    //2. 取“前两行 + 自己 + 后两行”，左右边界不足 5 行时向内收缩
    const int rowSpan = 5;
    int begin = selfIndex - 2;
    if (begin + rowSpan > rowCount)
        begin = rowCount - rowSpan;
    if (begin < 0)
        begin = 0;
    const int selfRow = selfIndex - begin;    //本人在表格中的行号

    //3. 重建表格：5 行 7 列
    ui->tableInfo->clear();                   //同时清除表头与所有 item
    ui->tableInfo->setColumnCount(RosterColumnCount);
    ui->tableInfo->setRowCount(rowSpan);

    for (int c = 0; c < RosterColumnCount; ++c)
    {
        QTableWidgetItem *headerItem = new QTableWidgetItem(QString::fromUtf8(headerText[c]));
        QFont font = headerItem->font();
        font.setBold(true);
        font.setPointSize(11);
        headerItem->setForeground(QBrush(Qt::red));
        headerItem->setFont(font);
        ui->tableInfo->setHorizontalHeaderItem(c, headerItem);
    }

    //4. 逐行填入数据
    for (int r = 0; r < rowSpan; ++r)
    {
        const RosterRow &row = rows[begin + r];
        const QString sex = QString::fromUtf8(row.sex);

        //学号
        QTableWidgetItem *item = new QTableWidgetItem(
            QString::fromUtf8(row.id), MainWindow::ctName);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui->tableInfo->setItem(r, colRosterId, item);
        QTableWidgetItem *idItem = item;

        //姓名：附带籍贯信息（存进 UserRole）
        item = new QTableWidgetItem(QString::fromUtf8(row.name), MainWindow::ctName);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setData(Qt::UserRole, QString::fromUtf8(row.hometown));
        ui->tableInfo->setItem(r, colRosterName, item);
        QTableWidgetItem *nameItem = item;

        //性别：带男/女图标，不允许编辑
        item = new QTableWidgetItem(sex, MainWindow::ctSex);
        QIcon icon;
        icon.addFile(sex == QStringLiteral("女") ? ":/images/icons/girl.ico"
                                                 : ":/images/icons/boy.ico");
        item->setIcon(icon);
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui->tableInfo->setItem(r, colRosterSex, item);

        //行政班级
        item = new QTableWidgetItem(QString::fromUtf8(row.cls));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableInfo->setItem(r, colRosterClass, item);

        //院(系)/部
        item = new QTableWidgetItem(QString::fromUtf8(row.dept));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableInfo->setItem(r, colRosterDept, item);

        //专业
        item = new QTableWidgetItem(QString::fromUtf8(row.major));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableInfo->setItem(r, colRosterMajor, item);

        //修读性质
        item = new QTableWidgetItem(QString::fromUtf8(row.nature));
        item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        ui->tableInfo->setItem(r, colRosterNature, item);

        //本人行：学号与姓名单元格设为粗体、红色
        if (r == selfRow)
        {
            for (QTableWidgetItem *selfItem : {idItem, nameItem})
            {
                QFont font = selfItem->font();
                font.setBold(true);
                selfItem->setFont(font);
                selfItem->setForeground(QBrush(Qt::red));
            }
        }
    }

    //5. 列宽与选中方式
    ui->tableInfo->resizeColumnsToContents();
    for (int c = 0; c < RosterColumnCount; ++c)
        if (ui->tableInfo->columnWidth(c) > 180)
            ui->tableInfo->setColumnWidth(c, 180);   //过宽的列收窄，避免占满整屏
    ui->tableInfo->setSelectionBehavior(QAbstractItemView::SelectRows);

    //6. 不自动选中本人行，保证“粗体红色”清晰可见；籍贯等选中行后再显示
    ui->tableInfo->clearSelection();
    ui->tableInfo->setCurrentItem(nullptr);
    labHometown->setText(QStringLiteral("籍贯：-"));
    ui->statusBar->clearMessage();   //不能用临时消息，否则会遮住状态栏上的籍贯标签
}
