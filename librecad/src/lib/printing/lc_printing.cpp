/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2024 LibreCAD.org
** Copyright (C) 2024 Dongxu Li (dongxuli2011@gmail.com)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**********************************************************************/

#include "lc_printing.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QRegularExpression>

#include "lc_graphicviewport.h"
#include "lc_printpreviewview.h"
#include "lc_printviewportrenderer.h"
#include "qc_mdiwindow.h"
#include "qg_graphicview.h"
#include "rs_debug.h"
#include "rs_painter.h"
#include "rs_settings.h"
#include "rs_units.h"

// fixme - sand - files - this class should not be in /lib, it has outer dependencies. Reivew!!!!
namespace {

// supported paper formats should be added here
const std::map<RS2::PaperFormat, QPageSize::PageSizeId> paperToPage = {
    {
     {RS2::A0, QPageSize::A0},
     {RS2::A1, QPageSize::A1},
     {RS2::A2, QPageSize::A2},
     {RS2::A3, QPageSize::A3},
     {RS2::A4, QPageSize::A4},

     /* Removed ISO "B" and "C" series,  C5E,  Comm10E,  DLE,  (envelope sizes) */

     {RS2::Letter, QPageSize::Letter},
     {RS2::Legal,  QPageSize::Legal},
     {RS2::Tabloid, QPageSize::Tabloid},

     //case RS2::Ansi_A, QPageSize::AnsiA},
     //case RS2::Ansi_B, QPageSize::AnsiB},
     {RS2::Ansi_C, QPageSize::AnsiC},
     {RS2::Ansi_D, QPageSize::AnsiD},
     {RS2::Ansi_E, QPageSize::AnsiE},

     {RS2::Arch_A, QPageSize::ArchA},
     {RS2::Arch_B, QPageSize::ArchB},
     {RS2::Arch_C, QPageSize::ArchC},
     {RS2::Arch_D, QPageSize::ArchD},
     {RS2::Arch_E, QPageSize::ArchE},
     }
};

/**
 * @brief setFileNameColor - get the printing output file name and black/white or color mode
 * @returns QString - the PDF file name to export to
 * @param printer - a QPrinter
 * @param graphic - the graphic to print
 */
QString setFileNameColor(QPrinter& printer, RS_Graphic& graphic)
{
    QString pdfFileName = graphic.getFilename();
    if (pdfFileName.isEmpty()) {
        pdfFileName = graphic.getAutoSaveFileName();
    }
    static QRegularExpression reSuffix{R"(\.dxf$)", QRegularExpression::CaseInsensitiveOption};
    pdfFileName.replace(reSuffix, ".pdf");

    // color or black/white mode
    LC_GROUP_GUARD("Print");
    printer.setColorMode((QPrinter::ColorMode) LC_GET_INT("ColorMode", (int) QPrinter::Color));
    return pdfFileName;
}
}

QPageSize::PageSizeId LC_Printing::rsToQtPaperFormat(RS2::PaperFormat paper){
    return (paperToPage.count(paper) == 1) ? paperToPage.at(paper) : QPageSize::Custom;
}

