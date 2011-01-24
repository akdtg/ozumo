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

    QTextStream in(&file0);
    QTextStream out(&file1);

    in.setCodec("EUC-JP");

    QString content = in.readAll();
    content.replace(">", "<");
    content.replace("\n", " ");
    QStringList list = content.split("<");
    //qDebug() << list;

    int i = 0;
    int trClass = 0;
    QString className[] = {"\"odd\"", "\"even\""};

    out << "<table>\n";
    out << "<tr>\n";
    out << QString::fromUtf8("<th>Ранг</th>\n");
    out << QString::fromUtf8("<th>Сикона</th>\n");
    out << QString::fromUtf8("<th>Счет</th>\n");
    out << QString::fromUtf8("<th>Результат</th>\n");
    out << QString::fromUtf8("<th>Кимаритэ</th>\n");
    out << QString::fromUtf8("<th>Результат</th>\n");
    out << QString::fromUtf8("<th>Сикона</th>\n");
    out << QString::fromUtf8("<th>Счет</th>\n");
    out << QString::fromUtf8("<th>Ранг</th>\n");
    out << "</tr>\n\n";
    while (i < list.count())
    {
        if (list.value(i).contains("torikumi_riki2"))
        {
            out << "<tr class=" + className[trClass] + ">\n";
            out << "<td>" << list.value(i +  1) << "</td>\n";   // rank 1
            out << "<td>" << list.value(i +  9) << "</td>\n";   // shikona 1
            out << "<td>" << list.value(i + 17) << "</td>\n";   // +- 1
            out << "<td>" << list.value(i + 23) << "</td>\n";   // bout 1
            out << "<td>" << list.value(i + 29) << "</td>\n";   // kimarite
            out << "<td>" << list.value(i + 35) << "</td>\n";   // bout 2
            out << "<td>" << list.value(i + 51) << "</td>\n";   // shikona 2
            out << "<td>" << list.value(i + 43) << "</td>\n";   // +- 2
            out << "<td>" << list.value(i + 57) << "</td>\n";   // rank 2
            out << "</tr>\n\n";

            i += 57;
            trClass ^= 1;
        }
        i++;
    }
    out << "</table>\n";

    file0.close();
    file1.close();

    return true;
}
