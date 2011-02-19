#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <QtGui>
#include <QtSql>

#define START_INDEX 491
#define BASE_URL "http://sumo.goo.ne.jp/hon_basho/torikumi/"

#ifdef __WIN32__
#define WORK_DIR ""
#else
#define WORK_DIR "/mnt/memory/"
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton_importAllTorikumi, SIGNAL(clicked()), this, SLOT(importAllTorikumi()));
    connect(ui->pushButton_torikumi2Banzuke, SIGNAL(clicked()), this, SLOT(torikumi2Banzuke()));
    connect(ui->pushButton_Hoshitori, SIGNAL(clicked()), this, SLOT(importAllHoshitori()));

    connect(ui->pushButton_generateTorikumi, SIGNAL(clicked()), this, SLOT(generateTorikumi()));
    connect(ui->pushButton_generateTorikumiResults, SIGNAL(clicked()), this, SLOT(generateTorikumiResults()));
    connect(ui->pushButton_downloadTorikumi, SIGNAL(clicked()), this, SLOT(downloadTorikumi()));

    ui->lineEdit_workDir->setText(WORK_DIR);

    if (!QFile::exists(WORK_DIR "ozumo.sqlite"))
    {
        QMessageBox::warning(this, tr("Unable to open database"), tr(WORK_DIR "ozumo.sqlite does not exist!"));
    }

    db = QSqlDatabase::addDatabase("QSQLITE", "ozumo");
    db.setDatabaseName(WORK_DIR "ozumo.sqlite");
    if (!db.open())
        QMessageBox::warning(this, tr("Unable to open database"),
                             tr("An error occurred while opening the connection: ") + db.lastError().text());

/*    importBanzuke(db, WORK_DIR "ban_1.html");
    importBanzuke(db, WORK_DIR "ban_2.html");
    importBanzuke(db, WORK_DIR "ban_3_1.html");
    importBanzuke(db, WORK_DIR "ban_4_1.html");
    importBanzuke(db, WORK_DIR "ban_4_2.html");
    importBanzuke(db, WORK_DIR "ban_5_1.html");
    importBanzuke(db, WORK_DIR "ban_5_2.html");
    importBanzuke(db, WORK_DIR "ban_5_3.html");
    importBanzuke(db, WORK_DIR "ban_6_1.html");*/

}

MainWindow::~MainWindow()
{
    db.close();

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

void readNames()
{
    QDir dir(WORK_DIR "rikishi/");

    QStringList filters;
    filters << "rikishi_*.html";
    dir.setNameFilters(filters);

    QStringList list = dir.entryList();

    for (int i = 0; i < list.count(); i++)
    {
        QFile file0(dir.absoluteFilePath(list.at(i)));

        if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() << "error: file.open(" << WORK_DIR << "rikishi/rikishi_...)";
            return;
        }

        QTextStream in(&file0);

        in.setCodec("EUC-JP");

        QString content = in.readAll();
        QString shikonaEn = content.mid(content.indexOf("<title>") + QString("<title>").size());
        shikonaEn.truncate(shikonaEn.indexOf(" "));
        QString id = list.at(i);
        id.replace(QRegExp("rikishi_"), "");
        id.replace(QRegExp(".html"), "");

        qDebug() << id << shikonaEn;
    }
}

