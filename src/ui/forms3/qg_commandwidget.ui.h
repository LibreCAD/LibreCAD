/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

void QG_CommandWidget::init() {
    actionHandler = NULL;
    //errStream = NULL;
    leCommand->setFrame(false);	
    // RVT_PORT leCommand->setFocusPolicy(QWidget::StrongFocus);
    leCommand->setFocusPolicy(Qt::StrongFocus);
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
    // RVT_PORT lCommand->setPaletteForegroundColor(Qt::blue);
    lCommand->setPaletteForegroundColor(Qt::blue);
}

void QG_CommandWidget::setNormalMode() {
    // RVT_PORT lCommand->setPaletteForegroundColor(Qt::black);
    lCommand->setPaletteForegroundColor(RS_Color(0,0,0));
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
