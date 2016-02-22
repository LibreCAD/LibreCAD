/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
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


#ifndef RS_FILTERDXF1_H
#define RS_FILTERDXF1_H

#include <QFile>

#include "rs_filterinterface.h"

/**
 * This format filter class can import and export old DXF files
 * from QCad 1.x.
 *
 * This is legacy code from QCad 1.x.
 *
 * @author Andrew Mustun
 */
class RS_FilterDXF1 : public RS_FilterInterface {
public:
	RS_FilterDXF1();

    virtual bool canImport(const QString& /*fileName*/, RS2::FormatType t) const {
		return (t==RS2::FormatDXF1);
	}
	
    virtual bool canExport(const QString& /*fileName*/, RS2::FormatType /*t*/) const {
		return false;
    }

    virtual bool fileImport(RS_Graphic& g, const QString& file, RS2::FormatType /*type*/);

    virtual bool fileExport(RS_Graphic& /*g*/, const QString& /*file*/,
		RS2::FormatType /*type*/);

    bool readFromBuffer();

    void    reset();
    void    resetBufP();

    void    setBufP(int _fBufP);
    int     getBufP() {
        return fBufP;
    }
    void    delBuffer();
    void    dos2unix();

    QString getBufLine();
    char*   getBufLineCh();
    char*   getBuf() {
        return fBuf;
    }
    void    setBuf(char* _buf) {
        fBuf=_buf;
    }
    void    setFSize(unsigned _s) {
        fSize=_s;
    }
    void    copyBufFrom(const char* _buf);
    bool    gotoBufLine(char* _lstr);
    bool    gotoBufLineString(char* _lstr);

    void    replaceBinaryBytesBy(char _c);
    void    separateBuf(char _c1=13,
                        char _c2=10,
                        char _c3=0,
                        char _c4=0);
    void    removeComment(char _fc='(',
                          char _lc=')');

    bool    readFileInBuffer(char* _name, int _bNum=-1);
    bool    readFileInBuffer(int _bNum=-1);

    void     strDecodeDxfString(QString& str);
    bool     mtCompFloat(double _v1, double _v2, double _tol=1.0e-6);

    static RS_FilterInterface* createFilter(){return new RS_FilterDXF1();}
    static RS2::LineWidth numberToWidth(int num);
    static int widthToNumber(RS2::LineWidth width);

protected:
    /** Pointer to the graphic we currently operate on. */
    RS_Graphic* graphic;
    FILE*   fPointer;         // File pointer
    char*   fBuf;             // Filebuffer
    int     fBufP;            // Filebuffer-Pointer (walks through 'fBuf')
    unsigned    fSize;            // Filesize
    bool    dosFile;          // File is in DOS-format
    int       numElements;
    QString name;
	QFile file;
}
;

#endif