int wgetDownload(QString url)
{
    QProcess process;

    process.start("wget -N -t 1 -T 30 " + url);
    if (process.waitForFinished(33000) == false)
        return -1;

    return process.exitCode();
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

QString translateShikona(QSqlDatabase db, QString shikona)
{
    QSqlQuery tmpQuery(db);
    QString shikonaRu = shikona;

    tmpQuery.prepare("SELECT ru FROM shikona WHERE kanji = :kanji");
    tmpQuery.bindValue(":kanji", shikona);
    tmpQuery.exec();
    if (tmpQuery.next())
    {
        shikonaRu = tmpQuery.value(0).toString();
    }
    else
    {
        tmpQuery.prepare("SELECT en1 FROM names WHERE kanji1 = :kanji AND en1 IS NOT NULL AND en1 != ''");
        tmpQuery.bindValue(":kanji", shikona);
        tmpQuery.exec();
        if (tmpQuery.next())
        {
            shikonaRu = tmpQuery.value(0).toString();
        }
    }

    return shikonaRu;
}

QString translateKimarite(QSqlDatabase db, QString kimarite)
{
    QSqlQuery tmpQuery(db);
    QString kimariteRu = kimarite;

    tmpQuery.prepare("SELECT ru FROM kimarite WHERE kanji = :kanji");
    tmpQuery.bindValue(":kanji", kimarite);
    tmpQuery.exec();
    if (tmpQuery.next())
    {
        kimariteRu = tmpQuery.value(0).toString();
    }
    else
    {
        kimariteRu = QString::fromUtf8("неопределён");
    }

    return kimariteRu;
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

bool MainWindow::insertTorikumi(int id, int basho, int year, int month, int day,
                                int division,
                                int rikishi1, QString shikona1, QString rank1, int result1,
                                int rikishi2, QString shikona2, QString rank2, int result2,
                                QString kimarite,
                                int id_local)
{
    QSqlQuery query(db);

    query.prepare("DELETE FROM torikumi WHERE id = :id");

    query.bindValue(":id",       id);

    query.exec();

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

    return true;
}

QString MainWindow::torikumi2Html(int year, int month, int day, int division)
{
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

        shikona1Ru = translateShikona(db, shikona1);
        shikona2Ru = translateShikona(db, shikona2);

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
                history.prepend(QString::fromUtf8("−"));
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

    return Html;
}

QString MainWindow::torikumiResults2Html(int year, int month, int day, int division)
{
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

        shikona1Ru = translateShikona(db, shikona1);
        shikona2Ru = translateShikona(db, shikona2);

        kimariteRu = translateKimarite(db, kimarite);

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

    return Html;
}

int MainWindow::getAndImportTorikumi(int year, int month, int day, int division)
{
    // "http://sumo.goo.ne.jp/hon_basho/torikumi/tori_545_1_1.html"
    QString url = BASE_URL;

    int basho = (year - 2002) * 6 + START_INDEX + ((month - 1) >> 1);

    if (day > 15)
        day = 15;

    QString fName = "tori_" + QString::number(basho) + "_" + QString::number(division) + "_" + QString::number(day) + ".html";
    url += fName;

    int exitCode = wgetDownload(url);

    if (exitCode == 0)
    {
        importTorikumi(fName);
    }

    return exitCode;
}

void MainWindow::downloadTorikumi()
{
    int exitCode = getAndImportTorikumi(
            ui->comboBox_year->currentIndex() + 2002,
            ui->comboBox_basho->currentIndex() * 2 + 1,
            ui->comboBox_day->currentIndex() + 1,
            ui->comboBox_division->currentIndex() + 1);

    QString result;
    if (exitCode == 0)
        result = "Successful";
    else
        result = "Failed: " + QString::number(exitCode);

    ui->textEdit_htmlCode->setPlainText(result);
}

void MainWindow::generateTorikumi()
{
    ui->textEdit_htmlCode->setPlainText(torikumi2Html(
            ui->comboBox_year->currentIndex() + 2002,
            ui->comboBox_basho->currentIndex() * 2 + 1,
            ui->comboBox_day->currentIndex() + 1,
            ui->comboBox_division->currentIndex() + 1));

    ui->textEdit_htmlPreview->setHtml(ui->textEdit_htmlCode->toPlainText());
}

void MainWindow::generateTorikumiResults()
{
    ui->textEdit_htmlCode->setPlainText(torikumiResults2Html(
            ui->comboBox_year->currentIndex() + 2002,
            ui->comboBox_basho->currentIndex() * 2 + 1,
            ui->comboBox_day->currentIndex() + 1,
            ui->comboBox_division->currentIndex() + 1));

    ui->textEdit_htmlPreview->setHtml(ui->textEdit_htmlCode->toPlainText());
}

void MainWindow::parsingTorikumi3456(QString content, int basho, int year, int month, int day, int division)
{
    int dayx = day, id_local = 0;
    int rikishi1 = 0, rikishi2 = 0;

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

        if (!insertTorikumi(index, basho, year, month, dayx,
                            division,
                            rikishi1, shikona1, rank1, result1 == QString::fromUtf8("○") ? 1:0,
                            rikishi2, shikona2, rank2, result2 == QString::fromUtf8("○") ? 1:0,
                            kimarite,
                            id_local)
            )
            qDebug() << "-";
    }

}

void MainWindow::parsingTorikumi12(QString content, int basho, int year, int month, int day, int division)
{
    int i = 0;
    int dayx = day, id_local = 0;
    QString rikishi1, rikishi2;

    QStringList list = readAndSimplifyBashoContent(content);

    while (i < list.count())
    {
        if (list.value(i).contains("rank"))
        {
            QString rank1 = list.value(i +  1);

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

            QString result1 = list.value(i +  6);

            QString kimarite = list.value(i +  8);
            if (kimarite == "kimarite_id")
            {
                i++;
                kimarite = list.value(i +  8);  // kimarite id, just skipped at this moment
                i++;
                kimarite = list.value(i +  8);
            }

            QString result2 = list.value(i + 10);

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

            QString rank2 = list.value(i + 15);

            ++id_local;

            int index = (((basho) * 100 + dayx) * 10 + division) * 100 + id_local;

            if (!insertTorikumi(index, basho, year, month, dayx,
                                division,
                                rikishi1.toInt(), shikona1, rank1, result1 == QString::fromUtf8("○") ? 1:0,
                                rikishi2.toInt(), shikona2, rank2, result2 == QString::fromUtf8("○") ? 1:0,
                                kimarite,
                                id_local)
                )
                qDebug() << "-";

            i += 15;
        }
        i++;
    }

}

bool MainWindow::importTorikumi(QString fName)
{
    QFile file0(fName);

    if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "error: file.open(" << fName << ")";
        return false;
    }

    QTextStream in(&file0);

    in.setCodec("EUC-JP");

    QString content = in.readAll();

    file0.close();

    int year, month;
    int basho, division, day;

    fName.replace('.', '_');
    QStringList tempList= fName.split('_');
    basho    = tempList.at(1).toInt();
    division = tempList.at(2).toInt();
    day      = tempList.at(3).toInt();

    if (!findDate(content, &year, &month) || (year == 0) || (month == 0))
    {
        year = 2002 + (basho - START_INDEX) / 6;
        month = (basho - START_INDEX) % 6 * 2 + 1;
    }

    if (division <= 2)
        parsingTorikumi12(content, basho, year, month, day, division);
    else
        parsingTorikumi3456(content, basho, year, month, day, division);

    return true;
}

