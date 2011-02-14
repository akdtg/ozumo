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

    connect(ui->pushButton_Torikumi, SIGNAL(clicked()), this, SLOT(convertTorikumi()));
    connect(ui->pushButton_Torikumi3456, SIGNAL(clicked()), this, SLOT(convertTorikumi3456()));
    connect(ui->pushButton_torikumi2Banzuke, SIGNAL(clicked()), this, SLOT(torikumi2Banzuke()));
    connect(ui->pushButton_Hoshitori, SIGNAL(clicked()), this, SLOT(convertHoshitori()));

    connect(ui->pushButton_generateTorikumi, SIGNAL(clicked()), this, SLOT(generateTorikumi()));
    connect(ui->pushButton_generateTorikumiResults, SIGNAL(clicked()), this, SLOT(generateTorikumiResults()));
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
    content = content.mid(content.indexOf(QString("<!-- /BASYO TITLE -->")));

    content.replace(QRegExp("<table([^<]*)>"), "");
    content.replace(QRegExp("</table>"), "");

    content.replace(QRegExp("<br>"), "<>");

    content.replace(QRegExp("</td>"), "");

    content.replace(QRegExp("<tr>"), "");
    content.replace(QRegExp("</tr>"), "");

    content.replace(QRegExp("<span([^<]*)>"), "");
    content.replace(QRegExp("</span>"), "");

    content.replace(QRegExp("<div class=\"torikumi_gyoji\">([^<]*)</div>"), "");
    content.replace(QRegExp("<div([^<]*)>"), "");
    content.replace(QRegExp("</div>"), "");

    content.replace(QRegExp("<td([^<]*)class=\"basho78-150\">"), "<date>");

    content.replace(QRegExp("<strong>"), "");
    content.replace(QRegExp("</strong>"), "");

    content.replace(QRegExp("<td([^<]*)class=\"torikumi_riki1\">"), "<shikona>");
    content.replace(QRegExp("<td([^<]*)class=\"torikumi_riki2\">"), "<rank>");
    content.replace(QRegExp("<td([^<]*)class=\"torikumi_riki3\">"), "<result>");

    content.replace(QRegExp("<a([^<]*)rikishi_(\\d+).html\">"), "<rikishi_id><\\2>");
    content.replace(QRegExp("<a([^<]*)kimarite/(\\d+).html\">"), "<kimarite_id><\\2>");

    content.replace(QRegExp("<a([^<]*)>"), "");
    content.replace(QRegExp("</a>"), "");

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

bool MainWindow::findDate(QString content, int *year, int *month)
{
    QStringList list = readAndSimplifyBashoContent(content);

    *year = 0;
    *month = 0;

    int i = 0;
    while (!list.value(i).contains("date"))
    {
        i++;
    }

    if (list.value(i).contains("date"))
    {
        QStringList tempList = list.value(i + 1).split(" ");
        QString date = tempList.at(3);
        tempList = date.split(QRegExp("(\\D+)"), QString::SkipEmptyParts);
        *year = tempList.at(0).toInt();
        *month = tempList.at(1).toInt();

        return true;
    }
    else
    {
        return false;
    }

}

bool MainWindow::insertTorikumi(QSqlDatabase db,
                                int id, int basho, int year, int month, int day,
                                int division,
                                int rikishi1, QString shikona1, QString rank1, int result1,
                                int rikishi2, QString shikona2, QString rank2, int result2,
                                QString kimarite,
                                int id_local)
{
    QSqlQuery query(db);

    query.prepare("INSERT INTO torikumi ("
                  "id, basho, year, month, day, division, "
                  "rikishi1, shikona1, rank1, result1, "
                  "rikishi2, shikona2, rank2, result2, "
                  "kimarite, "
                  "id_local) "
                  "VALUES ("
                  ":id, :basho, :year, :month, :day, :division, "
                  ":rikishi1, :shikona1, :rank1, :result1, "
                  ":rikishi2, :shikona2, :rank2, :result2, "
                  ":kimarite, "
                  ":id_local)");

    query.bindValue(":id",       id);
    query.bindValue(":basho",    basho);
    query.bindValue(":year",     year);
    query.bindValue(":month",    month);
    query.bindValue(":day",      day);
    query.bindValue(":division", division);
    query.bindValue(":rikishi1", rikishi1);
    query.bindValue(":shikona1", shikona1);
    query.bindValue(":rank1",    rank1);
    query.bindValue(":result1",  result1);
    query.bindValue(":rikishi2", rikishi2);
    query.bindValue(":shikona2", shikona2);
    query.bindValue(":rank2",    rank2);
    query.bindValue(":result2",  result2);
    query.bindValue(":kimarite", kimarite);
    query.bindValue(":id_local", id_local);

    return query.exec();
}

