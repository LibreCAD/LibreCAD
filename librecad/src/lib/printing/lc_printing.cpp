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

#include <map>

#include <QApplication>
#include <QFileDialog>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPrinter>
#include <QPrintDialog>

#include "lc_printing.h"

#include "qc_mdiwindow.h"
#include "qg_graphicview.h"

#include "rs_debug.h"
#include "rs_graphic.h"
#include "rs_painterqt.h"
#include "rs_settings.h"
#include "rs_staticgraphicview.h"
#include "rs_units.h"
#include "rs_vector.h"

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
}

QPageSize::PageSizeId LC_Printing::rsToQtPaperFormat(RS2::PaperFormat paper)
{
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

    bool landscape = false;
    RS2::PaperFormat pf = graphic->getPaperFormat(&landscape);
    QPageSize::PageSizeId paperSizeName = LC_Printing::rsToQtPaperFormat(pf);
    RS_Vector paperSize = graphic->getPaperSize();
    if (paperSizeName == QPageSize::Custom) {
        RS_Vector s = RS_Units::convert(paperSize, graphic->getUnit(), RS2::Millimeter);
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
    printer.setPageMargins(paperMargins);

    QString strDefaultFile("");
    LC_GROUP("Print");
    strDefaultFile = LC_GET_STR("FileName", "");
    printer.setOutputFileName(strDefaultFile);
    printer.setColorMode((QPrinter::ColorMode) LC_GET_INT("ColorMode", (int) QPrinter::Color));
    LC_GROUP_END();

    // printer setup:
    bool bStartPrinting = false;
    if (printerType == PrinterType::PDF) {
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setColorMode(QPrinter::Color);
        QFileInfo infDefaultFile(strDefaultFile);
        QFileDialog fileDlg(&mdiWindow, QObject::tr("Export as PDF"));
        QString defFilter("PDF files (*.pdf)");
        QStringList filters;

        filters << defFilter
                << "Any files (*)";

        fileDlg.setNameFilters(filters);
        fileDlg.setFileMode(QFileDialog::AnyFile);
        fileDlg.selectNameFilter(defFilter);
        fileDlg.setAcceptMode(QFileDialog::AcceptSave);
        fileDlg.setDefaultSuffix("pdf");
        fileDlg.setDirectory(infDefaultFile.dir().path());
        // bug#509 setting default file name restricts selection
        //        strPdfFileName = infDefaultFile.baseName();
        //        if( strPdfFileName.isEmpty())
        //            strPdfFileName = "unnamed";
        //fileDlg.selectFile(strPdfFileName);

        if (QDialog::Accepted == fileDlg.exec()) {
            QStringList files = fileDlg.selectedFiles();
            if (!files.isEmpty()) {
                if (!files[0].endsWith(R"(.pdf)", Qt::CaseInsensitive)) files[0] = files[0] + ".pdf";
                printer.setOutputFileName(files[0]);
                bStartPrinting = true;
            }
        }
    } else {
        printer.setOutputFileName(""); // uncheck 'Print to file' checkbox
        printer.setOutputFormat(QPrinter::NativeFormat);

        QPrintDialog printDialog(&printer, &mdiWindow);
        printDialog.setOption(QAbstractPrintDialog::PrintToFile);
        printDialog.setOption(QAbstractPrintDialog::PrintShowPageSize);
        bStartPrinting = (QDialog::Accepted == printDialog.exec());

        // fullPage must be set to true to get full width and height
        // (without counting margins).
        printer.setFullPage(true);
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

        RS_Vector paperSizeMm = RS_Units::convert(paperSize, graphic->getUnit(), RS2::Millimeter);
        QMarginsF printerMargins = printer.pageLayout().margins();
        QRectF paperRect = printer.paperRect(QPrinter::Millimeter);
        RS_Vector printerSizeMm{paperRect.width(), paperRect.height()};
        if (bStartPrinting
            && (!equalPaperSize(printerSizeMm, paperSizeMm) || !equalMargins(paperMargins))) {
            QMessageBox msgBox(&mdiWindow);
            msgBox.setWindowTitle("Paper settings");
            msgBox.setText("Paper size and/or margins have been changed!");
            msgBox.setInformativeText("Do you want to apply changes to current drawing?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            QString detailedText = QString("Drawing settings:\n"
                                           "\tsize: %1 x %2 (%3)\n"
                                           "\tmargins: %4, %5, %6, %7\n"
                                           "\n"
                                           "Printer settings:\n"
                                           "\tsize: %8 x %9 (%10)\n"
                                           "\tmargins: %11, %12, %13, %14\n")
                .arg(paperSize.x)
                .arg(paperSize.y)
                .arg(RS_Units::paperFormatToString(pf))
                .arg(RS_Units::convert(paperMargins.left(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(paperMargins.top(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(paperMargins.right(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(paperMargins.bottom(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerSizeMm.x, RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerSizeMm.y, RS2::Millimeter, graphic->getUnit()))
                .arg(printer.pageLayout().pageSize().name())
                .arg(RS_Units::convert(printerMargins.left(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerMargins.top(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerMargins.right(), RS2::Millimeter, graphic->getUnit()))
                .arg(RS_Units::convert(printerMargins.bottom(), RS2::Millimeter, graphic->getUnit()));
            msgBox.setDetailedText(detailedText);
            int answer = msgBox.exec();
            switch (answer) {
                case QMessageBox::Yes:
                    graphic->setPaperSize(RS_Units::convert(printerSizeMm, RS2::Millimeter, graphic->getUnit()));
                    graphic->setMargins(printerMargins.left(), printerMargins.top(),
                                        printerMargins.right(), printerMargins.bottom());
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
        // Try to set the printer to the highest resolution
        //todo: handler printer resolution better
        if (printer.outputFormat() == QPrinter::NativeFormat) {
            //bug#3448560
            //fixme: supportedResolutions() only reports resolution of 72dpi
            //this seems to be a Qt bug up to Qt-4.7.4
            //we might be ok to keep the default resolution

            //            QList<int> res=printer.supportedResolutions ();
            //            if (res.size()>0)
            //                printer.setResolution(res.last());
            //        for(int i=0;i<res.size();i++){
            //        std::cout<<"res.at(i)="<<res.at(i)<<std::endl;
            //        }
        } else {//pdf or postscript format
            //fixme: user should be able to set resolution output to file
            printer.setResolution(1200);
        }

        RS_DEBUG->print(RS_Debug::D_INFORMATIONAL, "QC_ApplicationWindow::slotFilePrint: resolution is %d", printer.resolution());
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        printer.setFullPage(true);

        RS_PainterQt painter(&printer);
        // RAII style to restore cursor. Not really a shared pointer for ownership
        std::shared_ptr<RS_PainterQt> painterPtr{&painter, []([[maybe_unused]] RS_PainterQt *painter) {
            QApplication::restoreOverrideCursor();
        }};
        painter.setDrawingMode(mdiWindow.getGraphicView()->getDrawingMode());

        QMarginsF margins = printer.pageLayout().margins();

        double printerFx = (double) printer.width() / printer.widthMM();
        double printerFy = (double) printer.height() / printer.heightMM();

        painter.setClipRect(margins.left() * printerFx, margins.top() * printerFy,
                            printer.width() - (margins.left() + margins.right()) * printerFx,
                            printer.height() - (margins.top() + margins.bottom()) * printerFy);

        RS_StaticGraphicView gv(printer.width(), printer.height(), &painter);
        gv.setPrinting(true);
        gv.setBorders(0, 0, 0, 0);
        gv.setLineWidthScaling(mdiWindow.getGraphicView()->getLineWidthScaling());

        double fx = printerFx * RS_Units::getFactorToMM(graphic->getUnit());
        double fy = printerFy * RS_Units::getFactorToMM(graphic->getUnit());
        //RS_DEBUG->print(RS_Debug::D_ERROR, "paper size=(%d, %d)\n",
        //                printer.widthMM(),printer.heightMM());

        double f = (fx + fy) / 2.0;

        double scale = graphic->getPaperScale();

        gv.setFactor(f * scale);
        //RS_DEBUG->print(RS_Debug::D_ERROR, "PaperSize=(%d, %d)\n",printer.widthMM(), printer.heightMM());
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
                gv.setOffset((int) ((baseX - offsetX) * f),
                             (int) ((baseY - offsetY) * f));
                //fixme, I don't understand the meaning of 'true' here
                //        gv.drawEntity(&painter, graphic, true);
                painter.setDrawSelectedOnly(true);
                gv.drawEntity(&painter, graphic);
                painter.setDrawSelectedOnly(false);
                gv.drawEntity(&painter, graphic);
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
