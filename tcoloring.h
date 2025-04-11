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
#ifndef TCOLORING_H
#define TCOLORING_H

#include <QColor>

class TColoring
{
    public:
        TColoring();
        ~TColoring();

        QColor getColor(const QString& id);
        qsizetype getNumberColors() { return mIDs.size(); }

    private:
        typedef struct COLID_t
        {
            QColor color;
            QString ID;
        }COLID_t;

        QList<COLID_t> mIDs;
        QList<QColor> mColors;
};

#endif // TCOLORING_H
