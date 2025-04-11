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
#include <QRect>
#include <QStringList>
#include <QColor>

#include <filesystem>
#include <iostream>
#include <algorithm>
#include <stdlib.h>

#include "tconfig.h"
#include "tlogger.h"

namespace fs = std::filesystem;
using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::endl;

QString TConfig::mBlockEntry;
QString TConfig::mBlockExit;
QString TConfig::mTagInfo;
QColor TConfig::mColorInfo;
QString TConfig::mTagWarning;
QColor TConfig::mColorWarning;
QString TConfig::mTagError;
QColor TConfig::mColorError;
QString TConfig::mTagDebug;
QColor TConfig::mColorDebug;
QString TConfig::mTagTrace;
QColor TConfig::mColorTrace;
QString TConfig::mDelimenter;
int TConfig::mColumns{0};
QStringList TConfig::mHeaders;
QString TConfig::mColAligns;
int TConfig::mColumnThreadID{0};
QList<TValueSelect::VALUES_t> TConfig::mValues;

QString TConfig::mLogfile;
QString TConfig::mSourcePath;
QString TConfig::mResultPath;

QString TConfig::mConfigFile;
int TConfig::mLogLevel{0};
QRect TConfig::mLastGeometry;
QString TConfig::mLastOpenPath;
QString TConfig::mLastSavePath;

using VALTYPES_t = TValueSelect::VALTYPES_t;
using VALUES_t = TValueSelect::VALUES_t;

