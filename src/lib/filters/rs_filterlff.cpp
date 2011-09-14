/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2011 Rallaz (rallazz@gmail.com)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
**
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
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


#include "rs_filterlff.h"

/*#include <iostream>
#include <fstream>*/
#include <QTextStream>
#include <QStringList>

#include "rs_font.h"
#include "rs_utility.h"
#include "rs_system.h"
#include "rs_block.h"
#include "rs_polyline.h"
#include "rs_insert.h"


/**
 * Default constructor.
 */
RS_FilterLFF::RS_FilterLFF() : RS_FilterInterface() {

    RS_DEBUG->print("Setting up LFF filter...");

    addImportFormat(RS2::FormatLFF);
    addExportFormat(RS2::FormatLFF);
}



/**
 * Implementation of the method used for RS_Import to communicate
 * with this filter.
 *
 * @param g The graphic in which the entities from the file
 * will be created or the graphics from which the entities are
 * taken to be stored in a file.
 */
bool RS_FilterLFF::fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/) {
    RS_DEBUG->print("LFF Filter: importing file '%s'...", file.toLatin1().data());

    //this->graphic = &g;
    bool success = false;

    // Load font file as we normally do, but the font doesn't own the
    //  letters (we'll add them to the graphic instead. Hence 'false').
    RS_Font font(file, false);
    success = font.loadFont();

    if (success==false) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "Cannot open LFF file '%s'.", file.toLatin1().data());
		return false;
    }

    g.addVariable("Names",
                         font.getNames().join(","), 0);
    g.addVariable("LetterSpacing", font.getLetterSpacing(), 0);
    g.addVariable("WordSpacing", font.getWordSpacing(), 0);
    g.addVariable("LineSpacingFactor", font.getLineSpacingFactor(), 0);
    g.addVariable("Authors", font.getAuthors().join(","), 0);
    g.addVariable("License", font.getFileLicense(), 0);
    if (!font.getEncoding().isEmpty()) {
        g.addVariable("Encoding", font.getEncoding(), 0);
    }

    RS_BlockList* letterList = font.getLetterList();
    for (uint i=0; i<font.countLetters(); ++i) {
        RS_Block* ch = font.letterAt(i);

        QString uCode;
        uCode.setNum(ch->getName().at(0).unicode(), 16);
        while (uCode.length()<4) {
//            uCode.rightJustified(4, '0');
            uCode="0"+uCode;
        }
        //ch->setName("[" + uCode + "] " + ch->getName());
        //letterList->rename(ch, QString("[%1]").arg(ch->getName()));
        letterList->rename(ch,
                           QString("[%1] %2").arg(uCode).arg(ch->getName().at(0)));

        g.addBlock(ch, false);
        ch->reparent(&g);
    }

    g.addBlockNotification();
	return true;
}



/**
 * Implementation of the method used for RS_Export to communicate
 * with this filter.
 *
 * @param file Full path to the LFF file that will be written.
 */
