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
#include <QFileDialog>
#include <QStringDecoder>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QMessageBox>
#include <QResizeEvent>
#include <QLabel>
#include <QTextList>
#include <QProgressDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QSaveFile>
#include <QClipboard>
#include <QToolTip>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include <filesystem>
#include <iostream>
#include <algorithm>

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "expand.h"
#include "tcoloring.h"
#include "tthreadselect.h"
#include "tqtsettings.h"
#include "tconfig.h"
#include "tlogger.h"

#define BUFFER_SIZE     16384
#define APPNAME         "logviewer"

#define TYPE_OK         0
#define TYPE_ERR        1
#define TYPE_WARN       2
#define TYPE_INFO       3
#define TYPE_DEBUG      4

namespace fs = std::filesystem;
using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::min;

using VALTYPES_t = TValueSelect::VALTYPES_t;
using VALUES_t = TValueSelect::VALUES_t;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    DECL_TRACER("MainWindow::MainWindow(QWidget *parent)");

    initialize();
}

MainWindow::MainWindow(QString& file, QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      mFile(file)
{
    DECL_TRACER("MainWindow::MainWindow(QString& file, QWidget *parent)");

    initialize();
    mFile = file;

    if (file.isEmpty())
        return;

    if (!mFile.isEmpty() && fs::exists(file.toStdString()) && fs::is_regular_file(file.toStdString()))
        parseFile();
    else
        QMessageBox::warning(this, APPNAME, tr("The logfile is not valid or not readable!"));
}

MainWindow::~MainWindow()
{
    DECL_TRACER("MainWindow::~MainWindow()");

    delete ui;
    TConfig::saveConfig();
}

void MainWindow::initialize()
{
    DECL_TRACER("MainWindow::initialize()");

    ui->setupUi(this);

    ui->actionFilter_thread->setChecked(true);
    ui->textEditResult->setAcceptRichText(true);
    ui->textEditResult->setReadOnly(true);
    ui->tableViewLog->setWordWrap(true);                                            // Enable word wrap
    ui->tableViewLog->setTextElideMode(Qt::ElideRight);                             // Draws elipses at the end of a line if the line is larger then the cell
    connect(ui->tableViewLog, &QTableView::pressed, this, &MainWindow::pressed);

    QRect geom = TConfig::lastGeometry();

    if (geom.width() > 0 && geom.height() > 0)
        setGeometry(geom);
}

QString MainWindow::getLogFileName(QString *filter)
{
    DECL_TRACER("MainWindow::getLogFileName(QString *filter)");

    QString file = QFileDialog::getOpenFileName(this, tr("Open Logfile"), TConfig::lastOpenPath(), tr("Log Files (*.log *.dat *.gz);;JSon (*.json *.log *.dat *.gz);;All (*)"), filter);
    qsizetype pos = file.lastIndexOf("/");

    if (pos == -1)
        TConfig::setLastOpenPath(QString::fromStdString(fs::current_path()));
    else if (pos > 0)
        TConfig::setLastOpenPath(file.left(pos));
    else
        TConfig::setLastOpenPath("/");

    return file;
}

bool MainWindow::parseFile(qsizetype totalLines, const QString& filter, const QString& thread_filter)
{
    DECL_TRACER("MainWindow::parseFile(qsizetype totalLines, const QString& filter, const QString& thread_filter)");

    if (ui->tableViewLog->model())
    {
        auto model = ui->tableViewLog->model();
        model->removeRows(0, model->rowCount());
    }

    if (mFile.isEmpty())
        return false;

    if (totalLines > 50000)                                             // Do we have more then 50000 lines?
        ui->tableViewLog->setWordWrap(false);                           // Yes, then disable wordwrap because it would take a long time to format the table
    else
        ui->tableViewLog->setWordWrap(true);

    mThreads.clear();
    QProgressDialog *progress = nullptr;
    bool canceled = false;
    QString target = mFile;

    if (mFile.endsWith(".gz"))
    {
        QString f = getFileName(mFile);
        Expand exp(mFile.toStdString());
        target = QString("/tmp/%1.temp").arg(f);

        if (fs::exists(target.toStdString()))
            fs::remove(target.toStdString());

        exp.setTemporaryFileName(target.toStdString());

        if (exp.unzip(false) == -1)
        {
            QMessageBox::critical(this, APPNAME, tr("Error unzipping file ")+f);
            return false;
        }

        mTempFile = target;
    }

    QStringList colAligns;
    QString cas = TConfig::getColAligns();

    if (!cas.isEmpty() && cas.contains(","))
        colAligns = cas.split(",", Qt::SkipEmptyParts);

    TColoring coloring;
    ifstream inFile;                                                                    // The stream used to read the file
    QStandardItemModel *model = new QStandardItemModel;                                 // The standard model holding each cell of the table
    model->setColumnCount(TConfig::getColumns());                                       // We're setting the number of columns
    QStringList headers = TConfig::headers();                                           // Get the headers from configuration
    model->setHorizontalHeaderLabels(headers);                                          // Set the headers to the columns of the table
    QStandardItem *hitem = model->horizontalHeaderItem(TConfig::getColumns()-1);        // Get the header item of the last column
    hitem->setTextAlignment(Qt::AlignLeft);                                             // Set the alignment to left (center is default)
    int lines = 0;      // Number of total lines
    int iTrace = 0;     // Number trace lines
    int iInfo = 0;      // Number info lines
    int iWarn = 0;      // Number warning lines
    int iError = 0;     // Number error lines
    int iDebug = 0;     // Number debug lines
    int iOther = 0;     // Number other lines
    int bopen = 0;      // Detects block starts
    int bclose = 0;     // Detects block ends

    if (filter.startsWith("JSon", Qt::CaseInsensitive))
    {
        MSG_INFO("Parsing a JSON file ...");

        if (TConfig::values().size() != TConfig::getColumns())
        {
            QMessageBox::warning(this, APPNAME, tr("JSON parsing was not configured!<br>Please configure JSON values first in the <i>settings</i>."));
            return false;
        }
    }

    try
    {
        inFile.open(target.toStdString());                                                  // Open file

        if (totalLines > 10000)                                                             // Do we have more then 10000 lines?
        {                                                                                   // Yes, then ...
            progress = new QProgressDialog(tr("Loading file ..."), tr("Cancel"), 0, totalLines, this);  // Allocate a progress bar
            progress->setWindowModality(Qt::WindowModal);                                   // Set the dialog with the progress bar as "modal"
            model->setRowCount(totalLines);                                                 // Set the total number of lines (progress bar will show percents)
        }

        for (string line; getline(inFile, line);)                                           // Loop over all lines in file
        {
            if (progress)                                                                   // Do we have a progress bar?
            {                                                                               // Yes, the feed it ...
                progress->setValue(lines);                                                  // Set the actual line number to the progress bar

                if (progress->wasCanceled())                                                // Did the user hit the cancel button?
                {                                                                           // Yes, then ...
                    canceled = true;                                                        // Mark the process as canceled
                    break;                                                                  // Stop the loop
                }
            }

            if (mLastFilterCheck && !thread_filter.isEmpty() && TConfig::getColumnThreadID() > 0)
            {
                if (line.find(thread_filter.toStdString()) == string::npos)
                    continue;
            }

            QStringList parts;                                                              // Holds the content of the columns
            QString qLine = QString::fromStdString(line);                                   // Assign the line to a QString class
            bool isJson = qLine.startsWith("{");                                            // If the line starts with a {, then it may be a JSON formatted line

            if (!filter.isEmpty() &&                                                        // Is the log in JSON format?
                filter.startsWith("JSon", Qt::CaseInsensitive) &&
                isJson)
            {                                                                               // Yes, then parse it first to a string
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
                QJsonDocument jdoc = QJsonDocument::fromJson(QByteArray(line.c_str()));             // Create JSON object out of string
#else
                QJsonDocument jdoc = QJsonDocument::fromJson(QByteArray(line));             // Create JSON object out of string
#endif
                QJsonObject jline = jdoc.object();                                          // Get out the base object
                QList<VALUES_t> values = TConfig::values();                                 // Get the wanted value names and types from config
                QList<VALUES_t>::iterator iter;                                             // Declare an iterator
                qLine.clear();                                                              // Clear qLine
                bool first = true;                                                          // This will be false after the first element was processed in the loop
                QString sIndex;                                                             // The name of the value is used as an index
                QJsonObject content;                                                        // Contains the object containing the wanted element

                for (iter = values.begin(); iter != values.end(); ++iter)                   // Loop through all JSON values
                {
                    if (!first)                                                             // If it is not the first element ...
                        qLine.append(TConfig::getDelimeter());                              // Append the delimiter

                    if (iter->name == values.last().name)                                   // If it is the last entry in the list ...
                        qLine.append(" ");                                                  // Append a blank to avoid cutting off first character

                    if (iter->name.contains("."))                                           // If the JSON name contains a dot (.) ...
                    {                                                                       // then we must split the name into parts because the first name is the object containing the wanted object.
                        // TODO: Make deeper objects available by looping through all parts!
                        //       This implies that arrays should also be possible
                        QStringList pa = iter->name.split(".");                             // Split the name
                        sIndex = pa[1];                                                     // Assign the value name as an index
                        content = jline[pa[0]].toObject();                                  // Get the top object
                    }
                    else
                    {
                        sIndex = iter->name;                                                // Assign the value name as an index
                        content = jline;                                                    // Assign the base object
                    }

                    switch(iter->type)                                                      // Switch through possible value types
                    {
                        case VALTYPES_t::VTYPE_STRING:
                        {
                            QString p = content[sIndex].toString(" ");                      // Get the string from the object
                            p.replace(",", " ");                                            // Replace all commas into spaces
                            qLine.append(p);                                                // Append it to the qLine
                        }
                        break;

                        case VALTYPES_t::VTYPE_INT:
                            qLine.append(QString("%1").arg(content[sIndex].toInt()));       // Append it to the qLine
                        break;

                        case VALTYPES_t::VTYPE_LONG:
                            qLine.append(QString("%1").arg(content[sIndex].toInteger()));   // Append it to the qLine
                        break;

                        case VALTYPES_t::VTYPE_FLOAT:
                        case VALTYPES_t::VTYPE_DOUBLE:
                            qLine.append(QString("%1").arg(content[sIndex].toDouble()));    // Append it to the qLine
                        break;

                        case VALTYPES_t::VTYPE_BOOL:
                            qLine.append(QString("%1").arg(content[sIndex].toBool()));      // Append it to the qLine
                        break;
                    }

                    first = false;                                                          // Mark first element as processed
                }
            }

            if (!filter.isEmpty() &&                                                        // if JSON format but line is not ...
                filter.startsWith("JSon", Qt::CaseInsensitive) &&
                !isJson)
            {
                // Here we have a line which is not in JSON format although it should be.
                // Therefore we'll put the whole line into the last column.
                for (int i = 0; i < TConfig::getColumns(); ++i)                             // Create empty colums
                    parts << QString();

                parts[parts.size()-1] = qLine;                                              // Assign whole line to last column
            }
            else if (qLine.contains(TConfig::getDelimeter()))                               // Does the line contain at least 1 delimiter?
                parts = split(qLine, TConfig::getDelimeter(), TConfig::getColumns() - 1);   // Split the line into parts seperated by the defined delimeter
            else                                                                            // No delimiters in line?
            {                                                                               // Then add whole line to the last column ...
                for (int i = 0; i < TConfig::getColumns(); ++i)                             // Create empty columns
                    parts << QString();

                parts[parts.size()-1] = qLine;                                              // Assign whole line to the last column
            }

            QStandardItem *item = nullptr;                                                  // Initialize the standard item used for the cells of the table
            QColor bgColor;                                                                 // The background color; Calculated for each row
            QColor bgThread(Qt::white);                                                     // The background color of the thread column, if there is any

            if (qLine.contains(TConfig::getTagInfo()))                                      // Test for tag INF
            {
                bgColor = TConfig::colorInfo();                                             // Set background color
                iInfo++;                                                                    // Increase counter
            }
            else if (qLine.contains(TConfig::getTagWarning()))                              // Test for tag WRN
            {
                bgColor = TConfig::colorWarning();                                          // Set background color
                iWarn++;                                                                    // Increase counter
            }
            else if (qLine.contains(TConfig::getTagError()))                                // Test for tag ERR
            {
                bgColor = TConfig::colorError();                                            // Set background color
                iError++;                                                                   // Increase counter
            }
            else if (qLine.contains(TConfig::getTagTrace()))                                // Test for tag TRC
            {
                bgColor = TConfig::colorTrace();                                            // Set background color
                iTrace++;                                                                   // Increase counter
            }
            else if (qLine.contains(TConfig::getTagDebug()))                                // Test for tag DBG
            {
                bgColor = TConfig::colorDebug();                                            // Set background color
                iDebug++;                                                                   // Increase counter
            }
            else                                                                            // Else we have other type (FNE, ...)
            {
                bgColor = QColor(Qt::white);                                                // Set background color
                iOther++;                                                                   // Increase counter
            }

            if (qLine.contains(TConfig::getBlockEntry()))                                   // Test for start of block
                bopen++;                                                                    // Increase counter
            else if (qLine.contains(TConfig::getBlockExit()))                               // Test for end of block
                bclose++;                                                                   // Increase counter

            for (int i = 0; i < TConfig::getColumns(); ++i)                                 // Loop for the defined number of columns
            {
                QString al = "l";                                                           // Assign default alignment of column to left

                if (i < colAligns.size())                                                   // Make sure we're not running out of alignment definitions
                    al = colAligns[i];                                                      // Assign alignment for current column

                int colThread = TConfig::getColumnThreadID();                               // Get the setting of the column marked as thread, if any

                if (colThread > 0 && (colThread - 1) == i && i < parts.size())              // Is there a thread column and do we have enough elements in "parts"?
                {
                    QString sthread = parts[i].trimmed();
                    bgThread = coloring.getColor(sthread);                                  // Yes, then set the color for the cell
                    bool found = false;
                    QList<TThreadSelect::THREAD_LIST_t>::iterator iter;

                    for (iter = mThreads.begin(); iter != mThreads.end(); ++iter)
                    {
                        if (iter->threadID == sthread)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (!found && !sthread.isEmpty())
                    {
                        TThreadSelect::THREAD_LIST_t tl;                                    // New element to prepare thread filter
                        tl.threadColor = bgThread;                                          // Assign thread color
                        tl.threadID = sthread;                                              // Assign thread ID
                        mThreads.append(tl);                                                // Add thread ID to list
                    }
                }

                if (i < parts.size() && i < (TConfig::getColumns() - 1))                    // Do this as long as we have data and have not reached the last column
                {
                    item = new QStandardItem(parts[i].trimmed());                           // Allocate a new item
                    item->setEditable(false);                                               // Set cell to not editable

                    if (colThread > 0 && (colThread - 1) == i)
                        item->setData(bgThread, Qt::BackgroundRole);                        // Set the background color of the cell it it is a thread column
                    else
                        item->setData(bgColor, Qt::BackgroundRole);                         // Set the background color of the cell

                    item->setData(QColor(Qt::black), Qt::ForegroundRole);                   // Set the foreground color (font color)

                    if (al == "r")                                                          // Should the cell be right aligned?
                        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);          // Align the content to right

                    model->setItem(lines, i, item);                                         // Insert the cell into the model
                }
                else if (i < parts.size() && i == (TConfig::getColumns() - 1))              // Test for data and if we've reached the last column
                {
                    item = new QStandardItem(parts[i]);                                     // Allocate new cell
                    item->setEditable(false);                                               // Define it not editable
                    item->setData(bgColor, Qt::BackgroundRole);                             // Set the background color of the cell
                    item->setData(QColor(Qt::black), Qt::ForegroundRole);                   // Set the foreground color (font color)

                    if (al == "r")                                                          // Test for right alignment
                        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);          // Align cell content to the right

                    model->setItem(lines, i, item);                                         // Add cell to model
                }
                else                                                                        // Empty column
                {
                    item = new QStandardItem;                                               // Allocate an empty cell

                    if (i < parts.size())                                                   // If we have still some columns in the buffer ...
                        item->setText(parts[i]);                                            // Set the text to the cell
                    else if (!parts.isEmpty() && i == (TConfig::getColumns() - 1))
                        item->setText(parts.last());

                    item->setEditable(false);                                               // Define it not editable

                    if (colThread > 0 && (colThread - 1) == i)
                        item->setData(bgThread, Qt::BackgroundRole);                        // Set the background color of the cell it it is a thread column
                    else
                        item->setData(bgColor, Qt::BackgroundRole);                         // Set the background color of the cell

                    item->setData(QColor(Qt::black), Qt::ForegroundRole);                   // Set the foreground color (font color)

                    if (al == "r")                                                          // Test for right alignment
                        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);          // Align cell content to the right

                    model->setItem(lines, i, item);                                         // Add cell to model
                }
            }

            lines++;                                                                        // increase line counter
        }

        inFile.close();                                                                     // Close file
    }
    catch (std::exception& e)                                                               // triggered if there was a read error
    {
        MSG_ERROR("Error reading file \"" << mFile.toStdString() << "\": " << e.what());

        if (inFile.is_open())
            inFile.close();

        QMessageBox::warning(this, APPNAME, tr("Error reading a logfile!"));
        return false;
    }

    if (progress)                                                                       // Did we had a progress bar?
        delete progress;                                                                // Yes, then delete it. This makes the dialog disappear

    if (canceled)                                                                       // Did the user hit the cancel button?
    {
        model->clear();                                                                 // Delete all cells from the model
        mTotalLines = 0;                                                                // Reset the counted lines
        ui->tableViewLog->setModel(model);                                              // Asign the model to the table (now the table will be empty)
        clearStatusbar();                                                               // Clear the statusbar
        mLbFile = new QLabel;                                                           // Allocate a new QLabel
        mLbFile->setText("File loading was caneled");                                   // Set the text
        ui->statusbar->addWidget(mLbFile);                                              // Add widget to the statusbar
        return false;
    }

    mTotalLines = lines;                                                                // Remember the number of total lines read
    ui->tableViewLog->setModel(model);                                                  // Asign the model to the table
    // The following limit is necessary because it would take too long to
    // format the lines. During this is working the app appears stalled.
    if (lines <= 50000)                                                                 // Only if the lines less then 50000.
        ui->tableViewLog->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);    // Rows with a multiline cell will be resized so that the content fits

    ui->tableViewLog->resizeColumnsToContents();                                        // Let the table resize the columns so that they have the width of the largest content

    // Statistics
    QString statistic;                                                                  // Variable contains the statistics
    statistic.append("<pre>");
    statistic.append(QString("<b>Number of lines</b>:           %1<br>").arg(lines));
    statistic.append(QString("<b>Number trace lines</b>:        %1<br>").arg(iTrace));
    statistic.append(QString("<b>Number information lines</b>:  %1<br>").arg(iInfo));
    statistic.append(QString("<b>Number warning lines</b>:      %1<br>").arg(iWarn));
    statistic.append(QString("<b>Number error lines</b>:        %1<br>").arg(iError));
    statistic.append(QString("<b>Number debug lines</b>:        %1<br>").arg(iDebug));


    if (iOther > 0)
        statistic.append(QString("<b>Number of other lines</b>:     %1<br>").arg(iOther));

    statistic.append(QString("<br><b>Number of block opener</b>:    %1<br>").arg(bopen));
    statistic.append(QString("<b>Number of block closer</b>:    %1<br>").arg(bclose));

    if (TConfig::getColumnThreadID() > 0)
        statistic.append(QString("<br><b>Number of threads</b>:         %1<br>").arg(coloring.getNumberColors()));

    statistic.append("</pre>");
    ui->textEditResult->setText(statistic);                                             // Assign the statistics to the text field
    // Statusbar
    clearStatusbar();                                                                   // Clear the statusbar
    mLbFile = new QLabel;                                                               // Allocate a new QLabel
    QString _f = getFileName(mFile);                                                    // Strip path and get file name only
    mLbFile->setText(QString("File: %1").arg(_f));                                      // Write the file name into the status bar.
    mLbFile->setFrameStyle(QFrame::Panel | QFrame::Sunken);                             // Set a fancy frame style
    ui->statusbar->addWidget(mLbFile);                                                  // Add the widget to the statusbar

    mLbLines = new QLabel;
    mLbLines->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    mLbLines->setText(QString("Lines: %1").arg(lines));
    ui->statusbar->addWidget(mLbLines);

    mLbTraces = new QLabel;
    mLbTraces->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    mLbTraces->setText(QString("Traces: %1").arg(iTrace));
    ui->statusbar->addWidget(mLbTraces);

    mLbInfos = new QLabel;
    mLbInfos->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    mLbInfos->setText(QString("Infos: %1").arg(iInfo));
    ui->statusbar->addWidget(mLbInfos);

    mLbWarnings = new QLabel;
    mLbWarnings->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    mLbWarnings->setText(QString("Warnings: %1").arg(iWarn));
    ui->statusbar->addWidget(mLbWarnings);

    mLbErrors = new QLabel;
    mLbErrors->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    mLbErrors->setText(QString("Errors: %1").arg(iError));
    ui->statusbar->addWidget(mLbErrors);

    mLbDebugs = new QLabel;
    mLbDebugs->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    mLbDebugs->setText(QString("Debugs: %1").arg(iDebug));
    ui->statusbar->addWidget(mLbDebugs);

    if (iOther > 0)
    {
        mLbOthers = new QLabel;
        mLbOthers->setFrameStyle(QFrame::Panel | QFrame::Sunken);
        mLbOthers->setText(QString("Others: %1").arg(iOther));
        ui->statusbar->addWidget(mLbOthers);
    }

    return true;
}

