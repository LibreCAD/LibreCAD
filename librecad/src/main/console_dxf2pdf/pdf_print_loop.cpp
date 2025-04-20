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
#include "rs_painter.h"
#include "lc_printing.h"
#include "rs_units.h"
#include "lc_graphicviewport.h"
#include "lc_printviewportrenderer.h"
#include "pdf_print_loop.h"

#include "lc_documentsstorage.h"
static bool openDocAndSetGraphic(RS_Document**, RS_Graphic**, const QString&);
static void touchGraphic(RS_Graphic*, PdfPrintParams&);
static void setupPrinterAndPaper(RS_Graphic*, QPrinter&, PdfPrintParams&);
static void drawGraphic(RS_Graphic *graphic, QPrinter &printer, RS_Painter &painter);

void PdfPrintLoop::run(){
    if (params.outFile.isEmpty()) {
        for (auto &&f : params.dxfFiles) {
            printOneDxfToOnePdf(f);
        }
    } else {
        printManyDxfToOnePdf();
    }
    emit finished();
}


void PdfPrintLoop::printOneDxfToOnePdf(const QString& dxfFile) {

    // Main code logic and flow for this method is originally stolen from
    // QC_ApplicationWindow::slotFilePrint(bool printPDF) method.
    // But finally it was split in to smaller parts.

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

    RS_Painter painter(&printer);

    if (params.monochrome)
        painter.setDrawingMode(RS2::ModeBW);

    drawGraphic(graphic, printer, painter);

    painter.end();

    qDebug() << "Printing" << dxfFile << "to" << params.outFile << "DONE";

    delete doc;
}


void PdfPrintLoop::printManyDxfToOnePdf() {
    struct DxfContentItems {
        RS_Document* doc;
        RS_Graphic* graphic;
        QString dxfFile;
        QPageSize::PageSizeId paperSize;
    };

    if (!params.outDir.isEmpty()) {
        QFileInfo outFileInfo(params.outFile);
        params.outFile = params.outDir + "/" + outFileInfo.fileName();
    }

    QVector<DxfContentItems> contentItems;
    int nrPages = 0;

    // FIXME: Should probably open and print all dxf files in one 'for' loop.
    // Tried but failed to do this. It looks like some 'chicken and egg'
    // situation for the QPrinter and RS_PainterQt. Therefore, first open
    // all dxf files and apply required actions. Then run another 'for'
    // loop for actual printing.
    for (auto dxfFile : params.dxfFiles) {
        DxfContentItems page;
        page.dxfFile = dxfFile;
        if (!openDocAndSetGraphic(&page.doc, &page.graphic, dxfFile))
            continue;

        qDebug() << "Opened" << dxfFile;

        touchGraphic(page.graphic, params);
        contentItems.append(page);
        nrPages++;
    }

    QPrinter printer(QPrinter::HighResolution);

    if (nrPages > 0) {
        // FIXME: Is it possible to set up printer and paper for every
        // opened dxf file and tie them with painter? For now just using
        // data extracted from the first opened dxf file for all pages.
        setupPrinterAndPaper(contentItems.at(0).graphic, printer, params);
    }

    RS_Painter painter(&printer);

    if (params.monochrome)
        painter.setDrawingMode(RS2::ModeBW);

    // And now it's time to actually print all previously opened dxf files.
    for (auto item : contentItems) {
        nrPages--;

        qDebug() << "Printing" << item.dxfFile
                 << "to" << params.outFile << ">>>>";

        drawGraphic(item.graphic, printer, painter);

        qDebug() << "Printing" << item.dxfFile
                 << "to" << params.outFile << "DONE";

        delete item.doc;

        if (nrPages > 0)
            printer.newPage();
    }

    painter.end();
}


