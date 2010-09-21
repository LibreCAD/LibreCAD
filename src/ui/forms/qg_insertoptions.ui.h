/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_InsertOptions::destroy() {
    RS_SETTINGS->beginGroup("/Insert");
    RS_SETTINGS->writeEntry("/InsertAngle", leAngle->text());
    RS_SETTINGS->writeEntry("/InsertFactor", leFactor->text());
    RS_SETTINGS->writeEntry("/InsertColumns", sbColumns->text());
    RS_SETTINGS->writeEntry("/InsertRows", sbRows->text());
    RS_SETTINGS->writeEntry("/InsertColumnSpacing", leColumnSpacing->text());
    RS_SETTINGS->writeEntry("/InsertRowSpacing", leRowSpacing->text());
    RS_SETTINGS->endGroup();
}

void QG_InsertOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionBlocksInsert) {
        action = (RS_ActionBlocksInsert*)a;

        QString sAngle;
        QString sFactor;
	QString sColumns;
    	QString sRows;
        QString sColumnSpacing;
        QString sRowSpacing;
        if (update) {
            sAngle = QString("%1").arg(RS_Math::rad2deg(action->getAngle()));
            sFactor = QString("%1").arg(action->getFactor());
            sColumns = QString("%1").arg(action->getColumns());
            sRows = QString("%1").arg(action->getRows());
            sColumnSpacing = QString("%1").arg(action->getColumnSpacing());
            sRowSpacing = QString("%1").arg(action->getRowSpacing());
        } else {
            RS_SETTINGS->beginGroup("/Insert");
            sAngle = RS_SETTINGS->readEntry("/InsertAngle", "0.0");
            sFactor = RS_SETTINGS->readEntry("/InsertFactor", "1.0");
            sColumns = RS_SETTINGS->readEntry("/InsertColumns", "1");
            sRows = RS_SETTINGS->readEntry("/InsertRows", "1");
            sColumnSpacing = RS_SETTINGS->readEntry("/InsertColumnSpacing", "1.0");
            sRowSpacing = RS_SETTINGS->readEntry("/InsertRowSpacing", "1.0");
            RS_SETTINGS->endGroup();
        }
	leAngle->setText(sAngle);
	leFactor->setText(sFactor);
    	sbColumns->setValue(sColumns.toInt());
        sbRows->setValue(sRows.toInt());
        leColumnSpacing->setText(sColumnSpacing);
        leRowSpacing->setText(sRowSpacing);
    } else {
        RS_DEBUG->print(RS_Debug::D_ERROR, 
			"QG_InsertOptions::setAction: wrong action type");
        action = NULL;
    }
}

void QG_InsertOptions::updateData() {
    if (action!=NULL) {
        action->setAngle(RS_Math::deg2rad(RS_Math::eval(leAngle->text())));
        action->setFactor(RS_Math::eval(leFactor->text()));
        action->setColumns(sbColumns->value());
        action->setRows(sbRows->value());
        action->setColumnSpacing(RS_Math::eval(leColumnSpacing->text()));
        action->setRowSpacing(RS_Math::eval(leRowSpacing->text()));
    }
}
