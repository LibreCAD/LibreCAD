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

#include "lc_dimstyle.h"

void LC_DimStyle::reset(){
    name = "";
    dimasz = dimtxt = dimexe = 0.18;
    dimexo = 0.0625;
    dimgap = dimcen = 0.09;
    dimtxsty = "Standard";
    dimscale = dimlfac = dimtfac = dimfxl = 1.0;
    dimdli = 0.38;
    dimrnd = dimdle = dimtp = dimtm = dimtsz = dimtvp = 0.0;
    dimaltf = 25.4;
    dimtol = dimlim = dimse1 = dimse2 = dimtad = dimzin = 0;
    dimtoh = dimtolj = 1;
    dimalt = dimtofl = dimsah = dimtix = dimsoxd = dimfxlon = 0;
    dimaltd = dimunit = dimaltu = dimalttd = dimlunit = 2;
    dimclrd = dimclre = dimclrt = dimjust = dimupt = 0;
    dimazin = dimaltz = dimaltttz = dimtzin = dimfrac = 0;
    dimtih = dimadec = dimaunit = dimsd1 = dimsd2 = dimtmove = 0;
    dimaltrnd = 0.0;
    dimdec = dimtdec = 4;
    dimfit = dimatfit = 3;
    dimdsep = '.';
    dimlwd = dimlwe = -2;
}

const QString &LC_DimStyle::getName() const {
    return name;
}

void LC_DimStyle::setName(const QString &name) {
    LC_DimStyle::name = name;
}

const QString &LC_DimStyle::getDimpost() const {
    return dimpost;
}

void LC_DimStyle::setDimpost(const QString &dimpost) {
    LC_DimStyle::dimpost = dimpost;
}

const QString &LC_DimStyle::getDimapost() const {
    return dimapost;
}

void LC_DimStyle::setDimapost(const QString &dimapost) {
    LC_DimStyle::dimapost = dimapost;
}

const QString &LC_DimStyle::getDimblk() const {
    return dimblk;
}

void LC_DimStyle::setDimblk(const QString &dimblk) {
    LC_DimStyle::dimblk = dimblk;
}

const QString &LC_DimStyle::getDimblk1() const {
    return dimblk1;
}

void LC_DimStyle::setDimblk1(const QString &dimblk1) {
    LC_DimStyle::dimblk1 = dimblk1;
}

const QString &LC_DimStyle::getDimblk2() const {
    return dimblk2;
}

void LC_DimStyle::setDimblk2(const QString &dimblk2) {
    LC_DimStyle::dimblk2 = dimblk2;
}

double LC_DimStyle::getDimscale() const {
    return dimscale;
}

void LC_DimStyle::setDimscale(double dimscale) {
    LC_DimStyle::dimscale = dimscale;
}

double LC_DimStyle::getDimasz() const {
    return dimasz;
}

void LC_DimStyle::setDimasz(double dimasz) {
    LC_DimStyle::dimasz = dimasz;
}

double LC_DimStyle::getDimexo() const {
    return dimexo;
}

void LC_DimStyle::setDimexo(double dimexo) {
    LC_DimStyle::dimexo = dimexo;
}

double LC_DimStyle::getDimdli() const {
    return dimdli;
}

void LC_DimStyle::setDimdli(double dimdli) {
    LC_DimStyle::dimdli = dimdli;
}

double LC_DimStyle::getDimexe() const {
    return dimexe;
}

void LC_DimStyle::setDimexe(double dimexe) {
    LC_DimStyle::dimexe = dimexe;
}

double LC_DimStyle::getDimrnd() const {
    return dimrnd;
}

void LC_DimStyle::setDimrnd(double dimrnd) {
    LC_DimStyle::dimrnd = dimrnd;
}

double LC_DimStyle::getDimdle() const {
    return dimdle;
}

void LC_DimStyle::setDimdle(double dimdle) {
    LC_DimStyle::dimdle = dimdle;
}

double LC_DimStyle::getDimtp() const {
    return dimtp;
}

void LC_DimStyle::setDimtp(double dimtp) {
    LC_DimStyle::dimtp = dimtp;
}

double LC_DimStyle::getDimtm() const {
    return dimtm;
}

void LC_DimStyle::setDimtm(double dimtm) {
    LC_DimStyle::dimtm = dimtm;
}

double LC_DimStyle::getDimfxl() const {
    return dimfxl;
}

void LC_DimStyle::setDimfxl(double dimfxl) {
    LC_DimStyle::dimfxl = dimfxl;
}

double LC_DimStyle::getDimtxt() const {
    return dimtxt;
}

void LC_DimStyle::setDimtxt(double dimtxt) {
    LC_DimStyle::dimtxt = dimtxt;
}

double LC_DimStyle::getDimcen() const {
    return dimcen;
}

void LC_DimStyle::setDimcen(double dimcen) {
    LC_DimStyle::dimcen = dimcen;
}

double LC_DimStyle::getDimtsz() const {
    return dimtsz;
}

void LC_DimStyle::setDimtsz(double dimtsz) {
    LC_DimStyle::dimtsz = dimtsz;
}