bool RS_FilterLFF::fileExport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/) {

    RS_DEBUG->print("LFF Filter: exporting file '%s'...", file.toLatin1().data());
    RS_DEBUG->print("RS_FilterLFF::fileExport: open");

    QFile f(file);
    QTextStream ts(&f);
    ts.setCodec("UTF-8");
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {


        RS_DEBUG->print("RS_FilterLFF::fileExport: open: OK");

        RS_DEBUG->print("RS_FilterLFF::fileExport: header");

        // header:
        ts << "# Format:            LibreCAD Font 1\n";
        ts << QString("# Creator:           %1\n").arg(RS_SYSTEM->getAppName());
        ts << QString("# Version:           %1\n").arg(RS_SYSTEM->getAppVersion());

        QString ns = g.getVariableString("Names", "");
        if (!ns.isEmpty()) {
            QStringList names = ns.split(',');
            RS_DEBUG->print("002");
            for (int i = 0; i < names.size(); ++i) {
                ts << QString("# Name:              %1\n").arg(names.at(i));
            }
        }

        QString es = g.getVariableString("Encoding", "");
        ts << QString("# Encoding:          UTF-8\n");
        ts << QString("# LetterSpacing:     %1\n").arg(
                  g.getVariableDouble("LetterSpacing", 3.0));
        ts << QString("# WordSpacing:       %1\n").arg(
                  g.getVariableDouble("WordSpacing", 6.75));
        ts << QString("# LineSpacingFactor: %1\n").arg(
                  g.getVariableDouble("LineSpacingFactor", 1.0));

        QString sa = g.getVariableString("Authors", "");
        RS_DEBUG->print("authors: %s", sa.toLocal8Bit().data());
        if (!sa.isEmpty()) {
            QStringList authors = sa.split(',');
            RS_DEBUG->print("count: %d", authors.count());

            QString a;
            for (int i = 0; i < authors.size(); ++i) {
                ts << QString("# Author:            %1\n").arg(authors.at(i));
            }
        }
        es = g.getVariableString("License", "");
        if (!es.isEmpty()) {
            ts << QString("# License:           %1\n").arg(es);
        } else
            ts << "# License:           unknown\n";

        RS_DEBUG->print("RS_FilterLFF::fileExport: header: OK");

        // iterate through blocks (=letters of font)
        for (uint i=0; i<g.countBlocks(); ++i) {
            RS_Block* blk = g.blockAt(i);

            RS_DEBUG->print("block: %d", i);

            if (blk!=NULL) {
                RS_DEBUG->print("002a: %s",
                                (blk->getName().toLocal8Bit().data()));

                ts << QString("\n%1\n").arg(blk->getName());

                // iterate through entities of this letter:
                for (RS_Entity* e=blk->firstEntity(RS2::ResolveNone);
                     e!=NULL;
                     e=blk->nextEntity(RS2::ResolveNone)) {

                    if (!e->isUndone()) {

                        // lines:
                        if (e->rtti()==RS2::EntityLine) {
                            RS_Line* l = (RS_Line*)e;
                            ts << QString("%1,%2;%3,%4\n").arg(
                                     l->getStartpoint().x).arg(
                                     l->getStartpoint().y).arg(
                                     l->getEndpoint().x).arg(l->getEndpoint().y);
                        }
                        // arcs:
                        else if (e->rtti()==RS2::EntityArc) {
                            RS_Arc* a = (RS_Arc*)e;
                            ts << QString("%1,%2;%3,%4,A%5\n").arg(
                                     a->getStartpoint().x).arg(
                                     a->getStartpoint().y).arg(
                                     a->getEndpoint().x).arg(
                                     a->getEndpoint().y).arg(a->getBulge());
                        }
                        else if (e->rtti()==RS2::EntityBlock) {
                            RS_Block* b = (RS_Block*)e;
                            QString uCode;
                            uCode.setNum(b->getName().at(0).unicode(), 16);
                            if (uCode.length()<4) {
                                uCode = uCode.rightJustified(4, '0');
                            }
                            ts << QString("C%1\n").arg(uCode);
                        }
                        else if (e->rtti()==RS2::EntityPolyline) {
                            RS_Polyline* p = (RS_Polyline*)e;
                            ts << QString::number(p->getStartpoint().x) <<',';
                            ts << QString::number(p->getStartpoint().y);
                            for (RS_Entity* e2=p->firstEntity(RS2::ResolveNone);
                                 e2!=NULL;
                                 e2=p->nextEntity(RS2::ResolveNone)) {
                                if (e2->rtti()==RS2::EntityLine){
                                    RS_Line* l = (RS_Line*)e2;
                                    ts << ';' << QString::number(l->getEndpoint().x) <<',';
                                    ts << QString::number(l->getEndpoint().y);
                                } else if (e2->rtti()==RS2::EntityArc){
                                    RS_Arc* a = (RS_Arc*)e2;
                                    ts << ';' << QString::number(a->getEndpoint().x) <<',';
                                    ts << QString::number(a->getEndpoint().y) <<',';
                                    ts << 'A' << QString::number(a->getBulge());
                                }
                            }
                            ts<<'\n';
                        }
                        // Ignore entities other than arcs / lines
                        else {}
                    }

                }
            }
        }
        f.close();
        RS_DEBUG->print("LFF Filter: exporting file: OK");
        return true;
    }
    else {
        RS_DEBUG->print("LFF Filter: exporting file failed");
    }

    return false;
}



/**
 * Streams a double value to the gien stream cutting away trailing 0's.
 *
 * @param value A double value. e.g. 2.700000
 */
void RS_FilterLFF::stream(std::ofstream& fs, double value) {
    fs << RS_Utility::doubleToString(value).toLatin1().data();
}