bool MainWindow::importAllTorikumi()
{
    QDir dir(WORK_DIR "torikumi/");

    QStringList filters;
    filters << "tori_*_*_*.html";
    dir.setNameFilters(filters);

    QStringList list = dir.entryList();

    for (int i = 0; i < list.count(); i++)
    {
        importTorikumi(dir.absoluteFilePath(list.at(i)));
    }

    return true;
}

bool MainWindow::importAllHoshitori()
{
    QDir dir(WORK_DIR "hoshi/");

    QStringList filters;
    filters << "hoshi_*_*.html";
    dir.setNameFilters(filters);

    QStringList list = dir.entryList();

    for (int i = 0; i < list.count(); i++)
    {
        importHoshitori(dir.absoluteFilePath(list.at(i)));
    }

    return true;
}

bool MainWindow::insertBanzuke(int year, int month, QString rank, int position, int side,
                               int rikishi, QString shikona)
{
    QSqlQuery query(db);

    int rank_id = 0;
    query.prepare("SELECT id, kanji FROM rank");
    query.exec();
    while (query.next())
    {
        if (rank.startsWith(query.value(1).toString()))
            rank_id = query.value(0).toInt();
    }

    int basho;

    query.prepare("SELECT id FROM basho WHERE year = :year AND month = :month");
    query.bindValue(":year",     year);
    query.bindValue(":month",    month);
    query.exec();
    if (query.next())
    {
        basho = query.value(0).toInt();
    }
    else
        return false;

    int id = ((basho * 100 + rank_id) * 1000 + position) * 10 + side;

    query.prepare("DELETE FROM banzuke WHERE id = :id");

    query.bindValue(":id",       id);

    query.exec();

    query.prepare("INSERT INTO banzuke ("
                  "id, year, month, rank, position, side, rikishi, shikona) "
                  "VALUES ("
                  ":id, :year, :month, :rank, :position, :side, :rikishi, :shikona)");

    query.bindValue(":id",       id);
    query.bindValue(":year",     year);
    query.bindValue(":month",    month);
    query.bindValue(":rank",     rank_id);
    query.bindValue(":position", position);
    query.bindValue(":side",     side);
    query.bindValue(":rikishi",  rikishi);
    query.bindValue(":shikona",  shikona);

    return query.exec();
}

