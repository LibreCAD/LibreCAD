/******************************************************************************
**
** This file was created for the LibreCAD project, a 2D CAD program.
**
** Copyright (C) 2018 Alexander Pravdin <aledin@mail.ru>
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
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
******************************************************************************/

#include <QtCore>

#include "rs.h"
#include "rs_graphic.h"
#include "rs_painterqt.h"
#include "lc_printing.h"
#include "rs_staticgraphicview.h"

#include "pdf_print_loop.h"


static bool openDocAndSetGraphic(RS_Document**, RS_Graphic**, QString&);
static void touchGraphic(RS_Graphic*, PdfPrintParams&);
static void setupPrinterAndPaper(RS_Graphic*, QPrinter&, PdfPrintParams&);
static void drawPage(RS_Graphic*, QPrinter&, RS_PainterQt&);


void PdfPrintLoop::run()
{
    if (params.outFile.isEmpty()) {
        for (auto f : params.dxfFiles) {
            printOneDxfToOnePdf(f);
        }
    } else {
        printManyDxfToOnePdf();
    }

    emit finished();
}


void PdfPrintLoop::printOneDxfToOnePdf(QString& dxfFile) {

    // Main code logic and flow for this method is originally stolen from
    // QC_ApplicationWindow::slotFilePrint(bool printPDF) method.
    // But finally it was splitted in smaller parts.

    QFileInfo dxfFileInfo(dxfFile);
    params.outFile =
        (params.outDir.isEmpty() ? dxfFileInfo.path() : params.outDir)
        + "/" + dxfFileInfo.completeBaseName() + ".pdf";

    RS_Document *doc;
    RS_Graphic *graphic;

    if (!openDocAndSetGraphic(&doc, &graphic, dxfFile))
        return;

    qDebug() << "Printing" << dxfFile << "to" << params.outFile << ">>>>";

    touchGraphic(graphic, params);

    QPrinter printer(QPrinter::HighResolution);

    setupPrinterAndPaper(graphic, printer, params);

    RS_PainterQt painter(&printer);

    if (params.monochrome)
        painter.setDrawingMode(RS2::ModeBW);

    drawPage(graphic, printer, painter);

    painter.end();

    qDebug() << "Printing" << dxfFile << "to" << params.outFile << "DONE";

    delete doc;
}


void PdfPrintLoop::printManyDxfToOnePdf() {

    struct DxfPage {
        RS_Document* doc;
        RS_Graphic* graphic;
        QString dxfFile;
        QPrinter::PageSize paperSize;
    };

    if (!params.outDir.isEmpty()) {
        QFileInfo outFileInfo(params.outFile);
        params.outFile = params.outDir + "/" + outFileInfo.fileName();
    }

    QVector<DxfPage> pages;
    int nrPages = 0;

    // FIXME: Should probably open and print all dxf files in one 'for' loop.
    // Tried but failed to do this. It looks like some 'chicken and egg'
    // situation for the QPrinter and RS_PainterQt. Therefore, first open
    // all dxf files and apply required actions. Then run another 'for'
    // loop for actual printing.
    for (auto dxfFile : params.dxfFiles) {

        DxfPage page;

        page.dxfFile = dxfFile;

        if (!openDocAndSetGraphic(&page.doc, &page.graphic, dxfFile))
            continue;

        qDebug() << "Opened" << dxfFile;

        touchGraphic(page.graphic, params);

        pages.append(page);

        nrPages++;
    }

    QPrinter printer(QPrinter::HighResolution);

    if (nrPages > 0) {
        // FIXME: Is it possible to set up printer and paper for every
        // opened dxf file and tie them with painter? For now just using
        // data extracted from the first opened dxf file for all pages.
        setupPrinterAndPaper(pages.at(0).graphic, printer, params);
    }

    RS_PainterQt painter(&printer);

    if (params.monochrome)
        painter.setDrawingMode(RS2::ModeBW);

    // And now it's time to actually print all previously opened dxf files.
    for (auto page : pages) {
        nrPages--;

        qDebug() << "Printing" << page.dxfFile
                 << "to" << params.outFile << ">>>>";

        drawPage(page.graphic, printer, painter);

        qDebug() << "Printing" << page.dxfFile
                 << "to" << params.outFile << "DONE";

        delete page.doc;

        if (nrPages > 0)
            printer.newPage();
    }

    painter.end();
}


