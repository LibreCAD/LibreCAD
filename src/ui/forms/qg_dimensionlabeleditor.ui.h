/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_DimensionLabelEditor::setLabel(const QString& l) {
    int i0, i1a, i1b, i2;
    QString label, tol1, tol2;
    bool hasDiameter = false;
    
    label = l;
    
    if (label.at(0)==QChar(0x2205) || label.at(0)==QChar(0xF8)) {
        hasDiameter = true;
        bDiameter->setOn(true);
    }
    
    i0 = l.find("\\S");
    if (i0>=0) {
        i1a = l.find("^ ", i0);
        i1b = i1a+1;
        if (i1a<0) {
            i1a = i1b = l.find('^', i0);
        }
        if (i1a>=0) {
            i2 = l.find(';', i1b);
            label = l.mid(0, i0);
            tol1 = l.mid(i0+2, i1a-i0-2);
            tol2 = l.mid(i1b+1, i2-i1b-1);
        }
    }
    
    leLabel->setText(label.mid(hasDiameter));
    leTol1->setText(tol1);
    leTol2->setText(tol2);
}

QString QG_DimensionLabelEditor::getLabel() {
    QString l = leLabel->text();
    
    // diameter:
    if (bDiameter->isOn()) {
        if (l.isEmpty()) {
            l = QString("%1<>").arg(QChar(0x2205));
        }
        else {
            l = QChar(0x2205) + l;
        }
    }
    
    if (leTol1->text().isEmpty() && leTol2->text().isEmpty()) {
        return l;
    }
    else {
        return l + "\\S" + leTol1->text() + 
            "^ " + leTol2->text() + ";";
    }
}

void QG_DimensionLabelEditor::insertSign(const QString& s) {
    leLabel->insert(s.left(1));
}