bool MainWindow::parsingBanzuke12(QString content)
{
    content = content.mid(content.indexOf(QString::fromUtf8("<strong>■")));
    content = content.mid(content.indexOf(">") + 2);
    QString division = content.left(content.indexOf("<")).simplified();

    int year, month;

    // 平成22年12月21日更新
    QRegExp rx(QString::fromUtf8("平成(\\d+)年(\\d+)月(\\d+)日"));
    if (rx.indexIn(content) != -1) {
        year  = 1988 + rx.cap(1).toInt();
        month = rx.cap(2).toInt() + 1;
        if (month == 13)
        {
            month = 1;
            year++;
        }
    }
    else
        return false;

    content.truncate(content.indexOf(QString("<!-- /BASYO CONTENTS -->")));

    QString prevRank;
    int position = 1;

    while (content.indexOf(QString::fromUtf8("=東=")) != -1)
    {
        content = content.mid(content.indexOf(QString::fromUtf8("=東=")));

        QString contentEast = content;
        if (content.indexOf(QString::fromUtf8("=格付=")) != -1)
            contentEast.truncate(content.indexOf(QString::fromUtf8("=格付=")));

        QString kanji1;
        int id1;
        if (contentEast.indexOf("<strong>") != -1)
        {
            QRegExp rx("rikishi_(\\d+)");
            if (rx.indexIn(contentEast) != -1) {
                id1 = rx.cap(1).toInt();
            }

            contentEast = contentEast.mid(contentEast.indexOf("<strong>"));
            contentEast = contentEast.mid(contentEast.indexOf(">") + 1).simplified();
            kanji1 = contentEast.left(contentEast.indexOf(" "));
        }

        content = content.mid(content.indexOf(QString::fromUtf8("=格付=")));

        QString contentRank = content;
        if (contentRank.indexOf(QString::fromUtf8("=西=")) != -1)
            contentRank.truncate(contentRank.indexOf(QString::fromUtf8("=西=")));

        contentRank = contentRank.mid(contentRank.indexOf("<td align='center' bgcolor='#d9eaf0' class='common12-18-333'>"));
        contentRank = contentRank.mid(contentRank.indexOf(">") + 1);
        QString rank = contentRank.left(contentRank.indexOf("<")).simplified();
        if (division == QString::fromUtf8("十両"))
            rank = division;

        if (rank != prevRank)
        {
            prevRank = rank;
            position = 1;
        }
        else
        {
            position++;
        }

        if (!kanji1.isEmpty())
            if (!insertBanzuke(year, month, rank, position, 0, id1, kanji1))
            {
                qDebug() << "-";
                return false;
            }

        content = content.mid(content.indexOf(QString::fromUtf8("=西=")));

        QString contentWest = content;
        if (contentWest.indexOf(QString::fromUtf8("=東=")) != -1)
            contentWest.truncate(contentWest.indexOf(QString::fromUtf8("=東=")));

        QString kanji2;
        int id2;
        if (contentWest.indexOf("<strong>") != -1)
        {
            QRegExp rx("rikishi_(\\d+)");
            if (rx.indexIn(contentWest) != -1) {
                id2 = rx.cap(1).toInt();
            }

            contentWest = contentWest.mid(contentWest.indexOf("<strong>"));
            contentWest = contentWest.mid(contentWest.indexOf(">") + 1).simplified();
            kanji2 = contentWest.left(contentWest.indexOf(" "));
        }
        //qDebug() << division << kanji1 << id1 << rank << i << kanji2 << id2;

        if (!kanji2.isEmpty())
            if (!insertBanzuke(year, month, rank, position, 1, id2, kanji2))
            {
                qDebug() << "-";
                return false;
            }
    }

    return true;
}