void MainWindow::clearStatusbar()
{
    DECL_TRACER("MainWindow::clearStatusbar()");

    if (mLbFile)
    {
        ui->statusbar->removeWidget(mLbFile);
        mLbFile = nullptr;
    }

    if (mLbLines)
    {
        ui->statusbar->removeWidget(mLbLines);
        mLbLines = nullptr;
    }

    if (mLbTraces)
    {
        ui->statusbar->removeWidget(mLbTraces);
        mLbTraces = nullptr;
    }

    if (mLbInfos)
    {
        ui->statusbar->removeWidget(mLbInfos);
        mLbInfos = nullptr;
    }

    if (mLbWarnings)
    {
        ui->statusbar->removeWidget(mLbWarnings);
        mLbWarnings = nullptr;
    }

    if (mLbErrors)
    {
        ui->statusbar->removeWidget(mLbErrors);
        mLbErrors = nullptr;
    }

    if (mLbDebugs)
    {
        ui->statusbar->removeWidget(mLbDebugs);
        mLbDebugs = nullptr;
    }

    if (mLbOthers)
    {
        ui->statusbar->removeWidget(mLbOthers);
        mLbOthers = nullptr;
    }
}

qsizetype MainWindow::countLines(const QString& file)
{
    DECL_TRACER("MainWindow::countLines(const QString& file)");

    ifstream inFile;
    qsizetype lines = 0;
    QString f = getFileName(file);

    if (!mLbFile)
    {
        mLbFile = new QLabel;
        ui->statusbar->addWidget(mLbFile);
    }

    mLbFile->setText(QString("Checking file: %1 ...").arg(f));

    try
    {
        inFile.open(file.toStdString());

        for (string l; getline(inFile, l);)
            lines++;

        inFile.close();
        mLbFile->setText(QString("Loading file: %1 with %2 lines ...").arg(f).arg(lines));
    }
    catch(std::exception& e)
    {
        MSG_ERROR("Error reading file: " << e.what());

        if (inFile.is_open())
            inFile.close();

        QMessageBox::critical(this, APPNAME, tr("Error reading a file: ")+e.what());
        return -1;
    }

    return lines;
}

