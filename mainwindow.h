#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
