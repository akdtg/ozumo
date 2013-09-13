#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore>
#include <QtGui>
#include <QtSql>

#define START_INDEX 491

#define BASE_URL    "http://www.sumo.or.jp/honbasho/"
#define BASE_URL_EN "http://www.sumo.or.jp/en/honbasho/"

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

QString color[] = { "#404040",
                    "#A00000",
                    "#A06000",
                    "#6000A0",
                    "#6000A0",
                    "#006000",
                    "#0060A0",
                    "#A000A0",
                    "#008080",
                    "#A00060",
                    "#A08000"};

#ifdef __WIN32__
#define WORK_DIR    ""
#else
#define WORK_DIR    "/mnt/memory/"
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

    ui->comboBox_year->setCurrentIndex(ui->comboBox_year->findText(QString::number(QDate::currentDate().year())));
    ui->comboBox_basho->setCurrentIndex((QDate::currentDate().month() - 1) >> 1);

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

        in.setCodec("UTF-8");

        QString content = in.readAll();

        file0.close();

        QString shikonaEn = content.mid(content.indexOf("<title>") + QString("<title>").size());
        shikonaEn.truncate(shikonaEn.indexOf(" "));
        QString id = list.at(i);
        id.replace(QRegExp("rikishi_"), "");
        id.replace(QRegExp(".html"), "");

        qDebug() << id << shikonaEn;
    }
}

QString res2mark(int result)
{
    switch (result)
    {
    case 0:
        return(LossMark);
    case 1:
        return(WinMark);
    default:
        return(DashMark);
    }
}

int mark2res(QString mark)
{
    if (mark == WinMark)
        return(1);
    else if (mark == LossMark)
        return(0);
    else
        return(-1);
}

int wgetDownload(QString url)
{
    QProcess process;

    process.start("wget -N -t 1 -T 30 " + url);
    if (process.waitForFinished(33000) == false)
        return -1;

    return process.exitCode();
}

QString MainWindow::translateShikona(QString shikona, int year, int month, bool asLink)
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

    if (asLink)
    {
        // <a href="?q=tosanoumi">Тосаноуми</a>
        tmpQuery.prepare("SELECT en FROM banzuke WHERE shikona = :k AND year = :y AND month = :m");
        tmpQuery.bindValue(":k", shikona);
        tmpQuery.bindValue(":y", year);
        tmpQuery.bindValue(":m", month);
        tmpQuery.exec();
        if (tmpQuery.next())
        {
            shikonaRu = "<a href=\"?q=" + tmpQuery.value(0).toString().toLower() + "\">" + shikonaRu + "</a>";
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
                  "ORDER BY id_local DESC");
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
                history.prepend(res2mark(tmpQuery.value(0).toInt()));
            }
            else
                history.prepend(DashMark);
            history.prepend(" ");
        }

        Html += "<tr class=" + className[trClass] + ">"
                "<td>" + translateShikona(shikona1, year, month, division < 3 ? true:false) + res1 + "</td>"
                "<td><font style=\"font-family: monospace; letter-spacing:4px;\">" + history.simplified() + "</font></td>"
                "<td>" + translateShikona(shikona2, year, month, division < 3 ? true:false) + res2 + "</td></tr>\n";

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
                  "ORDER BY id_local DESC");
    query.bindValue(":year", year);
    query.bindValue(":month", month);
    query.bindValue(":day", day);
    query.bindValue(":division", division);

    query.exec();

    while (query.next())
    {
        //int id = query.value(0).toInt();
        QString shikona1 = query.value(1).toString();

        QString result1 = res2mark(query.value(2).toInt());

        QString shikona2 = query.value(3).toString();

        QString result2 = res2mark(query.value(4).toInt());

        QString kimarite = query.value(5).toString();

        QString res1, res2;
        res1 += " (" + QString::number(getNumOfBoshi(year, month, day, shikona1, 1)) +
                "-"  + QString::number(getNumOfBoshi(year, month, day, shikona1, 0)) + ")";

        res2 += " (" + QString::number(getNumOfBoshi(year, month, day, shikona2, 1)) +
                "-"  + QString::number(getNumOfBoshi(year, month, day, shikona2, 0)) + ")";

        Html += "<tr class=" + className[trClass] + ">"
                "<td>" + translateShikona(shikona1, year, month, division < 3 ? true:false) + res1 + "</td>"
                "<td>" + result1 + "</td>"
                "<td>" + translateKimarite(kimarite) + "</td>"
                "<td>" + result2 + "</td>"
                "<td>" + translateShikona(shikona2, year, month, division < 3 ? true:false) + res2 + "</td></tr>\n";

        trClass ^= 1;
    }

    Html += "</tbody>\n</table>\n";

    return Html;
}