// The menu

/**
 * @brief MainWindow::on_actionOpen_triggered
 * Opens a file dialog and let the user select a log file.
 * If the file is a valid log file, it is parsed and can be viewed.
 */
void MainWindow::on_actionOpen_triggered()
{
    DECL_TRACER("MainWindow::on_actionOpen_triggered()");

    if (!mTempFile.isEmpty())
    {
        if (fs::exists(mTempFile.toStdString()))
            fs::remove(mTempFile.toStdString());

        mTempFile.clear();
    }

    mFile = getLogFileName(&mLastFileFilter);
    mLastSearchLine = 0;

    if (!mFile.isEmpty() && fs::exists(mFile.toStdString()) && fs::is_regular_file(mFile.toStdString()))
    {
        // Count lines in file
        qsizetype lines = countLines(mFile);
        parseFile(lines, mLastFileFilter);
    }
    else
        QMessageBox::warning(this, APPNAME, tr("The logfile is not valid or not readable!"));
}


void MainWindow::on_actionSave_result_triggered()
{
    DECL_TRACER("MainWindow::on_actionSave_result_triggered()");

    if (mSaveFile.isEmpty())
        on_actionSave_result_as_triggered();
    else
    {
        if (!writeFile(mSaveFile))
        {
            MSG_ERROR("Couldn't write file " << mSaveFile.toStdString());
            return;
        }

        QMessageBox::information(this, APPNAME, QString("File %1 was saved!").arg(mSaveFile));
    }
}

