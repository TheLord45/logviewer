/*
 * Copyright (C) 2024, 2025 by Andreas Theofilu <andreas@theosys.at>
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

#include <filesystem>
#include <cstring>

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>

#include "tlogger.h"

#include <QJsonObject>
#include <QJsonDocument>

using std::string;
using std::min;
using std::cout;
using std::cerr;
using std::endl;
using std::ofstream;
using std::streambuf;
using std::stringstream;
using std::atomic;
using std::lock_guard;
using std::mutex;

namespace fs = std::filesystem;

LOG_LEVEL_t TLogger::mLogLevel{LVL_WARN};
LOG_LEVEL_t TLogger::mCurLevel{LVL_ERROR};
bool TLogger::mSyslog{true};
atomic<int> TLogger::mIndent{0};
std::string TLogger::mLogFile;
std::ostream *TLogger::mCOut{&std::cout};
std::ostream *TLogger::mCErr{&std::cerr};
std::ofstream *TLogger::mOut{nullptr};
std::streambuf *TLogger::mCOutOrig{nullptr};
std::streambuf *TLogger::mCErrOrig{nullptr};
std::streambuf *TLogger::mOutSyslog{nullptr};
std::string IOLogger::mLogFile;

string _threadIDtoStr(threadID_t tid)
{
    std::stringstream s;
    s << std::hex << std::setw(8) << std::setfill('0') << tid;
    return s.str();
}

threadID_t _getThreadID()
{
#ifdef __APPLE__
    return pthread_mach_thread_np(pthread_self());
#else
    return std::this_thread::get_id();
#endif
}

class TSyslog : public streambuf
{
    public:
        enum { bufsize = 0x2000 };
        TSyslog() { this->setp(buffer, buffer + bufsize - 1); }

    private:
        int overflow(int c)
        {
            if (c == traits_type::eof())
            {
                *this->pptr() = traits_type::to_char_type(c);
                this->sbumpc();
            }

            return this->sync()? traits_type::eof(): traits_type::not_eof(c);
        }

        int sync()
        {
            int rc = 0;

            if (this->pbase() != this->pptr())
            {
                char writebuf[bufsize+1];
                memcpy(writebuf, this->pbase(), this->pptr() - this->pbase());
                writebuf[this->pptr() - this->pbase()] = '\0';
                int eType;

                switch(TLogger::getCurLevel())
                {
                    case LVL_INFO:      eType = LOG_INFO; break;
                    case LVL_WARN:      eType = LOG_WARNING; break;
                    case LVL_ERROR:     eType = LOG_ERR; break;
                    case LVL_TRACE:     eType = LOG_NOTICE; break;
                    case LVL_DEBUG:     eType = LOG_DEBUG; break;
                    case LVL_NOTICE:    eType = LOG_NOTICE; break;
                    case LVL_FATAL:     eType = LOG_CRIT; break;
                }

                syslog(LOG_DAEMON | eType, "%s", writebuf);
                rc = 1;
                this->setp(buffer, buffer + bufsize - 1);
            }

            return rc;
        }

        char buffer[bufsize];
};

TLogger::TLogger()
{
    mOutSyslog = new TSyslog;
    mCOutOrig = cout.rdbuf(mOutSyslog);
    mCErrOrig = cerr.rdbuf(mOutSyslog);
}

void TLogger::log(int level, const char *str, ...)
{
    char buffer[8192];
    va_list valist;

    va_start(valist, str);
    vsnprintf(buffer, sizeof(buffer), str, valist);
    va_end(valist);

    if (mSyslog)
    {
        int l = level;

        if (l <= 0)
            l = LOG_NOTICE;

        syslog(LOG_DAEMON | l, "%s", buffer);
    }
    else
        *mCOut << getTime() << ": " << getLevelStr(sysLevelToLogLevel(level)) << buffer << endl;
}

std::ostream *TLogger::getCurrent(LOG_LEVEL_t level)
{
    mCurLevel = level;
    *mCOut << getTime() << ": " << getLevelStr(level);
    return mCOut;
}

std::ostream *TLogger::getCurrentErr(LOG_LEVEL_t level)
{
    mCurLevel = level;
    *mCErr << getTime() << ": " << getLevelStr(level);
    return mCErr;
}

void TLogger::setLogfile(const string& f)
{
    try
    {
        if (mOut)
        {
            cout.rdbuf(mCOutOrig);
            cerr.rdbuf(mCErrOrig);

            if (mOut->is_open())
                mOut->close();

            delete mOut;
            mOut = nullptr;
        }

        mOut = new ofstream;
        mOut->open(f, std::ios::binary | std::ios::trunc);
        mCOutOrig = cout.rdbuf(mOut->rdbuf());
        mCErrOrig = cerr.rdbuf(mOut->rdbuf());
    }
    catch (std::exception& e)
    {
        syslog(LOG_DAEMON | LOG_ERR, "Couldn't redirect logging to file %s!", f.c_str());

        if (mOut && mOut->is_open())
            mOut->close();

        return;
    }

    mSyslog = false;
}

string TLogger::getLevelStr(LOG_LEVEL_t level)
{
    switch(level)
    {
        case LVL_INFO:      return "INFO:    ";
        case LVL_ERROR:     return "ERROR:   ";
        case LVL_FATAL:     return "FATAL:   ";
        case LVL_DEBUG:     return "DEBUG:   ";
        case LVL_WARN:      return "WARNING: ";
        case LVL_NOTICE:    return "NOTICE:  ";
        case LVL_TRACE:     return "TRACE:   ";
    }

    return string();    // should never be reached!
}

LOG_LEVEL_t TLogger::sysLevelToLogLevel(int lvl)
{
    switch(lvl)
    {
        case LOG_INFO:      return LVL_INFO;
        case LOG_EMERG:     return LVL_FATAL;
        case LOG_ALERT:     return LVL_FATAL;
        case LOG_CRIT:      return LVL_FATAL;
        case LOG_ERR:       return LVL_ERROR;
        case LOG_WARNING:   return LVL_WARN;
        case LOG_NOTICE:    return LVL_TRACE;
        case LOG_DEBUG:     return LVL_DEBUG;
    }

    return LVL_TRACE;
}

string TLogger::getTime()
{
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    rawtime = time(nullptr);
    timeinfo = localtime(&rawtime);

    if (!timeinfo)
    {
        cerr << "ERROR: Couldn't get the local time!" << endl;
        return string();
    }

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    string str(buffer);
    return str;
}

void TLogger::setLogLevel(LOG_LEVEL_t lvl)
{
    if (lvl > LVL_TRACE)
    {
        mLogLevel = LVL_TRACE;
        return;
    }

    if (lvl < LVL_FATAL)
    {
        mLogLevel = LVL_FATAL;
        return;
    }

    mLogLevel = lvl;
}

void TLogger::incIndent()
{
    mIndent++;
}

void TLogger::decIndent()
{
    if (mIndent > 0)
        mIndent--;
}

string TLogger::getIndent()
{
    if (mIndent == 0)
        return string();

    stringstream s;
    s << std::setw(mIndent * 2) << std::left << std::setfill(' ') << " ";
    return s.str();
}

/*
 * class TTracer
 *
 * This class is used in the start of a method or function to print out a text.
 * Using this class means to create an instance of it. When the method or function
 * ends, the class is also destroyed. On destroy the initial string is printed
 * again.
 * When printing the string, a static string is printed before the message,
 * indicating the start and the end of the method. Embedded in a macro, the
 * file name and line number in the file is printed also. Additionally it handles
 * indentation to make it easier to read.
 * This is meant for debugging purposes.
 */
