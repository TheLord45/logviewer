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
#include <QColorDialog>
#include <QKeyEvent>

#include "tqtsettings.h"
#include "ui_tqtsettings.h"
#include "tconfig.h"
#include "tlogger.h"

TQtSettings::TQtSettings(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TQtSettings)
{
    DECL_TRACER("TQtSettings::TQtSettings(QWidget *parent)");

    ui->setupUi(this);

    mBlockEntry = TConfig::getBlockEntry();
    mBlockExit = TConfig::getBlockExit();
    mTagInfo = TConfig::getTagInfo();
    mColorInfo = TConfig::colorInfo();
    mTagWarning = TConfig::getTagWarning();
    mColorWarning = TConfig::colorWarning();
    mTagError = TConfig::getTagError();
    mColorError = TConfig::colorError();
    mTagDebug = TConfig::getTagDebug();
    mColorDebug = TConfig::colorDebug();
    mTagTrace = TConfig::getTagTrace();
    mColorTrace = TConfig::colorTrace();
    mDelimeter = TConfig::getDelimeter();
    mColumns = TConfig::getColumns();
    mHeaders = TConfig::headers();
    mColAlign = TConfig::getColAligns();
    mColumnThreadID = TConfig::getColumnThreadID();
    mValues = TConfig::values();

    mLogfile = TConfig::getLogfile();
    mSourcePath = TConfig::getSourcePath();
    mResultPath = TConfig::getResultPath();
    mLogLevel = TConfig::getLogLevel();

    ui->lineEditStart->setText(mBlockEntry);
    ui->lineEditEnd->setText(mBlockExit);
    ui->lineEditInfo->setText(mTagInfo);
    ui->labelColorInfo->setText(colorToHexString(mColorInfo));
    ui->labelColorInfo->setStyleSheet(QString("background-color: #%1;").arg(colorToHexString(mColorInfo)));
    ui->lineEditWarning->setText(mTagWarning);
    ui->labelColorWarning->setText(colorToHexString(mColorWarning));
    ui->labelColorWarning->setStyleSheet(QString("background-color: #%1;").arg(colorToHexString(mColorWarning)));
    ui->lineEditError->setText(mTagError);
    ui->labelColorError->setText(colorToHexString(mColorError));
    ui->labelColorError->setStyleSheet(QString("background-color: #%1;").arg(colorToHexString(mColorError)));
    ui->lineEditDebug->setText(mTagDebug);
    ui->labelColorDebug->setText(colorToHexString(mColorDebug));
    ui->labelColorDebug->setStyleSheet(QString("background-color: #%1;").arg(colorToHexString(mColorDebug)));
    ui->lineEditTrace->setText(mTagTrace);
    ui->labelColorTrace->setText(colorToHexString(mColorTrace));
    ui->labelColorTrace->setStyleSheet(QString("background-color: #%1;").arg(colorToHexString(mColorTrace)));
    ui->lineEditDelimeter->setText(mDelimeter);
    ui->spinBoxColumns->setValue(mColumns);
    ui->listWidgetColumns->clear();
    ui->listWidgetColumns->addItems(mHeaders);
    ui->lineEditColAlign->setText(mColAlign);
    ui->spinBoxThreadID->setValue(mColumnThreadID);

    ui->lineEditLogfile->setText(mLogfile);
    ui->lineEditSourcePath->setText(mSourcePath);
    ui->lineEditResultPath->setText(mResultPath);
    ui->spinBoxLogLevel->setValue(mLogLevel);
}

TQtSettings::~TQtSettings()
{
    delete ui;
}

void TQtSettings::on_lineEditStart_textChanged(const QString &arg1)
{
    mBlockEntry = arg1;
}


void TQtSettings::on_lineEditEnd_textChanged(const QString &arg1)
{
    mBlockExit = arg1;
}


void TQtSettings::on_lineEditInfo_textChanged(const QString &arg1)
{
    mTagInfo = arg1;
}


void TQtSettings::on_lineEditWarning_textChanged(const QString &arg1)
{
    mTagWarning = arg1;
}


void TQtSettings::on_lineEditError_textChanged(const QString &arg1)
{
    mTagError = arg1;
}


void TQtSettings::on_lineEditDebug_textChanged(const QString &arg1)
{
    mTagDebug = arg1;
}


void TQtSettings::on_lineEditDelimeter_textChanged(const QString &arg1)
{
    mDelimeter = arg1;
}