void MainWindow::on_actionSave_result_as_triggered()
{
    DECL_TRACER("MainWindow::on_actionSave_result_as_triggered()");

    QString filter;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), TConfig::lastSavePath(), tr("Text (*.txt);;Markup (*.md);;HTML (*.html *.htm)"), &filter);

    if (fileName.isEmpty())
        return;

    qsizetype pos = fileName.lastIndexOf("/");

    if (pos == -1)
        TConfig::setLastSavePath(QString::fromStdString(fs::current_path()));
    else if (pos > 0)
        TConfig::setLastSavePath(fileName.left(pos));
    else
        TConfig::setLastSavePath("/");

    if (filter.isEmpty())
    {
        QList<QString> fends = { ".txt", ".md", ".html", ".htm" };
        QList<QString>::iterator iter;
        bool found = false;

        for (iter = fends.begin(); iter != fends.end(); ++iter)
        {
            if (fileName.endsWith(*iter))
            {
                found = true;
                break;
            }
        }

        if (!found)
            fileName.append(".txt");
    }
    else
    {
        if (filter.startsWith("Text") && !fileName.endsWith(".txt"))
            fileName.append(".txt");
        else if (filter.startsWith("Markup") && !fileName.endsWith(".md"))
            fileName.append(".md");
        else if (filter.startsWith("HTML") && !fileName.endsWith(".htm") && !fileName.endsWith(".html"))
            fileName.append(".html");
    }

    mSaveFile = fileName;

    if (!writeFile(fileName))
    {
        MSG_ERROR("Couldn't write file " << fileName.toStdString());
        return;
    }

    QMessageBox::information(this, APPNAME, QString("File %1 was saved!").arg(fileName));
}

