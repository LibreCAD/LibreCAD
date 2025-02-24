#ifndef QG_COLORWELL_H
#define QG_COLORWELL_H

#include <iostream>

#include <QWidget>
#include <QAbstractButton>
#include <QMouseEvent>

#include "rs_dxfcolor.h"

class QG_ColorPromptButton : public QAbstractButton
{
    Q_OBJECT
public:
    QG_ColorPromptButton(int color=0, QWidget *parent=nullptr);
    ~QG_ColorPromptButton() { }

    void setColor(int color) { m_color = color; update(); }
    int getColor() { return m_color; }

signals:
    void colorChanged(int color);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    int m_color;
    int m_initColor;

    int initScale;
    int prevScale;
    int lineWidth;
    int marginFill;
};

class QG_WellArray : public QWidget
{
    Q_OBJECT

public:
    QG_WellArray(int rows, int cols, QWidget* parent=nullptr);
    ~QG_WellArray() {}
    QString cellContent(int row, int col) const;

    int selectedColumn() const { return selCol; }
    int selectedRow() const { return selRow; }

    virtual void setCurrent(int row, int col);
    virtual void setSelected(int row, int col);

    void setCellHeight(int h) { cellh = h; }
    void setCellWidth(int w) { cellw = w; }

    QSize sizeHint() const override;

    inline int cellWidth() const
        { return cellw; }

    inline int cellHeight() const
        { return cellh; }

    inline int columnAt(int y) const
        { return y / cellh; }

    inline int rowAt(int x) const
        { if (isRightToLeft()) return nrows - (x / cellw) - 1; return x / cellw; }

    inline int columnY(int column) const
    { if (isRightToLeft()) return cellh * (ncols - column - 1); return cellh * column; }

    inline int rowX(int row) const
    { return cellw * row; }

    inline int numRows() const
        { return nrows; }

    inline int numCols() const
        {return ncols; }

    inline QRect cellRect() const
        { return QRect(0, 0, cellw, cellh); }

    inline QSize gridSize() const
        { return QSize(nrows * cellw, ncols * cellh); }

    QRect cellGeometry(int row, int column)
        {
            QRect r;
            if (row >= 0 && row < nrows && column >= 0 && column < ncols)
            {
                r.setRect(rowX(row), columnY(column), cellw, cellh);
            }
            return r;
        }

    inline void updateCell(int row, int column) { update(cellGeometry(row, column)); }

signals:
    void selected(int row, int col);
    void currentChanged(int row, int col);
    void colorChanged(int index, QRgb color);

protected:
    virtual void paintCell(QPainter *, int row, int col, const QRect&);
    virtual void paintCellContents(QPainter *, int row, int col, const QRect&);

    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void focusInEvent(QFocusEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void paintEvent(QPaintEvent *) override;

private:
    Q_DISABLE_COPY(QG_WellArray)

    int nrows;
    int ncols;
    int cellw;
    int cellh;
    int curRow;
    int curCol;
    int selRow;
    int selCol;
};

class QColorWell : public QG_WellArray
{
    Q_OBJECT
public:
    enum QColorWellOptions {
          Name   = 0x00000001,
          Shade  = 0x00000002,
          Solid  = 0x00000004,
          Pastel = 0x00000008,
         };

    QColorWell(QWidget *parent, QColorWellOptions options);

    void setCurrent(int row, int col) override;
    void setCurrentColor(int color);
    int getColor() { return indexColor; }

signals:
    void indexColorChanged(int color);

protected:
    void paintCellContents(QPainter *, int row, int col, const QRect&) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QColorWellOptions m_options;
    const int *palette;
    bool mousePressed;
    QPoint pressPos;
    QPoint oldCurrent;
    int indexColor = 0;

    void init();
    int initRow(QColorWellOptions options);
    int initCol(QColorWellOptions options);
};

#endif // QG_COLORWELL_H

