#include "qg_colorwell.h"

#include "qmenu.h"

#include "qevent.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qpainter.h"
#include <QPainterPath>

QG_ColorPromptButton::QG_ColorPromptButton(int color, QWidget *parent)
    : QAbstractButton(parent)
    , m_color(color)
    , m_initColor(color)
{
    initScale = 16;
    prevScale = 100 - initScale;
    marginFill = 2;
}

void QG_ColorPromptButton::mousePressEvent(QMouseEvent *e)
{
    // The current cell marker is set to the cell the mouse is pressed in
    QPoint pos = e->position().toPoint();

    QRect initialRect = rect().adjusted(lineWidth, lineWidth, -lineWidth, -lineWidth);
    initialRect = QRect(initialRect.x() + initialRect.width()/2,
                  initialRect.y(),
                  initialRect.width()/2,
                  initialRect.height()/2)
                  .adjusted(lineWidth, lineWidth, 0, 0)
                  .adjusted(marginFill, marginFill, -marginFill, -marginFill);

    QRect previewRect = rect().adjusted(lineWidth, lineWidth, -lineWidth, -lineWidth);
    previewRect = QRect(previewRect.x(),
                    previewRect.height() - previewRect.height() * prevScale/100,
                    previewRect.width() * prevScale/100,
                    previewRect.height() * prevScale/100)
                    .adjusted(lineWidth, lineWidth, 0, 0)
                    .adjusted(marginFill, marginFill, -marginFill, -marginFill);

    if (initialRect.contains(pos.x(), pos.y()) &&
            !previewRect.contains(pos.x(), pos.y()) &&
            m_color != m_initColor)
    {
        emit colorChanged(m_initColor);
    }
}

void QG_ColorPromptButton::paintEvent(QPaintEvent *e)
{
    const QPalette & g = palette();
    QStyleOptionFrame opt;
    opt.initFrom(this);
    lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, this);
    opt.lineWidth = lineWidth;
    opt.midLineWidth = lineWidth;

    qDebug() << "[lineWidth" << lineWidth;

    const QRect r = e->rect().adjusted(lineWidth, lineWidth, -lineWidth, -lineWidth);
    int x = r.x();
    int y = r.y();
    int h = r.height();
    int w = r.width();

    QRect initialRect = QRect(x + w/2, y, w/2, h/2);
    QRect previewRect = QRect(x,
                     y + h * initScale/100,
                     w * prevScale/100,
                     h * prevScale/100);

    opt.rect = previewRect;
    opt.palette = g;
    opt.state = QStyle::State_Enabled | QStyle::State_Sunken;

    QPen pen;  // creates a default pen
    pen.setStyle(Qt::SolidLine);
    pen.setWidth(lineWidth);
    pen.setBrush(Qt::lightGray);

    QPainter p(this);
    p.setPen(pen);

    style()->drawPrimitive(QStyle::PE_Frame, &opt, &p, this);
    opt.rect = initialRect;
    style()->drawPrimitive(QStyle::PE_Frame, &opt, &p, this);

    initialRect.adjust(lineWidth, lineWidth, -lineWidth, -lineWidth);
    previewRect.adjust(lineWidth, lineWidth, -lineWidth, -lineWidth);

    p.fillRect(initialRect, QBrush(this->palette().button().color()));
    p.fillRect(previewRect, QBrush(this->palette().button().color()));

    opt.rect = initialRect.adjusted(marginFill, marginFill, -marginFill, -marginFill);
    p.fillRect(opt.rect, RS_DXFColor::toQColor(m_initColor));

    if (m_initColor == 7)
    {
        style()->drawPrimitive(QStyle::PE_Frame, &opt, &p, this);
    }

    opt.rect = previewRect.adjusted(marginFill, marginFill, -marginFill, -marginFill);
    p.fillRect(opt.rect, RS_DXFColor::toQColor(m_color));

    if (m_color == 7)
    {
        style()->drawPrimitive(QStyle::PE_Frame, &opt, &p, this);
        p.fillRect(opt.rect.adjusted(x + w/2, 0, 0, -h/2), Qt::white);
    }
}