void MainWindow::on_actionLoad_profile_triggered()
{
    DECL_TRACER("MainWindow::on_actionLoad_profile_triggered()");

    QString file = QFileDialog::getOpenFileName(this, tr("Open profile"), TConfig::getSourcePath(), tr("Profiles (*.prof);;All (*)"));

    if (!fs::is_regular_file(file.toStdString()))
    {
        QString f = getFileName(file);
        QMessageBox::warning(this, APPNAME, tr("The file ")+f+tr(" is not a valid file!"));
        return;
    }

    TConfig::readProfile(file);

    if (!mFile.isEmpty())
        parseFile(mTotalLines);
}

void MainWindow::on_actionSave_profile_triggered()
{
    DECL_TRACER("MainWindow::on_actionSave_profile_triggered()");

    if (mProfile.isEmpty())
    {
        on_actionSave_profile_as_triggered();
        return;
    }

    TConfig::saveProfile(mProfile);
    QMessageBox::information(this, APPNAME, tr("File ")+mProfile+tr(" was saved!"));
}

void MainWindow::on_actionSave_profile_as_triggered()
{
    DECL_TRACER("MainWindow::on_actionSave_profile_as_triggered()");

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save profile"), TConfig::getSourcePath(), tr("Profile (*.prof);;All (*)"));

    if (fileName.isEmpty())
        return;

    if (!fileName.endsWith(".prof"))
        fileName.append(".prof");

    QString f = getFileName(fileName);

    if (fs::exists(fileName.toStdString()) && fs::is_regular_file(fileName.toStdString()))
    {
        if (QMessageBox::Yes != QMessageBox::question(this, APPNAME, tr("The file ")+f+tr(" exists!\nDo you want to overwrite it?")))
            return;
    }

    TConfig::saveProfile(fileName);
    QMessageBox::information(this, APPNAME, tr("File ")+f+tr(" was saved!"));
    mProfile = fileName;
}