static bool openDocAndSetGraphic(RS_Document** doc, RS_Graphic** graphic,
    QString& dxfFile)
{
    *doc = new RS_Graphic();

    if (!(*doc)->open(dxfFile, RS2::FormatUnknown)) {
        qDebug() << "ERROR: Failed to open document" << dxfFile;
        delete *doc;
        return false;
    }

    *graphic = (*doc)->getGraphic();
    if (*graphic == nullptr) {
        qDebug() << "ERROR: No graphic in" << dxfFile;
        delete *doc;
        return false;
    }

    return true;
}


static void touchGraphic(RS_Graphic* graphic, PdfPrintParams& params)
{
    graphic->calculateBorders();
    graphic->setMargins(params.margins.left, params.margins.top,
                        params.margins.right, params.margins.bottom);
    graphic->setPagesNum(params.pagesH, params.pagesV);

    if (params.scale > 0.0)
        graphic->setPaperScale(params.scale);

    if (params.pageSize != RS_Vector(0.0, 0.0))
        graphic->setPaperSize(params.pageSize);

    if (params.fitToPage)
        graphic->fitToPage(); // fit and center
    else if (params.centerOnPage)
        graphic->centerToPage();
}


static void setupPrinterAndPaper(RS_Graphic* graphic, QPrinter& printer,
    PdfPrintParams& params)
{
    bool landscape = false;

    RS2::PaperFormat pf = graphic->getPaperFormat(&landscape);
    QPrinter::PageSize paperSize = LC_Printing::rsToQtPaperFormat(pf);

    if (paperSize == QPrinter::Custom){
        RS_Vector r = graphic->getPaperSize();
        RS_Vector&& s = RS_Units::convert(r, graphic->getUnit(),
            RS2::Millimeter);
        if (landscape)
            s = s.flipXY();
        printer.setPaperSize(QSizeF(s.x,s.y), QPrinter::Millimeter);
    } else {
        printer.setPaperSize(paperSize);
    }

    if (landscape) {
        printer.setOrientation(QPrinter::Landscape);
    } else {
        printer.setOrientation(QPrinter::Portrait);
    }

    printer.setOutputFileName(params.outFile);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setResolution(params.resolution);
    printer.setFullPage(true);

    if (params.grayscale)
        printer.setColorMode(QPrinter::GrayScale);
    else
        printer.setColorMode(QPrinter::Color);
}


static void drawPage(RS_Graphic* graphic, QPrinter& printer,
    RS_PainterQt& painter)
{
    double printerFx = (double)printer.width() / printer.widthMM();
    double printerFy = (double)printer.height() / printer.heightMM();

    double marginLeft = graphic->getMarginLeft();
    double marginTop = graphic-> getMarginTop();
    double marginRight = graphic->getMarginRight();
    double marginBottom = graphic->getMarginBottom();

    painter.setClipRect(marginLeft * printerFx, marginTop * printerFy,
                        printer.width() - (marginLeft + marginRight) * printerFx,
                        printer.height() - (marginTop + marginBottom) * printerFy);

    RS_StaticGraphicView gv(printer.width(), printer.height(), &painter);
    gv.setPrinting(true);
    gv.setBorders(0,0,0,0);

    double fx = printerFx * RS_Units::getFactorToMM(graphic->getUnit());
    double fy = printerFy * RS_Units::getFactorToMM(graphic->getUnit());

    double f = (fx + fy) / 2.0;

    double scale = graphic->getPaperScale();

    gv.setOffset((int)(graphic->getPaperInsertionBase().x * f),
                 (int)(graphic->getPaperInsertionBase().y * f));
    gv.setFactor(f*scale);
    gv.setContainer(graphic);

    double baseX = graphic->getPaperInsertionBase().x;
    double baseY = graphic->getPaperInsertionBase().y;
    int numX = graphic->getPagesNumHoriz();
    int numY = graphic->getPagesNumVert();
    RS_Vector printArea = graphic->getPrintAreaSize(false);

    for (int pY = 0; pY < numY; pY++) {
        double offsetY = printArea.y * pY;
        for (int pX = 0; pX < numX; pX++) {
            double offsetX = printArea.x * pX;
            // First page is created automatically.
            // Extra pages must be created manually.
            if (pX > 0 || pY > 0) printer.newPage();
            gv.setOffset((int)((baseX - offsetX) * f),
                         (int)((baseY - offsetY) * f));
            gv.drawEntity(&painter, graphic );
        }
    }
}