void TConfig::readConfig()
{
    DECL_TRACER("TConfig::readConfig()");

    ifstream in;
    initialize();
    getConfigFile();

    try
    {
        char line[1024];

        in.open(mConfigFile.toStdString());

        while(in.getline(line, sizeof(line)))
        {
            string l(line, strlen(line) >= sizeof(line) ? (sizeof(line) - 1) : strlen(line));

            if (isRemark(l))
                continue;

            size_t pos = l.find("=");

            if (pos == string::npos)
                continue;

            string left = l.substr(0, pos);
            string right = l.substr(pos + 1);
            trim(left);
            trim(right);

            if (caseCompare(left, "BlockStart") == 0)
                mBlockEntry = QString::fromStdString(right);
            else if (caseCompare(left, "BlockEnd") == 0)
                mBlockExit = QString::fromStdString(right);
            else if (caseCompare(left, "TagInfo") == 0)
                mTagInfo = QString::fromStdString(right);
            else if (caseCompare(left, "ColorInfo") == 0)
            {
                QString color = QString::fromStdString(right);

                if (color.contains(","))
                {
                    QStringList parts = color.split(",");

                    if (parts.size() >= 3)
                    {
                        int red = parts[0].toInt();
                        int green = parts[1].toInt();
                        int blue = parts[2].toInt();
                        mColorInfo = qRgb(red, green, blue);
                    }
                }
            }
            else if (caseCompare(left, "TagError") == 0)
                mTagError = QString::fromStdString(right);
            else if (caseCompare(left, "ColorError") == 0)
            {
                QString color = QString::fromStdString(right);

                if (color.contains(","))
                {
                    QStringList parts = color.split(",");

                    if (parts.size() >= 3)
                    {
                        int red = parts[0].toInt();
                        int green = parts[1].toInt();
                        int blue = parts[2].toInt();
                        mColorError = qRgb(red, green, blue);
                    }
                }
            }
            else if (caseCompare(left, "TagWarning") == 0)
                mTagWarning = QString::fromStdString(right);
            else if (caseCompare(left, "ColorWarning") == 0)
            {
                QString color = QString::fromStdString(right);

                if (color.contains(","))
                {
                    QStringList parts = color.split(",");

                    if (parts.size() >= 3)
                    {
                        int red = parts[0].toInt();
                        int green = parts[1].toInt();
                        int blue = parts[2].toInt();
                        mColorWarning = qRgb(red, green, blue);
                    }
                }
            }
            else if (caseCompare(left, "TagDebug") == 0)
                mTagDebug = QString::fromStdString(right);
            else if (caseCompare(left, "ColorDebug") == 0)
            {
                QString color = QString::fromStdString(right);

                if (color.contains(","))
                {
                    QStringList parts = color.split(",");

                    if (parts.size() >= 3)
                    {
                        int red = parts[0].toInt();
                        int green = parts[1].toInt();
                        int blue = parts[2].toInt();
                        mColorDebug = qRgb(red, green, blue);
                    }
                }
            }
            else if (caseCompare(left, "TagTrace") == 0)
                mTagTrace = QString::fromStdString(right);
            else if (caseCompare(left, "ColorTrace") == 0)
            {
                QString color = QString::fromStdString(right);

                if (color.contains(","))
                {
                    QStringList parts = color.split(",");

                    if (parts.size() >= 3)
                    {
                        int red = parts[0].toInt();
                        int green = parts[1].toInt();
                        int blue = parts[2].toInt();
                        mColorTrace = qRgb(red, green, blue);
                    }
                }
            }
            else if (caseCompare(left, "Delimeter") == 0)
                mDelimenter = QString::fromStdString(right);
            else if (caseCompare(left, "Columns") == 0)
                mColumns = atoi(right.c_str());
            else if (caseCompare(left, "ColumnThreadID") == 0)
                mColumnThreadID = atoi(right.c_str());
            else if (caseCompare(left, "Headers") == 0)
            {
                QString heads = QString::fromStdString(right);

                if (heads.contains("|"))
                {
                    QStringList parts = heads.split("|");
                    QStringList::iterator iter;
                    mHeaders.clear();

                    for (iter = parts.begin(); iter != parts.end(); ++iter)
                        mHeaders << *iter;
                }
            }
            else if (caseCompare(left, "Values") == 0)
            {
                QString heads = QString::fromStdString(right);

                if (heads.contains("|"))
                {
                    QStringList parts = heads.split("|");
                    QStringList::iterator iter;
                    mValues.clear();

                    for (iter = parts.begin(); iter != parts.end(); ++iter)
                    {
                        QStringList c = iter->split(",");

                        if (c.size() >= 2)
                        {
                            VALUES_t vt;
                            vt.name = c[0];
                            vt.type = static_cast<VALTYPES_t>(c[1].toInt());
                            mValues.append(vt);
                        }
                        else    // Needed for backward compatibility
                        {
                            VALUES_t vt;
                            vt.name = *iter;
                            vt.type = VALTYPES_t::VTYPE_STRING;
                            mValues.append(vt);
                        }
                    }
                }
            }
            else if (caseCompare(left, "ColAligns") == 0)
                mColAligns = QString::fromStdString(right);
            else if (caseCompare(left, "LogFile") == 0)
                mLogfile = QString::fromStdString(right);
            else if (caseCompare(left, "SourcePath") == 0)
                mSourcePath = QString::fromStdString(right);
            else if (caseCompare(left, "ResultPath") == 0)
                mResultPath = QString::fromStdString(right);
            else if (caseCompare(left, "LogLevel") == 0)
            {
                mLogLevel = atoi(right.c_str());

                if (mLogLevel < 0)
                    mLogLevel = 0;
                else if (mLogLevel > 6)
                    mLogLevel = 6;
            }
            else if (caseCompare(left, "Geometry") == 0)
            {
                QString r = QString::fromStdString(right);
                QStringList parts = r.split(",");

                if (parts.size() >= 4)
                {
                    int x = parts[0].toInt();
                    int y = parts[1].toInt();
                    int width = parts[2].toInt();
                    int height = parts[3].toInt();
                    mLastGeometry = QRect(x, y, width, height);
                }
            }
            else if (caseCompare(left, "LastOpenPath") == 0)
                mLastOpenPath = QString::fromStdString(right);
            else if (caseCompare(left, "LastSavePath") == 0)
                mLastSavePath = QString::fromStdString(right);
        }

        in.close();

        MSG_DEBUG("Block start:    " << mBlockEntry.toStdString());
        MSG_DEBUG("Block end:      " << mBlockExit.toStdString());
        MSG_DEBUG("Tag info:       " << mTagInfo.toStdString());
        MSG_DEBUG("Color info:     " << mColorInfo.red() << ", " << mColorInfo.green() << ", " << mColorInfo.blue());
        MSG_DEBUG("Tag warning:    " << mTagWarning.toStdString());
        MSG_DEBUG("Color info:     " << mColorWarning.red() << ", " << mColorWarning.green() << ", " << mColorWarning.blue());
        MSG_DEBUG("Tag error:      " << mTagError.toStdString());
        MSG_DEBUG("Color info:     " << mColorError.red() << ", " << mColorError.green() << ", " << mColorError.blue());
        MSG_DEBUG("Tag debug:      " << mTagDebug.toStdString());
        MSG_DEBUG("Color info:     " << mColorDebug.red() << ", " << mColorDebug.green() << ", " << mColorDebug.blue());
        MSG_DEBUG("Tag trace:      " << mTagTrace.toStdString());
        MSG_DEBUG("Color info:     " << mColorTrace.red() << ", " << mColorTrace.green() << ", " << mColorTrace.blue());
        MSG_DEBUG("Delimeter:      " << mDelimenter.toStdString());
        MSG_DEBUG("Number columns: " << mColumns);
        MSG_DEBUG("Column threadID:" << mColumnThreadID);
        QStringList::iterator iter;
        QString heads;
        bool first = true;

        for (iter = mHeaders.begin(); iter != mHeaders.end(); ++iter)
        {
            if (!first)
                heads.append(", ");
            else
                first = false;

            heads.append(*iter);
        }

        MSG_DEBUG("Column headers: " << heads.toStdString());
        QString values;
        first = true;
        QList<VALUES_t>::iterator valIter;

        for (valIter = mValues.begin(); valIter != mValues.end(); ++valIter)
        {
            if (!first)
                values.append(" | ");
            else
                first = false;

            values.append(QString("%1,%2").arg(valIter->name).arg(valIter->type));
        }

        MSG_DEBUG("Thread values:  " << values.toStdString());
        MSG_DEBUG("Column aligns:  " << mColAligns.toStdString());
        MSG_DEBUG("Log file:       " << mLogfile.toStdString());
        MSG_DEBUG("Log level:      " << mLogLevel);
        MSG_DEBUG("Source path:    " << mSourcePath.toStdString());
        MSG_DEBUG("Result path:    " << mResultPath.toStdString());
        MSG_DEBUG("Last geometry:  " << mLastGeometry.x() << ", " << mLastGeometry.y() << ", " << mLastGeometry.width() << ", " << mLastGeometry.height());
        MSG_DEBUG("Last open path: " << mLastOpenPath.toStdString());
        MSG_DEBUG("Last save path: " << mLastSavePath.toStdString());
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error reading config file: " << e.what());

        if (in.is_open())
            in.close();
    }
}

