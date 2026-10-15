// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD contributors
#ifndef LC_GRIPINPUTWIDGET_H
#define LC_GRIPINPUTWIDGET_H

#include <QFrame>
#include "rs_vector.h"
class QLineEdit;
class QLabel;
class LC_GraphicViewport;

// Temporary dimensional input for a single grip; never owns drawing geometry.
class LC_GripInputWidget : public QFrame {
    Q_OBJECT
public:
    LC_GripInputWidget(QWidget* parent, LC_GraphicViewport* viewport);
    void begin(const RS_Vector& anchor, const RS_Vector& point, const QString& lengthCaption, bool reversed);
    void updatePoint(const RS_Vector& point);
    RS_Vector constrain(const RS_Vector& point) const;
    bool inputValid() const;
    void focusLength();
    bool eventFilter(QObject* watched, QEvent* event) override;
signals:
    void previewChanged();
    void accepted();
    void cancelled();
private:
    LC_GraphicViewport* m_viewport;
    QLineEdit* m_length;
    QLineEdit* m_angle;
    QLabel* m_lengthCaption;
    RS_Vector m_anchor;
    double m_planeZ{0};
    bool m_reversed{false};
    bool m_lengthLocked{false};
    bool m_angleLocked{false};
    bool m_dimensionLength{false};
    bool value(QLineEdit* field, double& result, bool positive) const;
};
#endif