void MainWindow::on_actionExit_triggered()
{
    DECL_TRACER("MainWindow::on_actionExit_triggered()");

    close();
}

void MainWindow::on_actionValidate_consistnace_triggered()
{
    DECL_TRACER("MainWindow::on_actionValidate_consistnace_triggered()");

    typedef struct CLASS_STACK_t
    {
        int line{0};            // The line number
        QString threadId;       // If there is such a column, this contains the threadID
        QString method;         // The name of the function / method (depends on language)
    }CLASS_STACK_t;

    mSaveFile.clear();
    // Progress meter
    QProgressDialog progress(tr("Validating lines ..."), tr("Cancel"), 0, mTotalLines, this);
    progress.setWindowModality(Qt::WindowModal);
    bool canceled = false;

    QStandardItemModel *model = static_cast<QStandardItemModel *>(ui->tableViewLog->model());

    if (!model)
    {
        MSG_ERROR("No model found!");
        return;
    }

    qsizetype rows = model->rowCount();
    int column = TConfig::getColumns() - 1;
    QList<QString> stack;
    vector<CLASS_STACK_t> classStack;
    QList<int> errorLines;
    QString startBlock = TConfig::getBlockEntry();
    QString endBlock = TConfig::getBlockExit();
    int colThread = TConfig::getColumnThreadID();

    for (qsizetype line = 0; line < rows; ++line)
    {
        progress.setValue(line);

        if (progress.wasCanceled())
        {
            canceled = true;
            break;
        }

        QStandardItem *item = model->item(line, column);
        QString qLine = item->text();

        if (qLine.contains(startBlock))
        {
            qsizetype pos = qLine.indexOf(startBlock);
            QString method = qLine.last(qLine.size() - pos - startBlock.length() - 1);

            if (colThread <= 0)
                stack.append(method);

            if ((pos = method.indexOf("::")) != -1)
            {
                QString left = method.left(pos);
                QString right = method.last(method.length() - pos - 2);

                if (right.contains(left) && !right.startsWith("~"))
                {
                    CLASS_STACK_t cs;
                    cs.line = line;
                    cs.method = left;

                    if (colThread > 0 && colThread < TConfig::getColumns())
                    {
                        QStandardItem *it = model->item(line, colThread - 1);
                        cs.threadId = it->text();
                    }

                    classStack.push_back(cs);
                }
            }
        }
        else if (qLine.contains(endBlock))
        {
            if (colThread <= 0)
            {
                if (stack.size() > 0 && qLine.contains(stack.last()))
                    stack.removeLast();
                else
                    errorLines.append(line);
            }

            if (!classStack.empty())
            {
                vector<CLASS_STACK_t>::reverse_iterator iter;
                int l = 0;

                for (iter = classStack.rbegin(); iter != classStack.rend(); ++iter)
                {
                    QString search = QString("::~%1").arg(iter->method);

                    if (colThread <= 0)
                    {
                        if (qLine.contains(search))
                        {
                            l = iter->line;
                            break;
                        }
                    }
                    else
                    {
                        if (qLine.contains(iter->threadId) && qLine.contains(search))
                        {
                            l = iter->line;
                            break;
                        }
                    }
                }

                if (l)
                {
                    vector<CLASS_STACK_t>::iterator iter;

                    for (iter = classStack.begin(); iter != classStack.end(); ++iter)
                    {
                        if (iter->line == l)
                        {
                            classStack.erase(iter);
                            break;
                        }
                    }
                }
            }
        }
    }

    ui->textEditResult->clear();

    if (canceled)
        return;

    QString report;

    if (errorLines.isEmpty())
        report.append("<h2>Result of block validation</h2><p>No errors found.</p>");
    else
    {
        report.append("<h2>Result of block validation</h2><p>");
        QList<int>::iterator iter;

        for (iter = errorLines.begin(); iter != errorLines.end(); ++iter)
        {
            report.append(QString("Error in line: %1<br>").arg(*iter+1));
        }

        report.append("</p>");
    }

    if (!classStack.empty())
    {
        report.append("<h2>Result of method match</h2><p>");

        vector<CLASS_STACK_t>::iterator iter;

        for (iter = classStack.begin(); iter != classStack.end(); ++iter)
        {
            report.append(QString("Method mismatch in line: %1, %2<br>").arg(iter->line+1).arg(iter->method));
        }

        report.append("</p>");
    }

    ui->textEditResult->setText(report);
}

void MainWindow::on_actionFind_exceptions_triggered()
{
    DECL_TRACER("MainWindow::on_actionFind_exceptions_triggered()");

    mSaveFile.clear();
    // Progress meter
    QProgressDialog progress(tr("Searching for exceptions ..."), tr("Cancel"), 0, mTotalLines, this);
    progress.setWindowModality(Qt::WindowModal);
    bool canceled = false;

    QStandardItemModel *model = static_cast<QStandardItemModel *>(ui->tableViewLog->model());

    if (!model)
    {
        MSG_ERROR("No model found!");
        return;
    }

    qsizetype rows = model->rowCount();
    int column = TConfig::getColumns() - 1;
    QList<int> exceptions;

    for (qsizetype line = 0; line < rows; ++line)
    {
        progress.setValue(line);

        if (progress.wasCanceled())
        {
            canceled = true;
            break;
        }

        QStandardItem *item = model->item(line, column);

        if (item && item->text().contains("exception", Qt::CaseInsensitive))
            exceptions.append(line+1);
    }

    // Report the result
    ui->textEditResult->clear();

    if (canceled)
        return;

    QString report;
    report.append("<h2>Exceptions found</h2><p>");

    if (exceptions.isEmpty())
        report.append("No exceptions found!</p>");
    else
    {
        QList<int>::iterator iter;

        for (iter = exceptions.begin(); iter != exceptions.end(); ++iter)
            report.append(QString("<b>Exception on line</b>: %1<br>").arg(*iter));

        report.append("</p>");
    }

    ui->textEditResult->setText(report);
}