void TConfig::saveConfig()
{
    DECL_TRACER("TConfig::saveConfig()");

    if (mConfigFile.isEmpty())
    {
        MSG_ERROR("No config file to write to!");
        return;
    }

    ofstream of;

    try
    {
        of.open(mConfigFile.toStdString());

        of << "BlockStart=" << mBlockEntry.toStdString() << endl
           << "BlockEnd=" << mBlockExit.toStdString() << endl
           << "TagInfo=" << mTagInfo.toStdString() << endl
           << "ColorInfo=" << mColorInfo.red() << "," << mColorInfo.green() << "," << mColorInfo.blue() << endl
           << "TagWarning=" << mTagWarning.toStdString() << endl
           << "ColorWarning=" << mColorWarning.red() << "," << mColorWarning.green() << "," << mColorWarning.blue() << endl
           << "TagError=" << mTagError.toStdString() << endl
           << "ColorError=" << mColorError.red() << "," << mColorError.green() << "," << mColorError.blue() << endl
           << "TagDebug=" << mTagDebug.toStdString() << endl
           << "ColorDebug=" << mColorDebug.red() << "," << mColorDebug.green() << "," << mColorDebug.blue() << endl
           << "TagTrace=" << mTagTrace.toStdString() << endl
           << "ColorTrace=" << mColorTrace.red() << "," << mColorTrace.green() << "," << mColorTrace.blue() << endl
           << "Delimeter=" << mDelimenter.toStdString() << endl
           << "Columns=" << mColumns << endl
           << "ColAligns=" << mColAligns.toStdString() << endl
           << "ColumnThreadID=" << mColumnThreadID << endl
           << "LogFile=" << mLogfile.toStdString() << endl
           << "SourcePath=" << mSourcePath.toStdString() << endl
           << "ResultPath=" << mResultPath.toStdString() << endl
           << "LogLevel=" << mLogLevel << endl
           << "Geometry=" << mLastGeometry.x() << "," << mLastGeometry.y() << "," << mLastGeometry.width() << "," << mLastGeometry.height() << endl
           << "LastOpenPath=" << mLastOpenPath.toStdString() << endl
           << "LastSavePath=" << mLastSavePath.toStdString() << endl;

        of << "Headers=";
        QStringList::iterator iter;

        bool first = true;

        for (iter = mHeaders.begin(); iter != mHeaders.end(); ++iter)
        {
            if (!first)
                of << "|";
            else
                first = false;

            of << iter->toStdString();
        }

        of << endl << "Values=";
        first = true;
        QList<VALUES_t>::iterator valIter;

        for (valIter = mValues.begin(); valIter != mValues.end(); ++valIter)
        {
            if (!first)
                of << "|";
            else
                first = false;

            of << valIter->name.toStdString() << "," << valIter->type;
        }

        of << endl;
        of.close();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error writing config file: " << e.what());

        if (of.is_open())
            of.close();
    }
}