void QG_WellArray::paintEvent(QPaintEvent *e)
{
    QRect r = e->rect();
    int cx = r.x();
    int cy = r.y();
    int ch = r.height();
    int cw = r.width();

    int colfirst = columnAt(cy);
    int collast = columnAt(cy + ch);
    int rowfirst = rowAt(cx);
    int rowlast = rowAt(cx + cw);

    if (isRightToLeft()) {
        int t = rowfirst;
        rowfirst = rowlast;
        rowlast = t;
    }

    QPainter painter(this);
    QPainter *p = &painter;
    QRect rect(0, 0, cellWidth(), cellHeight());

    // debug bg
    //p->fillRect(e->rect(), Qt::black);

    if (collast < 0 || collast >= ncols)
        collast = ncols -1;
    if (rowlast < 0 || rowlast >= nrows)
        rowlast = nrows -1;
    // Go through the rows

    for (int c = colfirst; c <= collast; ++c) {
        // get row position and height
        int colp = columnY(c);

        // Go through the columns in the row r
        // if we know from where to where, go through [colfirst, collast],
        // else go through all of them
        for (int r = rowfirst; r <= rowlast; ++r) {
            // get position and width of column c
            int rowp = rowX(r);

            // Translate painter and draw the cell
            rect.translate(rowp, colp);
            paintCell(p, r, c, rect);
            rect.translate(-rowp, -colp);
        }
    }
}

QG_WellArray::QG_WellArray(int rows, int cols, QWidget *parent)
    : QWidget(parent)
    , nrows(rows)
    , ncols(cols)
{
    setFocusPolicy(Qt::StrongFocus);
    cellw = 28;
    cellh = 24;
    curCol = 0;
    curRow = 0;
    selCol = -1;
    selRow = -1;
}

QSize QG_WellArray::sizeHint() const
{
    ensurePolished();
    return gridSize().boundedTo(QSize(720, 480));
}

void QG_WellArray::paintCell(QPainter* p, int row, int col, const QRect &rect)
{
    int b = 3; //margin

    const QPalette & g = palette();
    QStyleOptionFrame opt;
    opt.initFrom(this);
    int dfw = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, this);
    opt.lineWidth = dfw;
    opt.midLineWidth = 1;
    opt.rect = rect.adjusted(b, b, -b, -b);
    opt.palette = g;
    opt.state = QStyle::State_Enabled | QStyle::State_Sunken;
    style()->drawPrimitive(QStyle::PE_Frame, &opt, p, this);

    if ((row == curRow) && (col == curCol)) {
        if (hasFocus()) {
            QStyleOptionFocusRect opt;
            opt.palette = g;
            opt.rect = rect;
            opt.state = QStyle::State_None | QStyle::State_KeyboardFocusChange;
            style()->drawPrimitive(QStyle::PE_FrameFocusRect, &opt, p, this);
        }
    }
    paintCellContents(p, row, col, opt.rect.adjusted(dfw, dfw, -dfw, -dfw));
}

/*
  Reimplement this function to change the contents of the well array.
 */
void QG_WellArray::paintCellContents(QPainter *p, int row, int col, const QRect &r)
{
    Q_UNUSED(row);
    Q_UNUSED(col);
    p->fillRect(r, Qt::white);
    p->setPen(Qt::black);
    p->drawLine(r.topLeft(), r.bottomRight());
    p->drawLine(r.topRight(), r.bottomLeft());
}

void QG_WellArray::mousePressEvent(QMouseEvent *e)
{
    // The current cell marker is set to the cell the mouse is pressed in
    QPoint pos = e->position().toPoint();
    setCurrent(rowAt(pos.x()), columnAt(pos.y()));
}

void QG_WellArray::mouseReleaseEvent(QMouseEvent * /* event */)
{
    // The current cell marker is set to the cell the mouse is clicked in
    setSelected(curRow, curCol);
}

/*
  Sets the cell currently having the focus. This is not necessarily
  the same as the currently selected cell.
*/
void QG_WellArray::setCurrent(int row, int col)
{
    qDebug() << "[QG_WellArray::setCurrent] row" << row << "col" << col;
    if ((curRow == row) && (curCol == col))
    {
        return;
    }

    if (row < 0 || col < 0)
        row = col = -1;

    int oldRow = curRow;
    int oldCol = curCol;

    curRow = row;
    curCol = col;

    updateCell(oldRow, oldCol);
    updateCell(curRow, curCol);

    emit currentChanged(curRow, curCol);
}

/*
  Sets the currently selected cell to \a row, \a column. If \a row or
  \a column are less than zero, the current cell is unselected.

  Does not set the position of the focus indicator.
*/
void QG_WellArray::setSelected(int row, int col)
{
    int oldRow = selRow;
    int oldCol = selCol;

    if (row < 0 || col < 0)
        row = col = -1;

    selCol = col;
    selRow = row;

    updateCell(oldRow, oldCol);
    updateCell(selRow, selCol);
    if (row >= 0)
    {
        emit selected(row, col);
    }
}

