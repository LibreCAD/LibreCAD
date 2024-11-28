// Copyright (C) 2024 Emanuel Strobel
// GPLv2

#include "texteditor.h"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>
#include <QTextBlock>
#include <QtPrintSupport/qtprintsupportglobal.h>
#include <QPrintDialog>
#include <QPrinter>
#include <QDir>

#ifdef DEVELOPER

TextEditor::TextEditor(QWidget *parent, const QString& fileName)
    : QPlainTextEdit(parent)
    , m_lineNumberWidget(new LineNumberWidget(this))
    , m_fileName(fileName)
    , m_firstSave(false)
{
    setViewportMargins(43, 0, 0, 0);
    initHighlighter();
    highlightCurrentLine();

    connect(this, &QPlainTextEdit::updateRequest, this, &TextEditor::updateLineNumber);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &TextEditor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::blockCountChanged, this, &TextEditor::updateLineNumberMargin);

    load(m_fileName);
}

void TextEditor::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Return:
    {
        QString text = document()->toPlainText();

        if(text.length())
        {
            int pos = textCursor().position();

            QString currentLine = text.mid(text.lastIndexOf("\n", pos)+1, text.indexOf("\n", pos));

            QPlainTextEdit::keyPressEvent(event);

            if (currentLine.isEmpty())
            {
                return;
            }

            if(m_fileName.endsWith(".py") && text.at(pos-1) == ':')
            {
                insertPlainText("    ");
            }

            if(m_fileName.endsWith(".lsp") ||
                m_fileName.endsWith(".lisp") ||
                m_fileName.endsWith(".mal"))
            {
                int itto = 0;
                int ittc = 0;
                for (auto& c : currentLine)
                {
                    if (c == '(')
                    {
                        itto++;
                    }
                    if (c == ')')
                    {
                        ittc++;
                    }
                }
                itto -= ittc;
                if (itto > 0)
                {
                    for (int i = 0; i < itto; i++)
                    {
                        insertPlainText("    ");
                    }
                }
            }

            if(m_fileName.endsWith(".py") ||
                m_fileName.endsWith(".lsp") ||
                m_fileName.endsWith(".lisp") ||
                m_fileName.endsWith(".mal") ||
                m_fileName.endsWith(".dcl"))
            {
                for (auto& c : currentLine)
                {
                    if (c == ' ')
                    {
                        insertPlainText(" ");
                    }
                    else
                    {
                        break;
                    }
                }
            }
            return;
        }
        QPlainTextEdit::keyPressEvent(event);
    }
        break;
    case Qt::Key_Tab:
    {
        insertPlainText("    ");
        return;
    }
        break;
    default:
        QPlainTextEdit::keyPressEvent(event);
    }
}


TextEditor::~TextEditor()
{
    removeHighlighter();
}

void TextEditor::lineNumberPaintEvent(QPaintEvent *e)
{
    QTextBlock block = firstVisibleBlock();
    QPainter painter(m_lineNumberWidget);
    painter.fillRect(e->rect(), QColor(200, 200, 200, 100));
    painter.setPen(QColor(80, 80, 80));

    int top    = blockBoundingGeometry(block).translated(contentOffset()).top() + 1;
    int bottom = top + blockBoundingGeometry(block).height();

    while (block.isValid() && top <= e->rect().bottom())
    {
        int lineNumber = block.blockNumber();
        int lineHeight = blockBoundingGeometry(block).height();
        if (!block.next().isValid())
        {
            lineHeight -= 4;
        }
        QRect rect(0, top, getLineNumberWidth() - 2, lineHeight);
        QFont font = painter.font();
        font.setPointSize(9);
        painter.setFont(font);
        painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, QString::number(lineNumber + 1));

        block  = block.next();
        top    = bottom;
        bottom = top + blockBoundingGeometry(block).height();
    }
}

void TextEditor::load(QString fileName)
{
    if(fileName == "*librepad") {
        m_fileName = tr("unnamed.txt");
        setFont(QFont("Monospace", 10));
        setPlainText("");
        document()->setModified(false);
        emit documentChanged();
        return;
    }

    if(fileName == "*librelisp") {
        m_fileName = tr("unnamed.lsp");
        setFont(QFont("Monospace", 10));
        setPlainText("");
        initHighlighter();
        document()->setModified(false);
        emit documentChanged();
        return;
    }

    if(fileName == "*librepython") {
        m_fileName = tr("unnamed.py");
        setFont(QFont("Monospace", 10));
        setPlainText("");
        initHighlighter();
        document()->setModified(false);
        emit documentChanged();
        return;
    }

    QFile file(fileName);
    if (!file.exists())
    {
        QMessageBox::warning(this, tr("Warning"), tr("file not found: ") + file.errorString());
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        QMessageBox::critical(this, tr("Critical"), tr("Cannot read file: ") + file.errorString());
        return;
    }

    QString text = file.readAll();
    file.close();
    setPlainText(text);
    setFirstSave(true);
    document()->setModified(false);
    emit documentChanged();
}

