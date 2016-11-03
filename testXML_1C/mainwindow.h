#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtXml/QtXml>
#include <QFileDialog>
#include <QtSql/QtSql>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QDomDocument *doc;
    QSqlDatabase db_server;
    QStringList tempList;

private:
    Ui::MainWindow *ui;

public slots:
    void connectOC();
    void makeCategoryPath(int ID, int parent);

    void loadFile();
    void readCategory();
};

#endif // MAINWINDOW_H
