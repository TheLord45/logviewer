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
#include <QTableWidgetItem>
#include <QComboBox>

#include "tvalueselect.h"
#include "ui_tvalueselect.h"
#include "tlogger.h"

TValueSelect::TValueSelect(const QList<VALUES_t>& values, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::TValueSelect),
      mValues(values)
{
    DECL_TRACER("TThreadSelect::TThreadSelect(const QStringList& values, QWidget *parent)");

    ui->setupUi(this);

    QStringList headers;
    headers << "Value name" << "Value type";
    QColor bgColString(Qt::white);
    QColor bgColNumeric(Qt::yellow);
    QColor bgColBool(Qt::lightGray);

    QList<VALUES_t>::const_iterator iter;
    ui->tableWidgetValues->setRowCount(values.size());
    ui->tableWidgetValues->setColumnCount(2);
    ui->tableWidgetValues->setHorizontalHeaderLabels(headers);
    int row = 0;

    for (iter = values.cbegin(); iter != values.cend(); ++iter)
    {
        QTableWidgetItem *itemName = new QTableWidgetItem(iter->name);

        if (iter->type == VTYPE_STRING)
            itemName->setData(Qt::BackgroundRole, bgColString);
        else if (iter->type == VTYPE_BOOL)
            itemName->setData(Qt::BackgroundRole, bgColBool);
        else
            itemName->setData(Qt::BackgroundRole, bgColNumeric);

        ui->tableWidgetValues->setItem(row, 0, itemName);
        QComboBox *cbox = new QComboBox;
        QStringList list = typeList();

        for (int i = 0; i < list.size(); ++i)
        {
            cbox->addItem(list[i], row);
            connect(cbox, &QComboBox::currentIndexChanged, this, &TValueSelect::onCBoxcurrentIndexChanged);

            if (static_cast<VALTYPES_t>(i) == iter->type)
                cbox->setCurrentIndex(i);
        }

        ui->tableWidgetValues->setCellWidget(row, 1, cbox);
        row++;
    }

    ui->tableWidgetValues->resizeColumnsToContents();
    mInitialized = true;
}

TValueSelect::~TValueSelect()
{
    DECL_TRACER("TThreadSelect::~TThreadSelect()");

    delete ui;
}

void TValueSelect::on_tableWidgetValues_itemDoubleClicked(QTableWidgetItem *item)
{
    DECL_TRACER("TValueSelect::on_tableWidgetValues_itemDoubleClicked(QTableWidgetItem *item)");

    ui->tableWidgetValues->openPersistentEditor(item);
}


void TValueSelect::on_tableWidgetValues_itemChanged(QTableWidgetItem *item)
{
    DECL_TRACER("TValueSelect::on_tableWidgetValues_itemChanged(QTableWidgetItem *item)");

    if (!item || !mInitialized)
        return;

    int row = ui->tableWidgetValues->row(item);
    int col = ui->tableWidgetValues->column(item);

    if (row >= mValues.size())
        return;

    if (col == 0)
        mValues[row].name = item->text();
    else
    {
        QColor bgColString(Qt::white);
        QColor bgColNumeric(Qt::yellow);
        QColor bgColBool(Qt::lightGray);

        QComboBox *cbox = static_cast<QComboBox *>(ui->tableWidgetValues->cellWidget(row, col));

        if (!cbox)
            return;

        mValues[row].type = static_cast<VALTYPES_t>(cbox->currentIndex());
        QTableWidgetItem *itemName = ui->tableWidgetValues->item(row, 0);

        if (mValues[row].type == VTYPE_STRING)
            itemName->setData(Qt::BackgroundRole, bgColString);
        else if (mValues[row].type == VTYPE_BOOL)
            itemName->setData(Qt::BackgroundRole, bgColBool);
        else
            itemName->setData(Qt::BackgroundRole, bgColNumeric);
    }
}

void TValueSelect::onCBoxcurrentIndexChanged(int index)
{
    DECL_TRACER("TValueSelect::onCBoxcurrentIndexChanged(int index)");

    if (!mInitialized)
        return;

    QList<VALUES_t>::iterator iter;
    int row = 0;
    QColor bgColString(Qt::white);
    QColor bgColNumeric(Qt::yellow);
    QColor bgColBool(Qt::lightGray);


    for (iter = mValues.begin(); iter != mValues.end(); ++iter)
    {
        QComboBox *cbox = static_cast<QComboBox *>(ui->tableWidgetValues->cellWidget(row, 1));

        if (cbox)
        {
            mValues[row].type = static_cast<VALTYPES_t>(cbox->currentIndex());
            QTableWidgetItem *itemName = ui->tableWidgetValues->item(row, 0);

            if (mValues[row].type == VTYPE_STRING)
                itemName->setData(Qt::BackgroundRole, bgColString);
            else if (mValues[row].type == VTYPE_BOOL)
                itemName->setData(Qt::BackgroundRole, bgColBool);
            else
                itemName->setData(Qt::BackgroundRole, bgColNumeric);
        }

        row++;
    }
}

QString TValueSelect::typeToString(VALTYPES_t t)
{
    DECL_TRACER("TValueSelect::typeToString(VALTYPES_t t)");

    switch(t)
    {
        case VTYPE_STRING:      return "String";
        case VTYPE_INT:         return "Integer";
        case VTYPE_LONG:        return "Long integer";
        case VTYPE_FLOAT:       return "Float";
        case VTYPE_DOUBLE:      return "Double";
        case VTYPE_BOOL:        return "Bool";
    }

    return "Unknown";
}

QStringList TValueSelect::typeList()
{
    DECL_TRACER("TValueSelect::typeList()");

    QStringList list = { "String", "Integer", "Long integer", "Float", "Double", "Bool" };
    return list;
}