bool MainWindow::parsingBanzuke3456(QString content)
{
    content = content.mid(content.indexOf(QString::fromUtf8("<strong>■")));
    content = content.mid(content.indexOf(">") + 2);
    QString division = content.left(content.indexOf("<")).simplified();

    int year, month;

    // 平成22年12月21日更新
    QRegExp rx(QString::fromUtf8("平成(\\d+)年(\\d+)月(\\d+)日"));
    if (rx.indexIn(content) != -1) {
        year  = 1988 + rx.cap(1).toInt();
        month = rx.cap(2).toInt() + 1;
        if (month == 13)
        {
            month = 1;
            year++;
        }
    }
    else
        return false;

    while (content.indexOf("<td align=\"center\" bgcolor=\"#d0a3f5\" class=\"common12-18-333\">") != -1)
    {
        content = content.mid(content.indexOf("<td align=\"center\" bgcolor=\"#d0a3f5\" class=\"common12-18-333\">"));
        content = content.mid(content.indexOf(">") + 1);
        QString kanji1 = content.left(content.indexOf("<")).simplified();

        content = content.mid(content.indexOf("<td align=\"center\" bgcolor=\"#d9eaf0\" class=\"common12-18-333\">"));
        content = content.mid(content.indexOf(">") + 1);
        QString hiragana1 = content.left(content.indexOf("<")).simplified();

        content = content.mid(content.indexOf("<td align='center' bgcolor='#6b248f' class='common12-18-fff'>"));
        content = content.mid(content.indexOf(">") + 1);
        QString position = content.left(content.indexOf("<")).simplified();

        content = content.mid(content.indexOf("<td align=\"center\" bgcolor=\"#d0a3f5\" class=\"common12-18-333\">"));
        content = content.mid(content.indexOf(">") + 1);
        QString kanji2 = content.left(content.indexOf("<")).simplified();

        content = content.mid(content.indexOf("<td align=\"center\" bgcolor=\"#d9eaf0\" class=\"common12-18-333\">"));
        content = content.mid(content.indexOf(">") + 1);
        QString hiragana2 = content.left(content.indexOf("<")).simplified();

        content = content.mid(content.indexOf("<td align=\"center\" bgcolor=\"#d0a3f5\" class=\"common12-18-333\">"));
        content = content.mid(content.indexOf(">") + 1);

        // qDebug() << division << kanji1 << hiragana1 << rank << kanji2 << hiragana2;

        if (!kanji1.isEmpty())
            if (!insertBanzuke(year, month, division, position.toInt(), 0, 0, kanji1))
            {
                qDebug() << "-";
                return false;
            }

        if (!kanji2.isEmpty())
            if (!insertBanzuke(year, month, division, position.toInt(), 1, 0, kanji2))
            {
                qDebug() << "-";
                return false;
            }
    }

    return true;
}

bool MainWindow::importBanzuke(QString fName)
{
    QFile file0(fName);

    if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "error: file.open(" << fName << ")";
        return false;
    }

    QTextStream in(&file0);

    in.setCodec("EUC-JP");

    QString content = in.readAll();

    file0.close();

    int division;

    QFileInfo fi(fName);
    division = QString(fi.fileName().at(4)).toInt();

    if (division <= 2)
        parsingBanzuke12(content);
    else
        parsingBanzuke3456(content);

    return true;
}