bool MainWindow::torikumi2Banzuke()
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

    db.close();

    return true;
}

QString MainWindow::torikumi2Html(int year, int month, int day, int division)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ozumo");
    db.setDatabaseName("/mnt/memory/ozumo.sqlite");
    if (!db.open())
        QMessageBox::warning(this, tr("Unable to open database"),
                             tr("An error occurred while opening the connection: ") + db.lastError().text());

    QSqlQuery query(db);

    query.prepare("SELECT id_local, shikona1, rank1, shikona2, rank2, basho "
               "FROM torikumi "
               "WHERE year = :year AND month = :month AND day = :day AND division = :division "
               "ORDER BY id_local");
    query.bindValue(":year", year);
    query.bindValue(":month", month);
    query.bindValue(":day", day);
    query.bindValue(":division", division);

    query.exec();

    int trClass = 0;
    QString className[] = {"\"odd\"", "\"even\""};
    QString Html = "<!-- year:" + QString::number(year)
                   + " month:" + QString::number(month).rightJustified(2, '0')
                   + " day:" + QString::number(day).rightJustified(2, '0')
                   + " division:" + QString::number(division)
                   + " -->\n";

    Html += "<table>\n"
            "<thead><tr>"
            "<th width=\"33%\">" + QString::fromUtf8("Восток") + "</th>"
            "<th width=\"33%\">" + QString::fromUtf8("История последних встреч") + "</th>"
            "<th width=\"33%\">" + QString::fromUtf8("Запад") + "</th></tr></thead>\n"
            "<tbody>\n";

    while (query.next())
    {
        /*qDebug()<< query.value(0).toInt()
                << query.value(1).toString()
                << query.value(2).toString()
                << query.value(3).toString()
                << query.value(4).toString();*/

        //int id = query.value(0).toInt();
        QString shikona1 = query.value(1).toString();
        QString rank1    = query.value(2).toString();
        QString shikona2 = query.value(3).toString();
        QString rank2    = query.value(4).toString();
        int basho = query.value(5).toInt();

        QString shikona1Ru = shikona1, shikona2Ru = shikona2;

        QSqlQuery tmpQuery(db);

        tmpQuery.prepare("SELECT ru FROM shikona WHERE kanji = :kanji");
        tmpQuery.bindValue(":kanji", shikona1);
        tmpQuery.exec();
        if (tmpQuery.next())
            shikona1Ru = tmpQuery.value(0).toString();

        tmpQuery.prepare("SELECT ru FROM shikona WHERE kanji = :kanji");
        tmpQuery.bindValue(":kanji", shikona2);
        tmpQuery.exec();
        if (tmpQuery.next())
            shikona2Ru = tmpQuery.value(0).toString();

        QString res1, res2;
        tmpQuery.prepare("SELECT COUNT (*) FROM torikumi WHERE year = :y AND month = :m AND day < :d "
                         "AND ((shikona1 = :s1 AND result1 = 1) OR ( shikona2 = :s2 AND result2 = 1))");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":d", day);
        tmpQuery.bindValue(":s1", shikona1);
        tmpQuery.bindValue(":s2", shikona1);
        tmpQuery.exec();
        if (tmpQuery.next())
            res1 += tmpQuery.value(0).toString() + "-";

        tmpQuery.prepare("SELECT COUNT (*) FROM torikumi WHERE year = :y AND month = :m AND day < :d "
                         "AND ((shikona1 = :s1 AND result1 = 0) OR ( shikona2 = :s2 AND result2 = 0))");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":d", day);
        tmpQuery.bindValue(":s1", shikona1);
        tmpQuery.bindValue(":s2", shikona1);
        tmpQuery.exec();
        if (tmpQuery.next())
            res1 += tmpQuery.value(0).toString();

        tmpQuery.prepare("SELECT COUNT (*) FROM torikumi WHERE year = :y AND month = :m AND day < :d "
                         "AND ((shikona1 = :s1 AND result1 = 1) OR ( shikona2 = :s2 AND result2 = 1))");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":d", day);
        tmpQuery.bindValue(":s1", shikona2);
        tmpQuery.bindValue(":s2", shikona2);
        tmpQuery.exec();
        if (tmpQuery.next())
            res2 += tmpQuery.value(0).toString() + "-";

        tmpQuery.prepare("SELECT COUNT (*) FROM torikumi WHERE year = :y AND month = :m AND day < :d "
                         "AND ((shikona1 = :s1 AND result1 = 0) OR ( shikona2 = :s2 AND result2 = 0))");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":d", day);
        tmpQuery.bindValue(":s1", shikona2);
        tmpQuery.bindValue(":s2", shikona2);
        tmpQuery.exec();
        if (tmpQuery.next())
            res2 += tmpQuery.value(0).toString();

        QString history;
        for (int i = 1; i <= 6; i++)
        {
            //SELECT result1, result2 FROM torikumi WHERE (shikona1 = "白鵬" AND shikona2 = "魁皇") AND basho = 545
            //UNION ALL
            //SELECT result2, result1 FROM torikumi WHERE (shikona2 = "白鵬" AND shikona1 = "魁皇") AND basho = 545
            tmpQuery.prepare("SELECT result1, result2 FROM torikumi WHERE shikona1 = :shikona1a AND shikona2 = :shikona2a AND basho = :bashoa "
                                 "UNION "
                                 "SELECT result2, result1 FROM torikumi WHERE shikona2 = :shikona1b AND shikona1 = :shikona2b AND basho = :bashob ");

            tmpQuery.bindValue(":shikona1a", shikona1);
            tmpQuery.bindValue(":shikona2a", shikona2);
            tmpQuery.bindValue(":shikona1b", shikona1);
            tmpQuery.bindValue(":shikona2b", shikona2);
            tmpQuery.bindValue(":bashoa", basho - i);
            tmpQuery.bindValue(":bashob", basho - i);
            tmpQuery.exec();
            if (tmpQuery.next())
            {
                QString r = tmpQuery.value(0).toInt() == 1 ? QString::fromUtf8("○"):QString::fromUtf8("●");
                history.prepend(r);
            }
            else
                history.prepend(QString::fromUtf8("－"));
            history.prepend(" ");
        }

        //qDebug() << shikona1Ru << shikona2Ru;

        Html += "<tr class=" + className[trClass] + ">"
                "<td>" + shikona1Ru + " (" + res1 + ")</td>"
                "<td>" + history + "</td>"
                "<td>" + shikona2Ru + " (" + res2 + ")</td></tr>\n";
        //qDebug() << Html;

        trClass ^= 1;
    }

    Html += "</tbody>\n</table>\n";

    db.close();

    return Html;
}

