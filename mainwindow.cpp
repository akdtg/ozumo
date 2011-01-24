#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <QtGui>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton_Go, SIGNAL(clicked()), this, SLOT(convert_torikumi()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::convert_torikumi()
{
    QMessageBox::warning(this, QString::fromUtf8("Внимание"), QString::fromUtf8("Функция не реализована"), QMessageBox::Ok);

    QFile file0("/mnt/memory/tori_545_1_15.html");
    QFile file1("/mnt/memory/t.html");

    if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    if (!file1.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    file0.close();
    file1.close();

    return true;
}
