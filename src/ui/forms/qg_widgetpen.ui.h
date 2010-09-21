/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_WidgetPen::setPen(RS_Pen pen, bool showByLayer, 
                          bool showUnchanged, const QString& title) {
    cbColor->init(showByLayer, showUnchanged);
    cbWidth->init(showByLayer, showUnchanged);
    cbLineType->init(showByLayer, showUnchanged);
    if (!showUnchanged) {
       cbColor->setColor(pen.getColor());
       cbWidth->setWidth(pen.getWidth());
       cbLineType->setLineType(pen.getLineType());
    }

    if (!title.isEmpty()) {
        bgPen->setTitle(title);
    }
}

RS_Pen QG_WidgetPen::getPen() {
    RS_Pen pen;

    pen.setColor(cbColor->getColor());
    pen.setWidth(cbWidth->getWidth());
    pen.setLineType(cbLineType->getLineType());

    return pen;
}

bool QG_WidgetPen::isColorUnchanged() {
    return cbColor->isUnchanged();
}

bool QG_WidgetPen::isLineTypeUnchanged() {
    return cbLineType->isUnchanged();
}

bool QG_WidgetPen::isWidthUnchanged() {
    return cbWidth->isUnchanged();
}
