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
    void importAllTorikumi();
    bool parsingTorikumi12(QString content, int basho, int year, int month, int day, int division);
    bool parsingTorikumi3456(QString content, int basho, int year, int month, int day, int division);
    bool insertTorikumi(int id, int basho, int year, int month, int day,
                        int division,
                        int rikishi1, QString shikona1, QString rank1, int result1,
                        int rikishi2, QString shikona2, QString rank2, int result2,
                        QString kimarite,
                        int id_local);

    void generateTorikumi();
    QString torikumi2Html(int year, int month, int day, int division);
    void generateResults();
    QString torikumiResults2Html(int year, int month, int day, int division);

    void downloadTorikumi();
    int getAndImportTorikumi(int year, int month, int day, int division);
    bool importBanzuke(QString fName);
    bool parsingBanzuke12(QString content);
    bool parsingBanzuke3456(QString content);
    bool insertBanzuke(int year, int month, QString rank, int position, int side,
                       int rikishi_id, QString shikona);
    void importAllHoshitori();
    bool importHoshitori(QString fName);
    bool parsingHoshitori12(QString content, int basho, int division, int side);
    bool parsingHoshitori3456(QString content, int basho, int division, int side);
    void downloadBanzuke();
    void generateHoshitori();
    QString hoshitori2Html(int year, int month, int day, int division);
    QString translateShikona(QString shikona);
    QString translateKimarite(QString kimarite);

    void statistics();

    void generateBBCodeTorikumi();
    QString torikumi2BBCode(int year, int month, int day, int division);
    void generateBBCodeResults();
    QString torikumiResults2BBCode(int year, int month, int day, int division);

public:
    QSqlDatabase db;

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