double LC_DimStyle::getDimaltf() const {
    return dimaltf;
}

void LC_DimStyle::setDimaltf(double dimaltf) {
    LC_DimStyle::dimaltf = dimaltf;
}

double LC_DimStyle::getDimlfac() const {
    return dimlfac;
}

void LC_DimStyle::setDimlfac(double dimlfac) {
    LC_DimStyle::dimlfac = dimlfac;
}

double LC_DimStyle::getDimtvp() const {
    return dimtvp;
}

void LC_DimStyle::setDimtvp(double dimtvp) {
    LC_DimStyle::dimtvp = dimtvp;
}

double LC_DimStyle::getDimtfac() const {
    return dimtfac;
}

void LC_DimStyle::setDimtfac(double dimtfac) {
    LC_DimStyle::dimtfac = dimtfac;
}

double LC_DimStyle::getDimgap() const {
    return dimgap;
}

void LC_DimStyle::setDimgap(double dimgap) {
    LC_DimStyle::dimgap = dimgap;
}

double LC_DimStyle::getDimaltrnd() const {
    return dimaltrnd;
}

void LC_DimStyle::setDimaltrnd(double dimaltrnd) {
    LC_DimStyle::dimaltrnd = dimaltrnd;
}

int LC_DimStyle::getDimtol() const {
    return dimtol;
}

void LC_DimStyle::setDimtol(int dimtol) {
    LC_DimStyle::dimtol = dimtol;
}

int LC_DimStyle::getDimlim() const {
    return dimlim;
}

void LC_DimStyle::setDimlim(int dimlim) {
    LC_DimStyle::dimlim = dimlim;
}

int LC_DimStyle::getDimtih() const {
    return dimtih;
}

void LC_DimStyle::setDimtih(int dimtih) {
    LC_DimStyle::dimtih = dimtih;
}

int LC_DimStyle::getDimtoh() const {
    return dimtoh;
}

void LC_DimStyle::setDimtoh(int dimtoh) {
    LC_DimStyle::dimtoh = dimtoh;
}

int LC_DimStyle::getDimse1() const {
    return dimse1;
}

void LC_DimStyle::setDimse1(int dimse1) {
    LC_DimStyle::dimse1 = dimse1;
}

int LC_DimStyle::getDimse2() const {
    return dimse2;
}

void LC_DimStyle::setDimse2(int dimse2) {
    LC_DimStyle::dimse2 = dimse2;
}

int LC_DimStyle::getDimtad() const {
    return dimtad;
}

void LC_DimStyle::setDimtad(int dimtad) {
    LC_DimStyle::dimtad = dimtad;
}

int LC_DimStyle::getDimzin() const {
    return dimzin;
}

void LC_DimStyle::setDimzin(int dimzin) {
    LC_DimStyle::dimzin = dimzin;
}

int LC_DimStyle::getDimazin() const {
    return dimazin;
}

void LC_DimStyle::setDimazin(int dimazin) {
    LC_DimStyle::dimazin = dimazin;
}

int LC_DimStyle::getDimalt() const {
    return dimalt;
}

void LC_DimStyle::setDimalt(int dimalt) {
    LC_DimStyle::dimalt = dimalt;
}

int LC_DimStyle::getDimaltd() const {
    return dimaltd;
}

void LC_DimStyle::setDimaltd(int dimaltd) {
    LC_DimStyle::dimaltd = dimaltd;
}

int LC_DimStyle::getDimtofl() const {
    return dimtofl;
}

void LC_DimStyle::setDimtofl(int dimtofl) {
    LC_DimStyle::dimtofl = dimtofl;
}

int LC_DimStyle::getDimsah() const {
    return dimsah;
}

void LC_DimStyle::setDimsah(int dimsah) {
    LC_DimStyle::dimsah = dimsah;
}

int LC_DimStyle::getDimtix() const {
    return dimtix;
}

void LC_DimStyle::setDimtix(int dimtix) {
    LC_DimStyle::dimtix = dimtix;
}

int LC_DimStyle::getDimsoxd() const {
    return dimsoxd;
}

void LC_DimStyle::setDimsoxd(int dimsoxd) {
    LC_DimStyle::dimsoxd = dimsoxd;
}

int LC_DimStyle::getDimclrd() const {
    return dimclrd;
}

void LC_DimStyle::setDimclrd(int dimclrd) {
    LC_DimStyle::dimclrd = dimclrd;
}

int LC_DimStyle::getDimclre() const {
    return dimclre;
}

void LC_DimStyle::setDimclre(int dimclre) {
    LC_DimStyle::dimclre = dimclre;
}

int LC_DimStyle::getDimclrt() const {
    return dimclrt;
}

void LC_DimStyle::setDimclrt(int dimclrt) {
    LC_DimStyle::dimclrt = dimclrt;
}

int LC_DimStyle::getDimadec() const {
    return dimadec;
}

void LC_DimStyle::setDimadec(int dimadec) {
    LC_DimStyle::dimadec = dimadec;
}

int LC_DimStyle::getDimunit() const {
    return dimunit;
}