QString MainWindow::torikumiResults2Html(int year, int month, int day, int division)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ozumo");
    db.setDatabaseName("/mnt/memory/ozumo.sqlite");
    if (!db.open())
        QMessageBox::warning(this, tr("Unable to open database"),
                             tr("An error occurred while opening the connection: ") + db.lastError().text());

    QSqlQuery query(db);

    query.prepare("SELECT id_local, shikona1, result1, shikona2, result2, kimarite "
               "FROM torikumi "
               "WHERE year = :year AND month = :month AND day = :day AND division = :division "
               "ORDER BY id_local");
    query.bindValue(":year", year);
    query.bindValue(":month", month);
    query.bindValue(":day", day);
    query.bindValue(":division", division);

    query.exec();

    int trClass = 0;
    QString className[] = {"\"odd\"", "\"even\""};
    QString Html = "<!-- year:" + QString::number(year)
                   + " month:" + QString::number(month).rightJustified(2, '0')
                   + " day:" + QString::number(day).rightJustified(2, '0')
                   + " division:" + QString::number(division)
                   + " -->\n";

    Html += "<table>\n"
            "<thead><tr>"
            "<th width=\"25%\">" + QString::fromUtf8("Восток") + "</th>"
            "<th width=\"15%\">" + QString::fromUtf8("") + "</th>"
            "<th width=\"20%\">" + QString::fromUtf8("Кимаритэ") + "</th>"
            "<th width=\"15%\">" + QString::fromUtf8("") + "</th>"
            "<th width=\"25%\">" + QString::fromUtf8("Запад") + "</th></tr></thead>\n"
            "<tbody>\n";

    while (query.next())
    {
        /*qDebug()<< query.value(0).toInt()
                << query.value(1).toString()
                << query.value(2).toString()
                << query.value(3).toString()
                << query.value(4).toString();*/

        //int id = query.value(0).toInt();
        QString shikona1 = query.value(1).toString();
        QString result1  = query.value(2).toInt() == 1 ? QString::fromUtf8("○"):QString::fromUtf8("●");
        QString shikona2 = query.value(3).toString();
        QString result2  = query.value(4).toInt() == 1 ? QString::fromUtf8("○"):QString::fromUtf8("●");
        QString kimarite = query.value(5).toString();

        QString shikona1Ru = shikona1, shikona2Ru = shikona2, kimariteRu = kimarite;

        QSqlQuery tmpQuery(db);

        tmpQuery.prepare("SELECT ru FROM shikona WHERE kanji = :kanji");
        tmpQuery.bindValue(":kanji", shikona1);
        tmpQuery.exec();
        if (tmpQuery.next())
            shikona1Ru = tmpQuery.value(0).toString();

        tmpQuery.prepare("SELECT ru FROM shikona WHERE kanji = :kanji");
        tmpQuery.bindValue(":kanji", shikona2);
        tmpQuery.exec();
        if (tmpQuery.next())
            shikona2Ru = tmpQuery.value(0).toString();

        tmpQuery.prepare("SELECT ru FROM kimarite WHERE kanji = :kanji");
        tmpQuery.bindValue(":kanji", kimarite);
        tmpQuery.exec();
        if (tmpQuery.next())
            kimariteRu = tmpQuery.value(0).toString();

        QString res1, res2;
        tmpQuery.prepare("SELECT COUNT (*) FROM torikumi WHERE year = :y AND month = :m AND day <= :d "
                         "AND ((shikona1 = :s1 AND result1 = 1) OR (shikona2 = :s2 AND result2 = 1))");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":d", day);
        tmpQuery.bindValue(":s1", shikona1);
        tmpQuery.bindValue(":s2", shikona1);
        tmpQuery.exec();
        if (tmpQuery.next())
            res1 += tmpQuery.value(0).toString() + "-";

        tmpQuery.prepare("SELECT COUNT (*) FROM torikumi WHERE year = :y AND month = :m AND day <= :d "
                         "AND ((shikona1 = :s1 AND result1 = 0) OR (shikona2 = :s2 AND result2 = 0))");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":d", day);
        tmpQuery.bindValue(":s1", shikona1);
        tmpQuery.bindValue(":s2", shikona1);
        tmpQuery.exec();
        if (tmpQuery.next())
            res1 += tmpQuery.value(0).toString();

        tmpQuery.prepare("SELECT COUNT (*) FROM torikumi WHERE year = :y AND month = :m AND day <= :d "
                         "AND ((shikona1 = :s1 AND result1 = 1) OR (shikona2 = :s2 AND result2 = 1))");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":d", day);
        tmpQuery.bindValue(":s1", shikona2);
        tmpQuery.bindValue(":s2", shikona2);
        tmpQuery.exec();
        if (tmpQuery.next())
            res2 += tmpQuery.value(0).toString() + "-";

        tmpQuery.prepare("SELECT COUNT (*) FROM torikumi WHERE year = :y AND month = :m AND day <= :d "
                         "AND ((shikona1 = :s1 AND result1 = 0) OR (shikona2 = :s2 AND result2 = 0))");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":d", day);
        tmpQuery.bindValue(":s1", shikona2);
        tmpQuery.bindValue(":s2", shikona2);
        tmpQuery.exec();
        if (tmpQuery.next())
            res2 += tmpQuery.value(0).toString();

        //qDebug() << shikona1Ru << shikona2Ru;

        Html += "<tr class=" + className[trClass] + ">"
                "<td>" + shikona1Ru + " (" + res1 + ")</td>"
                "<td>" + result1 + "</td>"
                "<td>" + kimariteRu + "</td>"
                "<td>" + result2 + "</td>"
                "<td>" + shikona2Ru + " (" + res2 + ")</td></tr>\n";
        //qDebug() << Html;

        trClass ^= 1;
    }

    Html += "</tbody>\n</table>\n";

    db.close();

    return Html;
}

