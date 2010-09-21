/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void QG_CommandWidget::init() {
    actionHandler = NULL;
    //errStream = NULL;
    leCommand->setFrame(false);
    leCommand->setFocusPolicy(Qt::StrongFocus);
    //setNormalMode();
}

bool QG_CommandWidget::checkFocus() {
    return leCommand->hasFocus();
}

void QG_CommandWidget::setFocus() {
    //setCommandMode();
    leCommand->setFocus();
}


void QG_CommandWidget::setCommand(const QString& cmd) {
    if (cmd!="") {
        lCommand->setText(cmd);
    } else {
        lCommand->setText(tr("Command:"));
    }
    leCommand->setText("");
}


void QG_CommandWidget::appendHistory(const QString& msg) {
    teHistory->append(msg);
}


void QG_CommandWidget::trigger() {
    QString cmd = leCommand->text();

    if (cmd=="") {
        cmd="\n";
    } else {
        appendHistory(cmd);
    }

    if (actionHandler!=NULL) {
        actionHandler->command(cmd);
    }

    leCommand->setText("");
}

void QG_CommandWidget::tabPressed() {
    if (actionHandler!=NULL) {
        QStringList reducedChoice;
        QString typed = leCommand->text();
        QStringList choice;
        
        // check current command:
        choice = actionHandler->getAvailableCommands();
        if (choice.count()==0) {
            choice = RS_COMMANDS->complete(typed);
        }
        
        for (QStringList::Iterator it = choice.begin(); it != choice.end(); ++it) {
            if (typed.isEmpty() || (*it).startsWith(typed)) {
                reducedChoice << (*it);
            }
        }
        
        // command found:
        if (reducedChoice.count()==1) {
            leCommand->setText(reducedChoice.first());
        }
        else if (reducedChoice.count()>0) {
            appendHistory(reducedChoice.join(", "));
        }
    }
}

void QG_CommandWidget::escape() {
    //leCommand->clearFocus();

    if (actionHandler!=NULL) {
		actionHandler->slotFocusNormal();
	}
}

/*void QG_CommandWidget::cmdChanged(const QString& text) {
    // three equal letters enable hotkeys and move the focus away from the command line:
    if (text.length()==3) {
        if (text.at(0)==text.at(1) && text.at(0)==text.at(2)) {
            escape();
        }
    }
}*/

void QG_CommandWidget::setActionHandler(QG_ActionHandler* ah) {
    actionHandler = ah;
}

void QG_CommandWidget::setCommandMode() {
    lCommand->setPaletteForegroundColor(Qt::blue);
}

void QG_CommandWidget::setNormalMode() {
    lCommand->setPaletteForegroundColor(Qt::black);
}

void QG_CommandWidget::redirectStderr() {
    //fclose(stderr);
    //ferr = new QFile();
    //ferr->open(IO_ReadWrite, stderr);
    //std::streambuf buf;
    //errStream = new std::ostream(&errBuf);
    //std::cerr.rdbuf(errStream->rdbuf());
}

void QG_CommandWidget::processStderr() {
	/*
    if (errStream==NULL) {
        return;
    }
    
    std::string s = errBuf.str();
    if (s.length()!=0) {
        appendHistory(QString("%1").arg(s.c_str()));
    }
    //char c;
    / *while ((c=ferr->getch())!=-1) {
        appendHistory(QString("%1").arg(c));
    }
    ferr->close();* /
	*/
}