void MainWindow::on_actionSearch_triggered()
{
    DECL_TRACER("MainWindow::on_actionSearch_triggered()");

    bool ok;

    mMenuColumn = -1;
    mLastSearchLine = 0;

    QString text = QInputDialog::getText(this, tr("Search"), tr("Enter string to search for"), QLineEdit::Normal, mLastSearchText, &ok);

    if (!ok || text.isEmpty())
        return;

    mLastSearchText = text;
    mLastSearchLine = search(text);
}

void MainWindow::on_actionFilter_thread_triggered(bool checked)
{
    DECL_TRACER("MainWindow::on_actionFilter_thread_triggered(bool checked)");

    if (TConfig::getColumnThreadID() <= 0 || (!checked && !mLastFilterCheck))
        return;

    mLastFilterCheck = checked;
    TThreadSelect *tss = nullptr;
    TThreadSelect::THREAD_LIST_t tl;

    if (checked)
    {
        tss = new TThreadSelect(this);
        tss->setThreads(mThreads);

        if (tss->exec() == QDialog::Rejected)
        {
            delete tss;
            return;
        }

        tl = tss->getSelectedThread();

        if (tl.threadID.isEmpty())
        {
            parseFile(mTotalLines, mLastFileFilter);
            delete tss;
            return;
        }

        delete tss;
        tss = nullptr;
    }

    // Filter list to show only the selected thread
    // The lines who are filtered out are hidden, not deleted!
    MSG_DEBUG("Filtering for thread\"" << tl.threadID.toStdString() << "\" ...");
    parseFile(mTotalLines, mLastFileFilter, tl.threadID);
    ui->actionFilter_thread->setChecked(true);
}

void MainWindow::on_actionReload_triggered()
{
    DECL_TRACER("MainWindow::on_actionReload_triggered()");

    if (!mFile.isEmpty())
    {
        qsizetype lines = countLines(mTempFile.isEmpty() ? mFile : mTempFile);
        parseFile(lines);
    }
}

void MainWindow::on_actionSettings_triggered()
{
    DECL_TRACER("MainWindow::on_actionSettings_triggered()");

    TQtSettings settings(this);

    if (settings.exec() == QDialog::Rejected)
        return;

    settings.saveValues();
}

void MainWindow::on_actionAbout_triggered()
{
    DECL_TRACER("MainWindow::on_actionAbout_triggered()");

    QString about("<b>%1 %2</b><br>"
                  "Author: <i>Andreas Theofilu &lt;andreas@theosys.at&gt;</i><br>"
                  "Copyright © 2025 byAndreas Theofilu.<br>"
                  "This program is licensed under the GPL 3!");
    about = about.arg(APPNAME).arg(VERSION_STRING());
    QMessageBox::about(this, APPNAME, about);
}

void MainWindow::pressed(const QModelIndex &index)
{
    DECL_TRACER("MainWindow::pressed(const QModelIndex &index)");

    Qt::MouseButtons mbt = QGuiApplication::mouseButtons();

    if (mbt != Qt::RightButton)
        return;

    if (!mPopupMenu)
    {
        mPopupMenu = new QMenu;
        QAction *menuCopy = new QAction(tr("Copy column content"));
        QAction *menuSearch = new QAction(tr("Search for content in column"));
        mPopupMenu->addAction(menuCopy);
        mPopupMenu->addAction(menuSearch);
        connect(menuCopy, &QAction::triggered, this, &MainWindow::onPopupMenuCopyTriggered);
        connect(menuSearch, &QAction::triggered, this, &MainWindow::onPopupMenuSearchTriggered);
    }

    mModelIndex = index;
    mModelMenu = index.model();
    QWidget *w = ui->tableViewLog->viewport();
    QPoint pt(ui->tableViewLog->columnViewportPosition(index.column()), ui->tableViewLog->rowViewportPosition(index.row()));

    if (w)
        pt = w->mapToGlobal(pt);

    QList<QAction *> alist = mPopupMenu->actions();

    if (alist.size() > 0)
    {
        mPopupMenu->setActiveAction(alist[0]);
        alist[0]->hover();
    }

    mPopupMenu->popup(pt);
}

void MainWindow::onPopupMenuCopyTriggered(bool checked)
{
    DECL_TRACER("MainWindow::onPopupMenuCopyTriggered(bool checked)");

    Q_UNUSED(checked);

    if (!mModelMenu)
        return;

    QString text = mModelMenu->data(mModelIndex).toString();
    QClipboard *cboard = QGuiApplication::clipboard();
    MSG_DEBUG("Copying text \"" << text.toStdString() << "\" to clipboard ...");

    if (cboard && !text.isEmpty())
        cboard->setText(text);
/*
    if (cboard)
    {
        QWidget *w = ui->tableViewLog->viewport();
        QPoint pt(ui->tableViewLog->columnViewportPosition(mModelIndex.column()), ui->tableViewLog->rowViewportPosition(mModelIndex.row()));

        if (w)
            pt = w->mapToGlobal(pt);

        cboard->setText(text);
        QRect rect;
        QToolTip::showText(pt, tr("Content of column copied to clipboard"), nullptr, rect, 3000);
    }
*/
}

