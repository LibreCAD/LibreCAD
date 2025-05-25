/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

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
 ******************************************************************************/

#ifndef LC_DIMSTYLE_H
#define LC_DIMSTYLE_H

#include <QString>

class LC_DimStyle{
public:
    LC_DimStyle() { reset();}

    void reset();
    const QString &getName() const;
    void setName(const QString &name);
    const QString &getDimpost() const;
    void setDimpost(const QString &dimpost);
    const QString &getDimapost() const;
    void setDimapost(const QString &dimapost);
    const QString &getDimblk() const;
    void setDimblk(const QString &dimblk);
    const QString &getDimblk1() const;
    void setDimblk1(const QString &dimblk1);
    const QString &getDimblk2() const;
    void setDimblk2(const QString &dimblk2);
    double getDimscale() const;
    void setDimscale(double dimscale);
    double getDimasz() const;
    void setDimasz(double dimasz);
    double getDimexo() const;
    void setDimexo(double dimexo);
    double getDimdli() const;
    void setDimdli(double dimdli);
    double getDimexe() const;
    void setDimexe(double dimexe);
    double getDimrnd() const;
    void setDimrnd(double dimrnd);
    double getDimdle() const;
    void setDimdle(double dimdle);
    double getDimtp() const;
    void setDimtp(double dimtp);
    double getDimtm() const;
    void setDimtm(double dimtm);
    double getDimfxl() const;
    void setDimfxl(double dimfxl);
    double getDimtxt() const;
    void setDimtxt(double dimtxt);
    double getDimcen() const;
    void setDimcen(double dimcen);
    double getDimtsz() const;
    void setDimtsz(double dimtsz);
    double getDimaltf() const;
    void setDimaltf(double dimaltf);
    double getDimlfac() const;
    void setDimlfac(double dimlfac);
    double getDimtvp() const;
    void setDimtvp(double dimtvp);
    double getDimtfac() const;
    void setDimtfac(double dimtfac);
    double getDimgap() const;
    void setDimgap(double dimgap);
    double getDimaltrnd() const;
    void setDimaltrnd(double dimaltrnd);
    int getDimtol() const;
    void setDimtol(int dimtol);
    int getDimlim() const;
    void setDimlim(int dimlim);
    int getDimtih() const;
    void setDimtih(int dimtih);
    int getDimtoh() const;
    void setDimtoh(int dimtoh);
    int getDimse1() const;
    void setDimse1(int dimse1);
    int getDimse2() const;
    void setDimse2(int dimse2);
    int getDimtad() const;
    void setDimtad(int dimtad);
    int getDimzin() const;
    void setDimzin(int dimzin);
    int getDimazin() const;
    void setDimazin(int dimazin);
    int getDimalt() const;
    void setDimalt(int dimalt);
    int getDimaltd() const;
    void setDimaltd(int dimaltd);
    int getDimtofl() const;
    void setDimtofl(int dimtofl);
    int getDimsah() const;
    void setDimsah(int dimsah);
    int getDimtix() const;
    void setDimtix(int dimtix);
    int getDimsoxd() const;
    void setDimsoxd(int dimsoxd);
    int getDimclrd() const;
    void setDimclrd(int dimclrd);
    int getDimclre() const;
    void setDimclre(int dimclre);
    int getDimclrt() const;
    void setDimclrt(int dimclrt);
    int getDimadec() const;
    void setDimadec(int dimadec);
    int getDimunit() const;
    void setDimunit(int dimunit);
    int getDimdec() const;
    void setDimdec(int dimdec);
    int getDimtdec() const;
    void setDimtdec(int dimtdec);
    int getDimaltu() const;
    void setDimaltu(int dimaltu);
    int getDimalttd() const;
    void setDimalttd(int dimalttd);
    int getDimaunit() const;
    void setDimaunit(int dimaunit);
    int getDimfrac() const;
    void setDimfrac(int dimfrac);
    int getDimlunit() const;
    void setDimlunit(int dimlunit);
    int getDimdsep() const;
    void setDimdsep(int dimdsep);
    int getDimtmove() const;
    void setDimtmove(int dimtmove);
    int getDimjust() const;
    void setDimjust(int dimjust);
    int getDimsd1() const;
    void setDimsd1(int dimsd1);
    int getDimsd2() const;
    void setDimsd2(int dimsd2);
    int getDimtolj() const;
    void setDimtolj(int dimtolj);
    int getDimtzin() const;
    void setDimtzin(int dimtzin);
    int getDimaltz() const;
    void setDimaltz(int dimaltz);
    int getDimaltttz() const;
    void setDimaltttz(int dimaltttz);
    int getDimfit() const;
    void setDimfit(int dimfit);
    int getDimupt() const;
    void setDimupt(int dimupt);
    int getDimatfit() const;
    void setDimatfit(int dimatfit);
    int getDimfxlon() const;
    void setDimfxlon(int dimfxlon);
    const QString &getDimtxsty() const;
    void setDimtxsty(const QString &dimtxsty);
    const QString &getDimldrblk() const;
    void setDimldrblk(const QString &dimldrblk);
    int getDimlwd() const;
    void setDimlwd(int dimlwd);
    int getDimlwe() const;
    void setDimlwe(int dimlwe);
protected:
         QString name;
        //V12
        QString dimpost;       /*!< code 3 */
        QString dimapost;      /*!< code 4 */
/* handle are code 105 */
        QString dimblk;        /*!< code 5, code 342 V2000+ */
        QString dimblk1;       /*!< code 6, code 343 V2000+ */
        QString dimblk2;       /*!< code 7, code 344 V2000+ */
        double dimscale;          /*!< code 40 */
        double dimasz;            /*!< code 41 */
        double dimexo;            /*!< code 42 */
        double dimdli;            /*!< code 43 */
        double dimexe;            /*!< code 44 */
        double dimrnd;            /*!< code 45 */
        double dimdle;            /*!< code 46 */
        double dimtp;             /*!< code 47 */
        double dimtm;             /*!< code 48 */
        double dimfxl;            /*!< code 49 V2007+ */
        double dimtxt;            /*!< code 140 */
        double dimcen;            /*!< code 141 */
        double dimtsz;            /*!< code 142 */
        double dimaltf;           /*!< code 143 */
        double dimlfac;           /*!< code 144 */
        double dimtvp;            /*!< code 145 */
        double dimtfac;           /*!< code 146 */
        double dimgap;            /*!< code 147 */
        double dimaltrnd;         /*!< code 148 V2000+ */
        int dimtol;               /*!< code 71 */
        int dimlim;               /*!< code 72 */
        int dimtih;               /*!< code 73 */
        int dimtoh;               /*!< code 74 */
        int dimse1;               /*!< code 75 */
        int dimse2;               /*!< code 76 */
        int dimtad;               /*!< code 77 */
        int dimzin;               /*!< code 78 */
        int dimazin;              /*!< code 79 V2000+ */
        int dimalt;               /*!< code 170 */
        int dimaltd;              /*!< code 171 */
        int dimtofl;              /*!< code 172 */
        int dimsah;               /*!< code 173 */
        int dimtix;               /*!< code 174 */
        int dimsoxd;              /*!< code 175 */
        int dimclrd;              /*!< code 176 */
        int dimclre;              /*!< code 177 */
        int dimclrt;              /*!< code 178 */
        int dimadec;              /*!< code 179 V2000+ */
        int dimunit;              /*!< code 270 R13+ (obsolete 2000+, use dimlunit & dimfrac) */
        int dimdec;               /*!< code 271 R13+ */
        int dimtdec;              /*!< code 272 R13+ */
        int dimaltu;              /*!< code 273 R13+ */
        int dimalttd;             /*!< code 274 R13+ */
        int dimaunit;             /*!< code 275 R13+ */
        int dimfrac;              /*!< code 276 V2000+ */
        int dimlunit;             /*!< code 277 V2000+ */
        int dimdsep;              /*!< code 278 V2000+ */
        int dimtmove;             /*!< code 279 V2000+ */
        int dimjust;              /*!< code 280 R13+ */
        int dimsd1;               /*!< code 281 R13+ */
        int dimsd2;               /*!< code 282 R13+ */
        int dimtolj;              /*!< code 283 R13+ */
        int dimtzin;              /*!< code 284 R13+ */
        int dimaltz;              /*!< code 285 R13+ */
        int dimaltttz;            /*!< code 286 R13+ */
        int dimfit;               /*!< code 287 R13+  (obsolete 2000+, use dimatfit & dimtmove)*/
        int dimupt;               /*!< code 288 R13+ */
        int dimatfit;             /*!< code 289 V2000+ */
        int dimfxlon;             /*!< code 290 V2007+ */
        QString dimtxsty;      /*!< code 340 R13+ */
        QString dimldrblk;     /*!< code 341 V2000+ */
        int dimlwd;               /*!< code 371 V2000+ */
        int dimlwe;               /*!< code 372 V2000+ */
    };

#endif // LC_DIMSTYLE_H
