/*
 * Copyright (C) 2025 by Andreas Theofilu <andreas@theosys.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */
#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QCommandLineParser>

#include "tlogger.h"
#include "tconfig.h"

int main(int argc, char *argv[])
{
    TLogger::setLogLevel(LVL_ERROR);
    QApplication a(argc, argv);
    a.setApplicationName("ITPP Log Analyzer");
    a.setApplicationVersion(VERSION_STRING());
    a.setWindowIcon(QIcon("resource:/itpploganalyzer.png"));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();

    for (const QString &locale : uiLanguages)
    {
        const QString baseName = "itpploganalyzer_" + QLocale(locale).name();

        if (translator.load(":/i18n/" + baseName))
        {
            a.installTranslator(&translator);
            break;
        }
    }

    QCommandLineParser parser;

    parser.addOptions(
            {
                { "loglevel", QApplication::translate("main", "The loglevel; 0 = no logging, 6 = full logging"),
                 "loglevel" },
                { "logfile", QApplication::translate("main", "The path and name of the logfile. This is where the app writes it's internal logging!"),
                 "logfile" },
                { "file", QApplication::translate("main", "An ITPP logfile to initialy open."), "file" }
            }
        );

    parser.addHelpOption();
    parser.process(a);
    TLogger::setLogLevel(LVL_INFO);
    TConfig::readConfig();

    if (!parser.value("loglevel").isEmpty())
    {
        int ll = parser.value("loglevel").toInt();

        if (ll >= 0 && ll <= 6)
            TLogger::setLogLevel(static_cast<LOG_LEVEL_t>(ll));
    }
    else
        TLogger::setLogLevel(static_cast<LOG_LEVEL_t>(TConfig::getLogLevel()));

    if (!parser.value("logfile").isEmpty())
    {
        QString lf = parser.value("logfile");
        TLogger::setLogfile(lf.toStdString());
        TConfig::setLogfile(lf);
    }
    else
        TLogger::setLogfile(TConfig::getLogfile().toStdString());

    QString file;

    if (!parser.value("file").isEmpty())
        file = parser.value("file");

    MainWindow w(file);
    w.show();
    return a.exec();
}
