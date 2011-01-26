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

QStringList Shikonas, Kimarite;

void readShikonas()
{
    QFile file0("shikonas.txt");

    if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

   QTextStream in(&file0);

    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (!line.isEmpty() && !line.isNull())
        {
            QString szPair = line.trimmed();
            QStringList list = szPair.split(" ");
            Shikonas << list;
        }
    }

    file0.close();
    //qDebug() << Shikonas;
}

void readKimarite()
{
    QFile file0("kimarite.txt");

    if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

   QTextStream in(&file0);

    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (!line.isEmpty() && !line.isNull())
        {
            QString szPair = line.trimmed();
            QStringList list = szPair.split(" - ");
            Kimarite << list;
        }
    }

    file0.close();
    //qDebug() << Kimarite;
}

int collectShikonas()
{
    int listCount = 0;
    QStringList listShikonas;

    for (int b = 491; b <= 545; b++)
    {
        for (int d = 1; d <= 15; d++)
        {
            QString fName = "/mnt/memory/x/tori_" + QString::number(b) + "_1_" + QString::number(d) + ".html";
            QFile file0(fName);
            QFile file1("/mnt/memory/x/a.txt");

            if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                return -1;
            }

            if (!file1.open(QIODevice::Append | QIODevice::Text))
            {
                return -1;
            }

            QTextStream in(&file0);
            QTextStream out(&file1);

            in.setCodec("EUC-JP");

            QString content = in.readAll();
            content.replace(">", "<");
            content.replace("\n", " ");
            QStringList list = content.split("<");

            int i = 0;
            while (i < list.count())
            {
                if (list.value(i).contains("torikumi_riki2"))
                {
                    out << (list.value(i +  9)) << "\n";   // shikona 1

                    int hack;
                    if (list.value(i + 27).contains(QString::fromUtf8("不戦")))
                        hack = -4;
                    else
                        hack = 0;
                    out << (list.value(i + 43 + hack)) << "\n";   // shikona 2

                    listShikonas << list.value(i +  9);
                    listCount++;
                    listShikonas << list.value(i + 43 + hack);
                    listCount++;
                    i += 57;
                }
                i++;
            }

            file0.close();
            file1.close();
        }
    }

    //qDebug() << listCount;

    listShikonas.removeDuplicates();
    qDebug() << listShikonas.count();

    QFile file1("/mnt/memory/x/aa.txt");
    QTextStream out(&file1);
    if (!file1.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return -1;
    }

    for (int i = 0; i < listShikonas.count(); i++)
    {
        out << listShikonas.at(i) << "\n";
    }
    file1.close();

    return listCount;
}

QString translateShikona(QString shikona)
{
    for (int i = 0; i < Shikonas.count(); i++, i++)
    {
        if (shikona == Shikonas.at(i))
            return (Shikonas.at(i + 1));
    }
    return shikona;
}

QString translateKimarite(QString kimarite)
{
    for (int i = 0; i < Kimarite.count(); i++, i++, i++)
    {
        if (kimarite == Kimarite.at(i + 1 + 1))
            return (Kimarite.at(i));
    }
    return kimarite;
}

bool MainWindow::convert_torikumi()
{
    readKimarite();
    //return true;

    readShikonas();

    QFile file0(ui->lineEdit->text()
                + "/tori_"
                + QString::number((ui->comboBox_year->currentIndex() * 6) + ui->comboBox_basho->currentIndex() + 491)
                + "_1_"
                + QString::number(ui->comboBox_day->currentIndex() + 1)
                + ".html");

    QFile file1("torikumi.html");

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
    content.truncate(content.indexOf(QString("<!-- /BASYO CONTENTS -->")));
    content = content.mid(content.indexOf(QString("<!-- BASYO CONTENTS -->")));

    content.replace(QRegExp("<table([^<]*)>"), "");
    content.replace(QRegExp("</table>"), "");

    content.replace(QRegExp("<a([^<]*)>"), "");
    content.replace(QRegExp("</a>"), "");

    content.replace(QRegExp("<br>"), "<>");

    content.replace(QRegExp("</td>"), "");

    content.replace(QRegExp("<tr>"), "");
    content.replace(QRegExp("</tr>"), "");

    content.replace(QRegExp("<span([^<]*)>"), "");
    content.replace(QRegExp("</span>"), "");

    content.replace(QRegExp("<div([^<]*)>"), "");
    content.replace(QRegExp("</div>"), "");

    content.replace(QRegExp("<td([^<]*)class=\"torikumi_riki1\">"), "<shikona>");
    content.replace(QRegExp("<td([^<]*)class=\"torikumi_riki2\">"), "<rank>");
    content.replace(QRegExp("<td([^<]*)class=\"torikumi_riki3\">"), "<result>");

    content.replace(">", "<");
    content = content.simplified();
    content.replace("< <", "<");
    content = content.simplified();
    QStringList list = content.split("<", QString::SkipEmptyParts);

    for (int i = 0; i < list.count(); i++)
    {
        list.replace(i, list.value(i).simplified());
    }
    //qDebug() << list;

    int i = 0;
    int trClass = 0;
    QString className[] = {"\"odd\"", "\"even\""};

    out << "<table>\n";
    out << "<thead>\n";
    out << "<tr>\n";
    //out << QString::fromUtf8("<th>Ранг</th>\n");
    out << QString::fromUtf8("<th>Восток</th>\n");
    //out << QString::fromUtf8("<th>Счет</th>\n");
    out << QString::fromUtf8("<th></th>\n");    // Результат
    out << QString::fromUtf8("<th>Кимаритэ</th>\n");
    out << QString::fromUtf8("<th></th>\n");    // Результат
    out << QString::fromUtf8("<th>Запад</th>\n");
    //out << QString::fromUtf8("<th>Счет</th>\n");
    //out << QString::fromUtf8("<th>Ранг</th>\n");
    out << "</tr>\n";
    out << "</thead>\n\n";

    out << "<tbody>\n";
    while (i < list.count())
    {
        if (list.value(i).contains("rank"))
        {
            out << "<tr class=" + className[trClass] + ">\n";

            //out << "<td>" << list.value(i +  1) << "</td>\n";   // rank 1

            out << "<td><strong>" << translateShikona(list.value(i +  3)) << "</strong><br />";   // shikona 1

            QString sum = list.value(i + 4);
            sum.replace(QString::fromUtf8("勝"), QString("-"));
            sum.replace(QString::fromUtf8("敗"), QString(""));
            out << sum << "</td>\n";   // +- 1
            out << "<td>" << list.value(i + 6) << "</td>\n";   // bout 1

            out << "<td>" << translateKimarite(list.value(i + 8)) << "</td>\n";   // kimarite

            out << "<td>" << list.value(i + 10) << "</td>\n";   // bout 2

            out << "<td><strong>" << translateShikona(list.value(i + 12)) << "</strong><br />";   // shikona 2

            sum = list.value(i + 13);
            sum.replace(QString::fromUtf8("勝"), QString("-"));
            sum.replace(QString::fromUtf8("敗"), QString(""));
            out << sum << "</td>\n";   // +- 2

            //out << "<td>" << list.value(i + 15) << "</td>\n";   // rank 2

            out << "</tr>\n\n";

            i += 15;
            trClass ^= 1;
        }
        i++;
    }
    out << "</tbody>\n";
    out << "</table>\n";

    file0.close();
    file1.close();

    return true;
}
