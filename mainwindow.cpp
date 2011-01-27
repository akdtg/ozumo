#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <QtGui>

#define START_INDEX 491

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
        qDebug() << "error: file.open(shikonas.txt)";
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
        qDebug() << "error: file.open(kimarite.txt)";
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

QStringList readAndSimplifyBashoContent(QString content)
{
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

    content.replace(QRegExp("<div class=\"torikumi_gyoji\">([^<]*)</div>"), "");
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
    return list;
}

int collectShikonas()
{
    int listCount = 0;
    QStringList listShikonas;

    for (int basho = START_INDEX; basho <= 545; basho++)
    {
        for (int day = 1; day <= 15; day++)
        {
            for (int division = 1; division <= 2; division++)
            {
                QString fName = "/mnt/memory/torikumi/tori_"
                                + QString::number(basho) + "_"
                                + QString::number(division) + "_"
                                + QString::number(day) + ".html";
                QFile file0(fName);
                QFile file1("/mnt/memory/s-all.txt");

                if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                    qDebug() << "error: file.open(/mnt/memory/torikumi/tori_...)";
                    return -1;
                }

                if (!file1.open(QIODevice::Append | QIODevice::Text))
                {
                    qDebug() << "error: file.open(/mnt/memory/s-all.txt)";
                    return -1;
                }

                QTextStream in(&file0);
                QTextStream out(&file1);

                in.setCodec("EUC-JP");

                QString content = in.readAll();

                QStringList list = readAndSimplifyBashoContent(content);

                int i = 0;
                while (i < list.count())
                {
                    if (list.value(i).contains("rank"))
                    {
                        out << (list.value(i +  3)) << "\n";   // shikona 1

                        listShikonas << list.value(i +  3);
                        listCount++;

                        QString sum = list.value(i + 4);
                        if (sum.contains(QRegExp("\\d+")))
                        {
                        }
                        else
                        {
                            sum = "";
                            i--;
                        }

                        out << (list.value(i + 12)) << "\n";   // shikona 2

                        listShikonas << list.value(i + 12);
                        listCount++;

                        sum = list.value(i + 13);
                        if (sum.contains(QRegExp("\\d+")))
                        {
                        }
                        else
                        {
                            sum = "";
                            i--;
                        }

                        i += 15;
                    }
                    i++;
                }

                file0.close();
                file1.close();
            }
        }
    }

    //qDebug() << listCount;

    listShikonas.removeDuplicates();
    qDebug() << listShikonas.count();

    QFile file1("/mnt/memory/s-nodup.txt");
    QTextStream out(&file1);
    if (!file1.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "error: file.open(/mnt/memory/s-nodup.txt)";
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
    //collectShikonas();
    //return true;

    readKimarite();
    readShikonas();

    QFile file0(ui->lineEdit->text()
                + "/tori_"
                + QString::number((ui->comboBox_year->currentIndex() * 6) + ui->comboBox_basho->currentIndex() + START_INDEX)
                + "_"
                + QString::number(ui->comboBox_division->currentIndex() + 1)
                + "_"
                + QString::number(ui->comboBox_day->currentIndex() + 1)
                + ".html");

    //"tori-2011-1-15-m.html";
    QString toriFname= "tori-"
                       + QString::number(ui->comboBox_year->currentIndex() + 2002)
                       + "-"
                       + QString::number(ui->comboBox_basho->currentIndex() + 1)
                       + "-"
                       + QString::number(ui->comboBox_day->currentIndex() + 1)
                       + "-"
                       + QString::number(ui->comboBox_division->currentIndex() + 1)
                       + ".html";

    QFile file1(toriFname);

    if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "error: file.open(/mnt/memory/torikumi/tori_...)";
        return false;
    }

    if (!file1.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "error: file.open(torikumi.html)";
        return false;
    }

    QTextStream in(&file0);
    QTextStream out(&file1);

    in.setCodec("EUC-JP");

    QString content = in.readAll();

    QStringList list = readAndSimplifyBashoContent(content);

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

            QString id = list.value(i +  1) + " "
                         + list.value(i +  3) + " "
                         + QString(list.value(i +  4).contains(QRegExp("\\d+")) ? list.value(i +  4) : "-") + " "
                         + list.value(i +  6) + " "
                         + list.value(i +  8) + " "
                         + list.value(i + 10) + " "
                         + list.value(i + 12) + " "
                         + QString(list.value(i + 13).contains(QRegExp("\\d+")) ? list.value(i + 13) : "-") + " "
                         + list.value(i + 15);

            out << "<!--" << id << "-->\n";

            //out << "<td>" << list.value(i +  1) << "</td>\n";   // rank 1

            out << "<td><strong>" << translateShikona(list.value(i +  3)) << "</strong><br />";   // shikona 1

            QString sum = list.value(i + 4);
            if (sum.contains(QRegExp("\\d+")))
            {
                sum.replace(QString::fromUtf8("勝"), QString("-"));
                sum.replace(QString::fromUtf8("敗"), QString(""));
            }
            else
            {
                sum = "";
                i--;
            }
            out << sum << "</td>\n";   // +- 1

            out << "<td>" << list.value(i + 6) << "</td>\n";   // bout 1

            out << "<td>" << translateKimarite(list.value(i + 8)) << "</td>\n";   // kimarite

            out << "<td>" << list.value(i + 10) << "</td>\n";   // bout 2

            out << "<td><strong>" << translateShikona(list.value(i + 12)) << "</strong><br />";   // shikona 2

            sum = list.value(i + 13);
            if (sum.contains(QRegExp("\\d+")))
            {
                sum.replace(QString::fromUtf8("勝"), QString("-"));
                sum.replace(QString::fromUtf8("敗"), QString(""));
            }
            else
            {
                sum = "";
                i--;
            }
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
