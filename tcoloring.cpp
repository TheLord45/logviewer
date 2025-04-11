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
#include "tcoloring.h"
#include "tlogger.h"

TColoring::TColoring()
{
    DECL_TRACER("TColoring::TColoring()");

    // Initialize a lot of colors
    for (int r = 255; r > 127; r -= 16)
    {
        for (int g = 255; g > 127; g -= 16)
        {
            for (int b = 255; b > 127; b -= 16)
                mColors.append(qRgb(r, g, b));
        }
    }
}

TColoring::~TColoring()
{
    DECL_TRACER("TColoring::~TColoring()");
}

QColor TColoring::getColor(const QString& id)
{
    DECL_TRACER("TColoring::getColor(const QString& id)");

    QList<COLID_t>::iterator iter;

    if (!mIDs.isEmpty())
    {
        for (iter = mIDs.begin(); iter != mIDs.end(); ++iter)
        {
            if (iter->ID == id)
                return iter->color;
        }
    }

    qsizetype colidx = mIDs.size();

    if (colidx < mColors.size())
    {
        COLID_t cid;
        cid.ID = id;
        cid.color = mColors[colidx];
        mIDs.append(cid);
        return mColors[colidx];
    }

    return Qt::white;
}
