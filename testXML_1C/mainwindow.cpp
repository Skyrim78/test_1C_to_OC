#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connectOC();
    connect(ui->toolButton_file, SIGNAL(clicked(bool)), this, SLOT(loadFile()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectOC()
{
    db_server = QSqlDatabase::addDatabase("QMYSQL");
    db_server.setHostName("127.0.0.1");
    db_server.setDatabaseName("oc2302");
    //db_server.setDatabaseName("ocStore21021");
    db_server.setPort(3306);
    db_server.setUserName("root");
    db_server.setPassword("");
    db_server.open();
    qDebug() << "Server: " << db_server.isOpen();

}

void MainWindow::loadFile()
{
    ui->lineEdit_file->setText(QFileDialog::getOpenFileName(this, "Open...", "/home/ev", "XML (*.xml)"));
    QFile file(ui->lineEdit_file->text());
    if (file.open(QIODevice::ReadOnly)){
        doc = new QDomDocument();
        doc->setContent(&file);
        readCategory();
    }
    file.close();
}

void MainWindow::readCategory()
{
    QDomNode nodeCom = doc->childNodes().at(1); //коммерч информация
    QDomNode nodeCl = nodeCom.childNodes().at(0); //классификатор

    QDomNode nodeGroups = nodeCl.childNodes().at(2);

    for (int x = 0; x < nodeGroups.childNodes().count(); x++){
        QDomNode nodeGroup = nodeGroups.childNodes().at(x);

        QString _ID = nodeGroup.firstChildElement("Ид").text();
        QString _NAME = nodeGroup.firstChildElement("Наименование").text();
        QString _PARENT = nodeGroup.firstChildElement("Родитель").text();

        //test
        QSqlQuery queryTest(QString("SELECT oc_category_description.category_id "
                                    "FROM oc_category_description "
                                    "WHERE oc_category_description.meta_keyword = \'%1\' ")
                            .arg(_ID));
        queryTest.next();
        if (queryTest.isValid()){
            int _id = queryTest.value(0).toInt();

            int _parent;
            int _top;
            if (_PARENT.size() == 0){
                _parent = 0;
                _top = 1;
            }  else {
                QSqlQuery queryParent(QString("SELECT oc_category_description.category_id "
                                            "FROM oc_category_description "
                                            "WHERE oc_category_description.meta_keyword = \'%1\' ")
                                    .arg(_PARENT));
                queryParent.next();
                qDebug() << "queryParent for update: " << queryParent.lastError();
                _parent = queryParent.value(0).toInt();
                _top = 0;
            }

            QSqlQuery queryUP(QString("UPDATE oc_category SET `parent_id` = \'%1\', `top` = \'%2\', `date_modified` = \'%3\' "
                                      "WHERE oc_category.category_id = \'%4\' ")
                              .arg(_parent)
                              .arg(_top)
                              .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:dd"))
                              .arg(_id));
            queryUP.exec();

            if (queryUP.lastError().text().size() > 3){
                qDebug() << "queryUP for update: " << queryUP.lastError().text();
                break;
            } else {
                QSqlQuery queryDescUP(QString("UPDATE oc_category_description SET name = \'%1\', description = \'%2\', meta_keyword = \'%3\' "
                                              "WHERE oc_category_description.category_id = \'%4\' ")
                                      .arg(_NAME)
                                      .arg(_NAME)
                                      .arg(_ID)
                                      .arg(_id));
                queryDescUP.exec();
                qDebug() << "queryDescUP for update: " << queryDescUP.lastError();
            }



        } else {
            int _id;
            int _parent;
            int _top;
            if (_PARENT.size() == 0){
                _parent = 0;
                _top = 1;
            }  else {
                QSqlQuery queryParent(QString("SELECT oc_category_description.category_id "
                                            "FROM oc_category_description "
                                            "WHERE oc_category_description.meta_keyword = \'%1\' ")
                                    .arg(_PARENT));
                queryParent.next();
                qDebug() << "queryParent for insert: " << queryParent.lastError();
                _parent = queryParent.value(0).toInt();
                _top = 0;
            }

            QSqlQuery queryIn("INSERT INTO `oc_category` (`parent_id`, `top`, `status`, `sort_order`, `date_added`, `date_modified`) "
                              "VALUES (?, ?, ?, ?, ?, ?)");
            queryIn.bindValue(0, _parent);
            queryIn.bindValue(1, _top);
            queryIn.bindValue(2, 1);
            queryIn.bindValue(3, 0);
            queryIn.bindValue(4, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:dd"));
            queryIn.bindValue(5, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:dd"));
            queryIn.exec();

            _id = queryIn.lastInsertId().toInt();

            if (queryIn.lastError().text().size() > 3){
                qDebug() << "queryIn for insert: " << queryIn.lastError().text();
                break;
            } else {
                QSqlQuery queryDesc("INSERT INTO oc_category_description (category_id, language_id, name, description, meta_keyword) "
                                    "VALUES (?, ?, ?, ?, ?)");
                queryDesc.bindValue(0, _id);
                queryDesc.bindValue(1, 1);
                queryDesc.bindValue(2, _NAME);
                queryDesc.bindValue(3, _NAME);
                queryDesc.bindValue(4, _ID);
                queryDesc.exec();

                qDebug() << "queryDesc for insert: " << queryDesc.lastError();

                QSqlQuery queryStore("INSERT INTO oc_category_to_store (category_id, store_id) "
                                     "VALUES (?, ?)");
                queryStore.bindValue(0, _id);
                queryStore.bindValue(1, 0);
                queryStore.exec();

                qDebug() << "queryStore for insert: " << queryStore.lastError();
            }
        }
    }
    QStringList pathList;
    QSqlQuery queryS6_Del("DELETE FROM oc_category_path");
    queryS6_Del.exec();
    qDebug() << "S6 - server | delete oc_categor_path -- err: " << queryS6_Del.lastError().text();

    //выборка категорий
    QSqlQuery queryS6_a ("SELECT oc_category.category_id, oc_category.parent_id FROM oc_category");
    while (queryS6_a.next()){
        tempList.clear();
        int id = queryS6_a.value(0).toInt();
        int parent = queryS6_a.value(1).toInt();
        tempList.append(QString("%1|%1").arg(id));
        if (parent > 0){
            makeCategoryPath(id, parent);
        }

        int level = tempList.size() - 1;
        for (int x = 0; x < tempList.size(); x++){
            pathList.append(QString("%1|%2").arg(tempList.at(x)).arg(level));
            level--;
        }
    }
    //путь категорий в таб oc_cayegory_padd
    if (pathList.size() > 0){
        for (int x = 0; x < pathList.size(); x++){
               QSqlQuery queryS6_b("INSERT INTO oc_category_path (category_id, path_id, level) "
                                   "VALUES (?, ?, ?) ", db_server);
               queryS6_b.bindValue(0, pathList.at(x).split("|").at(0));
               queryS6_b.bindValue(1, pathList.at(x).split("|").at(1));
               queryS6_b.bindValue(2, pathList.at(x).split("|").at(2));
               queryS6_b.exec();
        }
    }
}

void MainWindow::makeCategoryPath(int ID, int parent)
{
    tempList.append(QString("%1|%2").arg(ID).arg(parent));
    QSqlQuery query(QString("SELECT oc_category.parent_id FROM oc_category WHERE oc_category.category_id = \'%1\' ")
                    .arg(parent), db_server);
    query.next();
    int id_parent = query.value(0).toInt();
    if (id_parent > 0){
        makeCategoryPath(ID, id_parent);
    }
}
