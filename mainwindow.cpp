#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <QtGui>
#include <QtSql>

#define START_INDEX 491
#define BASE_URL "http://sumo.goo.ne.jp/hon_basho/"

#define WIN_MARK    "○"
#define LOSS_MARK   "●"
#define DASH_MARK   "−"

#define FUZEN       "不戦"
#define FUZEN_WIN   "□"
#define FUZEN_LOSS  "■"

QString WinMark   = QString::fromUtf8(WIN_MARK);
QString LossMark  = QString::fromUtf8(LOSS_MARK);
QString DashMark  = QString::fromUtf8(DASH_MARK);

QString Fuzen     = QString::fromUtf8(FUZEN);
QString FuzenWin  = QString::fromUtf8(FUZEN_WIN);
QString FuzenLoss = QString::fromUtf8(FUZEN_LOSS);

QString phpBBcolor[] = {"[color=#404040]",
                        "[color=#A00000]",
                        "[color=#A06000]",
                        "[color=#6000A0]",
                        "[color=#6000A0]",
                        "[color=#006000]",
                        "[color=#0060A0]",
                        "[color=#A000A0]",
                        "[color=#008080]",
                        "[color=#A00060]",
                        "[color=#A08000]"};

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
    connect(ui->pushButton_importAllHoshitori, SIGNAL(clicked()), this, SLOT(importAllHoshitori()));
    connect(ui->pushButton_downloadBanzuke, SIGNAL(clicked()), this, SLOT(downloadBanzuke()));

    connect(ui->pushButton_downloadTorikumi, SIGNAL(clicked()), this, SLOT(downloadTorikumi()));

    connect(ui->pushButton_generateHoshitori, SIGNAL(clicked()), this, SLOT(generateHoshitori()));
    connect(ui->pushButton_generateTorikumi, SIGNAL(clicked()), this, SLOT(generateTorikumi()));
    connect(ui->pushButton_generateResults, SIGNAL(clicked()), this, SLOT(generateResults()));

    connect(ui->pushButton_generateBBCodeTorikumi, SIGNAL(clicked()), this, SLOT(generateBBCodeTorikumi()));
    connect(ui->pushButton_generateBBCodeResults, SIGNAL(clicked()), this, SLOT(generateBBCodeResults()));

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

    statistics();
}

