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
#ifndef TQTSETTINGS_H
#define TQTSETTINGS_H

#include <QDialog>

#include "tvalueselect.h"

namespace Ui {
class TQtSettings;
}

class QListWidgetItem;

class TQtSettings : public QDialog
{
    Q_OBJECT

    public:
        explicit TQtSettings(QWidget *parent = nullptr);
        ~TQtSettings();

        void saveValues();

    protected:
        void keyPressEvent(QKeyEvent *event) override;

    private slots:
        void on_lineEditStart_textChanged(const QString &arg1);
        void on_lineEditEnd_textChanged(const QString &arg1);
        void on_lineEditInfo_textChanged(const QString &arg1);
        void on_toolButtonColInfo_clicked();
        void on_lineEditWarning_textChanged(const QString &arg1);
        void on_toolButtonColWarning_clicked();
        void on_lineEditError_textChanged(const QString &arg1);
        void on_toolButtonColError_clicked();
        void on_lineEditDebug_textChanged(const QString &arg1);
        void on_toolButtonColDebug_clicked();
        void on_lineEditTrace_textChanged(const QString &arg1);
        void on_toolButtonColTrace_clicked();
        void on_lineEditDelimeter_textChanged(const QString &arg1);
        void on_spinBoxColumns_valueChanged(int arg1);
        void on_listWidgetColumns_itemChanged(QListWidgetItem *item);
        void on_listWidgetColumns_itemDoubleClicked(QListWidgetItem *item);
        void on_lineEditColAlign_textChanged(const QString &arg1);
        void on_spinBoxThreadID_valueChanged(int arg1);
        void on_toolButtonValue_clicked();

        void on_lineEditLogfile_textChanged(const QString &arg1);
        void on_lineEditResultPath_textChanged(const QString &arg1);
        void on_lineEditSourcePath_textChanged(const QString &arg1);
        void on_spinBoxLogLevel_valueChanged(int arg1);

        void on_toolButtonLogfile_clicked();
        void on_toolButtonResultPath_clicked();
        void on_toolButtonSourcePath_clicked();

    private:
        QString colorToHexString(const QColor& col);

        Ui::TQtSettings *ui;
        QString mBlockEntry;
        QString mBlockExit;
        QString mTagInfo;
        QColor mColorInfo;
        QString mTagWarning;
        QColor mColorWarning;
        QString mTagError;
        QColor mColorError;
        QString mTagDebug;
        QColor mColorDebug;
        QString mTagTrace;
        QColor mColorTrace;
        QString mDelimeter;
        int mColumns{0};
        QStringList mHeaders;
        QString mColAlign;
        int mColumnThreadID{0};

        QString mLogfile;
        QString mSourcePath;
        QString mResultPath;
        int mLogLevel{0};
        QListWidgetItem *mLastEditItem{nullptr};
        QList<TValueSelect::VALUES_t> mValues;
};

#endif // TQTSETTINGS_H