QString TConfig::getConfigFile()
{
    DECL_TRACER("TConfig::getConfigFile()");

    char *HOME = getenv("HOME");
    string p;

    if (!HOME)
        p = ".";
    else
        p = HOME;

    vector<string> files = { ".itpploganalyzer", ".config/itpploganalyzer.rc" };
    vector<string>::iterator iter;

    for (iter = files.begin(); iter != files.end(); ++iter)
    {
        string path = p + "/" + *iter;

        if (fs::exists(path) && fs::is_regular_file(path))
        {
            mConfigFile = QString("%1/%2").arg(QString::fromStdString(p)).arg(QString::fromStdString(*iter));
            MSG_DEBUG("Found log file: " << mConfigFile.toStdString());
            break;
        }
    }

    if (!mConfigFile.isEmpty())
        return mConfigFile;

    // New config file! Fill the config with default values
    initialize();

    mLogfile = QString("%1/itpploganalyzer.log").arg(QString::fromStdString(p));
    mSourcePath = QString("%1").arg(QString::fromStdString(p));
    mResultPath = QString("%1").arg(QString::fromStdString(p));

    if (fs::exists(p+"/.config") && fs::is_directory(p+"/.config"))
        mConfigFile = QString("%1/.config/itpploganalyzer.rc").arg(QString::fromStdString(p));
    else
        mConfigFile = QString("%1/.itpploganalyzer").arg(QString::fromStdString(p));

    saveConfig();
    MSG_DEBUG("Configuration file created at " << mConfigFile.toStdString() << "!");
    return mConfigFile;
}

