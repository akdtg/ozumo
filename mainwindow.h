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
    bool convertTorikumi();
    bool convertHoshitori();
    bool convertTorikumi3456();
    bool torikumi2Banzuke();
    bool findDate(QString content, int *year, int *month);
    bool insertTorikumi(QSqlDatabase db,
                        int id, int basho, int year, int month, int day,
                        int division,
                        int rikishi1, QString shikona1, QString rank1, int result1,
                        int rikishi2, QString shikona2, QString rank2, int result2,
                        QString kimarite,
                        int id_local);
    QString torikumi2Html(int year, int month, int day, int division);
    void generateTorikumi();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
