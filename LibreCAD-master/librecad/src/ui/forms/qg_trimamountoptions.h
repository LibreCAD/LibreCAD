/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/
#ifndef QG_TRIMAMOUNTOPTIONS_H
#define QG_TRIMAMOUNTOPTIONS_H

#include<memory>
#include<QWidget>

class RS_ActionInterface;
class RS_ActionModifyTrimAmount;
namespace Ui {
class Ui_TrimAmountOptions;
}

class QG_TrimAmountOptions : public QWidget
{
    Q_OBJECT

public:
    QG_TrimAmountOptions(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~QG_TrimAmountOptions();

public slots:
    virtual void setAction( RS_ActionInterface * a, bool update );
    virtual void updateDist( const QString & d );

protected:
    RS_ActionModifyTrimAmount* action;

protected slots:
    virtual void languageChange();

private slots:
    void on_cbTotalLength_toggled(bool checked);
	void on_leDist_editingFinished();

private:
	void saveSettings();
	std::unique_ptr<Ui::Ui_TrimAmountOptions> ui;
};

#endif // QG_TRIMAMOUNTOPTIONS_H