void TConfig::initialize()
{
    mBlockEntry = "{entry:";
    mBlockExit = "}exit:";
    mTagInfo = "INF";
    mColorInfo = qRgb(176, 255, 181);
    mTagWarning = "WRN";
    mColorWarning = qRgb(248, 255, 185);
    mTagError = "ERR";
    mColorError = qRgb(255, 179, 179);
    mTagDebug = "DBG";
    mColorDebug = qRgb(227, 227, 227);
    mTagTrace = "TRC";
    mColorTrace = qRgb(252, 239, 173);
    mColumns = 8;
    mColAligns = "l,r,l,l,l,l,r,l";
    mHeaders << "Timestamp" << "PID" << "User" << "Type"
             << "SType" << "File" << "Line" << "Content";

    VALUES_t vt;
    vt.name = "header.timestamp";
    vt.type = VALTYPES_t::VTYPE_STRING;
    mValues.append(vt);
    vt.name = "header.pid";
    vt.type = VALTYPES_t::VTYPE_INT;
    mValues.append(vt);
    vt.name = "header.username";
    vt.type = VALTYPES_t::VTYPE_STRING;
    mValues.append(vt);
    vt.name = "header.loglevel";
    vt.type = VALTYPES_t::VTYPE_STRING;
    mValues.append(vt);
    vt.name = "header.logpackage";
    vt.type = VALTYPES_t::VTYPE_STRING;
    mValues.append(vt);
    vt.name = "header.file";
    vt.type = VALTYPES_t::VTYPE_STRING;
    mValues.append(vt);
    vt.name = "header.line";
    vt.type = VALTYPES_t::VTYPE_INT;
    mValues.append(vt);
    vt.name = "message";
    vt.type = VALTYPES_t::VTYPE_STRING;

    mDelimenter = ",";
    mColumnThreadID = 8;
    mLogLevel = 1;
}

void TConfig::readProfile(const QString& pf)
{
    DECL_TRACER("TConfig::readProfile(const QString& pf)");

    ifstream in;
    initialize();
    getConfigFile();

    try
    {
        char line[1024];

        in.open(pf.toStdString());

        while(in.getline(line, sizeof(line)))
        {
            string l(line, strlen(line) >= sizeof(line) ? (sizeof(line) - 1) : strlen(line));

            if (isRemark(l))
                continue;

            size_t pos = l.find("=");

            if (pos == string::npos)
                continue;

            string left = l.substr(0, pos);
            string right = l.substr(pos + 1);
            trim(left);
            trim(right);

            if (caseCompare(left, "BlockStart") == 0)
                mBlockEntry = QString::fromStdString(right);
            else if (caseCompare(left, "BlockEnd") == 0)
                mBlockExit = QString::fromStdString(right);
            else if (caseCompare(left, "TagInfo") == 0)
                mTagInfo = QString::fromStdString(right);
            else if (caseCompare(left, "ColorInfo") == 0)
            {
                QString color = QString::fromStdString(right);

                if (color.contains(","))
                {
                    QStringList parts = color.split(",");

                    if (parts.size() >= 3)
                    {
                        int red = parts[0].toInt();
                        int green = parts[1].toInt();
                        int blue = parts[2].toInt();
                        mColorInfo = qRgb(red, green, blue);
                    }
                }
            }
            else if (caseCompare(left, "TagError") == 0)
                mTagError = QString::fromStdString(right);
            else if (caseCompare(left, "ColorError") == 0)
            {
                QString color = QString::fromStdString(right);

                if (color.contains(","))
                {
                    QStringList parts = color.split(",");

                    if (parts.size() >= 3)
                    {
                        int red = parts[0].toInt();
                        int green = parts[1].toInt();
                        int blue = parts[2].toInt();
                        mColorError = qRgb(red, green, blue);
                    }
                }
            }
            else if (caseCompare(left, "TagWarning") == 0)
                mTagWarning = QString::fromStdString(right);
            else if (caseCompare(left, "ColorWarning") == 0)
            {
                QString color = QString::fromStdString(right);

                if (color.contains(","))
                {
                    QStringList parts = color.split(",");

                    if (parts.size() >= 3)
                    {
                        int red = parts[0].toInt();
                        int green = parts[1].toInt();
                        int blue = parts[2].toInt();
                        mColorWarning = qRgb(red, green, blue);
                    }
                }
            }
            else if (caseCompare(left, "TagDebug") == 0)
                mTagDebug = QString::fromStdString(right);
            else if (caseCompare(left, "ColorDebug") == 0)
            {
                QString color = QString::fromStdString(right);

                if (color.contains(","))
                {
                    QStringList parts = color.split(",");

                    if (parts.size() >= 3)
                    {
                        int red = parts[0].toInt();
                        int green = parts[1].toInt();
                        int blue = parts[2].toInt();
                        mColorDebug = qRgb(red, green, blue);
                    }
                }
            }
            else if (caseCompare(left, "TagTrace") == 0)
                mTagTrace = QString::fromStdString(right);
            else if (caseCompare(left, "ColorTrace") == 0)
            {
                QString color = QString::fromStdString(right);

                if (color.contains(","))
                {
                    QStringList parts = color.split(",");

                    if (parts.size() >= 3)
                    {
                        int red = parts[0].toInt();
                        int green = parts[1].toInt();
                        int blue = parts[2].toInt();
                        mColorTrace = qRgb(red, green, blue);
                    }
                }
            }
            else if (caseCompare(left, "Delimeter") == 0)
                mDelimenter = QString::fromStdString(right);
            else if (caseCompare(left, "Columns") == 0)
                mColumns = atoi(right.c_str());
            else if (caseCompare(left, "ColumnThreadID") == 0)
                mColumnThreadID = atoi(right.c_str());
            else if (caseCompare(left, "Headers") == 0)
            {
                QString heads = QString::fromStdString(right);

                if (heads.contains("|"))
                {
                    QStringList parts = heads.split("|");
                    QStringList::iterator iter;
                    mHeaders.clear();

                    for (iter = parts.begin(); iter != parts.end(); ++iter)
                        mHeaders << *iter;
                }
            }
            else if (caseCompare(left, "Values") == 0)
            {
                QString heads = QString::fromStdString(right);

                if (heads.contains("|"))
                {
                    QStringList parts = heads.split("|");
                    QStringList::iterator iter;
                    mValues.clear();

                    for (iter = parts.begin(); iter != parts.end(); ++iter)
                    {
                        if (iter->contains(","))
                        {
                            QStringList v = iter->split(",");

                            if (v.size() >= 2)
                            {
                                VALUES_t vt;
                                vt.name = v[0];
                                vt.type = static_cast<VALTYPES_t>(v[1].toInt());
                                mValues.append(vt);
                            }
                        }
                        else    // Needed for backward compatibility
                        {
                            VALUES_t vt;
                            vt.name = *iter;
                            vt.type = VALTYPES_t::VTYPE_STRING;
                            mValues.append(vt);
                        }
                    }
                }
            }
            else if (caseCompare(left, "ColAligns") == 0)
                mColAligns = QString::fromStdString(right);
        }

        in.close();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error reading profile file: " << e.what());

        if (in.is_open())
            in.close();
    }
}

