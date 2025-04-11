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
#ifndef TCONFIG_H
#define TCONFIG_H

#include <QString>

#include "tvalueselect.h"

class QRect;
class QColor;

class TConfig
{
    public:
        static void readConfig();
        static void saveConfig();
        static void readProfile(const QString& pf);
        static void saveProfile(const QString& pf);

        static QString& getBlockEntry() { return mBlockEntry; }
        static void setBlockEntry(const QString& str) { mBlockEntry = str; }
        static QString& getBlockExit() { return mBlockExit; }
        static void setsetBlockExit(const QString& str) { mBlockExit = str; }
        static QString& getTagInfo() { return mTagInfo; }
        static void setTagInfo(const QString& str) { mTagInfo = str; }
        static QString& getTagWarning() { return mTagWarning; }
        static void setTagWarning(const QString& str) { mTagWarning = str; }
        static QString& getTagError() { return mTagError; }
        static void setTagError(const QString& str) { mTagError = str; }
        static QString& getTagDebug() { return mTagDebug; }
        static void setTagDebug(const QString& str) { mTagDebug = str; }
        static QString& getTagTrace() { return mTagTrace; }
        static void setTagTrace(const QString& str) { mTagTrace = str; }
        static QString& getDelimeter() { return mDelimenter; }
        static void setDelimeter(const QString& str) { mDelimenter = str; }
        static int getColumns() { return mColumns; }
        static void setColumns(int col) { mColumns = col; }
        static QString& getLogfile() { return mLogfile; }
        static void setLogfile(const QString& str) { mLogfile = str; }
        static QString& getSourcePath() { return mSourcePath; }
        static void setSourcePath(const QString& str) { mSourcePath = str; }
        static QString& getResultPath() { return mResultPath; }
        static void setResultPath(const QString& str) { mResultPath = str; }
        static int getLogLevel() { return mLogLevel; };
        static void setLogLevel(int newLogLevel) { mLogLevel = newLogLevel; };
        static QString& getColAligns() { return mColAligns; }
        static void setColAligns(const QString& str) { mColAligns = str; }
        static int getColumnThreadID() { return mColumnThreadID; }
        static void setColumnThreadID(int col) { mColumnThreadID = col; }

        static QRect lastGeometry();
        static void setLastGeometry(const QRect &newLastGeometry);
        static QString lastOpenPath();
        static void setLastOpenPath(const QString &newLastOpenPath);
        static QString lastSavePath();
        static void setLastSavePath(const QString &newLastSavePath);

        static void setConfigFile(const QString &newConfigFile);

        static QColor colorInfo();

        static void setColorInfo(const QColor &newColorInfo);

        static QColor colorWarning();
        static void setColorWarning(const QColor &newColorWarning);

        static QColor colorError();
        static void setColorError(const QColor &newColorError);

        static QColor colorDebug();
        static void setColorDebug(const QColor &newColorDebug);

        static QColor colorTrace();
        static void setColorTrace(const QColor &newColorTrace);

        static QStringList headers();
        static void setHeaders(const QStringList &newHeaders);

        static QList<TValueSelect::VALUES_t> values();
        static void setValues(const QList<TValueSelect::VALUES_t>& newValues);

    protected:
        static QString getConfigFile();
        static bool isRemark(const std::string& line);
        static int caseCompare(const std::string& str1, const std::string& str2);
        static std::string& ltrim(std::string &s);
        static std::string& rtrim(std::string &s);
        static std::string& trim(std::string &s);

    private:
        TConfig() {};
        static void initialize();

        static QString mBlockEntry;
        static QString mBlockExit;
        static QString mTagInfo;
        static QColor mColorInfo;
        static QString mTagWarning;
        static QColor mColorWarning;
        static QString mTagError;
        static QColor mColorError;
        static QString mTagDebug;
        static QColor mColorDebug;
        static QString mTagTrace;
        static QColor mColorTrace;
        static QString mDelimenter;
        static int mColumns;
        static QStringList mHeaders;
        static QString mColAligns;
        static int mColumnThreadID;
        static QList<TValueSelect::VALUES_t> mValues;

        static QString mLogfile;
        static int mLogLevel;
        static QString mSourcePath;
        static QString mResultPath;

        static QString mConfigFile;

        static QRect mLastGeometry;
        static QString mLastOpenPath;
        static QString mLastSavePath;
};

#endif // TCONFIG_H
