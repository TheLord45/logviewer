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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>

#include "tthreadselect.h"

#define V_MAJOR     1
#define V_MINOR     1
#define V_PATCH     0

/**
 * @def VERSION
 * Defines the version of this application.
 */
#define VERSION      ((V_MAJOR * 0x10000) + (V_MINOR * 0x100) + V_PATCH)

#define VERSION_STRING() _GET_X_VERSION(V_MAJOR, V_MINOR, V_PATCH)
#define _GET_X_VERSION(a, b, c) _GET_VERSION(a, b, c)
#define _GET_VERSION(a, b, c) ( #a "." #b "." #c )          // Release version

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QLabel;
class TWait;
class QAbstractItemModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        MainWindow(QString& file, QWidget *parent = nullptr);
        ~MainWindow();

    protected:
        void initialize();
        QString getLogFileName(QString *filter=nullptr);
        bool parseFile(qsizetype totalLines=0, const QString& filter="", const QString& thread_ilter="");
        void pressed(const QModelIndex &index);

        void keyPressEvent(QKeyEvent *event) override;
        void resizeEvent(QResizeEvent *event) override;
        void closeEvent(QCloseEvent *event) override;

    private slots:
        void on_actionOpen_triggered();
        void on_actionSave_result_triggered();
        void on_actionSave_result_as_triggered();
        void on_actionLoad_profile_triggered();
        void on_actionSave_profile_triggered();
        void on_actionSave_profile_as_triggered();
        void on_actionExit_triggered();
        void on_actionValidate_consistnace_triggered();
        void on_actionFind_exceptions_triggered();
        void on_actionSearch_triggered();
        void on_actionFilter_thread_triggered(bool checked);
        void on_actionReload_triggered();
        void on_actionSettings_triggered();
        void on_actionAbout_triggered();

        void on_textEditResult_selectionChanged();

        void onPopupMenuCopyTriggered(bool checked=false);
        void onPopupMenuSearchTriggered(bool checked=false);

    private:
        QList<QString> split(const QString& str, const QString& deli, int cols=-1);
        qsizetype search(const QString& text, qsizetype offset=0, int col=-1);
        bool writeFile(const QString& file);
        QString getFileName(const QString& name);
        void clearStatusbar();
        void filterThread(const QString& threadID);
        qsizetype countLines(const QString& file);

        Ui::MainWindow *ui;
        qsizetype mTotalLines{0};
        QString mFile;
        QLabel *mLbFile{nullptr};
        QLabel *mLbLines{nullptr};
        QLabel *mLbTraces{nullptr};
        QLabel *mLbInfos{nullptr};
        QLabel *mLbWarnings{nullptr};
        QLabel *mLbErrors{nullptr};
        QLabel *mLbDebugs{nullptr};
        QLabel *mLbOthers{nullptr};
        TWait *mWait{nullptr};
        qsizetype mLastSearchLine{0};
        QString mLastSearchText;
        QString mSaveFile;
        QString mTempFile;
        QString mProfile;
        QString mLastFileFilter;
        QList<TThreadSelect::THREAD_LIST_t> mThreads;   // If there is a thread column, this contains a list of all different thread IDs
        bool mLastFilterCheck{false};                   // If there is a thread column, this defines whether a filter should be applied or not
        QMenu *mPopupMenu{nullptr};
        const QAbstractItemModel *mModelMenu{nullptr};
        QModelIndex mModelIndex;
        int mMenuColumn{-1};
};
#endif // MAINWINDOW_H