void TConfig::saveProfile(const QString& pf)
{
    DECL_TRACER("TConfig::saveProfile(const QString& pf)");

    if (pf.isEmpty())
    {
        MSG_ERROR("No profile to write to!");
        return;
    }

    ofstream of;

    try
    {
        of.open(pf.toStdString());

        of << "BlockStart=" << mBlockEntry.toStdString() << endl
           << "BlockEnd=" << mBlockExit.toStdString() << endl
           << "TagInfo=" << mTagInfo.toStdString() << endl
           << "ColorInfo=" << mColorInfo.red() << "," << mColorInfo.green() << "," << mColorInfo.blue() << endl
           << "TagWarning=" << mTagWarning.toStdString() << endl
           << "ColorWarning=" << mColorWarning.red() << "," << mColorWarning.green() << "," << mColorWarning.blue() << endl
           << "TagError=" << mTagError.toStdString() << endl
           << "ColorError=" << mColorError.red() << "," << mColorError.green() << "," << mColorError.blue() << endl
           << "TagDebug=" << mTagDebug.toStdString() << endl
           << "ColorDebug=" << mColorDebug.red() << "," << mColorDebug.green() << "," << mColorDebug.blue() << endl
           << "TagTrace=" << mTagTrace.toStdString() << endl
           << "ColorTrace=" << mColorTrace.red() << "," << mColorTrace.green() << "," << mColorTrace.blue() << endl
           << "Delimeter=" << mDelimenter.toStdString() << endl
           << "Columns=" << mColumns << endl
           << "ColAligns=" << mColAligns.toStdString() << endl
           << "ColumnThreadID=" << mColumnThreadID << endl;

        of << "Headers=";
        QStringList::iterator iter;

        bool first = true;

        for (iter = mHeaders.begin(); iter != mHeaders.end(); ++iter)
        {
            if (!first)
                of << "|";
            else
                first = false;

            of << iter->toStdString();
        }

        of << endl << "Values=";
        first = true;
        QList<VALUES_t>::iterator valIter;

        for (valIter = mValues.begin(); valIter != mValues.end(); ++valIter)
        {
            if (!first)
                of << "|";
            else
                first = false;

            of << valIter->name.toStdString() << "," << valIter->type;
        }

        of << endl;
        of.close();
    }
    catch (std::exception& e)
    {
        MSG_ERROR("Error writing profile: " << e.what());

        if (of.is_open())
            of.close();
    }
}

