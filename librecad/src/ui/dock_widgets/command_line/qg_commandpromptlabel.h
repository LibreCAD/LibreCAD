// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD contributors

#ifndef QG_COMMANDPROMPTLABEL_H
#define QG_COMMANDPROMPTLABEL_H

#include <QLabel>
#include <QResizeEvent>

// A long action prompt must not widen the command dock or squeeze out its input.
class QG_CommandPromptLabel : public QLabel {
public:
    explicit QG_CommandPromptLabel(QWidget* parent = nullptr) : QLabel(parent) {
        setTextFormat(Qt::PlainText);
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }
    void setPrompt(const QString& prompt) {
        m_prompt = prompt;
        setToolTip(prompt);
        setAccessibleName(prompt);
        elide();
        updateGeometry();
    }
    QSize sizeHint() const override {
        return {qMin(360, fontMetrics().horizontalAdvance(m_prompt) + 4), fontMetrics().height() + 4};
    }
    QSize minimumSizeHint() const override {return {0, fontMetrics().height() + 4};}
protected:
    void resizeEvent(QResizeEvent* event) override {
        QLabel::resizeEvent(event);
        elide();
    }
private:
    QString m_prompt;
    void elide() {setText(fontMetrics().elidedText(m_prompt, Qt::ElideMiddle, qMax(0, width() - 4)));}
};

#endif
