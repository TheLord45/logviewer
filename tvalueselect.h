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
#ifndef TVALUESELECT_H
#define TVALUESELECT_H

#include <QDialog>

namespace Ui {
class TValueSelect;
}

class QTableWidgetItem;

class TValueSelect : public QDialog
{
    Q_OBJECT

    public:
        typedef enum VALTYPES_t
        {
            VTYPE_STRING,
            VTYPE_INT,
            VTYPE_LONG,
            VTYPE_FLOAT,
            VTYPE_DOUBLE,
            VTYPE_BOOL
        }VALTYPES_t;

        typedef struct VALUES_t
        {
            VALTYPES_t type;
            QString name;
        }VALUES_t;

        explicit TValueSelect(const QList<VALUES_t>& values, QWidget *parent = nullptr);
        ~TValueSelect();

        QList<VALUES_t>& getValues() { return mValues; }

    protected:
        QString typeToString(VALTYPES_t t);
        QStringList typeList();
        void onCBoxcurrentIndexChanged(int index);

    private slots:
        void on_tableWidgetValues_itemDoubleClicked(QTableWidgetItem *item);
        void on_tableWidgetValues_itemChanged(QTableWidgetItem *item);

    private:
        Ui::TValueSelect *ui;
        QList<VALUES_t> mValues;
        bool mInitialized{false};
};

#endif // TVALUESELECT_H