void MainWindow::generateTorikumi()
{
    ui->textEdit->setPlainText(torikumi2Html(
            ui->comboBox_year->currentIndex() + 2002,
            ui->comboBox_basho->currentIndex() * 2 + 1,
            ui->comboBox_day->currentIndex() + 1,
            ui->comboBox_division->currentIndex() + 1));
}

void MainWindow::generateTorikumiResults()
{
    ui->textEdit->setPlainText(torikumiResults2Html(
            ui->comboBox_year->currentIndex() + 2002,
            ui->comboBox_basho->currentIndex() * 2 + 1,
            ui->comboBox_day->currentIndex() + 1,
            ui->comboBox_division->currentIndex() + 1));
}

bool MainWindow::convertTorikumi3456()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ozumo");
    db.setDatabaseName("/mnt/memory/ozumo.sqlite");
    if (!db.open())
        QMessageBox::warning(this, tr("Unable to open database"),
                             tr("An error occurred while opening the connection: ") + db.lastError().text());

    for (int basho = START_INDEX; basho <= 545; basho++)
    {
        for (int day = 1; day <= 15; day++)
        {
            for (int division = 3; division <= 6; division++)
            {
                QFile file0("/mnt/memory/torikumi-3456/"
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

                int year, month;

                if (!findDate(content, &year, &month) || (year == 0) || (month == 0))
                {
                    year = 2002 + (basho - START_INDEX) / 6;
                    month = (basho - START_INDEX) % 6 * 2 + 1;
                }

                int rikishi1 = 0, rikishi2 = 0;
                int dayx = day, id_local = 0;

                content.truncate(content.indexOf(QString("<!-- /BASYO CONTENTS -->")));
                content = content.mid(content.indexOf(QString("<!-- /BASYO TITLE -->"))).simplified();
                //content = content.mid(content.indexOf(QString("<!-- BASYO CONTENTS -->"))).simplified();

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

                    ++id_local;

                    int index = (((basho) * 100 + dayx) * 10 + division) * 100 + id_local;

                    if (!insertTorikumi(db, index, basho, year, month, dayx,
                                        division,
                                        rikishi1, shikona1, rank1, result1 == QString::fromUtf8("○") ? 1:0,
                                        rikishi2, shikona2, rank2, result2 == QString::fromUtf8("○") ? 1:0,
                                        kimarite,
                                        id_local)
                        )
                        qDebug() << "-";
                }

                file0.close();
            }
        }
    }

    db.close();

    return true;
}

