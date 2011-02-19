#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtSql>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    bool importTorikumi(QString fName);
    bool importAllTorikumi();
    void parsingTorikumi12(QString content, int basho, int year, int month, int day, int division);
    void parsingTorikumi3456(QString content, int basho, int year, int month, int day, int division);
    bool torikumi2Banzuke();
    bool findDate(QString content, int *year, int *month);
    bool insertTorikumi(int id, int basho, int year, int month, int day,
                        int division,
                        int rikishi1, QString shikona1, QString rank1, int result1,
                        int rikishi2, QString shikona2, QString rank2, int result2,
                        QString kimarite,
                        int id_local);
    void generateTorikumi();
    QString torikumi2Html(int year, int month, int day, int division);
    void generateTorikumiResults();
    QString torikumiResults2Html(int year, int month, int day, int division);
    void downloadTorikumi();
    int getAndImportTorikumi(int year, int month, int day, int division);
    bool importBanzuke(QString fName);
    bool parsingBanzuke12(QString content);
    bool parsingBanzuke3456(QString content);
    bool insertBanzuke(int year, int month, QString rank, int position, int side,
                       int rikishi_id, QString shikona);
    bool importAllHoshitori();
    bool importHoshitori(QString fName);
    bool parsingHoshitori12(QString content, int basho, int division, int side);
    bool parsingHoshitori3456(QString content, int basho, int division, int side);

public:
    QSqlDatabase db;

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