void TextEditor::save()
{
    if (!firstSave()) {
        saveAs();
        return;
    }

    QFile file(m_fileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        QMessageBox::critical(this, tr("Critical"), tr("Cannot write file: ") + file.errorString());
        return;
    }
    else {
        if (!toPlainText().endsWith("\n")) {
            appendPlainText("\n");
        }
        file.write(toPlainText().toUtf8());
        file.close();
        document()->setModified(false);
        emit documentChanged();
    }
}

void TextEditor::saveAs()
{
    if (!toPlainText().endsWith("\n")) {
        appendPlainText("\n");
    }
    saveFileContent(toPlainText().toUtf8(), fileName());
}

void TextEditor::saveFileContent(const QByteArray &fileContent, const QString &fileNameHint)
{
    QFileDialog *dialog = new QFileDialog();
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    dialog->setFileMode(QFileDialog::AnyFile);
    dialog->selectFile(fileNameHint);
    auto fileSelected = [=](const QString &fileName) {
        if (!fileName.isNull()) {
            QFile selectedFile(fileName);
            if (selectedFile.open(QIODevice::WriteOnly)) {
                selectedFile.write(fileContent);
                selectedFile.close();
                setFirstSave(true);
                m_fileName = fileName;
                reload();
            }
            else {
                QMessageBox::critical(this, tr("Critical"), tr("Cannot write file: ") + selectedFile.errorString());
            }
        }
    };
    auto dialogClosed = [=](int code) {
        Q_UNUSED(code);
        delete dialog;
    };
    connect(dialog, &QFileDialog::fileSelected, fileSelected);
    connect(dialog, &QFileDialog::finished, dialogClosed);
    dialog->show();
}

void TextEditor::reload()
{
    if (!firstSave()) {
        saveAs();
    }
    QFile file(m_fileName);
    file.open(QIODevice::ReadOnly | QFile::Text);
    QString text = file.readAll();
    file.close();
    removeHighlighter();
    initHighlighter();
    setPlainText(text);
    document()->setModified(false);
    emit documentChanged();
}

void TextEditor::printer()
{
    if (m_fileName.isEmpty())
    {
        return;
    }
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Rejected) {
        return;
    }
    print(&printer);
}

void TextEditor::updateLineNumber(const QRect &rect, int dy)
{
    if (dy > 0)
    {
        m_lineNumberWidget->scroll(0, dy);
    }
    m_lineNumberWidget->update(0, rect.y(), getLineNumberWidth(), rect.height());
}

void TextEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    m_lineNumberWidget->setGeometry(0, 0, getLineNumberWidth(), contentsRect().height());
}

void TextEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor(248, 247, 246));
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();

    extraSelections.append(selection);

    setExtraSelections(extraSelections);
}

void TextEditor::updateLineNumberMargin()
{
    setViewportMargins(getLineNumberWidth(), 0, 0, 0);
}

int TextEditor::getLineNumberWidth()
{
    //int default = 22;
    int defalut = 40;
    defalut     = 4 + QString::number(blockCount()).length() * fontMetrics().horizontalAdvance('0');
    defalut     = qMax(40, defalut);

    return defalut;
}

LineNumberWidget::LineNumberWidget(TextEditor *editor)
    : QWidget(editor)
{
    m_editor = editor;
}

void TextEditor::initHighlighter()
{
    if(m_fileName.endsWith(".lsp") ||
        m_fileName.endsWith(".mal"))
    {
        m_lispHighlighter = new LispHighlighter(document());
        m_lispHighlighter->rehighlight();
    }

    if(m_fileName.endsWith(".dcl"))
    {
        m_dclHighlighter = new DclHighlighter(document());
        m_dclHighlighter->rehighlight();
    }

    if(m_fileName.endsWith(".py"))
    {
        m_pythonHighlighter = new PythonHighlighter(document());
        m_pythonHighlighter->rehighlight();
    }
}

void TextEditor::removeHighlighter()
{
    if(m_lispHighlighter != nullptr)
    {
        delete m_lispHighlighter;
        m_lispHighlighter = nullptr;
    }

    if(m_dclHighlighter != nullptr)
    {
        delete m_dclHighlighter;
        m_dclHighlighter = nullptr;
    }

    if(m_pythonHighlighter != nullptr)
    {
        delete m_pythonHighlighter;
        m_pythonHighlighter = nullptr;
    }
}

void LineNumberWidget::paintEvent(QPaintEvent *event)
{
    m_editor->lineNumberPaintEvent(event);
}

#endif // DEVELOPER
