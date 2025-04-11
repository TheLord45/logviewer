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

#ifndef __TLOGGER_H__
#define __TLOGGER_H__

#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>

#include <syslog.h>

class QJsonObject;

typedef enum
{
    LVL_FATAL,
    LVL_ERROR,
    LVL_WARN,
    LVL_INFO,
    LVL_DEBUG,
    LVL_NOTICE,
    LVL_TRACE
}LOG_LEVEL_t;

#ifdef __APPLE__
    typedef uint64_t threadID_t;
#elif defined(__EMSCRIPTEN__)
    typedef std::__thread_id threadID_t;
#else
    typedef std::thread::id threadID_t;
#endif

threadID_t _getThreadID();

class TLogger
{
    public:
        TLogger();
        TLogger(const std::string& lf) { mLogFile = lf; }
        ~TLogger() {}

        static std::ostream *getStdOut() { return mCOut; }
        static std::ostream *getStdErr() { return mCErr; }
        static std::ostream *getCurrent(LOG_LEVEL_t level);
        static std::ostream *getCurrentErr(LOG_LEVEL_t level);
        static void setLogfile(const std::string& f);
        static void log(int level, const char *str, ...);
        static void setSyslog(bool s) { mSyslog = s; }
        static LOG_LEVEL_t getLogLevel() { return mLogLevel; }
        static void setLogLevel(LOG_LEVEL_t lvl);
        static LOG_LEVEL_t getCurLevel() { return mCurLevel; }
        static void incIndent();
        static void decIndent();
        static std::string getIndent();

        static std::string getTime();

    protected:
        static std::string getLevelStr(LOG_LEVEL_t level);
        static LOG_LEVEL_t sysLevelToLogLevel(int lvl);

    private:
        static LOG_LEVEL_t mLogLevel;
        static LOG_LEVEL_t mCurLevel;
        static bool mSyslog;
        static std::atomic<int> mIndent;
        static std::string mLogFile;
        static std::ostream *mCOut;
        static std::ostream *mCErr;
        static std::ofstream *mOut;
        static std::streambuf *mCOutOrig;
        static std::streambuf *mCErrOrig;
        static std::streambuf *mOutSyslog;
};

class TTracer
{
    public:
        TTracer(const std::string& msg, int line, const char *file, threadID_t tid);
        ~TTracer();

    private:
        std::string mHeadMsg;
        int mLine{0};
        std::string mFile;
        std::chrono::steady_clock::time_point mTimePoint;
#ifdef __EMSCRIPTEN__
        threadID_t mThreadID;
#else
        threadID_t mThreadID{0};
#endif
        std::mutex mMutexLogger;
};

class IOLogger
{
    public:
        static void logMsg(const std::string& msg, const std::string& str="");
        static void logMsg(const QJsonObject& obj, const std::string& msg="");
        static void setLogFile(const std::string& fn) { mLogFile = fn; }

    private:
        IOLogger() {}
        ~IOLogger() {}

        static std::string mLogFile;
};

#define MSG_INFO(msg)           if (TLogger::getLogLevel() >= LVL_INFO) *TLogger::getCurrent(LVL_INFO) << msg << std::endl
#define MSG_WARN(msg)           if (TLogger::getLogLevel() >= LVL_WARN) *TLogger::getCurrent(LVL_WARN) << msg << std::endl
#define MSG_ERROR(msg)          *TLogger::getCurrentErr(LVL_ERROR) << msg << std::endl
#define MSG_FATAL(msg)          *TLogger::getCurrentErr(LVL_FATAL) << msg << std::endl
#define MSG_DEBUG(msg)          if (TLogger::getLogLevel() >= LVL_DEBUG) *TLogger::getCurrent(LVL_DEBUG) << msg << std::endl
#define MSG_NOTICE(msg)         if (TLogger::getLogLevel() >= LVL_NOTICE) *TLogger::getCurrent(LVL_NOTICE) << msg << std::endl
#define MSG_TRACE(msg)          if (TLogger::getLogLevel() == LVL_TRACE) *TLogger::getCurrent(LVL_TRACE) << msg << std::endl

#define DECL_TRACER(msg)        TTracer _hidden_tracer(msg, __LINE__, static_cast<const char *>(__FILE__), _getThreadID())

#define MSG_IO(obj, msg)        IOLogger::logMsg(obj, msg)

#define DB_ERROR(query)         {   if (query.lastError().type() != QSqlError::NoError) {\
                                        std::string sql = query.executedQuery().toStdString();\
                                        MSG_ERROR("SQL query failed: " << sql);\
                                        if (sql.find("?") != std::string::npos) {\
                                            QVariantList qlist = query.boundValues();\
                                            if (!qlist.isEmpty()) {\
                                                QVariantList::iterator iter;\
                                                int pos = 0;\
                                                for (iter = qlist.begin(); iter != qlist.end(); ++iter) {\
                                                    MSG_INFO("Value " << pos << ": " << iter->toString().toStdString());\
                                                    pos++;\
                                                }\
                                            }\
                                        }\
                                        MSG_ERROR("SQL database error: " << query.lastError().databaseText().toStdString());\
                                        MSG_ERROR("SQL driver error: " << query.lastError().driverText().toStdString());\
                                        MSG_ERROR("SQL native error: " << query.lastError().nativeErrorCode().toStdString());\
                                    }\
                                    else\
                                        MSG_DEBUG("SQL query: " << query.lastQuery().toStdString());\
                                }

#define DB_DBSTATE(db)          {\
                                    MSG_ERROR("SQL database error: " << db.lastError().databaseText().toStdString());\
                                    MSG_ERROR("SQL driver error: " << db.lastError().driverText().toStdString());\
                                    MSG_ERROR("SQL native error: " << db.lastError().nativeErrorCode().toStdString());\
                                }
#endif
