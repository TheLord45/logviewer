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
#include <QStringListModel>

#include "tthreadselect.h"
#include "ui_tthreadselect.h"
#include "tlogger.h"

TThreadSelect::TThreadSelect(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TThreadSelect)
{
    DECL_TRACER("TThreadSelect::TThreadSelect(QWidget *parent) :");

    ui->setupUi(this);
    mModel = new QStringListModel(this);
}

TThreadSelect::~TThreadSelect()
{
    DECL_TRACER("TThreadSelect::~TThreadSelect()");

    delete ui;
}

void TThreadSelect::on_listViewThreads_clicked(const QModelIndex &index)
{
    DECL_TRACER(("TThreadSelect::on_listViewThreads_clicked(const QModelIndex &index)"));

    mSelectedThread = index.row();
}

TThreadSelect::THREAD_LIST_t TThreadSelect::getSelectedThread()
{
    DECL_TRACER("TThreadSelect::getSelectedThread()");

    if (mSelectedThread < 0 || mSelectedThread >= mThreads.size())
        return THREAD_LIST_t();

    return mThreads[mSelectedThread];
}

QList<TThreadSelect::THREAD_LIST_t> TThreadSelect::threads() const
{
    DECL_TRACER(("QList<TThreadSelect::THREAD_LIST_t> TThreadSelect::threads() const"));

    return mThreads;
}

void TThreadSelect::setThreads(const QList<THREAD_LIST_t> &newThreads)
{
    DECL_TRACER("TThreadSelect::setThreads(const QList<THREAD_LIST_t> &newThreads)");

    MSG_DEBUG("Got " << newThreads.size() << "threads");
    mThreads = newThreads;
    QList<THREAD_LIST_t>::iterator iter;
    QStringList list;

    for (iter = mThreads.begin(); iter != mThreads.end(); ++iter)
        list << iter->threadID;

    mModel->setStringList(list);
    int row = 0;

    for (iter = mThreads.begin(); iter != mThreads.end(); ++iter)
    {
        QModelIndex index = mModel->index(row);
        mModel->setData(index, iter->threadColor, Qt::BackgroundRole);
        row++;
    }

    ui->listViewThreads->setModel(mModel);
}