void LC_Printing::Print(QC_MDIWindow &mdiWindow, PrinterType printerType)
{
    RS_Graphic *graphic = mdiWindow.getDocument()->getGraphic();

    if (graphic == nullptr) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                        "QC_ApplicationWindow::slotFilePrint: "
                        "no graphic");
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    // fullPage must be set to true to get full width and height
    // (without counting margins).
    printer.setFullPage(true);

    bool landscape = false;
    RS2::PaperFormat paperformat = graphic->getPaperFormat(&landscape);
    QPageSize::PageSizeId paperSizeName = LC_Printing::rsToQtPaperFormat(paperformat);
    RS_Vector paperSize = graphic->getPaperSize();
    RS2::Unit unit = graphic->getUnit();
    if (paperSizeName == QPageSize::Custom) {
        RS_Vector s = RS_Units::convert(paperSize, unit, RS2::Millimeter);
        if (landscape) s = s.flipXY();
        printer.setPageSize(QPageSize{QSizeF(s.x, s.y), QPageSize::Millimeter});
        // RS_DEBUG->print(RS_Debug::D_ERROR, "set Custom paper size to (%g, %g)\n", s.x,s.y);
    } else {
        printer.setPageSize(QPageSize{static_cast<QPageSize::PageSizeId>(paperSizeName)});
    }
    // qDebug()<<"paper size=("<<printer.paperSize(QPrinter::Millimeter).width()<<", "<<printer.paperSize(QPrinter::Millimeter).height()<<")";
    printer.setPageOrientation(landscape ? QPageLayout::Landscape : QPageLayout::Portrait);
    QMarginsF paperMargins{graphic->getMarginLeft(),
                           graphic->getMarginRight(),
                           graphic->getMarginTop(),
                           graphic->getMarginBottom()};
    printer.setPageMargins(paperMargins, QPageLayout::Millimeter);

    // Issue #2130: populate the output file name for
    QString defaultFile = setFileNameColor(printer, *graphic);

    // printer setup:
    bool bStartPrinting = false;
    if (printerType == PrinterType::PDF) {
        printer.setFullPage(true);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setColorMode(QPrinter::Color);
        printer.setResolution(1200);
        // Issue #1897, exporting PDF margins to to follow the drawing settings
        QPageLayout layout = printer.pageLayout();
        layout.setMode(QPageLayout::FullPageMode);
        layout.setUnits(QPageLayout::Millimeter);
        layout.setMinimumMargins({});
        RS_Vector s=RS_Units::convert(paperSize, unit, RS2::Millimeter);
        if(landscape)
            s=s.flipXY();
        layout.setPageSize(QPageSize{QSizeF(s.x, s.y), QPageSize::Millimeter}, paperMargins);
        printer.setPageLayout(layout);
        QString pdfFie = QFileDialog::getSaveFileName(
            &mdiWindow,
            QObject::tr("Export to PDF"),
            defaultFile,
            QObject::tr("PDF files (*.pdf);;All files (*.*)"));
        if (pdfFie.isEmpty())
            pdfFie = defaultFile;
        printer.setOutputFileName(pdfFie);
        bStartPrinting = true;
    } else {
        printer.setOutputFileName(""); // uncheck 'Print to file' checkbox
        printer.setOutputFormat(QPrinter::NativeFormat);

        QPrintDialog printDialog(&printer, &mdiWindow);
        printDialog.setOption(QAbstractPrintDialog::PrintToFile);
        printDialog.setOption(QAbstractPrintDialog::PrintShowPageSize);
        bStartPrinting = (QDialog::Accepted == printDialog.exec());

        auto equalPaperSize = [&printer](const RS_Vector &v0, const RS_Vector &v1) {
            // from DPI to pixel/mm
            auto resolution = RS_Units::convert(1., RS2::Millimeter, RS2::Inch) * printer.resolution();
            // ignore difference within two pixels
            return v0.distanceTo(v1) * resolution <= 2.;
        };
        auto equalMargins = [&printer](const QMarginsF &drawingMargins) {
            QMarginsF printerMarginsPixels = printer.pageLayout().marginsPixels(printer.resolution());
            // from DPI to pixel/mm
            auto resolution = RS_Units::convert(1., RS2::Millimeter, RS2::Inch) * printer.resolution();
            // assuming drawingMargins in mm
            QMarginsF drawingMarginsPixels = drawingMargins * resolution;
            QMarginsF diff = printerMarginsPixels - drawingMarginsPixels;
            // ignore difference within two pixels
            return std::max({std::abs(diff.left()), std::abs(diff.right()), std::abs(diff.top()), std::abs(diff.bottom())}) <= 2.;
        };

        RS_Vector paperSizeMm = RS_Units::convert(paperSize, unit, RS2::Millimeter);
        QMarginsF printerMargins = printer.pageLayout().margins();
        QRectF paperRect = printer.paperRect(QPrinter::Millimeter);
        RS_Vector printerSizeMm{paperRect.width(), paperRect.height()};
        if (bStartPrinting
            && (!equalPaperSize(printerSizeMm, paperSizeMm) || !equalMargins(paperMargins))) {
            QMessageBox msgBox(&mdiWindow);
            // FIXME - SAND - localization
            msgBox.setWindowTitle("Paper settings");
            msgBox.setText("Paper size and/or margins have been changed!");
            msgBox.setInformativeText("Do you want to apply changes to current drawing?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            qreal printMarginsLeft = printerMargins.left();
            qreal printMarginsTop = printerMargins.top();
            qreal printMarginsRight = printerMargins.right();
            qreal printMarginsBottom = printerMargins.bottom();
            QString detailedText = QString("Drawing settings:\n"
                                           "\tsize: %1 x %2 (%3)\n"
                                           "\tmargins: %4, %5, %6, %7\n"
                                           "\n"
                                           "Printer settings:\n"
                                           "\tsize: %8 x %9 (%10)\n"
                                           "\tmargins: %11, %12, %13, %14\n")
                .arg(paperSize.x)
                .arg(paperSize.y)
                .arg(RS_Units::paperFormatToString(paperformat))
                .arg(RS_Units::convert(paperMargins.left(), RS2::Millimeter, unit))
                .arg(RS_Units::convert(paperMargins.top(), RS2::Millimeter, unit))
                .arg(RS_Units::convert(paperMargins.right(), RS2::Millimeter, unit))
                .arg(RS_Units::convert(paperMargins.bottom(), RS2::Millimeter, unit))
                .arg(RS_Units::convert(printerSizeMm.x, RS2::Millimeter, unit))
                .arg(RS_Units::convert(printerSizeMm.y, RS2::Millimeter, unit))
                .arg(printer.pageLayout().pageSize().name())
                .arg(RS_Units::convert(printMarginsLeft, RS2::Millimeter, unit))
                .arg(RS_Units::convert(printMarginsTop, RS2::Millimeter, unit))
                .arg(RS_Units::convert(printMarginsRight, RS2::Millimeter, unit))
                .arg(RS_Units::convert(printMarginsBottom, RS2::Millimeter, unit));
            msgBox.setDetailedText(detailedText);
            int answer = msgBox.exec();
            switch (answer) {
                case QMessageBox::Yes:
                    graphic->setPaperSize(RS_Units::convert(printerSizeMm, RS2::Millimeter, unit));
                    graphic->setMargins(printMarginsLeft, printMarginsTop,
                                        printMarginsRight, printMarginsBottom);
                    break;
                case QMessageBox::No:
                    break;
                case QMessageBox::Cancel:
                    bStartPrinting = false;
                    break;
            }
        }
    }

    if (bStartPrinting) {
        RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "QC_ApplicationWindow::slotFilePrint: resolution is %d", printer.resolution());
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        RS_Painter painter(&printer);
        // RAII style to restore cursor. Not really a shared pointer for ownership
        std::shared_ptr<RS_Painter> painterPtr{&painter, []([[maybe_unused]] RS_Painter *painter) {
            QApplication::restoreOverrideCursor();
        }};

        // fixme - sand rework this later - it seems that printing in general should be refined.  
        QG_GraphicView *graphicView = mdiWindow.getGraphicView();
        RS2::DrawingMode drawingMode = RS2::DrawingMode::ModeAuto;
        if (graphicView->isPrintPreview()){
            LC_PrintPreviewView* printPreview = dynamic_cast<LC_PrintPreviewView *>(graphicView);
            if (printPreview != nullptr){
                drawingMode = printPreview->getDrawingMode();
            }
        }
        painter.setDrawingMode(drawingMode);

        QMarginsF margins = printer.pageLayout().margins(QPageLayout::Millimeter);
//        LC_ERR << "Printer margins (mm): " << margins.left()<<": "<<margins.top()<<" : "<<margins.right()<<" : "<<margins.bottom();

        double printerWidth = printer.width();
        double printerHeight = printer.height();

        double printerFx = (double) printerWidth / printer.widthMM();
        double printerFy = (double) printerHeight / printer.heightMM();

        painter.setClipRect(margins.left() * printerFx, margins.top() * printerFy,
                            printerWidth - (margins.left() + margins.right()) * printerFx,
                            printerHeight - (margins.top() + margins.bottom()) * printerFy);

        LC_GraphicViewport viewport;
        viewport.setContainer(graphic);
        viewport.setBorders(0, 0, 0, 0);
        viewport.setSize(printerWidth, printerHeight);

        LC_PrintViewportRenderer renderer(&viewport, &painter);
        viewport.loadSettings();
        renderer.loadSettings();

        bool scaleLineWidth = mdiWindow.getGraphicView()->getLineWidthScaling();
        renderer.setLineWidthScaling(scaleLineWidth);

        double fx = printerFx * RS_Units::getFactorToMM(unit);
        double fy = printerFy * RS_Units::getFactorToMM(unit);
        //RS_DEBUG->print(RS_Debug::D_ERROR, "paper size=(%d, %d)\n",
        //                printer.widthMM(),printer.heightMM());

        double f = (fx + fy) / 2.0;

        double scale = graphic->getPaperScale();
        double factor = f * scale;

        //RS_DEBUG->print(RS_Debug::D_ERROR, "PaperSize=(%d, %d)\n",printer.widthMM(), printer.heightMM());

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
                if (pX > 0 || pY > 0) {
                    printer.newPage();
                }

                viewport.justSetOffsetAndFactor((int) ((baseX - offsetX) * f),
                                                (int) ((baseY - offsetY) * f),
                                                factor);


                painter.setViewPort(&viewport);  // update offset
                renderer.render();

//                painter.setDrawSelectedOnly(true);
//                gv.drawEntity(&painter, graphic);
//                painter.setDrawSelectedOnly(false);
//                gv.drawEntity(&painter, graphic);
            }
        }

        // GraphicView deletes painter
        // Calling QPainter::end() is automatic at QPainter destructor
        // painter.end();

        LC_GROUP_GUARD("Print");
        {
            LC_SET("ColorMode", printer.colorMode());
            LC_SET("FileName", printer.outputFileName());
        }
    }
}
