/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_PolylineOptions::destroy() {
    RS_SETTINGS->beginGroup("/Draw");
    RS_SETTINGS->writeEntry("/PolylineMode",cbMode->currentItem());
    RS_SETTINGS->writeEntry("/PolylineRadius", leRadius->text());    
    RS_SETTINGS->writeEntry("/PolylineAngle", leAngle->text()); 
    RS_SETTINGS->writeEntry("/PolylineReversed", (int)rbNeg->isChecked());
    RS_SETTINGS->endGroup();
}

void QG_PolylineOptions::setAction(RS_ActionInterface* a, bool update) {
    if (a!=NULL && a->rtti()==RS2::ActionDrawPolyline) {
        action = (RS_ActionDrawPolyline*)a;
	
        QString sd1,sd2;
	int mode;
	bool reversed;

        if (update) {
            sd1 = QString("%1").arg(action->getRadius());
            sd2 = QString("%1").arg(action->getAngle());
            mode = action->getMode();
        } else {
            RS_SETTINGS->beginGroup("/Draw");
            sd1 = RS_SETTINGS->readEntry("/PolylineRadius", "1.0");
            sd2 = RS_SETTINGS->readEntry("/PolylineAngle", "1.0");
            mode = RS_SETTINGS->readNumEntry("/PolylineMode",0);
	    reversed = RS_SETTINGS->readNumEntry("/PolylineReversed", 0);
            RS_SETTINGS->endGroup();
            action->setRadius(sd1.toDouble());
	    action->setAngle(sd2.toDouble());
	    action->setMode(mode);
	    action->setReversed(reversed);
        }
        leRadius->setText(sd1);
	leAngle->setText(sd2);
	cbMode->setCurrentItem(mode);
	rbNeg->setChecked(reversed);
	updateMode(mode);
    } else {
        std::cerr << "QG_PolylineOptions::setAction: wrong action type\n";
        action = NULL;
    }
}

void QG_PolylineOptions::close() {
    if (action!=NULL) {
        action->close();
    }
}

void QG_PolylineOptions::undo() {
    if (action!=NULL) {
        action->undo();
    }
}

void QG_PolylineOptions::updateRadius(const QString& s) {
    if (action!=NULL) {
        action->setRadius(RS_Math::eval(s));
    }
}

void QG_PolylineOptions::updateAngle(const QString& s) {
    if (action!=NULL) {
	double a=RS_Math::eval(s);
//	QString sr;
	if (a>359.999) {
	    a=359.999;
	    leAngle->setText(QString("%1").arg(a));
	}
	else if (a<0.0) {
	    a=0.0;
            leAngle->setText(QString("%1").arg(a));
	}    
	action->setAngle(a);
    }
}

void QG_PolylineOptions::updateDirection(bool /*pos*/) {
    if (action!=NULL) {
        action->setReversed(!(rbPos->isChecked()));
    }
}

void QG_PolylineOptions::updateMode( int m )
{
    enum Mode {
	Line,
	Tangential,
	TanRad,
//	TanAng,
//	TanRadAng,
	Ang,
//	RadAngEndp,
//	RadAngCenp
    };
    
    if (action!=NULL) {
        action->setMode(m);
    }
    switch(m) {
        case Line:
	case Tangential:
	    leRadius->setDisabled(true);
	    leAngle->setDisabled(true);
	    lRadius->setDisabled(true);
	    lAngle->setDisabled(true);
	    buttonGroup1->setDisabled(true);
	    break;
        case TanRad:
            leRadius->setDisabled(false);
            leAngle->setDisabled(true);
            lRadius->setDisabled(false);
            lAngle->setDisabled(true);
	    buttonGroup1->setDisabled(true);
            break;
//        case TanAng:
        case Ang:
            leRadius->setDisabled(true);
            leAngle->setDisabled(false);
            lRadius->setDisabled(true);
            lAngle->setDisabled(false);
	    buttonGroup1->setDisabled(false);
            break;
/*        case TanRadAng:
        case RadAngEndp:
	case RadAngCenp:
            leRadius->setDisabled(false);
            leAngle->setDisabled(false);
            lRadius->setDisabled(false);
            lAngle->setDisabled(false);
	    buttonGroup1->setDisabled(false);*/
    }
}
