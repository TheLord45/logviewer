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
#ifndef TTHREADSELECT_H
#define TTHREADSELECT_H

#include <QDialog>

namespace Ui {
    class TThreadSelect;
}

class QStringListModel;

class TThreadSelect : public QDialog
{
        Q_OBJECT

    public:
        typedef struct THREAD_LIST_t
        {
            QString threadID;
            QColor threadColor;
        }THREAD_LIST_t;

        explicit TThreadSelect(QWidget *parent = nullptr);
        ~TThreadSelect();

        THREAD_LIST_t getSelectedThread();
        QList<THREAD_LIST_t> threads() const;
        void setThreads(const QList<THREAD_LIST_t> &newThreads);

    private slots:
        void on_listViewThreads_clicked(const QModelIndex &index);

    private:
        Ui::TThreadSelect *ui;
        QList<THREAD_LIST_t> mThreads;
        int mSelectedThread{-1};
        QStringListModel *mModel{0};
};

#endif // TTHREADSELECT_H