void LC_DimStyle::setDimunit(int dimunit) {
    LC_DimStyle::dimunit = dimunit;
}

int LC_DimStyle::getDimdec() const {
    return dimdec;
}

void LC_DimStyle::setDimdec(int dimdec) {
    LC_DimStyle::dimdec = dimdec;
}

int LC_DimStyle::getDimtdec() const {
    return dimtdec;
}

void LC_DimStyle::setDimtdec(int dimtdec) {
    LC_DimStyle::dimtdec = dimtdec;
}

int LC_DimStyle::getDimaltu() const {
    return dimaltu;
}

void LC_DimStyle::setDimaltu(int dimaltu) {
    LC_DimStyle::dimaltu = dimaltu;
}

int LC_DimStyle::getDimalttd() const {
    return dimalttd;
}

void LC_DimStyle::setDimalttd(int dimalttd) {
    LC_DimStyle::dimalttd = dimalttd;
}

int LC_DimStyle::getDimaunit() const {
    return dimaunit;
}

void LC_DimStyle::setDimaunit(int dimaunit) {
    LC_DimStyle::dimaunit = dimaunit;
}

int LC_DimStyle::getDimfrac() const {
    return dimfrac;
}

void LC_DimStyle::setDimfrac(int dimfrac) {
    LC_DimStyle::dimfrac = dimfrac;
}

int LC_DimStyle::getDimlunit() const {
    return dimlunit;
}

void LC_DimStyle::setDimlunit(int dimlunit) {
    LC_DimStyle::dimlunit = dimlunit;
}

int LC_DimStyle::getDimdsep() const {
    return dimdsep;
}

void LC_DimStyle::setDimdsep(int dimdsep) {
    LC_DimStyle::dimdsep = dimdsep;
}

int LC_DimStyle::getDimtmove() const {
    return dimtmove;
}

void LC_DimStyle::setDimtmove(int dimtmove) {
    LC_DimStyle::dimtmove = dimtmove;
}

int LC_DimStyle::getDimjust() const {
    return dimjust;
}

void LC_DimStyle::setDimjust(int dimjust) {
    LC_DimStyle::dimjust = dimjust;
}

int LC_DimStyle::getDimsd1() const {
    return dimsd1;
}

void LC_DimStyle::setDimsd1(int dimsd1) {
    LC_DimStyle::dimsd1 = dimsd1;
}

int LC_DimStyle::getDimsd2() const {
    return dimsd2;
}

void LC_DimStyle::setDimsd2(int dimsd2) {
    LC_DimStyle::dimsd2 = dimsd2;
}

int LC_DimStyle::getDimtolj() const {
    return dimtolj;
}

void LC_DimStyle::setDimtolj(int dimtolj) {
    LC_DimStyle::dimtolj = dimtolj;
}

int LC_DimStyle::getDimtzin() const {
    return dimtzin;
}

void LC_DimStyle::setDimtzin(int dimtzin) {
    LC_DimStyle::dimtzin = dimtzin;
}

int LC_DimStyle::getDimaltz() const {
    return dimaltz;
}

void LC_DimStyle::setDimaltz(int dimaltz) {
    LC_DimStyle::dimaltz = dimaltz;
}

int LC_DimStyle::getDimaltttz() const {
    return dimaltttz;
}

void LC_DimStyle::setDimaltttz(int dimaltttz) {
    LC_DimStyle::dimaltttz = dimaltttz;
}

int LC_DimStyle::getDimfit() const {
    return dimfit;
}

void LC_DimStyle::setDimfit(int dimfit) {
    LC_DimStyle::dimfit = dimfit;
}

int LC_DimStyle::getDimupt() const {
    return dimupt;
}

void LC_DimStyle::setDimupt(int dimupt) {
    LC_DimStyle::dimupt = dimupt;
}

int LC_DimStyle::getDimatfit() const {
    return dimatfit;
}

void LC_DimStyle::setDimatfit(int dimatfit) {
    LC_DimStyle::dimatfit = dimatfit;
}

int LC_DimStyle::getDimfxlon() const {
    return dimfxlon;
}

void LC_DimStyle::setDimfxlon(int dimfxlon) {
    LC_DimStyle::dimfxlon = dimfxlon;
}

const QString &LC_DimStyle::getDimtxsty() const {
    return dimtxsty;
}

void LC_DimStyle::setDimtxsty(const QString &dimtxsty) {
    LC_DimStyle::dimtxsty = dimtxsty;
}

const QString &LC_DimStyle::getDimldrblk() const {
    return dimldrblk;
}

void LC_DimStyle::setDimldrblk(const QString &dimldrblk) {
    LC_DimStyle::dimldrblk = dimldrblk;
}

int LC_DimStyle::getDimlwd() const {
    return dimlwd;
}

void LC_DimStyle::setDimlwd(int dimlwd) {
    LC_DimStyle::dimlwd = dimlwd;
}

int LC_DimStyle::getDimlwe() const {
    return dimlwe;
}

void LC_DimStyle::setDimlwe(int dimlwe) {
    LC_DimStyle::dimlwe = dimlwe;
}