void TQtSettings::on_spinBoxColumns_valueChanged(int arg1)
{
    DECL_TRACER("TQtSettings::on_spinBoxColumns_valueChanged(int arg1)");

    MSG_DEBUG("Old columns: " << mColumns << ", new columns: " << arg1);

    if (arg1 < mColumns)
    {
        for (int i = arg1; i < mColumns; ++i)
        {
            mHeaders.removeLast();
            mValues.removeLast();
        }
    }
    else
    {
        for (int i = mColumns; i < arg1; ++i)
        {
            mHeaders << QString("Col %1").arg(i+1);
            TValueSelect::VALUES_t vt;
            vt.name = QString("Value %1").arg(i+1);
            vt.type = TValueSelect::VTYPE_STRING;
            mValues.append(vt);
        }
    }

    ui->listWidgetColumns->clear();
    ui->listWidgetColumns->addItems(mHeaders);
    mColumns = arg1;

    if (mColumns > mColumnThreadID && mColumnThreadID > 0)
    {
        mColumnThreadID = mColumns;
        ui->spinBoxThreadID->setValue(mColumnThreadID);
    }
}

void TQtSettings::on_spinBoxThreadID_valueChanged(int arg1)
{
    DECL_TRACER("TQtSettings::on_spinBoxThreadID_valueChanged(int arg1)");

    if (arg1 <= mColumns)
        mColumnThreadID = arg1;
    else
        ui->spinBoxThreadID->setValue(mColumnThreadID);
}

void TQtSettings::on_toolButtonValue_clicked()
{
    DECL_TRACER("TQtSettings::on_toolButtonValue_clicked()");

    TValueSelect ts(mValues, this);

    if (ts.exec() == QDialog::Rejected)
        return;

    mValues = ts.getValues();
}

void TQtSettings::on_lineEditLogfile_textChanged(const QString &arg1)
{
    mLogfile = arg1;
}


void TQtSettings::on_lineEditResultPath_textChanged(const QString &arg1)
{
    mResultPath = arg1;
}


void TQtSettings::on_lineEditSourcePath_textChanged(const QString &arg1)
{
    mSourcePath = arg1;
}

void TQtSettings::on_toolButtonLogfile_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Log File"),
                                                    ".",
                                                    tr("Files (*.log)"));

    if (!fileName.isEmpty())
    {
        mLogfile = fileName;
        ui->lineEditLogfile->setText(mLogfile);
    }
}

void TQtSettings::on_toolButtonResultPath_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    ".",
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty())
    {
        mResultPath = dir;
        ui->lineEditResultPath->setText(dir);
    }
}

void TQtSettings::on_toolButtonSourcePath_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    ".",
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty())
    {
        mSourcePath = dir;
        ui->lineEditSourcePath->setText(dir);
    }
}

void TQtSettings::on_spinBoxLogLevel_valueChanged(int arg1)
{
    DECL_TRACER("TQtSettings::on_spinBoxLogLevel_valueChanged(int arg1)");

    mLogLevel = arg1;
}

void TQtSettings::on_lineEditTrace_textChanged(const QString &arg1)
{
    DECL_TRACER("TQtSettings::on_lineEditTrace_textChanged(const QString &arg1)");

    mTagTrace = arg1;
}

void TQtSettings::on_lineEditColAlign_textChanged(const QString &arg1)
{
    DECL_TRACER("TQtSettings::on_lineEditColAlign_textChanged(const QString &arg1)");

    mColAlign = arg1;
}

void TQtSettings::on_toolButtonColInfo_clicked()
{
    DECL_TRACER("TQtSettings::on_toolButtonColInfo_clicked()");

    QColorDialog colDialog(mColorInfo, this);

    if (colDialog.exec() == QDialog::Accepted)
    {
        mColorInfo = colDialog.selectedColor();
        ui->labelColorInfo->setText(colorToHexString(mColorInfo));
        ui->labelColorInfo->setStyleSheet(QString("background-color: #%1;").arg(colorToHexString(mColorInfo)));
    }
}

void TQtSettings::on_toolButtonColWarning_clicked()
{
    DECL_TRACER("TQtSettings::on_toolButtonColWarning_clicked()");

    QColorDialog colDialog(mColorWarning, this);

    if (colDialog.exec() == QDialog::Accepted)
    {
        mColorWarning = colDialog.selectedColor();
        ui->labelColorWarning->setText(colorToHexString(mColorWarning));
        ui->labelColorWarning->setStyleSheet(QString("background-color: #%1;").arg(colorToHexString(mColorWarning)));
    }
}