static bool openDocAndSetGraphic(RS_Document** doc, RS_Graphic** graphic,
    const QString& dxfFile){
    *doc = new RS_Graphic();
    LC_DocumentsStorage storage;
    if (!storage.loadDocument((*doc)->getGraphic(), dxfFile, RS2::FormatUnknown)) {
    // if (!(*doc)->open(dxfFile, RS2::FormatUnknown)) {
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


static void touchGraphic(RS_Graphic* graphic, PdfPrintParams& params){
    graphic->calculateBorders();
    graphic->setMargins(params.margins.left, params.margins.top,
                        params.margins.right, params.margins.bottom);
    graphic->setPagesNum(params.pagesH, params.pagesV);

    if (params.scale > 0.0) {
        graphic->setPaperScale(params.scale);
    }

    if (params.pageSize != RS_Vector(0.0, 0.0)) {
        graphic->setPaperSize(params.pageSize);
    }

    if (params.fitToPage) {
        graphic->fitToPage(); // fit and center
    }
    else if (params.centerOnPage) {
        graphic->centerToPage();
    }
}

static void setupPrinterAndPaper(RS_Graphic* graphic, QPrinter& printer,
    PdfPrintParams& params){
    bool landscape = false;

    RS2::PaperFormat pf = graphic->getPaperFormat(&landscape);
    QPageSize::PageSizeId paperSize = LC_Printing::rsToQtPaperFormat(pf);

    if (paperSize == QPageSize::Custom){
        RS_Vector r = graphic->getPaperSize();
        RS_Vector s = RS_Units::convert(r, graphic->getUnit(),
            RS2::Millimeter);
        if (landscape)
            s = s.flipXY();
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        printer.setPageSize(QPageSize{QSizeF{s.x,s.y}, QPageSize::Millimeter});
#else
        printer.setPaperSize(QSizeF{s.x,s.y}, QPrinter::Millimeter);
#endif
    } else {
        printer.setPageSize(paperSize);
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    printer.setPageOrientation(landscape ? QPageLayout::Landscape : QPageLayout::Portrait);
#else
    printer.setOrientation(landscape ? QPrinter::Landscape : QPrinter::Portrait);
#endif

    printer.setOutputFileName(params.outFile);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setResolution(params.resolution);
    printer.setFullPage(true);

    if (params.grayscale)
        printer.setColorMode(QPrinter::GrayScale);
    else
        printer.setColorMode(QPrinter::Color);
}

// fixme - sand - printing - refactor to separate class?
static void drawGraphic(RS_Graphic* graphic, QPrinter& printer,
                        RS_Painter& painter){
    double printerFx = (double)printer.width() / printer.widthMM();
    double printerFy = (double)printer.height() / printer.heightMM();

    double marginLeft = graphic->getMarginLeft();
    double marginTop = graphic-> getMarginTop();
    double marginRight = graphic->getMarginRight();
    double marginBottom = graphic->getMarginBottom();

    painter.setClipRect(marginLeft * printerFx, marginTop * printerFy,
                        printer.width() - (marginLeft + marginRight) * printerFx,
                        printer.height() - (marginTop + marginBottom) * printerFy);

    LC_GraphicViewport viewport;
    viewport.setContainer(graphic);
    viewport.setSize(printer.width(), printer.height());
    viewport.setBorders(0,0,0,0);

    LC_PrintViewportRenderer renderer(&viewport, &painter);
    viewport.loadSettings();
    renderer.loadSettings();

    RS2::Unit unit = graphic->getUnit();
    double fx = printerFx * RS_Units::getFactorToMM(unit);
    double fy = printerFy * RS_Units::getFactorToMM(unit);

    double f = (fx + fy) / 2.0;

    double paperScale = graphic->getPaperScale();
    renderer.setPaperScale(paperScale);

    double paperInsertionX = graphic->getPaperInsertionBase().x;
    double paperInsertionY = graphic->getPaperInsertionBase().y;

    double factor = f * paperScale;
    viewport.justSetOffsetAndFactor((int)(paperInsertionX * f),
                                    (int)(paperInsertionY * f),
                                    factor);
    
    double baseX = paperInsertionX;
    double baseY = paperInsertionY;
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

            viewport.justSetOffsetAndFactor((int)((baseX - offsetX) * f),
                                            (int)((baseY - offsetY) * f),
                                            factor);
            painter.setViewPort(&viewport);  // update offset
            renderer.render();
        }
    }
}
