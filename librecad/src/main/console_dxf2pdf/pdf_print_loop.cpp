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

#include <cstdlib>

#include "pdf_print_loop.h"

#include <QPrinter>
#include <QtCore>

#include "lc_documentsstorage.h"
#include "lc_graphicviewport.h"
#include "lc_printing.h"
#include "lc_printviewportrenderer.h"
#include "rs.h"
#include "rs_graphic.h"
#include "rs_painter.h"
#include "rs_units.h"

static bool openDocAndSetGraphic(RS_Document**, RS_Graphic**, const QString&);
static void touchGraphic(RS_Graphic*, const PdfPrintParams&);
static void setupPrinterAndPaper(const RS_Graphic*, QPrinter&, PdfPrintParams&);
static void drawGraphic(RS_Graphic *graphic, QPrinter &printer, RS_Painter &painter);

void PdfPrintLoop::run(){
    int failed = 0;
    if (m_params.outFile.isEmpty()) {
        for (auto &&f : m_params.inputFiles) {
            if (!printOneFileToOnePdf(f))
                ++failed;
        }
    } else {
        failed = printManyFilesToOnePdf();
    }
    emit finished(failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}


bool PdfPrintLoop::printOneFileToOnePdf(const QString& inputFile) {

    // Main code logic and flow for this method is originally stolen from
    // QC_ApplicationWindow::slotFilePrint(bool printPDF) method.
    // But finally it was split in to smaller parts.

    QFileInfo inputFileInfo(inputFile);
    m_params.outFile =
        (m_params.outDir.isEmpty() ? inputFileInfo.path() : m_params.outDir)
        + "/" + inputFileInfo.completeBaseName() + ".pdf";

    RS_Document *doc = nullptr;
    RS_Graphic *graphic = nullptr;

    if (!openDocAndSetGraphic(&doc, &graphic, inputFile))
        return false;

    qDebug() << "Printing" << inputFile << "to" << m_params.outFile << ">>>>";

    touchGraphic(graphic, m_params);

    QPrinter printer(QPrinter::HighResolution);

    setupPrinterAndPaper(graphic, printer, m_params);

    RS_Painter painter(&printer);

    if (m_params.monochrome) {
        painter.setDrawingMode(RS2::ModeBW);
    }

    drawGraphic(graphic, printer, painter);

    painter.end();

    qDebug() << "Printing" << inputFile << "to" << m_params.outFile << "DONE";

    delete doc;
    return true;
}


int PdfPrintLoop::printManyFilesToOnePdf() {
    struct PrintContentItem {
        RS_Document* doc;
        RS_Graphic* graphic;
        QString inputFile;
        QPageSize::PageSizeId paperSize;
    };

    if (!m_params.outDir.isEmpty()) {
        const QFileInfo outFileInfo(m_params.outFile);
        m_params.outFile = m_params.outDir + "/" + outFileInfo.fileName();
    }

    QVector<PrintContentItem> contentItems;
    int nrPages = 0;
    int failed = 0;

    // FIXME: Should probably open and print all input files in one 'for' loop.
    // Tried but failed to do this. It looks like some 'chicken and egg'
    // situation for the QPrinter and RS_PainterQt. Therefore, first open
    // all input files and apply required actions. Then run another 'for'
    // loop for actual printing.
    for (auto inputFile : m_params.inputFiles) {
        PrintContentItem page;
        page.inputFile = inputFile;
        if (!openDocAndSetGraphic(&page.doc, &page.graphic, inputFile)) {
            ++failed;
            continue;
        }

        qDebug() << "Opened" << inputFile;

        touchGraphic(page.graphic, m_params);
        contentItems.append(page);
        nrPages++;
    }

    if (contentItems.isEmpty())
        return failed == 0 ? m_params.inputFiles.size() : failed;

    QPrinter printer(QPrinter::HighResolution);

    if (nrPages > 0) {
        // FIXME: Is it possible to set up printer and paper for every
        // opened dxf file and tie them with painter? For now just using
        // data extracted from the first opened dxf file for all pages.
        setupPrinterAndPaper(contentItems.at(0).graphic, printer, m_params);
    }

    RS_Painter painter(&printer);

    if (m_params.monochrome) {
        painter.setDrawingMode(RS2::ModeBW);
    }

    // And now it's time to actually print all previously opened dxf files.
    for (const auto &item : contentItems) {
        nrPages--;

        qDebug() << "Printing" << item.inputFile
                 << "to" << m_params.outFile << ">>>>";

        drawGraphic(item.graphic, printer, painter);

        qDebug() << "Printing" << item.inputFile
                 << "to" << m_params.outFile << "DONE";

        delete item.doc;

        if (nrPages > 0) {
            printer.newPage();
        }
    }

    painter.end();
    return failed;
}

static bool openDocAndSetGraphic(RS_Document** doc, RS_Graphic** graphic,
    const QString& dxfFile){
    *doc = new RS_Graphic();
    const LC_DocumentsStorage storage;
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

static void touchGraphic(RS_Graphic* graphic, const PdfPrintParams& params){
    graphic->calculateBorders();
    LC_PlotSettings* ps = graphic->getPlotSettings();
    ps->setMarginsInMm(params.margins.left, params.margins.top,
                        params.margins.right, params.margins.bottom);
    ps->setPagesNum(params.pagesH, params.pagesV);

    if (params.scale > 0.0) {
        ps->setPaperScale(params.scale);
    }

    if (params.pageSize != RS_Vector(0.0, 0.0)) {
        ps->setPaperSize(params.pageSize);
    }

    if (params.fitToPage) {
        graphic->fitToPage(); // fit and center
    }
    else if (params.centerOnPage) {
        graphic->centerToPage();
    }
}

static void setupPrinterAndPaper(const RS_Graphic* graphic, QPrinter& printer,
    PdfPrintParams& params){
    bool landscape = false;
    const LC_PlotSettings* ps = graphic->getPlotSettings();
    const RS2::PaperFormat pf = ps->getPaperFormat(&landscape);
    const QPageSize::PageSizeId paperSizeName = LC_Printing::rsToQtPaperFormat(pf);

    RS_Vector paperSize = ps->getPaperSize();
    QMarginsF paperMargins{ps->getMarginLeftMm(),
                           ps->getMarginRightMm(),
                           ps->getMarginTopMm(),
                           ps->getMarginBottomMm()};

    LC_Printing::setupPageLayout(printer, landscape, paperSizeName, paperSize, graphic->getUnit(), paperMargins);

    printer.setOutputFileName(params.outFile);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setResolution(params.resolution);
    printer.setFullPage(true);

    if (params.grayscale) {
        printer.setColorMode(QPrinter::GrayScale);
    }
    else {
        printer.setColorMode(QPrinter::Color);
    }
}

// fixme - sand - printing - refactor to separate class?
static void drawGraphic(RS_Graphic* graphic, QPrinter& printer,
                        RS_Painter& painter){
    const double printerWidth = printer.width();
    const double printerHeight = printer.height();
    const double printerFx = printerWidth / printer.widthMM();
    const double printerFy = printerHeight / printer.heightMM();

    const LC_PlotSettings* ps = graphic->getPlotSettings();

    const double marginLeft = ps->getMarginLeftMm();
    const double marginTop = ps->getMarginTopMm();
    const double marginRight = ps->getMarginRightMm();
    const double marginBottom = ps->getMarginBottomMm();

    painter.setClipRect(marginLeft * printerFx, marginTop * printerFy,
                        printer.width() - (marginLeft + marginRight) * printerFx,
                        printer.height() - (marginTop + marginBottom) * printerFy);

    LC_GraphicViewport viewport;
    viewport.setDocument(graphic);
    viewport.setSize(printer.width(), printer.height());
    viewport.setBorders(0,0,0,0);

    LC_PrintViewportRenderer renderer(&viewport, &painter);
    viewport.loadSettings();
    renderer.loadSettings();

    const RS2::Unit unit = graphic->getUnit();
    const double fx = printerFx * RS_Units::getFactorToMM(unit);
    const double fy = printerFy * RS_Units::getFactorToMM(unit);

    const double f = (fx + fy) / 2.0;

    const double paperScale = ps->getPaperScale();
    renderer.setPaperScale(paperScale);

    const double paperInsertionX = graphic->getPaperInsertionBase().x;
    const double paperInsertionY = graphic->getPaperInsertionBase().y;

    const double factor = f * paperScale;
    viewport.justSetOffsetAndFactor(static_cast<int>(paperInsertionX * f),
                                    static_cast<int>(paperInsertionY * f),
                                    factor);

    const double baseX = paperInsertionX;
    const double baseY = paperInsertionY;
    const int numX = ps->getPagesNumHoriz();
    const int numY = ps->getPagesNumVert();
    const RS_Vector printArea = ps->getPrintAreaSize(false);

    for (int pY = 0; pY < numY; pY++) {
        const double offsetY = printArea.y * pY;
        for (int pX = 0; pX < numX; pX++) {
            const double offsetX = printArea.x * pX;
            // First page is created automatically.
            // Extra pages must be created manually.
            if (pX > 0 || pY > 0) {
                printer.newPage();
            }

            viewport.justSetOffsetAndFactor(static_cast<int>((baseX - offsetX) * f),
                                            static_cast<int>((baseY - offsetY) * f),
                                            factor);
            painter.setViewPort(&viewport);  // update offset
            renderer.render();
        }
    }
}