bool MainWindow::convertTorikumi()
{
//    readKimarite();
//    readShikonas();

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "ozumo");
    db.setDatabaseName("/mnt/memory/ozumo.sqlite");
    if (!db.open())
        QMessageBox::warning(this, tr("Unable to open database"),
                             tr("An error occurred while opening the connection: ") + db.lastError().text());

    for (int basho = START_INDEX; basho <= 545; basho++)
    {
        for (int day = 1; day <= 15; day++)
        {
            for (int division = 1; division <= 2; division++)
            {
                QFile file0("/mnt/memory/torikumi-12/"
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

                int year, month;

                if (!findDate(content, &year, &month) || (year == 0) || (month == 0))
                {
                    year = 2002 + (basho - START_INDEX) / 6;
                    month = (basho - START_INDEX) % 6 * 2 + 1;
                }

                QStringList list = readAndSimplifyBashoContent(content);

                int i = 0;
                int dayx = day, id_local = 0;
                QString rikishi1, rikishi2;

                while (i < list.count())
                {
                    if (list.value(i).contains("rank"))
                    {
                        //東前14 栃栄 3勝0敗 ○ 押し出し ● 寺尾 0勝3敗 西十2

                        QString rank1 = list.value(i +  1);
                        //out << "<td>" << list.value(i +  1) << "</td>\n";   // rank 1

                        QString shikona1 = list.value(i +  3);
                        if (shikona1 == "rikishi_id")
                        {
                            i++;
                            rikishi1 = list.value(i +  3);
                            i++;
                            shikona1 = list.value(i +  3);
                        }
                        else
                        {
                            rikishi1 = "0";
                        }
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
                        if (kimarite == "kimarite_id")
                        {
                            i++;
                            kimarite = list.value(i +  8);  // kimarite id, just skipped at this moment
                            i++;
                            kimarite = list.value(i +  8);
                        }
                        //out << "<td>" << translateKimarite(list.value(i + 8)) << "</td>\n";   // kimarite

                        QString result2 = list.value(i + 10);
                        //out << "<td>" << list.value(i + 10) << "</td>\n";   // bout 2

                        QString shikona2 = list.value(i + 12);
                        if (shikona2 == "rikishi_id")
                        {
                            i++;
                            rikishi2 = list.value(i +  12);
                            i++;
                            shikona2 = list.value(i +  12);
                        }
                        else
                        {
                            rikishi2 = "0";
                        }
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

                        ++id_local;

                        int index = (((basho) * 100 + dayx) * 10 + division) * 100 + id_local;

                        if (!insertTorikumi(db, index, basho, year, month, dayx,
                                            division,
                                            rikishi1.toInt(), shikona1, rank1, result1 == QString::fromUtf8("○") ? 1:0,
                                            rikishi2.toInt(), shikona2, rank2, result2 == QString::fromUtf8("○") ? 1:0,
                                            kimarite,
                                            id_local)
                            )
                            qDebug() << "-";

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

    db.close();

    return true;
}