void TQtSettings::on_toolButtonColError_clicked()
{
    DECL_TRACER("TQtSettings::on_toolButtonColError_clicked()");

    QColorDialog colDialog(mColorError, this);

    if (colDialog.exec() == QDialog::Accepted)
    {
        mColorError = colDialog.selectedColor();
        ui->labelColorError->setText(colorToHexString(mColorError));
        ui->labelColorError->setStyleSheet(QString("background-color: #%1;").arg(colorToHexString(mColorError)));
    }
}

void TQtSettings::on_toolButtonColDebug_clicked()
{
    DECL_TRACER("TQtSettings::on_toolButtonColDebug_clicked()");

    QColorDialog colDialog(mColorDebug, this);

    if (colDialog.exec() == QDialog::Accepted)
    {
        mColorDebug = colDialog.selectedColor();
        ui->labelColorDebug->setText(colorToHexString(mColorDebug));
        ui->labelColorDebug->setStyleSheet(QString("background-color: #%1;").arg(colorToHexString(mColorDebug)));
    }
}

void TQtSettings::on_toolButtonColTrace_clicked()
{
    DECL_TRACER("TQtSettings::on_toolButtonColTrace_clicked()");

    QColorDialog colDialog(mColorTrace, this);

    if (colDialog.exec() == QDialog::Accepted)
    {
        mColorTrace = colDialog.selectedColor();
        ui->labelColorTrace->setText(colorToHexString(mColorTrace));
        ui->labelColorTrace->setStyleSheet(QString("background-color: #%1;").arg(colorToHexString(mColorTrace)));
    }
}

void TQtSettings::on_listWidgetColumns_itemChanged(QListWidgetItem *item)
{
    DECL_TRACER("TQtSettings::on_listWidgetColumns_itemChanged(QListWidgetItem *item)");

    int row = ui->listWidgetColumns->row(item);

    if (row >= mColumns || row >= mHeaders.size())
        return;

    mHeaders[row] = item->text();
}

void TQtSettings::on_listWidgetColumns_itemDoubleClicked(QListWidgetItem *item)
{
    DECL_TRACER("TQtSettings::on_listWidgetColumns_itemDoubleClicked(QListWidgetItem *item)");

    mLastEditItem = item;
    ui->listWidgetColumns->openPersistentEditor(item);
}

void TQtSettings::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) && mLastEditItem)
    {
        ui->listWidgetColumns->closePersistentEditor(mLastEditItem);
        mLastEditItem = nullptr;
    }
    else
        QDialog::keyPressEvent(event);
}

void TQtSettings::saveValues()
{
    DECL_TRACER("TQtSettings::saveValues()");

    TConfig::setBlockEntry(mBlockEntry);
    TConfig::setsetBlockExit(mBlockExit);
    TConfig::setTagInfo(mTagInfo);
    TConfig::setColorInfo(mColorInfo);
    TConfig::setTagWarning(mTagWarning);
    TConfig::setColorWarning(mColorWarning);
    TConfig::setTagError(mTagError);
    TConfig::setColorError(mColorError);
    TConfig::setTagDebug(mTagDebug);
    TConfig::setColorDebug(mColorDebug);
    TConfig::setTagTrace(mTagTrace);
    TConfig::setColorTrace(mColorTrace);
    TConfig::setDelimeter(mDelimeter);
    TConfig::setColumns(mColumns);
    TConfig::setHeaders(mHeaders);
    TConfig::setColAligns(mColAlign);
    TConfig::setColumnThreadID(mColumnThreadID);
    TConfig::setValues(mValues);
    TConfig::setSourcePath(mSourcePath);
    TConfig::setResultPath(mResultPath);

    if (mLogfile != TConfig::getLogfile())
    {
        TConfig::setLogfile(mLogfile);
        TLogger::setLogfile(mLogfile.toStdString());
    }

    if (mLogLevel != TConfig::getLogLevel())
    {
        TConfig::setLogLevel(mLogLevel);
        TLogger::setLogLevel(static_cast<LOG_LEVEL_t>(mLogLevel));
    }

    TConfig::saveConfig();
}

QString TQtSettings::colorToHexString(const QColor& col)
{
    DECL_TRACER("TQtSettings::colorToHexString(const QColor& col)");

    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0') << std::hex << col.red();
    ss << std::setw(2) << std::setfill('0') << std::hex << col.green();
    ss << std::setw(2) << std::setfill('0') << std::hex << col.blue();
    return QString::fromStdString(ss.str());
}
