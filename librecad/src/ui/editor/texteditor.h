// Copyright (C) 2024 Emanuel Strobel
// GPLv2

#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include <QPlainTextEdit>
#include <QFileInfo>
#include "dclhighlighter.h"
#include "lisphighlighter.h"
#include "pythonhighlighter.h"

#ifdef DEVELOPER

class LineNumberWidget;
class TextEditor : public QPlainTextEdit
{
    Q_OBJECT
public:
    TextEditor(QWidget *parent, const QString& fileName);
    ~TextEditor();

    void lineNumberPaintEvent(QPaintEvent *e);

    void load(QString fileName);
    void reload();
    void save();
    void saveAs();
    void printer();
    bool firstSave() const { return m_firstSave; }

    QString path() const { return m_fileName; }

    QString fileName() const
    {
        QFileInfo info(m_fileName);
        return info.fileName();
    }

signals:
    void documentChanged();

public slots:
    void updateLineNumber(const QRect &rect, int dy);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void highlightCurrentLine();
    void updateLineNumberMargin();
    int getLineNumberWidth();

private:
    LineNumberWidget *m_lineNumberWidget;
    QString m_fileName;
    bool m_firstSave;

    DclHighlighter *m_dclHighlighter = nullptr;
    LispHighlighter *m_lispHighlighter = nullptr;
    PythonHighlighter *m_pythonHighlighter = nullptr;

    void initHighlighter();
    void removeHighlighter();

    void setFirstSave(bool state) { m_firstSave = state; }
    void saveFileContent(const QByteArray &fileContent, const QString &fileNameHint);
};

class LineNumberWidget : public QWidget
{
public:
    LineNumberWidget(TextEditor *editor);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    TextEditor *m_editor;
};

#endif // DEVELOPER

#endif   // TEXTEDITOR_H