TTracer::TTracer(const std::string& msg, int line, const char *file, threadID_t tid)
    : mThreadID(tid)
{
    if (TLogger::getLogLevel() < LVL_TRACE)
        return;

    lock_guard<mutex> guard(mMutexLogger);      // Block the method to avoid mixing of threads
    string indent = TLogger::getIndent();
    mFile = file;
    size_t pos = mFile.find_last_of("/");

    if (pos != string::npos)
        mFile = mFile.substr(pos + 1);

    *TLogger::getCurrent(LVL_TRACE) << std::setw(5) << std::right << line << ", " << std::setw(20) << std::left << mFile << ", " << _threadIDtoStr(mThreadID) << " " << indent << "{entry " << msg << std::endl;

    TLogger::incIndent();
    mHeadMsg = msg;
    mLine = line;
}

TTracer::~TTracer()
{
    if (TLogger::getLogLevel() < LVL_TRACE)
        return;

    lock_guard<mutex> guard(mMutexLogger);      // Block the method to avoid mixing of threads
    TLogger::decIndent();
    string indent = TLogger::getIndent();
    *TLogger::getCurrent(LVL_TRACE) << "     , " << std::setw(20) << std::left << mFile << ", " << _threadIDtoStr(mThreadID) << " " << indent << "}exit " << mHeadMsg << std::endl;
     mHeadMsg.clear();
}

/*
 * class IOLogger
 */
void IOLogger::logMsg(const string& msg, const string& str)
{
    if (mLogFile.empty())
        return;

    string t = TLogger::getTime();
    ofstream of;
    string seperator = "-----------------------------------------------------------\n";

    try
    {
        of.open(mLogFile, std::ios::out | std::ios::app);
        of.write(t.c_str(), t.length());
        of.write(": ", 2);

        if (!str.empty())
        {
            of.write(str.c_str(), str.length());
            of.write("\n", 1);
        }

        of.write(msg.c_str(), msg.length());
        of.write("\n", 1);
        of.write(seperator.c_str(), seperator.length());
        of.close();
    }
    catch (std::exception &e)
    {
        cerr << "Error on file " << mLogFile << ": " << e.what() << endl;

        if (of.is_open())
            of.close();
    }
}

void IOLogger::logMsg(const QJsonObject& obj, const string& msg)
{
    if (mLogFile.empty())
        return;

    QJsonDocument jdoc;
    jdoc.setObject(obj);
    QByteArray json = jdoc.toJson(QJsonDocument::Compact);
    logMsg(json.toStdString(), msg);
}
