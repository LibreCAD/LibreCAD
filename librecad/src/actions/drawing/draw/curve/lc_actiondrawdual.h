/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 Dongxu Li (github.com/dxli)
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
//! File: lc_actiondrawdual.h


#ifndef LC_ACTIONDRAWDUAL_H
#define LC_ACTIONDRAWDUAL_H

#include "lc_actionpreselectionawarebase.h"

class LC_ActionDrawDual : public LC_ActionPreSelectionAwareBase
{
  Q_OBJECT

public:
  enum Status {
    ChooseCenter,
    Finished
  };

public:
  explicit LC_ActionDrawDual(LC_ActionContext* context);
  ~LC_ActionDrawDual() override = default;

  void init(int status) override;
  void trigger() override;

  void mouseMoveEvent(QMouseEvent* e) override;
  void mouseReleaseEvent(QMouseEvent* e) override;
  void coordinateEvent(RS_CoordinateEvent* e) override;

  void updateMouseButtonHints() override;
  void updateMouseButtonHintsForSelection() override;
  void doTrigger(bool /*keepSelected*/) override;

private:
  RS_Vector m_center{false};

  void createDualForSelected();
  RS_Vector symmetricOf(const RS_Vector& p) const;
};

#endif // LC_ACTIONDRAWDUAL_H