#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <QtGui>
#include <QtSql>

#define START_INDEX 491

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton_Torikumi, SIGNAL(clicked()), this, SLOT(convertTorikumi3456()));
    connect(ui->pushButton_Hoshitori, SIGNAL(clicked()), this, SLOT(convertHoshitori()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

QStringList Shikonas, Kimarite;

void readShikonas()
{
    QFile file0("shikonas.tsv");

    if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "error: file.open(shikonas.tsv)";
        return;
    }

   QTextStream in(&file0);

    while (!in.atEnd())
    {
        QString line = in.readLine();
        if (!line.isEmpty() && !line.isNull())
        {
            Shikonas << line.simplified().split(" ");
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
            Kimarite << line.simplified().split(" - ");
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

QString translateShikona(QString shikona)
{
    for (int i = 0; i < Shikonas.count(); i++)
    {
        if (shikona == Shikonas.at(i))
            return (Shikonas.at(i + 1));
    }
    return shikona;
}

QString translateKimarite(QString kimarite)
{
    for (int i = 0 + 1 + 1; i < Kimarite.count(); i++, i++, i++)
    {
        if (kimarite == Kimarite.at(i + 1 + 1))
            return (Kimarite.at(i));
    }
    return kimarite;
}

bool MainWindow::convertHoshitori()
{
    readKimarite();
    readShikonas();

    // Makuuchi
    for (int i = 1; i <= 3; i++)
    {
        // hoshi_545_1_1.html
        QFile file0("hoshi_"
                    + QString::number((ui->comboBox_year->currentIndex() * 6) + ui->comboBox_basho->currentIndex() + START_INDEX)
                    + "_1_"
                    + QString::number(i)
                    + ".html");

        QFile file1("hoshi_"
                    + QString::number(ui->comboBox_year->currentIndex() + 2002)
                    + "-"
                    + QString::number(ui->comboBox_basho->currentIndex() + 1)
                    + "-"
                    + QString::number(ui->comboBox_division->currentIndex() + 1)
                    + ".html");

        if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "error: file.open(hoshi_...)";
            return false;
        }

        if (!file1.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            qDebug() << "error: file.open(hoshitori.html)";
            return false;
        }

        QTextStream in(&file0);
        QTextStream out(&file1);

        in.setCodec("EUC-JP");

        QString content = in.readAll();

        //QStringList list = readAndSimplifyBashoContent(content);
        content.truncate(content.indexOf(QString("<!-- /BASYO CONTENTS -->")));
        content = content.mid(content.indexOf(QString("<!-- BASYO CONTENTS -->")));

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
        //out << QString::fromUtf8("<th>Кимаритэ</th>\n");
        out << QString::fromUtf8("<th></th>\n");    // Результат
        out << QString::fromUtf8("<th>Запад</th>\n");
        //out << QString::fromUtf8("<th>Счет</th>\n");
        //out << QString::fromUtf8("<th>Ранг</th>\n");
        out << "</tr>\n";
        out << "</thead>\n\n";

        out << "<tbody>\n";

        while (content.indexOf("hoshitori_riki1") != -1)
        {
            content = content.mid(content.indexOf("hoshitori_riki1"));
            content = content.mid(content.indexOf("strong"));
            content = content.mid(content.indexOf(">") + 1);
            QString shikona = content.left(content.indexOf("<"));
            int x = content.indexOf("class=\"hoshitori_riki3-1\">");
            int z = content.indexOf("hoshitori_riki1");
            QString xxboshi = "";
            do
            {
            if (x > 0)
            {
                content = content.mid(x);
                content = content.mid(content.indexOf(">") + 1);
                xxboshi += content.left(content.indexOf("<"));

                x = content.indexOf("class=\"hoshitori_riki3-1\">");
                z = content.indexOf("hoshitori_riki1");
                if (z < 0) z = 1000000;
            }
            }
            while ((x > 0) && (x < z));

            qDebug() << shikona << xxboshi;
            if ((content.indexOf("&nbsp;") != -1)
                && ((content.indexOf("&nbsp;") < (content.indexOf("hoshitori_riki1")))
                 || (content.indexOf("hoshitori_riki1") == -1)))
                qDebug() << "-----";
        }

        /*while (i < list.count())
        {
            if (list.value(i) == ("strong"))
            {
                out << "<tr class=" + className[trClass] + ">\n";

                //out << "<td>" << list.value(i +  1) << "</td>\n";   // rank 1

                out << "<td><strong>" << translateShikona(list.value(i +  1)) << "</strong>";   // shikona 1

                QString sum = list.value(i + 4);
                sum.replace(QString::fromUtf8("勝"), QString("-"));
                sum.replace(QString::fromUtf8("敗"), QString(""));
                out << sum << "<br />";

                out << "<td>" << list.value(i + 6) << "</td>\n";   // bout 1

                //out << "<td>" << translateKimarite(list.value(i + 8)) << "</td>\n";   // kimarite

                out << "</tr>\n\n";

                i += 15;
                trClass ^= 1;
            }
            i++;
        }*/

        file0.close();
        file1.close();
    }

    return true;
}

void splitRank (QString kanjiRank, int *side, int *rank, int *pos)
{
    if (kanjiRank == QString::fromUtf8("幕付出"))
    {
        *side = 2;
        *rank = 7;
        *pos  = 15;
        return;
    }

    *side = kanjiRank.startsWith(QString::fromUtf8("東")) ? 0:1;
    kanjiRank = kanjiRank.mid(1);

    if (kanjiRank.startsWith(QString::fromUtf8("序")))
    {
        *rank = 10;
        *pos = kanjiRank.mid(1).toInt();
    }

    if (kanjiRank.startsWith(QString::fromUtf8("二")))
    {
        *rank = 9;
        *pos = kanjiRank.mid(1).toInt();
    }

    if (kanjiRank.startsWith(QString::fromUtf8("三")))
    {
        *rank = 8;
        *pos = kanjiRank.mid(1).toInt();
    }

    if (kanjiRank.startsWith(QString::fromUtf8("幕")))
    {
        *rank = 7;
        *pos = kanjiRank.mid(1).toInt();
    }

    if (kanjiRank.startsWith(QString::fromUtf8("十")))
    {
        *rank = 6;
        *pos = kanjiRank.mid(1).toInt();
    }

    if (kanjiRank.startsWith(QString::fromUtf8("前")))
    {
        *rank = 5;
        *pos = kanjiRank.mid(1).toInt();
    }

    if (kanjiRank.startsWith(QString::fromUtf8("小結")))
    {
        *rank = 4;
        *pos = kanjiRank.mid(2).toInt();
    }

    if (kanjiRank.startsWith(QString::fromUtf8("関脇")))
    {
        *rank = 3;
        *pos = kanjiRank.mid(2).toInt();
    }

    if (kanjiRank.startsWith(QString::fromUtf8("大関")))
    {
        *rank = 2;
        *pos = kanjiRank.mid(2).toInt();
    }

    if (kanjiRank.startsWith(QString::fromUtf8("横綱")))
    {
        *rank = 1;
        *pos = kanjiRank.mid(2).toInt();
    }
}

void torikumi2Banzuke (void)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ozumo");
    db.setDatabaseName("/mnt/memory/ozumo.sqlite");
    if (!db.open())
        QMessageBox::warning(0, ("Unable to open database"),
                             ("An error occurred while opening the connection: ") + db.lastError().text());

    QSqlQuery query(db), queryOut(db);

    query.exec("DROP TABLE IF EXISTS banzuke");

    query.exec("CREATE TABLE \"banzuke\" ("
               "\"year\" INTEGER, "
               "\"month\" INTEGER, "
               "\"rank\" INTEGER, "
               "\"position\" INTEGER, "
               "\"side\" INTEGER, "
               "\"rikishi\" INTEGER, "
               "\"shikona\" VARCHAR)");

    query.exec("SELECT year, month, rank1, rikishi1, shikona1 FROM torikumi "
               "UNION "
               "SELECT year, month, rank2, rikishi2, shikona2 FROM torikumi");

    while (query.next())
    {
        int year        = query.value(0).toInt();
        int month       = query.value(1).toInt();
        QString rank    = query.value(2).toString();
        int rikishi     = query.value(3).toInt();
        QString shikona = query.value(4).toString();

        int side, rankN, pos;
        splitRank(rank, &side, &rankN, &pos);

        queryOut.prepare("INSERT INTO banzuke (year, month, rank, position, side, rikishi, shikona)"
                         "VALUES (:year, :month, :rank, :position, :side, :rikishi, :shikona)");
        queryOut.bindValue(":year", year);
        queryOut.bindValue(":month", month);
        queryOut.bindValue(":rank", rankN);
        queryOut.bindValue(":position", pos);
        queryOut.bindValue(":side", side);
        queryOut.bindValue(":rikishi", rikishi);
        queryOut.bindValue(":shikona", shikona);
        queryOut.exec();
    }
}

bool MainWindow::convertTorikumi3456()
{
    torikumi2Banzuke();
    return true;


    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ozumo");
    db.setDatabaseName("/mnt/memory/ozumo.sqlite");
    if (!db.open())
        QMessageBox::warning(this, tr("Unable to open database"),
                             tr("An error occurred while opening the connection: ") + db.lastError().text());

    QSqlQuery query(db);

    for (int basho = START_INDEX; basho <= 545; basho++)
    {
        for (int day = 1; day <= 15; day++)
        {
            for (int division = 3; division <= 6; division++)
            {
                QFile file0("/mnt/memory/torikumi/"
                            + QString("tori_")
                            + QString::number(basho)
                            + "_"
                            + QString::number(division)
                            + "_"
                            + QString::number(day)
                            + ".html");

                if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                    qDebug() << "error: file.open(/mnt/memory/torikumi/tori_...)";
                    return false;
                }

                QTextStream in(&file0);

                in.setCodec("EUC-JP");

                QString content = in.readAll();

                content.truncate(content.indexOf(QString("<!-- /BASYO CONTENTS -->")));
                content = content.mid(content.indexOf(QString("<!-- BASYO CONTENTS -->"))).simplified();

                int dayx = day, id_local = 0;

                while (content.indexOf("torikumi_riki2") != -1)
                {
                    content = content.mid(content.indexOf("torikumi_riki2"));
                    content = content.mid(content.indexOf(">") + 1);
                    QString rank1 = content.left(content.indexOf("<")).simplified();

                    content = content.mid(content.indexOf("torikumi_riki4"));
                    content = content.mid(content.indexOf(">") + 1);
                    QString shikona1 = content.left(content.indexOf("<")).simplified();

                    content = content.mid(content.indexOf("torikumi_riki3"));
                    content = content.mid(content.indexOf(">") + 1);
                    QString result1 = content.left(content.indexOf("<")).simplified();

                    content = content.mid(content.indexOf("torikumi_riki3"));
                    content = content.mid(content.indexOf(">") + 1);
                    if (content.startsWith("<a"))
                        content = content.mid(content.indexOf(">") + 1);
                    QString kimarite = content.left(content.indexOf("<")).simplified();

                    content = content.mid(content.indexOf("torikumi_riki3"));
                    content = content.mid(content.indexOf(">") + 1);
                    QString result2 = content.left(content.indexOf("<")).simplified();

                    content = content.mid(content.indexOf("torikumi_riki4"));
                    content = content.mid(content.indexOf(">") + 1);
                    QString shikona2 = content.left(content.indexOf("<")).simplified();
                    if (shikona2.contains(QRegExp("\\d+")))
                    {
                        content = content.mid(content.indexOf("torikumi_riki4"));
                        content = content.mid(content.indexOf(">") + 1);
                        shikona2 = content.left(content.indexOf("<")).simplified();
                    }
                    else
                        dayx = 16;

                    content = content.mid(content.indexOf("torikumi_riki2"));
                    content = content.mid(content.indexOf(">") + 1);
                    QString rank2 = content.left(content.indexOf("<")).simplified();

                    query.prepare("INSERT INTO torikumi (id, basho, day, rikishi1, shikona1, rank1, result1, "
                                  "rikishi2, shikona2, rank2, result2, kimarite, id_local) "
                                  "VALUES (:id, :basho, :day, :rikishi1, :shikona1, :rank1, :result1, "
                                  ":rikishi2, :shikona2, :rank2, :result2, :kimarite, :id_local)");

                    ++id_local;
                    int index = (((((2002 + (basho - START_INDEX) / 6) * 10 + (basho - START_INDEX) % 6 + 1) * 100)+ day) * 10 + division) * 100 + id_local;
                    query.bindValue(":id", index);
                    query.bindValue(":basho", basho);
                    query.bindValue(":day", dayx);
                    query.bindValue(":rikishi1", 0);
                    query.bindValue(":shikona1", shikona1);
                    query.bindValue(":rank1", rank1);
                    query.bindValue(":result1", result1 == QString::fromUtf8("○") ? 1:0);
                    query.bindValue(":rikishi2", 0);
                    query.bindValue(":shikona2", shikona2);
                    query.bindValue(":rank2", rank2);
                    query.bindValue(":result2", result2 == QString::fromUtf8("○") ? 1:0);
                    query.bindValue(":kimarite", kimarite);
                    query.bindValue(":id_local", id_local);
                    if (!query.exec())
                    {
                        qDebug() << "-";
                    };
                }
            }
        }
    }

    QSqlDatabase::database("ozumo", false).close();
    QSqlDatabase::removeDatabase("ozumo");

    return true;
}

bool MainWindow::convertTorikumi()
{
    //collectShikonas();
    //return true;

    readKimarite();
    readShikonas();

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ozumo");
    db.setDatabaseName("/mnt/memory/ozumo.sqlite");
    if (!db.open())
        QMessageBox::warning(this, tr("Unable to open database"),
                             tr("An error occurred while opening the connection: ") + db.lastError().text());

    QSqlQuery query(db);

    for (int basho = START_INDEX; basho <= 545; basho++)
    {
        for (int day = 1; day <= 15; day++)
        {
            for (int division = 1; division <= 2; division++)
            {
                QFile file0("/mnt/memory/torikumi-makuuchi/"
                            + QString("tori_")
                            + QString::number(basho)
                            + "_"
                            + QString::number(division)
                            + "_"
                            + QString::number(day)
                            + ".html");

                if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                    qDebug() << "error: file.open(/mnt/memory/torikumi/tori_...)";
                    return false;
                }

                QTextStream in(&file0);

                in.setCodec("EUC-JP");

                QString content = in.readAll();

                QStringList list = readAndSimplifyBashoContent(content);

                int i = 0;
                int dayx = day, id_local = 0;
                while (i < list.count())
                {
                    if (list.value(i).contains("rank"))
                    {
                        //東前14 栃栄 3勝0敗 ○ 押し出し ● 寺尾 0勝3敗 西十2

                        QString rank1 = list.value(i +  1);
                        //out << "<td>" << list.value(i +  1) << "</td>\n";   // rank 1

                        QString shikona1 = list.value(i +  3);
                        //out << "<td><strong>" << translateShikona(list.value(i +  3)) << "</strong><br />";   // shikona 1

                        QString sum = list.value(i + 4);
                        if (sum.contains(QRegExp("\\d+")))
                        {
                            sum.replace(QString::fromUtf8("勝"), QString("-"));
                            sum.replace(QString::fromUtf8("敗"), QString(""));
                        }
                        else
                        {
                            sum = "";
                            dayx = 16;
                            i--;
                        }
                        //out << sum << "</td>\n";   // +- 1

                        QString result1 = list.value(i +  6);
                        //out << "<td>" << list.value(i + 6) << "</td>\n";   // bout 1

                        QString kimarite = list.value(i +  8);
                        //out << "<td>" << translateKimarite(list.value(i + 8)) << "</td>\n";   // kimarite

                        QString result2 = list.value(i + 10);
                        //out << "<td>" << list.value(i + 10) << "</td>\n";   // bout 2

                        QString shikona2 = list.value(i + 12);
                        //out << "<td><strong>" << translateShikona(list.value(i + 12)) << "</strong><br />";   // shikona 2

                        sum = list.value(i + 13);
                        if (sum.contains(QRegExp("\\d+")))
                        {
                            sum.replace(QString::fromUtf8("勝"), QString("-"));
                            sum.replace(QString::fromUtf8("敗"), QString(""));
                        }
                        else
                        {
                            sum = "";
                            dayx = 16;
                            i--;
                        }
                        //out << sum << "</td>\n";   // +- 2

                        QString rank2 = list.value(i + 15);
                        //out << "<td>" << list.value(i + 15) << "</td>\n";   // rank 2

                        QString id = QString::number(basho) + QString::number(day).rightJustified(2, '0');

                        /*QString id = list.value(i +  1) + " "
                                 + list.value(i +  3) + " "
                                 + QString(list.value(i +  4).contains(QRegExp("\\d+")) ? list.value(i +  4) : "-") + " "
                                 + list.value(i +  6) + " "
                                 + list.value(i +  8) + " "
                                 + list.value(i + 10) + " "
                                 + list.value(i + 12) + " "
                                 + QString(list.value(i + 13).contains(QRegExp("\\d+")) ? list.value(i + 13) : "-") + " "
                                 + list.value(i + 15);*/

                        query.prepare("INSERT INTO torikumi (id, basho, day, rikishi1, shikona1, rank1, result1, "
                                      "rikishi2, shikona2, rank2, result2, kimarite, id_local) "
                                      "VALUES (:id, :basho, :day, :rikishi1, :shikona1, :rank1, :result1, "
                                      ":rikishi2, :shikona2, :rank2, :result2, :kimarite, :id_local)");

                        ++id_local;
                        int index = (((((2002 + (basho - START_INDEX) / 6) * 10 + (basho - START_INDEX) % 6 + 1) * 100)+ day) * 10 + division) * 100 + id_local;
                        query.bindValue(":id", index);
                        query.bindValue(":basho", basho);
                        query.bindValue(":day", dayx);
                        query.bindValue(":rikishi1", 0);
                        query.bindValue(":shikona1", shikona1);
                        query.bindValue(":rank1", rank1);
                        query.bindValue(":result1", result1 == QString::fromUtf8("○") ? 1:0);
                        query.bindValue(":rikishi2", 0);
                        query.bindValue(":shikona2", shikona2);
                        query.bindValue(":rank2", rank2);
                        query.bindValue(":result2", result2 == QString::fromUtf8("○") ? 1:0);
                        query.bindValue(":kimarite", kimarite);
                        query.bindValue(":id_local", id_local);
                        if (!query.exec())
                        {
                            qDebug() << "-";
                        };
                        //qDebug() << query.lastQuery();

                        //out << "<!-- Original data: " << id << " -->\n";

                        //out << "</tr>\n\n";

                        i += 15;
                    }
                    i++;
                }

                file0.close();
            }
        }
    }

    QSqlDatabase::database("ozumo", false).close();
    QSqlDatabase::removeDatabase("ozumo");

/*
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

            QString id = list.value(i +  1) + " "
                         + list.value(i +  3) + " "
                         + QString(list.value(i +  4).contains(QRegExp("\\d+")) ? list.value(i +  4) : "-") + " "
                         + list.value(i +  6) + " "
                         + list.value(i +  8) + " "
                         + list.value(i + 10) + " "
                         + list.value(i + 12) + " "
                         + QString(list.value(i + 13).contains(QRegExp("\\d+")) ? list.value(i + 13) : "-") + " "
                         + list.value(i + 15);

            out << "<!-- Original data: " << id << " -->\n";

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
*/
    return true;
}

