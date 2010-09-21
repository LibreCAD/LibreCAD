/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DlgOptionsVariables::init() {
    graphic = NULL;

    // variables
    tabVariables->verticalHeader()->hide();
    tabVariables->verticalHeader()->setFixedWidth(0);
    tabVariables->setColumnReadOnly(0, true);
    tabVariables->setColumnReadOnly(1, true);
    tabVariables->setColumnReadOnly(2, true);
}


/**
 * Sets the graphic and updates the GUI to match the drawing.
 */
void QG_DlgOptionsVariables::setGraphic(RS_Graphic* g) {
    graphic = g;
    updateVariables();
}


/**
 * Updates the Variables tab from the graphic values.
 */
void QG_DlgOptionsVariables::updateVariables() {
    if (graphic==NULL) {
        return;
    }
    
    QVector<int> r(tabVariables->numRows());
    for (int i=0; i<tabVariables->numRows(); ++i) {
        r[i] = i;
    }
    tabVariables->removeRows(r);
    RS_DictIterator<RS_Variable> it(graphic->getVariableDict());
    for (; it.current(); ++it) {
        tabVariables->insertRows(tabVariables->numRows(), 1);
        
        tabVariables->setText(tabVariables->numRows()-1, 0, it.currentKey());
        tabVariables->setText(tabVariables->numRows()-1, 1, QString("%1").arg(it.current()->getCode()));
        QString str = "";
        switch (it.current()->getType()) {
            case RS2::VariableVoid:
                break;
            case RS2::VariableInt:
                str = QString("%1").arg(it.current()->getInt());
                break;
            case RS2::VariableDouble:
                str = QString("%1").arg(it.current()->getDouble());
                break;
            case RS2::VariableString:
                str = QString("%1").arg(it.current()->getString());
                break;
            case RS2::VariableVector:
                str = QString("%1/%2")
                      .arg(it.current()->getVector().x)
                      .arg(it.current()->getVector().y);
                if (RS_FilterDXF::isVariableTwoDimensional(it.currentKey())==false) {
                    str+= QString("/%1").arg(it.current()->getVector().z);
                }
                break;
        }
        tabVariables->setText(tabVariables->numRows()-1, 2, str);
    }
}