void MainWindow::onPopupMenuSearchTriggered(bool checked)
{
    DECL_TRACER("MainWindow::onPopupMenuSearchTriggered(bool checked)");

    Q_UNUSED(checked);

    if (!mModelMenu)
        return;

    mMenuColumn = mModelIndex.column();
    mLastSearchLine = mModelIndex.row() + 1;
    mLastSearchText = mModelMenu->data(mModelIndex).toString();
    bool ok;

    QString text = QInputDialog::getText(this, tr("Search"), tr("Enter string to search for"), QLineEdit::Normal, mLastSearchText, &ok);

    if (!ok || text.isEmpty())
        return;

    mLastSearchText = text;
    mLastSearchLine = search(mLastSearchText, mLastSearchLine, mMenuColumn);
}

void MainWindow::on_textEditResult_selectionChanged()
{
    DECL_TRACER("MainWindow::on_textEditResult_selectionChanged()");

    QTextCursor cursor = ui->textEditResult->textCursor();
    QString text = cursor.selectedText();
    MSG_DEBUG("Selected text: " << text.toStdString());

    if (text.isEmpty())
        return;

    if (text.at(0) >= '0' && text.at(0) <= '9')
    {
        int line = text.toInt();

        if (line > 0)
            ui->tableViewLog->selectRow(line-1);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_F3)
    {
        if (mLastSearchLine > 0)
        {
            mLastSearchLine = search(mLastSearchText, mLastSearchLine, mMenuColumn);
            return;
        }
        else
        {
            bool ok;

            QString text = QInputDialog::getText(this, tr("Search"), tr("Enter string to search for"), QLineEdit::Normal, mLastSearchText, &ok);

            if (!ok || text.isEmpty())
                return;

            mLastSearchText = text;
            mLastSearchLine = search(mLastSearchText, 0, mMenuColumn);
        }
    }

    QWidget::keyPressEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    DECL_TRACER("MainWindow::resizeEvent(QResizeEvent *event)");

    QRect rect = frameGeometry();
    rect.setWidth(event->size().width());
    rect.setHeight(event->size().height());
    TConfig::setLastGeometry(rect);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    DECL_TRACER("MainWindow::closeEvent(QCloseEvent *event)");

    Q_UNUSED(event);

    if (!mTempFile.isEmpty())
    {
        if (fs::exists(mTempFile.toStdString()))
            fs::remove(mTempFile.toStdString());
    }
}

qsizetype MainWindow::search(const QString& text, qsizetype offset, int col)
{
    DECL_TRACER("MainWindow::search(const QString& text, qsizetype offset, int col)");

    MSG_DEBUG("Searching for \"" << text.toStdString() << "\" from offset " << offset << " ...");
    // Progress meter
    QProgressDialog progress(tr("Searching for a string ..."), tr("Cancel"), 0, mTotalLines, this);
    progress.setWindowModality(Qt::WindowModal);

    QStandardItemModel *model = static_cast<QStandardItemModel *>(ui->tableViewLog->model());

    if (!model)
    {
        MSG_ERROR("No model found!");
        return -1;
    }

    qsizetype rows = model->rowCount();
    int column = 0;

    if (col >= 0 && col < TConfig::getColumns())
        column = col;
    else
        column = TConfig::getColumns() - 1;

    for (qsizetype line = offset; line < rows; ++line)
    {
        progress.setValue(line);

        if (progress.wasCanceled())
            break;

        QStandardItem *item = model->item(line, column);

        if (item && item->text().contains(text))
        {
            ui->tableViewLog->selectRow(line);
            return line + 1;
        }
    }

    return -1;
}

bool MainWindow::writeFile(const QString& file)
{
    DECL_TRACER("MainWindow::writeFile(const QString& file)");

    if (file.isEmpty())
        return false;

    ofstream oFile;

    try
    {
        oFile.open(file.toStdString(), ofstream::trunc);

        if (file.endsWith(".md"))
            oFile << ui->textEditResult->toMarkdown().toStdString();
        else if (file.endsWith(".html") || file.endsWith(".htm"))
            oFile << ui->textEditResult->toHtml().toStdString();
        else
            oFile << ui->textEditResult->toPlainText().toStdString();

        oFile.close();
    }
    catch(std::exception& e)
    {
        MSG_ERROR("Error writing a file: " << e.what());

        if (oFile.is_open())
            oFile.close();

        return false;
    }

    return true;
}

void MainWindow::filterThread(const QString& threadID)
{
    DECL_TRACER("MainWindow::filterThread(const QString& threadID)");

    if (threadID.isEmpty())
        return;
}

QList<QString> MainWindow::split(const QString& str, const QString& deli, int cols)
{
    qsizetype pos1 = 0, pos2 = 0;
    int column = 0;
    QList<QString> parts;

    while(pos1 < str.length() && (pos2 = str.indexOf(deli, pos1)) != -1)
    {
        if (cols > 0 && column >= cols)
        {
            parts.append(str.right(str.length() - pos1 - deli.length()));
            break;
        }

        QString p = str.mid(pos1, pos2 - pos1);
        parts.append(p);
        column++;
        pos1 = pos2 + deli.length();
    }

    if (pos1 && pos1 < str.length())
        parts.append(str.right(str.length() - pos1 - deli.length()));
/*
    if (TLogger::getLogLevel() >= LVL_DEBUG && parts.size() > 0)
    {
        QList<QString>::iterator iter;
        QString s;

        for (iter = parts.begin(); iter != parts.end(); ++iter)
        {
            if (!s.isEmpty())
                s.append("|");

            s.append(*iter);
        }

        MSG_DEBUG("String: " << s.toStdString());
    }
*/
    return parts;
}

QString MainWindow::getFileName(const QString& name)
{
    DECL_TRACER("MainWindow::getFileName(const QString& name)");

    qsizetype pos = name.lastIndexOf("/");
    QString f = name;

    if (pos != -1)
       f = mFile.right(name.length() - pos - 1);

    return f;
}