bool MainWindow::parsingHoshitori12(QString content, int basho, int division, int side)
{
    int year, month;

    QSqlQuery query(db);

    query.prepare("SELECT year, month FROM basho WHERE id = :basho");
    query.bindValue(":basho", basho);
    query.exec();
    if (query.next())
    {
        year  = query.value(0).toInt();
        month = query.value(1).toInt();
    }
    else
        return false;

    content = content.mid(content.indexOf(QString::fromUtf8("<div class=\"torikumi_boxbg\"><table")));

    QString prevRank;
    int position = 1;

    while (content.indexOf(QString::fromUtf8("<div class=\"torikumi_boxbg\"><table")) != -1)
    {
        content = content.mid(content.indexOf(QString::fromUtf8("<div class=\"torikumi_boxbg\"><table")));

        QString contentRow = content;
        contentRow.truncate(contentRow.indexOf(QString::fromUtf8("</div>")));

        contentRow = contentRow.mid(contentRow.indexOf("<td colspan=\"4\" bgcolor=\"#6b248f\" class=\"common12-18-fff\">"));
        contentRow = contentRow.mid(contentRow.indexOf(">") + 1);
        QString rank = contentRow.left(contentRow.indexOf("<")).simplified();

        if (QString(rank.at(0)) != prevRank)
        {
            prevRank = QString(rank.at(0));
            position = 1;
            if ((division == 1) && (side == 3))
            {
                if (QString(rank.at(2)) == QString::fromUtf8("九"))
                    position = 9;
                else
                    position = 10;
            }
            if ((division == 2) && (side == 5))
                position = 8;
        }
        else
        {
            position++;
        }

        QString shikona1;
        int id1;
        if (contentRow.indexOf("<strong>") != -1)
        {
            QRegExp rx("rikishi_(\\d+)");
            if (rx.indexIn(contentRow) != -1) {
                id1 = rx.cap(1).toInt();
            }

            contentRow = contentRow.mid(contentRow.indexOf("<strong>"));
            contentRow = contentRow.mid(contentRow.indexOf(">") + 1).simplified();
            shikona1 = contentRow.left(contentRow.indexOf("<"));
        }

        //qDebug() << rank << position << id1 << shikona1;
        if (!shikona1.isEmpty())
            if (!insertBanzuke(year, month, rank, position, 0, id1, shikona1))
            {
                qDebug() << "-";
                return false;
            }

        QString shikona2;
        int id2;
        if (contentRow.indexOf("<strong>") != -1)
        {
            QRegExp rx("rikishi_(\\d+)");
            if (rx.indexIn(contentRow) != -1) {
                id2 = rx.cap(1).toInt();
            }

            contentRow = contentRow.mid(contentRow.indexOf("<strong>"));
            contentRow = contentRow.mid(contentRow.indexOf(">") + 1).simplified();
            shikona2 = contentRow.left(contentRow.indexOf("<"));
        }

        //qDebug() << rank << position << id2 << shikona2;
        if (!shikona2.isEmpty())
            if (!insertBanzuke(year, month, rank, position, 1, id2, shikona2))
            {
                qDebug() << "-";
                return false;
            }

        content = content.mid(content.indexOf(QString::fromUtf8("</div>")));
    }

    return true;
}

bool MainWindow::parsingHoshitori3456(QString content, int basho, int division, int side)
{
    QString rank;
    int year, month;

    QSqlQuery query(db);

    query.prepare("SELECT year, month FROM basho WHERE id = :basho");
    query.bindValue(":basho", basho);
    query.exec();
    if (query.next())
    {
        year  = query.value(0).toInt();
        month = query.value(1).toInt();
    }
    else
        return false;

    query.prepare("SELECT kanji FROM rank WHERE division_id = :division");
    query.bindValue(":division", division);
    query.exec();
    if (query.next())
    {
        rank = query.value(0).toString();
    }
    else
        return false;

    content = content.mid(content.indexOf(QString::fromUtf8("torikumi_boxbg")));
    content.truncate(content.indexOf(QString("</table>")));

    while (content.indexOf(QString::fromUtf8("hoshitori_riki3-1")) != -1)
    {
        content = content.mid(content.indexOf(QString::fromUtf8("hoshitori_riki3-1")));
        content = content.mid(content.indexOf(">") + 1).simplified();
        int position = content.left(content.indexOf("<")).toInt();

        content = content.mid(content.indexOf(QString::fromUtf8("hoshitori_riki5")));
        content = content.mid(content.indexOf(">") + 1).simplified();
        QString shikona = content.left(content.indexOf("<"));

        content = content.mid(content.indexOf(QString::fromUtf8("hoshitori_riki3-1")) + 1);

        //qDebug() << rank << shikona;

        if (!shikona.isEmpty())
            if (!insertBanzuke(year, month, rank, position, side, 0, shikona))
            {
                qDebug() << "-";
                return false;
            }
    }

    return true;
}

bool MainWindow::importHoshitori(QString fName)
{
    QFile file0(fName);

    if (!file0.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "error: file.open(" << fName << ")";
        return false;
    }

    QTextStream in(&file0);

    in.setCodec("EUC-JP");

    QString content = in.readAll();

    file0.close();

    int basho, division, side;

    QFileInfo fi(fName);
    // hoshi_545_3_1_1.html
    QRegExp rx(QString::fromUtf8("hoshi_(\\d+)_(\\d+)_(\\d+).?"));
    if (rx.indexIn(fi.fileName()) != -1) {
        basho = rx.cap(1).toInt();
        division = rx.cap(2).toInt();
        side = rx.cap(3).toInt();
    }
    else
        return false;

    if (division <= 2)
        parsingHoshitori12(content, basho, division, side);
    else
        parsingHoshitori3456(content, basho, division, side - 1);

    return true;
}
