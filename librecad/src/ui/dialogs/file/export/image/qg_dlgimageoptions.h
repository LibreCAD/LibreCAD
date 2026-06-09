/*
 * **************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2025 LibreCAD.org
 * Copyright (C) 2025 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * *********************************************************************
 *
 */
#ifndef QG_IMAGEOPTIONSDIALOG_H
#define QG_IMAGEOPTIONSDIALOG_H

#include "lc_dialog.h"
#include "rs_vector.h"
#include "ui_qg_dlgimageoptions.h"

class QG_ImageOptionsDialog : public LC_Dialog, public Ui::QG_ImageOptionsDialog{
    Q_OBJECT
public:
    explicit QG_ImageOptionsDialog(QWidget* parent = nullptr);
    ~QG_ImageOptionsDialog() override = default;
    QSize getSize() const;
    QSize getBorders() const;
    bool isBackgroundBlack() const;
    bool isBlackWhite() const;
public slots:
    void setGraphicSize( const RS_Vector & s );
    void ok();
    void sizeChanged();
    void resolutionChanged();
    void sameBordersChanged() const;
    void borderChanged() const;
protected slots:
    void languageChange();
private:
    RS_Vector m_graphicSize;
    bool m_updateEnabled;
    bool m_useResolution;
    void init();
};

#endif