#define TORIKUMI_DIR  "torikumi-new"

int MainWindow::getAndImportTorikumi(int year, int month, int day, int division)
{
    // http://www.sumo.or.jp/honbasho/main/torikumi?day=17&rank=7
    QString url = BASE_URL "main/torikumi";

    QDir dir(WORK_DIR);
    dir.mkdir(TORIKUMI_DIR);

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

    url += "?day=" + QString::number(day) + "&rank=" + QString::number(division);
    QString fName = "tori_r" + QString::number(division) + "_d" + QString::number(day) + ".html";
    QString path = QString(TORIKUMI_DIR) + "/" + fName;

    int exitCode = wgetDownload(url + " -O " + path);

    if (exitCode == 0)
    {
        exitCode = -1;
        if (importTorikumi(path))
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

void MainWindow::downloadHoshitori()
{
    int year = QDate::currentDate().year();
    int month = QDate::currentDate().month();

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
    {
        return;
    }

    QStringList parts = (QStringList()
                         << "hoshi_" + QString::number(basho) + "_1_1.html"
                         << "hoshi_" + QString::number(basho) + "_1_2.html"
                         << "hoshi_" + QString::number(basho) + "_1_3.html"
                         << "hoshi_" + QString::number(basho) + "_2_4.html"
                         << "hoshi_" + QString::number(basho) + "_2_5.html"
                        );
    QString url;
    int exitCode = 0;

    for (int i = 0; i < parts.size(); i++)
    {
        url = BASE_URL "hoshitori/" + parts[i];
        exitCode = wgetDownload("-P hoshitori " + url);

        if (exitCode != 0)
        {
            ui->textEdit_htmlCode->append("Downloading " + url + " failed, Code: " + QString::number(exitCode));
            return;
        }
    }

    importAllHoshitori();
}

struct strings_pair_s
{
    QString src;
    QString dst;
};

struct strings_pair_s BanzukeJP[] =
{
    {"index?rank=1",        "div1.jp.html"},
    {"index?rank=2",        "div2.jp.html"},
    {"index?rank=3",        "div3.jp.html"},
    {"index?rank=3&page=2", "div3p2.jp.html"},
    {"index?rank=4",        "div4.jp.html"},
    {"index?rank=4&page=2", "div4p2.jp.html"},
    {"index?rank=5",        "div5.jp.html"},
    {"index?rank=5&page=2", "div5p2.jp.html"},
    {"index?rank=6",        "div6.jp.html"}
};

struct strings_pair_s BanzukeEN[] =
{
    {"index?rank=1",        "div1.en.html"},
    {"index?rank=2",        "div2.en.html"},
    {"index?rank=3",        "div3.en.html"},
    {"index?rank=3&page=2", "div3p2.en.html"},
    {"index?rank=4",        "div4.en.html"},
    {"index?rank=4&page=2", "div4p2.en.html"},
    {"index?rank=5",        "div5.en.html"},
    {"index?rank=5&page=2", "div5p2.en.html"},
    {"index?rank=6",        "div6.en.html"}
};

#define BANZUKE_DIR  "banzuke-new"

void MainWindow::downloadBanzuke()
{
    ui->textEdit_htmlCode->clear();

    QDir dir(WORK_DIR);
    dir.mkdir(BANZUKE_DIR);

    int size = sizeof(BanzukeJP)/sizeof(BanzukeJP[0]);

    QString url;
    QString srcFile;
    QString outFile;
    int exitCode;

    for (int i = 0; i < size; i++)
    {
        srcFile = BanzukeJP[i].src;
        outFile = BanzukeJP[i].dst;

        url = QString(BASE_URL) + "banzuke/" + srcFile;

        exitCode = wgetDownload(url + " -O " + QString(BANZUKE_DIR) + "/" + outFile);

        if (exitCode != 0)
        {
            ui->textEdit_htmlCode->append("Downloading " + srcFile + " failed, Code: " + QString::number(exitCode));
            return;
        }
        else
        {
            ui->textEdit_htmlCode->append("Downloading " + srcFile + " complete");

            if (!importBanzuke(QString(WORK_DIR) + QString(BANZUKE_DIR) + "/" + outFile))
            {
                ui->textEdit_htmlCode->append("Parsing " + outFile + " failed");
                return;
            }
            else
            {
                ui->textEdit_htmlCode->append("Parsing " + outFile + " complete");
            }
        }

        url = QString(BASE_URL_EN) + "banzuke/" + srcFile;

        srcFile = BanzukeEN[i].src;
        outFile = BanzukeEN[i].dst;

        exitCode = wgetDownload(url + " -O " + QString(BANZUKE_DIR) + "/" + outFile);

        if (exitCode != 0)
        {
            ui->textEdit_htmlCode->append("Downloading " + srcFile + " failed, Code: " + QString::number(exitCode));
            return;
        }
        else
        {
            ui->textEdit_htmlCode->append("Downloading " + srcFile + " complete");

            if (!importBanzuke(QString(WORK_DIR) + QString(BANZUKE_DIR) + "/" + outFile))
            {
                ui->textEdit_htmlCode->append("Parsing " + outFile + " failed");
                return;
            }
            else
            {
                ui->textEdit_htmlCode->append("Parsing " + outFile + " complete");
            }
        }

    }

///    downloadHoshitori();
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

    ui->textEdit_htmlCode->setPlainText(QString::fromUtf8("[spoiler=Торикуми, старшие дивизионы][pre]\n") +
                                        juryo + "\n" + makuuchi +
                                        "[/pre][/spoiler]");

    QString makushita= torikumi2BBCode(year, month, day, 3);
    QString sandanme = torikumi2BBCode(year, month, day, 4);
    QString jonidan  = torikumi2BBCode(year, month, day, 5);
    QString jonokuchi= torikumi2BBCode(year, month, day, 6);

    ui->textEdit_htmlPreview->setPlainText(QString::fromUtf8("[spoiler=Торикуми, младшие дивизионы][pre]\n") +
                                           jonokuchi + "\n" + jonidan + "\n" + sandanme + "\n" + makushita +
                                           "[/pre][/spoiler]");
}

void MainWindow::generateBBCodeResults()
{
    int year = ui->comboBox_year->currentIndex() + 2002;
    int month = ui->comboBox_basho->currentIndex() * 2 + 1;
    int day = ui->comboBox_day->currentIndex() + 1;

    QString makuuchi = torikumiResults2BBCode(year, month, day, 1);
    QString juryo    = torikumiResults2BBCode(year, month, day, 2);

    ui->textEdit_htmlCode->setPlainText(QString::fromUtf8("[spoiler=Результаты, старшие дивизионы][pre]\n") +
                                        juryo + "\n" + makuuchi +
                                        "[/pre][/spoiler]");

    QString makushita= torikumiResults2BBCode(year, month, day, 3);
    QString sandanme = torikumiResults2BBCode(year, month, day, 4);
    QString jonidan  = torikumiResults2BBCode(year, month, day, 5);
    QString jonokuchi= torikumiResults2BBCode(year, month, day, 6);

    ui->textEdit_htmlPreview->setPlainText(QString::fromUtf8("[spoiler=Результаты, младшие дивизионы][pre]\n") +
                                           jonokuchi + "\n" + jonidan + "\n" + sandanme + "\n" + makushita +
                                           "[/pre][/spoiler]");
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
                            rikishi1, shikona1, rank1, mark2res(result1),
                            rikishi2, shikona2, rank2, mark2res(result2),
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

            int result1 = mark2res(res1), result2 = mark2res(res2);

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

    in.setCodec("UTF-8");

    QString content = in.readAll();

    file0.close();

    int year = 0, month = 0, day = 0;
    int basho = 0;
    int division = 0, dayN = 0;
    QString dayNum;
    QString rank;

    QRegExp rx;
    rx.setMinimal(true);

    // folder/tori_r2_d14.html
    rx.setPattern(QString::fromUtf8(".*_r(\\d)_d(\\d{1,2}).*"));
    if (rx.indexIn(fName) != -1)
    {
        division = rx.cap(1).toInt();
        dayN = rx.cap(2).toInt();
    }
    else
    {
        qDebug() << "the file name is invalid";
        return false;
    }

    // <span class="dayWrap">
    //     <span class="dayNum">初日</span>
    //     <span class="date">2013年9月15日(日)</span>
    //     <span class="rank">幕内</span>
    // </span>
    rx.setPattern(QString::fromUtf8(
        "<span class=\"dayWrap\">"
        ".*"
        "<span class=\"dayNum\">(\\w+)日</span>"
        ".*"
        "<span class=\"date\">(\\d{4})年(\\d{1,2})月(\\d{1,2})日.*</span>"
        ".*"
        "<span class=\"rank\">(\\w+)</span>"
        ".*"
        "</span>"
        ));

    if (rx.indexIn(content) != -1)
    {
        dayNum = rx.cap(1);
        year = rx.cap(2).toInt();
        month = rx.cap(3).toInt();
        day = rx.cap(4).toInt();
        rank = rx.cap(5);
    }
    else
    {
        qDebug() << "cannot find the date in the torikumi file" << fName;
        return false;
    }

    // qDebug () << "div" << division << "dayN" << dayN;
    // qDebug () << "dayNum" << dayNum << "year" << year << "month" << month << "day" << day << "rank" << rank;

    bool parsingResult;
    if (division <= 2)
    {
        parsingResult = parsingTorikumi12(content, basho, year, month, dayN, division);
    }
    else
    {
        parsingResult = parsingTorikumi3456(content, basho, year, month, dayN, division);
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

struct numbers_s
{
        QString c;
        int n;
};

struct numbers_s Numbers[] =
{
    {QString::fromUtf8("〇"), 0},
    {QString::fromUtf8("一"), 1},
    {QString::fromUtf8("二"), 2},
    {QString::fromUtf8("三"), 3},
    {QString::fromUtf8("四"), 4},
    {QString::fromUtf8("五"), 5},
    {QString::fromUtf8("六"), 6},
    {QString::fromUtf8("七"), 7},
    {QString::fromUtf8("八"), 8},
    {QString::fromUtf8("九"), 9},
    {QString::fromUtf8("十"), 10},
    {QString::fromUtf8("百"), 100},
    {QString::fromUtf8("千"), 1000},
    {QString::fromUtf8("万"), 10000}
};

int Chinese2Arabic(QString str)
{
    int number = 0;
    int c = 0;

    for (int i = 0; i < str.length(); i++)
    {
        for (unsigned j = 0; j < sizeof(Numbers)/sizeof(Numbers[0]); j++)
        {
            if (QString(str.at(i)) == Numbers[j].c)
            {
                c = Numbers[j].n;
            }
        }

        if (c >= 10)
            number = (number == 0) ? c : (number * c);
        else
            number += c;
    }

    return number;
}

bool MainWindow::parsingBanzuke12(QString content)
{
    int year, month;

    QRegExp rx;
    rx.setMinimal(true);

    // 平成二十五年七月場所
    rx.setPattern(QString::fromUtf8("平成(.{1,3})年(.{1,3})月場所"));
    if (rx.indexIn(content) != -1)
    {
        year = 1988 + Chinese2Arabic(rx.cap(1));
        month = Chinese2Arabic(rx.cap(2));
    }
    else
    {
        qDebug() << "the date is not found";
        return false;
    }

    QString prevRank;
    int position = 1;

    rx.setPattern(QString::fromUtf8(
        "<td class=\"east\">"
        ".*"
        "/sumo_data/rikishi/profile[?]id=(\\d+)\">(.*)&#12288;"
        ".*"
        "<td class=\"rank\">(.*)</td>"
        ".*"
        "<td class=\"west\">"
        ".*"
        "/sumo_data/rikishi/profile[?]id=(\\d+)\">(.*)&#12288;"
        ));

    QString kanji1;
    int id1 = 0;
    QString rank;
    QString kanji2;
    int id2 = 0;

    int pos = 0;
    while ((pos = rx.indexIn(content, pos)) != -1)
    {
        id1 = rx.cap(1).toInt();
        kanji1 = rx.cap(2);
        rank = rx.cap(3);
        id2 = rx.cap(4).toInt();
        kanji2 = rx.cap(5);
        pos += rx.matchedLength();

        rank.truncate(2);

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

        if (!kanji2.isEmpty())
            if (!insertBanzuke(year, month, rank, position, 1, id2, kanji2, ""))
            {
                qDebug() << "-";
                return false;
            }
    }

    return true;
}

unsigned Month2Number(QString month)
{
    QString months[] =
        { "January",
          "February",
          "March",
          "April",
          "May",
          "June",
          "July",
          "August",
          "September",
          "October",
          "November",
          "December"
        };

    for (unsigned i = 0; i < sizeof(months)/sizeof(months[0]); i++)
        if (month.compare(months[i], Qt::CaseInsensitive) == 0)
            return (i + 1);

    return 0;
}

bool MainWindow::parsingBanzuke12_EN(QString content)
{
    int year, month;

    QRegExp rx;
    rx.setMinimal(true);

    // <p class="mdDate">2013 September  Grand Sumo Tournament</p>
    rx.setPattern(QString::fromUtf8("<p class=\"mdDate\">.*(\\d{4})\\s(\\w+).*</p>"));
    if (rx.indexIn(content) != -1)
    {
        year = rx.cap(1).toInt();
        month = Month2Number(rx.cap(2));
    }
    else
    {
        qDebug() << "the date is not found";
        return false;
    }

    QString prevRank;
    int position = 1;

    rx.setPattern(QString::fromUtf8(
        "<td class=\"east\">"
        ".*"
        "/sumo_data/rikishi/profile[?]id=(\\d+)\">(\\w+)</a>"
        ".*"
        "<td class=\"rank\">(.*)</td>"
        ".*"
        "<td class=\"west\">"
        ".*"
        "/sumo_data/rikishi/profile[?]id=(\\d+)\">(\\w+)</a>"
        ));

    QString kanji1;
    int id1 = 0;
    QString rank;
    QString kanji2;
    int id2 = 0;

    int pos = 0;
    while ((pos = rx.indexIn(content, pos)) != -1)
    {
        id1 = rx.cap(1).toInt();
        kanji1 = rx.cap(2);
        rank = rx.cap(3);
        id2 = rx.cap(4).toInt();
        kanji2 = rx.cap(5);
        pos += rx.matchedLength();

        rank.truncate(2);

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
        {
            QSqlQuery query(db);
            query.prepare("UPDATE banzuke SET en = :en WHERE rikishi = :id AND year = :year AND month = :month");
            query.bindValue(":year",     year);
            query.bindValue(":month",    month);
            query.bindValue(":id",       id1);
            query.bindValue(":en",       kanji1);

            if (!query.exec())
            {
                qDebug() << "-";
                return false;
            }
        }

        if (!kanji2.isEmpty())
        {
            QSqlQuery query(db);
            query.prepare("UPDATE banzuke SET en = :en WHERE rikishi = :id AND year = :year AND month = :month");
            query.bindValue(":year",     year);
            query.bindValue(":month",    month);
            query.bindValue(":id",       id2);
            query.bindValue(":en",       kanji2);

            if (!query.exec())
            {
                qDebug() << "-";
                return false;
            }
        }
    }

    return true;
}

bool MainWindow::parsingBanzuke3456(QString content)
{
    int year, month;

    QRegExp rx;
    rx.setMinimal(true);

    // 平成二十五年七月場所
    rx.setPattern(QString::fromUtf8("平成(.{1,3})年(.{1,3})月場所"));
    if (rx.indexIn(content) != -1)
    {
        year = 1988 + Chinese2Arabic(rx.cap(1));
        month = Chinese2Arabic(rx.cap(2));
    }
    else
    {
        qDebug() << "the date is not found";
        return false;
    }

    QString division = "D";

    // <span class="dayNum">幕下 筆頭～五十枚目</span>
    rx.setPattern(QString::fromUtf8("<span class=\"dayNum\">(.{3}).*</span>"));
    if (rx.indexIn(content) != -1)
    {
        division = rx.cap(1);
        division = division.simplified();
    }
//    qDebug() << "division:" << division;

    QString prevRank;
    int position = 1;

    rx.setPattern(QString::fromUtf8(
        "<tr>.*<td.*"
        "(?:<dl class=\"player\">"
        ".*"
        "<dt>(.*)[(](.*)[)]</dt>"
        ".*)?"
        "<td class=\"num\">(.*)</td>"
        ".*"
        "(?:<dl class=\"player\">"
        ".*"
        "<dt>(.*)[(](.*)[)]</dt>)?"
        ".*</td>.*</tr>"
        ));

    QString kanji1;
    QString hiragana1;
    QString positions;
    QString kanji2;
    QString hiragana2;

    int pos = 0;
    while ((pos = rx.indexIn(content, pos)) != -1)
    {
        kanji1 = rx.cap(1);
        hiragana1 = rx.cap(2);
        positions = rx.cap(3);
        kanji2 = rx.cap(4);
        hiragana2 = rx.cap(5);

        pos += rx.matchedLength();

        if (positions == QString::fromUtf8("筆頭"))
                position = 1;
        else
                position = Chinese2Arabic(positions);

//        qDebug() << kanji1 << hiragana1 << position;
//        qDebug() << kanji2 << hiragana2 << position;

        if (!kanji1.isEmpty())
            if (!insertBanzuke(year, month, division, position, 0, 0, kanji1, hiragana1))
            {
                qDebug() << "-";
                return false;
            }

        if (!kanji2.isEmpty())
            if (!insertBanzuke(year, month, division, position, 1, 0, kanji2, hiragana2))
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

    in.setCodec("UTF-8");

    QString content = in.readAll();

    file0.close();

    QRegExp rx;
    rx.setMinimal(true);

    int locale = 0;

    // <meta property="og:locale" content="ja_JP" />
    // <meta property="og:locale" content="en_US" />
    rx.setPattern(QString::fromUtf8("<meta property=\"og:locale\" content=\"(\\w*)\" />"));
    if (rx.indexIn(content) != -1)
    {
        if (rx.cap(1) == "ja_JP")
            locale = 1;
        else if (rx.cap(1) == "en_US")
            locale = 2;
        else
            locale = 0;
    }
    else
    {
        qDebug() << "cannot get locale";
        return false;
    }

    int division = 0;

    // <li class="rank3 current">Makushita Division</li>
    // <li class="rank6 current">序ノ口</li>
    rx.setPattern(QString::fromUtf8("<li class=\"rank(\\d) current\">"));
    if (rx.indexIn(content) != -1)
    {
        division = rx.cap(1).toInt();
    }
    else
    {
        qDebug() << "cannot get division";
        return false;
    }

    bool parsingResult = true;
    if (division <= 2)
    {
        if (locale == 1)
            parsingResult = parsingBanzuke12(content);
        else
            parsingResult = parsingBanzuke12_EN(content);
    }
    else
    {
        if (locale == 1)
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

    QRegExp rx("/eng/ozumo_meikan/rikishi_joho/rikishi_(\\d+).html\"><strong>([^<]+)</strong></a>");
    QStringList list;
    int pos = 0;

    while ((pos = rx.indexIn(content, pos)) != -1)
    {
        list << rx.cap(1) << rx.cap(2);
        pos += rx.matchedLength();

        query.prepare("UPDATE banzuke SET en=:en WHERE year=:y AND month=:m AND rikishi=:rid");

        query.bindValue(":en",  rx.cap(2));
        query.bindValue(":y",   year);
        query.bindValue(":m",   month);
        query.bindValue(":rid", rx.cap(1));
        query.exec();
    }
    //qDebug() << list;

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

    in.setCodec("UTF-8");

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
            "<th width=\"33%\">" + QString::fromUtf8("Восток") + "</th>"
            "<th width=\"33%\">" + QString::fromUtf8("Ранг")   + "</th>"
            "<th width=\"33%\">" + QString::fromUtf8("Запад")  + "</th>"
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

            for (int ext = 0; ext <= 4; ext ++, ext ++)
            {
                shikona[0] = shikona[1] = "";
                res[0]     = res[1]     = "";
                history[0] = history[1] = "";

                for (int side = 0; side < 2; side ++)
                {
                    query.prepare("SELECT shikona "
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
                        shikona[side] = query.value(0).toString();

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
                                history[side].append(res2mark(tmpQuery.value(0).toInt()));
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
                            "<td><strong>" + translateShikona(shikona[0], year, month, division < 3 ? true:false) + "</strong> " + res[0] + "<br/>"
                            "<font style=\"font-family: monospace; letter-spacing:4px;\">" + history[0].simplified() + "</font></td>"
                            "<td>" + rankRu + ((rank >= 5) ? "&nbsp;" + QString::number(row) : "") + ((ext >= 2) ? "*" : "") + "</td>"
                            "<td><strong>" + translateShikona(shikona[1], year, month, division < 3 ? true:false) + "</strong> " + res[1] + "<br/>"
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
        QString result1  = res2mark(query.value(2).toInt());
        QString shikona2 = query.value(3).toString();
        QString result2  = res2mark(query.value(4).toInt());
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

        if (getPosition(year, month, shikona1, &title1, &pos1, &side1) != 0)
        {
            splitRank(rank1, &side1, &title1, &pos1);
        }

        if (getPosition(year, month, shikona2, &title2, &pos2, &side2) != 0)
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

        BBCode += "[color=" + color[title1] + "]" +
                (rank1Ru + QString::number(pos1) + side1Ru).rightJustified(6, ' ') + "  " +
                (translateShikona(shikona1, year, month, false) + res1).leftJustified(20, ' ') + "[/color]" +
                "[color=" + color[0] + "]" +
                result1.leftJustified( 4, ' ') +
                translateKimarite(kimarite).leftJustified(16, ' ') +
                result2.leftJustified( 4, ' ') + "[/color]" +
                "[color=" + color[title2] + "]" +
                (rank2Ru + QString::number(pos2) + side2Ru).rightJustified(6, ' ') + "  " +
                (translateShikona(shikona2, year, month, false) + res2).leftJustified(20, ' ') + "[/color]" + "\n";
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
                    history.prepend(res2mark(tmpQuery.value(0).toInt()));
                }
            }
            else
                history.prepend(DashMark);
            history.prepend(" ");
        }

        int side1, title1, pos1;
        int side2, title2, pos2;

        QString rank1Ru, rank2Ru;
        QString side1Ru, side2Ru;

        if (getPosition(year, month, shikona1, &title1, &pos1, &side1) != 0)
        {
            splitRank(rank1, &side1, &title1, &pos1);
        }

        if (getPosition(year, month, shikona2, &title2, &pos2, &side2) != 0)
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

        BBCode += "[color=" + color[title1] + "]" +
                  (rank1Ru + QString::number(pos1) + side1Ru).rightJustified(6, ' ') + "  " +
                  (translateShikona(shikona1, year, month, false) + res1).leftJustified(24, ' ') + "[/color]" +
                  "[color=" + color[0] + "]" + history.simplified().leftJustified(20, ' ') + "[/color]" +
                  "[color=" + color[title2] + "]" +
                  (rank2Ru + QString::number(pos2) + side2Ru).rightJustified(6, ' ') + "  " +
                  (translateShikona(shikona2, year, month, false) + res2).leftJustified(20, ' ') + "[/color]" + "\n";
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

int MainWindow::getPosition(int year, int month, QString shikona, int *title, int *pos, int *side)
{
    QSqlQuery query(db);

    query.prepare("SELECT rank, position, side FROM banzuke WHERE year = :y AND month = :m AND shikona = :s");
    query.bindValue(":y", year);
    query.bindValue(":m", month);
    query.bindValue(":s", shikona);
    query.exec();
    if (query.next())
    {
        *title = query.value(0).toInt();
        *pos   = query.value(1).toInt();
        *side  = query.value(2).toInt();

        return 0;
    }

    return -1;
}