void QG_WellArray::focusInEvent(QFocusEvent*)
{
    updateCell(curRow, curCol);
    emit currentChanged(curRow, curCol);
}

void QG_WellArray::focusOutEvent(QFocusEvent*)
{
    updateCell(curRow, curCol);
}

void QG_WellArray::keyPressEvent(QKeyEvent* e)
{
    switch(e->key()) {
    case Qt::Key_Left:
        if (curRow > 0)
            setCurrent(curRow - 1, curCol);
        break;
    case Qt::Key_Right:
        if (curRow < numRows()-1)
            setCurrent(curRow + 1, curCol);
        break;
    case Qt::Key_Up:
        if (curCol > 0)
            setCurrent(curRow, curCol - 1);
        break;
    case Qt::Key_Down:
        if (curCol < numCols()-1)
            setCurrent(curRow, curCol + 1);
        break;
    case Qt::Key_Space:
        setSelected(curRow, curCol);
        break;
    default:
        e->ignore();
        return;
    }
}

QColorWell::QColorWell(QWidget *parent, QColorWellOptions options)
    :QG_WellArray(initRow(options), initCol(options), parent)
    , m_options(options)
    , mousePressed(false)
    , oldCurrent(-1, -1)
{
    init();
}

int QColorWell::initRow(QColorWellOptions options)
{
    int totalRow;

    switch (options) {
    case Name:
        totalRow = MAX_NAME_COLOR;
        break;
    case Shade:
        totalRow = MAX_SHADE_COLOR;
        break;
    default:
        totalRow = 24;
    }

    return totalRow;
}

int QColorWell::initCol(QColorWellOptions options)
{
    return options > Shade ? 5 : 1;
}

void QColorWell::init()
{
    setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

    switch (m_options) {
    case Name:
    {
        palette = RS_DXFColor::namePalette();
        setCellWidth(24);
    }
        break;
    case Shade:
    {
        palette = RS_DXFColor::shadePalette();
        setCellWidth(24);
    }
        break;
    case Solid:
        palette = RS_DXFColor::solidPalette();
        break;
    case Pastel:
        palette = RS_DXFColor::pastelPalette();
        break;
    default:
        palette = RS_DXFColor::namePalette();
    }
}

void QColorWell::paintCellContents(QPainter *p, int row, int col, const QRect &r)
{
    int i = row + col * numRows();
    p->fillRect(r, RS_DXFColor::toQColor(palette[i]));
}

void QColorWell::mousePressEvent(QMouseEvent *e)
{
#if 0
    oldCurrent = QPoint(selectedRow(), selectedColumn());
    QG_WellArray::mousePressEvent(e);
    mousePressed = true;
    pressPos = e->position().toPoint();
#else
    QG_WellArray::mousePressEvent(e);
#endif
}

void QColorWell::mouseMoveEvent(QMouseEvent *e)
{
    QG_WellArray::mouseMoveEvent(e);
}

void QColorWell::mouseReleaseEvent(QMouseEvent *e)
{
#if 0
    if (!mousePressed)
        return;
    QG_WellArray::mouseReleaseEvent(e);
    mousePressed = false;
#else
    QG_WellArray::mouseReleaseEvent(e);
#endif
}

void QColorWell::setCurrent(int row, int col)
{
    QG_WellArray::setCurrent(row, col);

    if (col < numCols())
    {
        indexColor = palette[row + col * numRows()];
        emit indexColorChanged(indexColor);
    }
}

void QColorWell::setCurrentColor(int color)
{
    unsigned int pSize = 0;

    switch (m_options) {
    case Name:
        pSize = MAX_NAME_COLOR;
        break;
    case Shade:
        pSize = MAX_SHADE_COLOR;
        break;
    case Solid:
        pSize = 120;
        break;
    case Pastel:
        pSize = 120;
        break;
    default:
        pSize = MAX_NAME_COLOR;
    }

    for (unsigned int i = 0; i < pSize; i++)
    {
        if (palette[i] == color)
        {
            if(numCols() == 1)
            {
                setFocus();
                setCurrent(i, 0);
            }
            else
            {
                int row = i;
                int col = 0;

                while (row > 24)
                {
                    row -= 24;
                    col += 1;
                }

                setCurrent(row, col);
                setFocus();
            }
            break;
        }
    }
}