MainWindow::~MainWindow()
{
    db.close();

    delete ui;
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

QString MainWindow::translateShikona(QString shikona, int year, int month)
{
    QSqlQuery tmpQuery(db);
    QString hiragana, shikonaRu = shikona;

    tmpQuery.prepare("SELECT hiragana FROM banzuke WHERE shikona = :s AND year = :y AND month = :m");
    tmpQuery.bindValue(":s", shikona);
    tmpQuery.bindValue(":y", year);
    tmpQuery.bindValue(":m", month);
    tmpQuery.exec();
    if (tmpQuery.next())
    {
        hiragana = tmpQuery.value(0).toString();
    }

    if (hiragana.size() > 0)
    {
        tmpQuery.prepare("SELECT ru FROM shikona WHERE kanji = :k AND hiragana = :h");
        tmpQuery.bindValue(":k", shikona);
        tmpQuery.bindValue(":h", hiragana);
    }
    else
    {
        tmpQuery.prepare("SELECT ru FROM shikona WHERE kanji = :k");
        tmpQuery.bindValue(":k", shikona);
    }
    tmpQuery.exec();
    if (tmpQuery.next())
    {
        shikonaRu = tmpQuery.value(0).toString();
    }
    else
    {
        tmpQuery.prepare("SELECT en1 FROM rikishi WHERE kanji1 = :kanji AND en1 IS NOT NULL AND en1 != ''");
        tmpQuery.bindValue(":kanji", shikona);
        tmpQuery.exec();
        if (tmpQuery.next())
        {
            shikonaRu = tmpQuery.value(0).toString();
        }
    }

    return shikonaRu;
}

QString MainWindow::translateKimarite(QString kimarite)
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

    return kimariteRu;
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

QString MainWindow::torikumi2Html(int year, int month, int day, int division)
{
    int trClass = 0;
    QString className[] = {"\"odd\"", "\"even\""};
    QString Html = "<!--"
                   " year:"     + QString::number(year    ) +
                   " month:"    + QString::number(month   ).rightJustified(2, '0') +
                   " day:"      + QString::number(day     ).rightJustified(2, '0') +
                   " division:" + QString::number(division) +
                   " -->\n";

    Html += "<table>\n"
            "<thead><tr>"
            "<th width=\"33%\">" + QString::fromUtf8("Восток") + "</th>"
            "<th width=\"33%\">" + QString::fromUtf8("История последних встреч") + "</th>"
            "<th width=\"33%\">" + QString::fromUtf8("Запад")  + "</th>"
            "</tr></thead>\n"
            "<tbody>\n";

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

    while (query.next())
    {
        //int id = query.value(0).toInt();
        QString shikona1 = query.value(1).toString();
        QString rank1    = query.value(2).toString();
        QString shikona2 = query.value(3).toString();
        QString rank2    = query.value(4).toString();
        int basho = query.value(5).toInt();

        QSqlQuery tmpQuery(db);

        QString res1, res2;
        res1 += " (" + QString::number(getNumOfBoshi(year, month, day-1, shikona1, 1)) +
                "-"  + QString::number(getNumOfBoshi(year, month, day-1, shikona1, 0)) + ")";

        res2 += " (" + QString::number(getNumOfBoshi(year, month, day-1, shikona2, 1)) +
                "-"  + QString::number(getNumOfBoshi(year, month, day-1, shikona2, 0)) + ")";

        QString history;
        for (int i = 1; i <= 6; i++)
        {
            tmpQuery.prepare("SELECT result1 FROM torikumi WHERE shikona1 = :s1a AND shikona2 = :s2a AND basho = :ba "
                             "UNION "
                             "SELECT result2 FROM torikumi WHERE shikona2 = :s1b AND shikona1 = :s2b AND basho = :bb ");

            tmpQuery.bindValue(":s1a", shikona1);
            tmpQuery.bindValue(":s2a", shikona2);
            tmpQuery.bindValue(":s1b", shikona1);
            tmpQuery.bindValue(":s2b", shikona2);
            tmpQuery.bindValue(":ba", basho - i);
            tmpQuery.bindValue(":bb", basho - i);
            tmpQuery.exec();
            if (tmpQuery.next())
            {
                QString r = tmpQuery.value(0).toInt() == 1 ? WinMark:LossMark;
                history.prepend(r);
            }
            else
                history.prepend(DashMark);
            history.prepend(" ");
        }

        Html += "<tr class=" + className[trClass] + ">"
                "<td>" + translateShikona(shikona1, year, month) + res1 + "</td>"
                "<td><font style=\"font-family: monospace; letter-spacing:4px;\">" + history.simplified() + "</font></td>"
                "<td>" + translateShikona(shikona2, year, month) + res2 + "</td></tr>\n";

        trClass ^= 1;
    }

    Html += "</tbody>\n</table>\n";

    return Html;
}

QString MainWindow::torikumiResults2Html(int year, int month, int day, int division)
{
    int trClass = 0;
    QString className[] = {"\"odd\"", "\"even\""};
    QString Html = "<!--"
                   " year:"     + QString::number(year    ) +
                   " month:"    + QString::number(month   ).rightJustified(2, '0') +
                   " day:"      + QString::number(day     ).rightJustified(2, '0') +
                   " division:" + QString::number(division) +
                   " -->\n";

    Html += "<table>\n"
            "<thead><tr>"
            "<th width=\"25%\">" + QString::fromUtf8("Восток")   + "</th>"
            "<th width=\"15%\">" + QString::fromUtf8("")         + "</th>"
            "<th width=\"20%\">" + QString::fromUtf8("Кимаритэ") + "</th>"
            "<th width=\"15%\">" + QString::fromUtf8("")         + "</th>"
            "<th width=\"25%\">" + QString::fromUtf8("Запад")    + "</th>"
            "</tr></thead>\n"
            "<tbody>\n";

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

    while (query.next())
    {
        //int id = query.value(0).toInt();
        QString shikona1 = query.value(1).toString();
        QString result1  = query.value(2).toInt() == 1 ? WinMark:LossMark;
        QString shikona2 = query.value(3).toString();
        QString result2  = query.value(4).toInt() == 1 ? WinMark:LossMark;
        QString kimarite = query.value(5).toString();

        QString res1, res2;
        res1 += " (" + QString::number(getNumOfBoshi(year, month, day, shikona1, 1)) +
                "-"  + QString::number(getNumOfBoshi(year, month, day, shikona1, 0)) + ")";

        res2 += " (" + QString::number(getNumOfBoshi(year, month, day, shikona2, 1)) +
                "-"  + QString::number(getNumOfBoshi(year, month, day, shikona2, 0)) + ")";

        Html += "<tr class=" + className[trClass] + ">"
                "<td>" + translateShikona(shikona1, year, month) + res1 + "</td>"
                "<td>" + result1 + "</td>"
                "<td>" + translateKimarite(kimarite) + "</td>"
                "<td>" + result2 + "</td>"
                "<td>" + translateShikona(shikona2, year, month) + res2 + "</td></tr>\n";

        trClass ^= 1;
    }

    Html += "</tbody>\n</table>\n";

    return Html;
}

int MainWindow::getAndImportTorikumi(int year, int month, int day, int division)
{
    // "http://sumo.goo.ne.jp/hon_basho/torikumi/tori_545_1_1.html"
    QString url = BASE_URL "torikumi/";

    int basho;

    QSqlQuery query(db);
    query.prepare("SELECT id FROM basho WHERE year = :year AND month = :month");
    query.bindValue(":year",  year);
    query.bindValue(":month", month);
    query.exec();
    if (query.next())
    {
        basho = query.value(0).toInt();
    }
    else
        return -1;

    if (day > 15)
        day = 15;

    QString fName = "tori_" + QString::number(basho) + "_" + QString::number(division) + "_" + QString::number(day) + ".html";
    url += fName;

    int exitCode = wgetDownload(url);

    if (exitCode == 0)
    {
        exitCode = -1;
        if (importTorikumi(fName))
        {
            exitCode = 0;
        }
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

void MainWindow::downloadBanzuke()
{
    QString url = BASE_URL "banzuke/";

    QStringList parts = (QStringList()
                         << "ban_1.html"
                         << "ban_2.html"
                         << "ban_3_1.html"
                         << "ban_4_1.html"
                         << "ban_4_2.html"
                         << "ban_5_1.html"
                         << "ban_5_2.html"
                         << "ban_5_3.html"
                         << "ban_6_1.html"
                         );

    ui->textEdit_htmlCode->clear();

    for (int i = 0; i < parts.size(); i++)
    {
        int exitCode = wgetDownload("-P banzuke " +  url + parts[i]);

        if (exitCode != 0)
        {
            ui->textEdit_htmlCode->append("Downloading " + url + parts[i] + " failed, Code: " + QString::number(exitCode));
            return;
        }
        else
        {
            ui->textEdit_htmlCode->append("Downloading " + url + parts[i] + " complete");

            if (!importBanzuke(WORK_DIR "banzuke/" + parts[i]))
            {
                ui->textEdit_htmlCode->append("Parsing " + url + parts[i] + " failed");
                return;
            }
            else
            {
                ui->textEdit_htmlCode->append("Parsing " + url + parts[i] + " complete");
            }
        }
    }
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

void MainWindow::generateResults()
{
    ui->textEdit_htmlCode->setPlainText(torikumiResults2Html(
            ui->comboBox_year->currentIndex() + 2002,
            ui->comboBox_basho->currentIndex() * 2 + 1,
            ui->comboBox_day->currentIndex() + 1,
            ui->comboBox_division->currentIndex() + 1));

    ui->textEdit_htmlPreview->setHtml(ui->textEdit_htmlCode->toPlainText());
}

void MainWindow::generateBBCodeTorikumi()
{
    int year = ui->comboBox_year->currentIndex() + 2002;
    int month = ui->comboBox_basho->currentIndex() * 2 + 1;
    int day = ui->comboBox_day->currentIndex() + 1;

    QString makuuchi = torikumi2BBCode(year, month, day, 1);
    QString juryo    = torikumi2BBCode(year, month, day, 2);

    ui->textEdit_htmlCode->setPlainText(juryo + "\n" + makuuchi);

    QString makushita= torikumi2BBCode(year, month, day, 3);
    QString sandanme = torikumi2BBCode(year, month, day, 4);
    QString jonidan  = torikumi2BBCode(year, month, day, 5);
    QString jonokuchi= torikumi2BBCode(year, month, day, 6);

    ui->textEdit_htmlPreview->setPlainText(jonokuchi + "\n" + jonidan + "\n" + sandanme + "\n" + makushita);
}

void MainWindow::generateBBCodeResults()
{
    int year = ui->comboBox_year->currentIndex() + 2002;
    int month = ui->comboBox_basho->currentIndex() * 2 + 1;
    int day = ui->comboBox_day->currentIndex() + 1;

    QString makuuchi = torikumiResults2BBCode(year, month, day, 1);
    QString juryo    = torikumiResults2BBCode(year, month, day, 2);

    ui->textEdit_htmlCode->setPlainText(juryo + "\n" + makuuchi);

    QString makushita= torikumiResults2BBCode(year, month, day, 3);
    QString sandanme = torikumiResults2BBCode(year, month, day, 4);
    QString jonidan  = torikumiResults2BBCode(year, month, day, 5);
    QString jonokuchi= torikumiResults2BBCode(year, month, day, 6);

    ui->textEdit_htmlPreview->setPlainText(jonokuchi + "\n" + jonidan + "\n" + sandanme + "\n" + makushita);
}


bool MainWindow::parsingTorikumi3456(QString content, int basho, int year, int month, int day, int division)
{
    int dayx = day, id_local = 0;
    int rikishi1 = 0, rikishi2 = 0;

    content.truncate(content.indexOf(QString("<!-- /BASYO CONTENTS -->")));
    content = content.mid(content.indexOf(QString("<!-- /BASYO TITLE -->"))).simplified();

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
                            rikishi1, shikona1, rank1, result1 == WinMark ? 1:0,
                            rikishi2, shikona2, rank2, result2 == WinMark ? 1:0,
                            kimarite,
                            id_local))
        {
            qDebug() << "-";
            return false;
        }
    }

    return true;
}

bool MainWindow::parsingTorikumi12(QString content, int basho, int year, int month, int day, int division)
{
    int dayx = day, id_local = 0;

    content = content.mid(content.indexOf(QString::fromUtf8("<div class=\"torikumi_boxbg\">")));
    content.truncate(content.indexOf(QString::fromUtf8("<!-- /BASYO CONTENTS -->")));

    while (content.indexOf(QString::fromUtf8("<tr>")) != -1)
    {
        content = content.mid(content.indexOf(QString::fromUtf8("<tr>")));

        QString contentRow = content.simplified();
        contentRow.truncate(contentRow.indexOf(QString::fromUtf8("</tr>")));

        if (contentRow.indexOf(QString::fromUtf8("優勝決定戦")) != -1)
        {
            dayx = 16;
            id_local = 0;
            //qDebug() << "ketteisen";
        }

        QRegExp rx("class=\"torikumi_riki2\">"
                   "(.{1,6})"     // rank1
                   "</td>"

                   ".*<span class=\"torikumi_btxt\">"
                   ".*(<a\\D+)?(\\d+)?(.+>)?([^\\s][^<]*)(</a>)?.*"     // '<a..' + id1 + '..>' + shikona1 + '</a>'
                   "</span>"

                   "(.+)"     // res1 + kimarite + res2

                   ".*<span class=\"torikumi_btxt\">"
                   ".*(<a\\D+)?(\\d+)?(.+>)?([^\\s][^<]*)(</a>)?.*"     // '<a..' + id2 + '..>' + shikona2 + '</a>'
                   "</span>"

                   ".*class=\"torikumi_riki2\">"
                   "(.{1,6})"     // rank2
                   "</td>");

        if (rx.indexIn(contentRow) != -1)
        {
            QString rank1, id1, shikona1, res1;
            QString rank2, id2, shikona2, res2;
            QString kimarite;

            QRegExp rxResults(".*class=\"torikumi_riki3\">(.{1})</td>"
                              ".*class=\"torikumi_riki3\">.*(<a.+>)?([^\\s][^<]+)(</a>)?.*</td>"
                              ".*class=\"torikumi_riki3\">(.{1})</td>");

            rank1    = rx.cap(1);
            id1      = rx.cap(3);
            shikona1 = rx.cap(5);

            QString results = rx.cap(7);
            if (rxResults.indexIn(results) != -1)
            {
                res1     = rxResults.cap(1);
                kimarite = rxResults.cap(3);
                res2     = rxResults.cap(5);
            }

            int result1, result2;

            if (res1.isEmpty())
            {
                result1 = -1;
            }
            else
            {
                result1 = res1 == WinMark ? 1:0;
            }

            if (res2.isEmpty())
            {
                result2 = -1;
            }
            else
            {
                result2 = res2 == WinMark ? 1:0;
            }

            id2      = rx.cap(9);
            shikona2 = rx.cap(11);
            rank2    = rx.cap(13);

            //qDebug() << rx.cap(1) << rx.cap(4) << rx.cap(5) << rx.cap(6) << rx.cap(7) << rx.cap(8);
            //qDebug() << rank1 << id1 << shikona1 << res1 << kimarite << res2 << id2 << shikona2 << rank2;

            ++id_local;

            int index = (((basho) * 100 + dayx) * 10 + division) * 100 + id_local;

            if (!insertTorikumi(index, basho, year, month, dayx,
                                division,
                                id1.toInt(), shikona1, rank1, result1,
                                id2.toInt(), shikona2, rank2, result2,
                                kimarite,
                                id_local))
            {
                qDebug() << "-";
                return false;
            }
        }

        content = content.mid(content.indexOf(QString::fromUtf8("</tr>")));
    }

    return true;
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

    int year = 0, month = 0;
    int basho, division, day;

    fName.replace('.', '_');
    QStringList tempList= fName.split('_');
    basho    = tempList.at(1).toInt();
    division = tempList.at(2).toInt();
    day      = tempList.at(3).toInt();

    QRegExp rx(QString::fromUtf8("(\\d{4})年(\\d{1,2})月(\\d{1,2})日"));

    if (rx.indexIn(content) != -1)
    {
        year  = rx.cap(1).toInt();
        month = rx.cap(2).toInt();
    }
    else
    {
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
        {
            return false;
        }
    }

    bool parsingResult;
    if (division <= 2)
    {
        parsingResult = parsingTorikumi12(content, basho, year, month, day, division);
    }
    else
    {
        parsingResult = parsingTorikumi3456(content, basho, year, month, day, division);
    }

    return parsingResult;
}

void MainWindow::importAllTorikumi()
{
    QDir dir(WORK_DIR "torikumi/");

    QStringList filters;
    filters << "tori_*_*_*.html";
    dir.setNameFilters(filters);

    QStringList list = dir.entryList();

    ui->textEdit_htmlCode->clear();

    bool importResult;
    for (int i = 0; i < list.count(); i++)
    {
        importResult = importTorikumi(dir.absoluteFilePath(list.at(i)));
        ui->textEdit_htmlCode->append(dir.filePath(list.at(i)) + ": " + QString(importResult == true ? "Ok":"*** Error ***"));
    }
}

void MainWindow::importAllHoshitori()
{
    QDir dir(WORK_DIR "hoshitori/");

    QStringList filters;
    filters << "hoshi_*_*.html";
    dir.setNameFilters(filters);

    QStringList list = dir.entryList();

    ui->textEdit_htmlCode->clear();

    bool importResult;
    for (int i = 0; i < list.count(); i++)
    {
        importResult = importHoshitori(dir.absoluteFilePath(list.at(i)));
        ui->textEdit_htmlCode->append(dir.filePath(list.at(i)) + ": " + QString(importResult == true ? "Ok":"*** Error ***"));
    }
}

bool MainWindow::insertBanzuke(int year, int month, QString rank, int position, int side,
                               int rikishi, QString shikona, QString hiragana)
{
    QSqlQuery query(db);

    int rank_id = 0;
    query.prepare("SELECT id FROM rank WHERE kanji = :kanji");
    query.bindValue(":kanji", rank);
    query.exec();
    if (query.next())
    {
        rank_id = query.value(0).toInt();
    }
    else
    {
        return false;
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
    {
        return false;
    }

    int id = ((basho * 100 + rank_id) * 1000 + position) * 10 + side;

    query.prepare("DELETE FROM banzuke WHERE id = :id");
    query.bindValue(":id",       id);
    query.exec();

    query.prepare("INSERT INTO banzuke ("
                  "id, year, month, rank, position, side, rikishi, shikona, hiragana) "
                  "VALUES ("
                  ":id, :year, :month, :rank, :position, :side, :rikishi, :shikona, :hiragana)");

    query.bindValue(":id",       id);
    query.bindValue(":year",     year);
    query.bindValue(":month",    month);
    query.bindValue(":rank",     rank_id);
    query.bindValue(":position", position);
    query.bindValue(":side",     side);
    query.bindValue(":rikishi",  rikishi);
    query.bindValue(":shikona",  shikona);
    query.bindValue(":hiragana", hiragana);

    return query.exec();
}

bool MainWindow::parsingBanzuke12(QString content)
{
    content = content.mid(content.indexOf(QString::fromUtf8("<strong>■")));
    content = content.mid(content.indexOf(">") + 2);
    QString division = content.left(content.indexOf("<")).simplified();

    int year, month;

    // 平成22年12月21日更新
    QRegExp rx(QString::fromUtf8("平成(\\d{1,2})年(\\d{1,2})月(\\d{1,2})日"));
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
        int id1 = 0;
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
            if (!insertBanzuke(year, month, rank, position, 0, id1, kanji1, ""))
            {
                qDebug() << "-";
                return false;
            }

        content = content.mid(content.indexOf(QString::fromUtf8("=西=")));

        QString contentWest = content;
        if (contentWest.indexOf(QString::fromUtf8("=東=")) != -1)
            contentWest.truncate(contentWest.indexOf(QString::fromUtf8("=東=")));

        QString kanji2;
        int id2 = 0;
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
            if (!insertBanzuke(year, month, rank, position, 1, id2, kanji2, ""))
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
    QRegExp rx(QString::fromUtf8("平成(\\d{1,2})年(\\d{1,2})月(\\d{1,2})日"));
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
            if (!insertBanzuke(year, month, division, position.toInt(), 0, 0, kanji1, hiragana1))
            {
                qDebug() << "-";
                return false;
            }

        if (!kanji2.isEmpty())
            if (!insertBanzuke(year, month, division, position.toInt(), 1, 0, kanji2, hiragana2))
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

    bool parsingResult;
    if (division <= 2)
    {
        parsingResult = parsingBanzuke12(content);
    }
    else
    {
        parsingResult = parsingBanzuke3456(content);
    }

    return parsingResult;
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

    QString prevRank;
    int position = 1;

    while (content.indexOf(QString::fromUtf8("<div class=\"torikumi_boxbg\"><table")) != -1)
    {
        content = content.mid(content.indexOf(QString::fromUtf8("<div class=\"torikumi_boxbg\"><table")));

        QString contentRow = content;
        contentRow.truncate(contentRow.indexOf(QString::fromUtf8("</div>")));

        QString contentEast = contentRow;
        contentEast = contentEast.mid(contentEast.indexOf(QString::fromUtf8("<td width=\"263\" valign=\"top\">")));
        contentEast = contentEast.mid(1);

        QString contentWest = contentEast;

        contentEast.truncate(contentEast.indexOf((QString::fromUtf8("<td width=\"263\" valign=\"top\">"))));
        contentWest = contentWest.mid(contentWest.indexOf(QString::fromUtf8("<td width=\"263\" valign=\"top\">")));

        QString rankEast = contentEast;
        rankEast.replace(QRegExp(".*class=\"common12-18-fff\">([^<]*)</td>.*"), "\\1");
        if (rankEast == contentEast)
        {
            rankEast.clear();
        }

        int idEast = 0;
        QString tempIdEast = contentEast;
        tempIdEast.replace(QRegExp(".*/ozumo_meikan/rikishi_joho/rikishi_([^<]*)\\.html.*"), "\\1");
        if (tempIdEast == contentEast)
        {
            tempIdEast.clear();
        }
        else
        {
            idEast = tempIdEast.toInt();
        }

        QString shikonaEast = contentEast;
        shikonaEast.replace(QRegExp(".*<strong>([^<]*)</strong><.*"), "\\1");
        if (shikonaEast == contentEast)
        {
            shikonaEast.clear();
        }

/*        qDebug() << "********** RANK EAST ************" << rankEast;
        qDebug() << "********** ID   EAST ************" << idEast;
        qDebug() << "******* SHIKONA EAST ************" << shikonaEast;*/

        QString rankWest = contentWest;
        rankWest.replace(QRegExp(".*class=\"common12-18-fff\">([^<]*)</td>.*"), "\\1");
        if (rankWest == contentWest)
        {
            rankWest.clear();
        }

        int idWest = 0;
        QString tempIdWest = contentWest;
        tempIdWest.replace(QRegExp(".*/ozumo_meikan/rikishi_joho/rikishi_([^<]*)\\.html.*"), "\\1");
        if (tempIdWest == contentWest)
        {
            tempIdWest.clear();
        }
        else
        {
            idWest = tempIdWest.toInt();
        }

        QString shikonaWest = contentWest;
        shikonaWest.replace(QRegExp(".*<strong>([^<]*)</strong><.*"), "\\1");
        if (shikonaWest == contentWest)
        {
            shikonaWest.clear();
        }

/*        qDebug() << "********** RANK WEST ************" << rankWest;
        qDebug() << "********** ID   WEST ************" << idWest;
        qDebug() << "******* SHIKONA WEST ************" << shikonaWest;*/

        QString rank;
        rank = rankEast.isEmpty() ? rankWest : rankEast;

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
            {
                position = 8;
            }
        }
        else
        {
            position++;
        }
        rank.truncate(2);

        /*qDebug() << "****** POSITION *****************" << position;*/

        if (!shikonaEast.isEmpty())
            if (!insertBanzuke(year, month, rank, position, 0, idEast, shikonaEast, ""))
            {
                qDebug() << "-";
                return false;
            }

        if (!shikonaWest.isEmpty())
            if (!insertBanzuke(year, month, rank, position, 1, idWest, shikonaWest, ""))
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

    int tsukedashi_position = 0;
    int tsukedashi_side = 2;
    while (content.indexOf(QString::fromUtf8("hoshitori_riki3-1")) != -1)
    {
        content = content.mid(content.indexOf(QString::fromUtf8("hoshitori_riki3-1")));
        content = content.mid(content.indexOf(">") + 1).simplified();
        int position = content.left(content.indexOf("<")).toInt();

        content = content.mid(content.indexOf(QString::fromUtf8("hoshitori_riki5")));
        content = content.mid(content.indexOf(">") + 1).simplified();
        QString shikona = content.left(content.indexOf("<"));

        content = content.mid(content.indexOf(QString::fromUtf8("hoshitori_riki5")));
        content = content.mid(content.indexOf(">") + 1).simplified();
        QString hiragana = content.left(content.indexOf("<"));

        content = content.mid(content.indexOf(QString::fromUtf8("hoshitori_riki3-1")) + 1);

        //qDebug() << rank << shikona;

        if (!shikona.isEmpty())
        {
            if (position != 0)
            {
                tsukedashi_position = position;
                if (!insertBanzuke(year, month, rank, position, side, 0, shikona, hiragana))
                {
                    qDebug() << "-";
                    return false;
                }
            }
            else
            {
                if (!insertBanzuke(year, month, rank, tsukedashi_position, side + tsukedashi_side, 0, shikona, hiragana))
                {
                    qDebug() << "-";
                    return false;
                }
                tsukedashi_side <<= 1;
            }
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

    bool parsingResult;
    if (division <= 2)
    {
        parsingResult = parsingHoshitori12(content, basho, division, side);
    }
    else
    {
        parsingResult = parsingHoshitori3456(content, basho, division, side - 1);
    }

    return parsingResult;
}

void MainWindow::generateHoshitori()
{
    ui->textEdit_htmlCode->setPlainText(hoshitori2Html(
            ui->comboBox_year->currentIndex() + 2002,
            ui->comboBox_basho->currentIndex() * 2 + 1,
            ui->comboBox_day->currentIndex() + 1,
            ui->comboBox_division->currentIndex() + 1));

    ui->textEdit_htmlPreview->setHtml(ui->textEdit_htmlCode->toPlainText());
}

QString MainWindow::hoshitori2Html(int year, int month, int day, int division)
{
    int trClass = 0;
    QString className[] = {"\"odd\"", "\"even\""};
    QString Html = "<!--"
                   " year:"     + QString::number(year    ) +
                   " month:"    + QString::number(month   ).rightJustified(2, '0') +
                   " day:"      + QString::number(day     ).rightJustified(2, '0') +
                   " division:" + QString::number(division) +
                   " -->\n";

    Html += "<table>\n"
            "<thead><tr>"
            "<th width=\"40%\">" + QString::fromUtf8("Восток") + "</th>"
            "<th width=\"20%\">" + QString::fromUtf8("Ранг")   + "</th>"
            "<th width=\"40%\">" + QString::fromUtf8("Запад")  + "</th>"
            "</tr></thead>\n"
            "<tbody>\n";

    QSqlQuery rankQuery(db);
    rankQuery.prepare("SELECT id FROM rank WHERE division_id = :division");
    rankQuery.bindValue(":division", division);
    rankQuery.exec();

    while (rankQuery.next())
    {
        int rank = rankQuery.value(0).toInt();

        QSqlQuery query(db);

        QString rankRu;
        query.prepare("SELECT ru FROM rank WHERE id = :rank");
        query.bindValue(":rank", rank);
        query.exec();
        if (query.next())
        {
            rankRu = query.value(0).toString();
        }

        int numOfRows = 0;
        query.prepare("SELECT MAX (position) "
                      "FROM banzuke "
                      "WHERE year = :year AND month = :month AND rank = :rank");
        query.bindValue(":year",  year);
        query.bindValue(":month", month);
        query.bindValue(":rank",  rank);
        query.exec();
        if (query.next())
        {
            numOfRows = query.value(0).toInt();
        }

        for (int row = 1; row <= numOfRows; row++)
        {
            QString shikona[2], res[2], history[2];
            int rikishiId[2];

            for (int ext = 0; ext <= 4; ext ++, ext ++)
            {
                shikona[0] = shikona[1] = "";
                res[0]     = res[1]     = "";
                history[0] = history[1] = "";

                for (int side = 0; side < 2; side ++)
                {
                    query.prepare("SELECT rikishi, shikona "
                                  "FROM banzuke "
                                  "WHERE year = :year AND month = :month AND rank = :rank AND position = :position AND side = :side");
                    query.bindValue(":year",  year);
                    query.bindValue(":month", month);
                    query.bindValue(":rank",  rank);
                    query.bindValue(":position", row);
                    query.bindValue(":side",  side + ext);
                    query.exec();

                    if (query.next())
                    {
                        rikishiId[side] = query.value(0).toInt();
                        shikona[side] = query.value(1).toString();

                        res[side] += " (" + QString::number(getNumOfBoshi(year, month, day, shikona[side], 1)) +
                                     "-"  + QString::number(getNumOfBoshi(year, month, day, shikona[side], 0)) + ")";

                        for (int d = 1; d <= day; d++)
                        {
                            QSqlQuery tmpQuery(db);

                            tmpQuery.prepare("SELECT result1 FROM torikumi WHERE shikona1 = :s1 AND year = :y1 AND month = :m1 AND day = :d1 "
                                             "UNION "
                                             "SELECT result2 FROM torikumi WHERE shikona2 = :s2 AND year = :y2 AND month = :m2 AND day = :d2 ");

                            tmpQuery.bindValue(":s1", shikona[side]);
                            tmpQuery.bindValue(":s2", shikona[side]);
                            tmpQuery.bindValue(":y1", year);
                            tmpQuery.bindValue(":y2", year);
                            tmpQuery.bindValue(":m1", month);
                            tmpQuery.bindValue(":m2", month);
                            tmpQuery.bindValue(":d1", d);
                            tmpQuery.bindValue(":d2", d);

                            tmpQuery.exec();

                            if (tmpQuery.next())
                            {
                                QString r = tmpQuery.value(0).toInt() == 1 ? WinMark:LossMark;
                                history[side].append(r);
                            }
                            else
                            {
                                history[side].append(DashMark);
                            }

                            //history[side].append("");
                        }
                    }
                }

                if (!shikona[0].isEmpty() || !shikona[1].isEmpty())
                {
                    Html += "<tr class=" + className[trClass] + ">"
                            "<td><strong>" + translateShikona(shikona[0], year, month) + "</strong> " + res[0] + "<br/>"
                            "<font style=\"font-family: monospace; letter-spacing:4px;\">" + history[0].simplified() + "</font></td>"
                            "<td>" + rankRu + ((rank >= 5) ? "&nbsp;" + QString::number(row) : "") + ((ext >= 2) ? "*" : "") + "</td>"
                            "<td><strong>" + translateShikona(shikona[1], year, month) + "</strong> " + res[1] + "<br/>"
                            "<font style=\"font-family: monospace; letter-spacing:4px;\">" + history[1].simplified() + "</font></td></tr>\n";

                    trClass ^= 1;
                }
            }
        }
    }

    Html += "</tbody>\n</table>\n";

    return Html;
}

void MainWindow::statistics()
{
    ui->textEdit_htmlCode->clear();
    ui->textEdit_htmlCode->append("Database content:");

    QSqlQuery query(db);

    query.exec("SELECT tbl_name FROM sqlite_master WHERE type = 'table' ORDER BY tbl_name");
    while (query.next())
    {
        QString tblName = query.value(0).toString();

        QSqlQuery tblQuery(db);

        tblQuery.exec("SELECT COUNT(*) FROM " + tblName);
        while (tblQuery.next())
        {
            ui->textEdit_htmlCode->append(tblName + ": " + tblQuery.value(0).toString());
        }
    }
}

QString MainWindow::torikumiResults2BBCode(int year, int month, int day, int division)
{
    QSqlQuery query(db);

    QString BBCode;
    QString divisionRu;

    BBCode = "[color=#0000DF]" + QString::number(year) + "/" + QString::number(month).rightJustified(2, '0') +
            QString::fromUtf8(", день ") + QString::number(day) + "[/color]\n\n";

    query.exec("SELECT ru FROM division WHERE id = " + QString::number(division));
    if (query.next())
    {
        divisionRu = query.value(0).toString();
        divisionRu[0] = divisionRu[0].toUpper();
    }

    BBCode += "[color=#0000DF]" + divisionRu + "[/color]\n\n";

    query.prepare("SELECT id_local, shikona1, result1, shikona2, result2, kimarite, rank1, rank2 "
                  "FROM torikumi "
                  "WHERE year = :year AND month = :month AND day = :day AND division = :division "
                  "ORDER BY id_local");
    query.bindValue(":year", year);
    query.bindValue(":month", month);
    query.bindValue(":day", day);
    query.bindValue(":division", division);

    query.exec();

    while (query.next())
    {
        //int id = query.value(0).toInt();
        QString shikona1 = query.value(1).toString();
        QString result1  = query.value(2).toInt() == 1 ? WinMark:LossMark;
        QString shikona2 = query.value(3).toString();
        QString result2  = query.value(4).toInt() == 1 ? WinMark:LossMark;
        QString kimarite = query.value(5).toString();
        QString rank1    = query.value(6).toString();
        QString rank2    = query.value(7).toString();

        if (kimarite == Fuzen)
        {
            result1  = query.value(2).toInt() == 1 ? FuzenWin:FuzenLoss;
            result2  = query.value(4).toInt() == 1 ? FuzenWin:FuzenLoss;
        }

        QSqlQuery tmpQuery(db);

        QString res1, res2;
        res1 += " (" + QString::number(getNumOfBoshi(year, month, day, shikona1, 1)) +
                "-"  + QString::number(getNumOfBoshi(year, month, day, shikona1, 0)) + ")";

        res2 += " (" + QString::number(getNumOfBoshi(year, month, day, shikona2, 1)) +
                "-"  + QString::number(getNumOfBoshi(year, month, day, shikona2, 0)) + ")";

        int side1, title1, pos1;
        int side2, title2, pos2;

        QString rank1Ru, rank2Ru;
        QString side1Ru, side2Ru;

        tmpQuery.prepare("SELECT rank, position, side FROM banzuke WHERE year = :y AND month = :m AND shikona = :s");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":s", shikona1);
        tmpQuery.exec();
        if (tmpQuery.next())
        {
            title1 = tmpQuery.value(0).toInt();
            pos1   = tmpQuery.value(1).toInt();
            side1  = tmpQuery.value(2).toInt();
        }
        else
        {
            splitRank(rank1, &side1, &title1, &pos1);
        }

        tmpQuery.prepare("SELECT rank, position, side FROM banzuke WHERE year = :y AND month = :m AND shikona = :s");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":s", shikona2);
        tmpQuery.exec();
        if (tmpQuery.next())
        {
            title2 = tmpQuery.value(0).toInt();
            pos2   = tmpQuery.value(1).toInt();
            side2  = tmpQuery.value(2).toInt();
        }
        else
        {
            splitRank(rank2, &side2, &title2, &pos2);
        }

        side1Ru = side1 == 0 ? QString::fromUtf8("в"):QString::fromUtf8("з");
        side2Ru = side2 == 0 ? QString::fromUtf8("в"):QString::fromUtf8("з");

        tmpQuery.prepare("SELECT ru_short FROM rank WHERE id = :id");
        tmpQuery.bindValue(":id", title1);
        tmpQuery.exec();
        if (tmpQuery.next())
            rank1Ru = tmpQuery.value(0).toString();

        tmpQuery.prepare("SELECT ru_short FROM rank WHERE id = :id");
        tmpQuery.bindValue(":id", title2);
        tmpQuery.exec();
        if (tmpQuery.next())
            rank2Ru = tmpQuery.value(0).toString();

        BBCode += phpBBcolor[title1] +
                (rank1Ru + QString::number(pos1) + side1Ru).rightJustified(6, ' ') + "  " +
                (translateShikona(shikona1, year, month) + res1).leftJustified(20, ' ') + "[/color]" +
                phpBBcolor[0] +
                result1.leftJustified( 4, ' ') +
                translateKimarite(kimarite).leftJustified(16, ' ') +
                result2.leftJustified( 4, ' ') + "[/color]" +
                phpBBcolor[title2] +
                (rank2Ru + QString::number(pos2) + side2Ru).rightJustified(6, ' ') + "  " +
                (translateShikona(shikona2, year, month) + res2).leftJustified(20, ' ') + "[/color]" + "\n";
    }

    return BBCode;
}

QString MainWindow::torikumi2BBCode(int year, int month, int day, int division)
{
    QSqlQuery query(db);

    QString BBCode;
    QString divisionRu;

    BBCode = "[color=#0000DF]" + QString::number(year) + "/" + QString::number(month).rightJustified(2, '0') +
            QString::fromUtf8(", день ") + QString::number(day) + "[/color]\n\n";

    query.exec("SELECT ru FROM division WHERE id = " + QString::number(division));
    if (query.next())
    {
        divisionRu = query.value(0).toString();
        divisionRu[0] = divisionRu[0].toUpper();
    }

    BBCode += "[color=#0000DF]" + divisionRu + "[/color]\n\n";

    query.prepare("SELECT id_local, shikona1, rank1, shikona2, rank2, basho "
                  "FROM torikumi "
                  "WHERE year = :year AND month = :month AND day = :day AND division = :division "
                  "ORDER BY id_local");
    query.bindValue(":year", year);
    query.bindValue(":month", month);
    query.bindValue(":day", day);
    query.bindValue(":division", division);

    query.exec();

    while (query.next())
    {
        //int id = query.value(0).toInt();
        QString shikona1 = query.value(1).toString();
        QString rank1    = query.value(2).toString();
        QString shikona2 = query.value(3).toString();
        QString rank2    = query.value(4).toString();
        int basho = query.value(5).toInt();

        QSqlQuery tmpQuery(db);

        QString res1, res2;
        res1 += " (" + QString::number(getNumOfBoshi(year, month, day-1, shikona1, 1)) +
                "-"  + QString::number(getNumOfBoshi(year, month, day-1, shikona1, 0)) + ")";

        res2 += " (" + QString::number(getNumOfBoshi(year, month, day-1, shikona2, 1)) +
                "-"  + QString::number(getNumOfBoshi(year, month, day-1, shikona2, 0)) + ")";
        QString history;
        for (int i = 1; i <= 6; i++)
        {
            tmpQuery.prepare("SELECT result1, kimarite FROM torikumi WHERE shikona1 = :s1a AND shikona2 = :s2a AND basho = :ba "
                             "UNION "
                             "SELECT result2, kimarite FROM torikumi WHERE shikona2 = :s1b AND shikona1 = :s2b AND basho = :bb ");

            tmpQuery.bindValue(":s1a", shikona1);
            tmpQuery.bindValue(":s2a", shikona2);
            tmpQuery.bindValue(":s1b", shikona1);
            tmpQuery.bindValue(":s2b", shikona2);
            tmpQuery.bindValue(":ba", basho - i);
            tmpQuery.bindValue(":bb", basho - i);
            tmpQuery.exec();
            if (tmpQuery.next())
            {
                if (tmpQuery.value(1).toString() == Fuzen)
                {
                    history.prepend(tmpQuery.value(0).toInt() == 1 ? FuzenWin:FuzenLoss);
                }
                else
                {
                    history.prepend(tmpQuery.value(0).toInt() == 1 ? WinMark:LossMark);
                }
            }
            else
                history.prepend(DashMark);
            history.prepend(" ");
        }

        int side1, title1, pos1;
        int side2, title2, pos2;
        splitRank(rank1, &side1, &title1, &pos1);
        splitRank(rank2, &side2, &title2, &pos2);

        QString rank1Ru, rank2Ru;
        QString side1Ru, side2Ru;

        tmpQuery.prepare("SELECT rank, position, side FROM banzuke WHERE year = :y AND month = :m AND shikona = :s");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":s", shikona1);
        tmpQuery.exec();
        if (tmpQuery.next())
        {
            title1 = tmpQuery.value(0).toInt();
            pos1   = tmpQuery.value(1).toInt();
            side1  = tmpQuery.value(2).toInt();
        }

        tmpQuery.prepare("SELECT rank, position, side FROM banzuke WHERE year = :y AND month = :m AND shikona = :s");
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.bindValue(":s", shikona2);
        tmpQuery.exec();
        if (tmpQuery.next())
        {

            title2 = tmpQuery.value(0).toInt();
            pos2   = tmpQuery.value(1).toInt();
            side2  = tmpQuery.value(2).toInt();
        }

        side1Ru = side1 == 0 ? QString::fromUtf8("в"):QString::fromUtf8("з");
        side2Ru = side2 == 0 ? QString::fromUtf8("в"):QString::fromUtf8("з");

        tmpQuery.prepare("SELECT ru_short FROM rank WHERE id = :id");
        tmpQuery.bindValue(":id", title1);
        tmpQuery.exec();
        if (tmpQuery.next())
            rank1Ru = tmpQuery.value(0).toString();

        tmpQuery.prepare("SELECT ru_short FROM rank WHERE id = :id");
        tmpQuery.bindValue(":id", title2);
        tmpQuery.exec();
        if (tmpQuery.next())
            rank2Ru = tmpQuery.value(0).toString();

        BBCode += phpBBcolor[title1] + (rank1Ru + QString::number(pos1) + side1Ru).rightJustified(6, ' ') + "  " +
                  (translateShikona(shikona1, year, month) + res1).leftJustified(24, ' ') + "[/color]" +
                  phpBBcolor[0]      + history.simplified().leftJustified(20, ' ') + "[/color]" +
                  phpBBcolor[title2] + (rank2Ru + QString::number(pos2) + side2Ru).rightJustified(6, ' ') + "  " +
                  (translateShikona(shikona2, year, month) + res2).leftJustified(20, ' ') + "[/color]" + "\n";

   }

    return BBCode;
}

int MainWindow::getNumOfBoshi(int year, int month, int day, QString shikona, int boshiColor)
{
    QSqlQuery query(db);

    query.prepare("SELECT COUNT (*) FROM torikumi WHERE year = :y AND month = :m AND day <= :d "
                     "AND ((shikona1 = :s1 AND result1 = :r1) OR (shikona2 = :s2 AND result2 = :r2))");

    query.bindValue(":y", year);
    query.bindValue(":m", month);
    query.bindValue(":d", day);
    query.bindValue(":s1", shikona);
    query.bindValue(":s2", shikona);
    query.bindValue(":r1", boshiColor);
    query.bindValue(":r2", boshiColor);

    query.exec();

    if (query.next())
    {
        return query.value(0).toInt();
    }
    else
    {
        return 0;
    }
}