bool TConfig::isRemark(const string& line)
{
    string::const_iterator iter;

    for (iter = line.cbegin(); iter != line.cend(); ++iter)
    {
        if (*iter != ' ' && *iter != '#')
            return false;
        else if (*iter == '#')
            return true;
    }

    return false;
}

int TConfig::caseCompare(const string& str1, const string& str2)
{
    size_t i = 0;

    for (string::const_iterator it = str1.begin(); it != str1.end(); ++it)
    {
        if (i >= str2.size())
            return -1;

        if (tolower(*it) != tolower(str2.at(i)))
            return (int)(*it - str2.at(i));

        i++;
    }

    if (i < str1.length() || i < str2.length())
        return 1;

    return 0;
}

string &TConfig::ltrim(string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {return !std::isspace(c);}));
    return s;
}

// trim from end
string &TConfig::rtrim(string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), s.end());
    return s;
}

// trim from both ends
string &TConfig::trim(string &s)
{
    return ltrim(rtrim(s));
}

// Getter / Setter
QStringList TConfig::headers()
{
    return mHeaders;
}

void TConfig::setHeaders(const QStringList &newHeaders)
{
    mHeaders = newHeaders;
}

QColor TConfig::colorTrace()
{
    return mColorTrace;
}

void TConfig::setColorTrace(const QColor &newColorTrace)
{
    mColorTrace = newColorTrace;
}

QColor TConfig::colorDebug()
{
    return mColorDebug;
}

void TConfig::setColorDebug(const QColor &newColorDebug)
{
    mColorDebug = newColorDebug;
}

QColor TConfig::colorError()
{
    return mColorError;
}

void TConfig::setColorError(const QColor &newColorError)
{
    mColorError = newColorError;
}

QColor TConfig::colorWarning()
{
    return mColorWarning;
}

void TConfig::setColorWarning(const QColor &newColorWarning)
{
    mColorWarning = newColorWarning;
}

void TConfig::setColorInfo(const QColor &newColorInfo)
{
    mColorInfo = newColorInfo;
}

QColor TConfig::colorInfo()
{
    return mColorInfo;
}

void TConfig::setLastSavePath(const QString &newLastSavePath)
{
    mLastSavePath = newLastSavePath;
}

QString TConfig::lastSavePath()
{
    return mLastSavePath;
}

void TConfig::setLastOpenPath(const QString &newLastOpenPath)
{
    mLastOpenPath = newLastOpenPath;
}

QString TConfig::lastOpenPath()
{
    return mLastOpenPath;
}

void TConfig::setLastGeometry(const QRect &newLastGeometry)
{
    mLastGeometry = newLastGeometry;
}

void TConfig::setConfigFile(const QString &newConfigFile)
{
    mConfigFile = newConfigFile;
}

QRect TConfig::lastGeometry()
{
    return mLastGeometry;
}

QList<VALUES_t> TConfig::values()
{
    return mValues;
}

void TConfig::setValues(const QList<VALUES_t>& newValues)
{
    mValues = newValues;
}
