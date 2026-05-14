/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2016-2022 A. Stebich (librecad@mail.lordofbikes.de)        **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <vector>
#include "drw_entities.h"
#include "intern/dxfreader.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_textcodec.h"
#include "intern/drw_dbg.h"
#include "intern/drw_reserve.h"

//! Calculate arbitrary axis
/*!
*   Calculate arbitrary axis for apply extrusions
*  @author Rallaz
*/
void DRW_Entity::calculateAxis(DRW_Coord extPoint){
    //Follow the arbitrary DXF definitions for extrusion axes.
    if (fabs(extPoint.x) < 0.015625 && fabs(extPoint.y) < 0.015625) {
        //If we get here, implement Ax = Wy x N where Wy is [0,1,0] per the DXF spec.
        //The cross product works out to Wy.y*N.z-Wy.z*N.y, Wy.z*N.x-Wy.x*N.z, Wy.x*N.y-Wy.y*N.x
        //Factoring in the fixed values for Wy gives N.z,0,-N.x
        extAxisX.x = extPoint.z;
        extAxisX.y = 0;
        extAxisX.z = -extPoint.x;
    } else {
        //Otherwise, implement Ax = Wz x N where Wz is [0,0,1] per the DXF spec.
        //The cross product works out to Wz.y*N.z-Wz.z*N.y, Wz.z*N.x-Wz.x*N.z, Wz.x*N.y-Wz.y*N.x
        //Factoring in the fixed values for Wz gives -N.y,N.x,0.
        extAxisX.x = -extPoint.y;
        extAxisX.y = extPoint.x;
        extAxisX.z = 0;
    }

    extAxisX.unitize();

    //Ay = N x Ax
    extAxisY.x = (extPoint.y * extAxisX.z) - (extAxisX.y * extPoint.z);
    extAxisY.y = (extPoint.z * extAxisX.x) - (extAxisX.z * extPoint.x);
    extAxisY.z = (extPoint.x * extAxisX.y) - (extAxisX.x * extPoint.y);

    extAxisY.unitize();
}

//! Extrude a point using arbitrary axis
/*!
*   apply extrusion in a point using arbitrary axis (previous calculated)
*  @author Rallaz
*/
void DRW_Entity::extrudePoint(DRW_Coord extPoint, DRW_Coord *point){
    double px, py, pz;
    px = (extAxisX.x*point->x)+(extAxisY.x*point->y)+(extPoint.x*point->z);
    py = (extAxisX.y*point->x)+(extAxisY.y*point->y)+(extPoint.y*point->z);
    pz = (extAxisX.z*point->x)+(extAxisY.z*point->y)+(extPoint.z*point->z);

    point->x = px;
    point->y = py;
    point->z = pz;
}

bool DRW_Entity::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 5:
        handle = reader->getHandleString();
        break;
    case 330:
        parentHandle = reader->getHandleString();
        break;
    case 8:
        layer = reader->getUtf8String();
        break;
    case 6:
        lineType = reader->getUtf8String();
        break;
    case 62:
        color = reader->getInt32();
        break;
    case 370:
        lWeight = DRW_LW_Conv::dxfInt2lineWidth(reader->getInt32());
        break;
    case 48:
        ltypeScale = reader->getDouble();
        break;
    case 60:
        visible = reader->getBool();
        break;
    case 420:
        color24 = reader->getInt32();
        break;
    case 430:
        colorName = reader->getString();
        break;
    case 67:
        space = static_cast<DRW::Space>(reader->getInt32());
        break;
    case 102:
        parseDxfGroups(code, reader);
        break;
    case 284:
        shadow = static_cast<DRW::ShadowMode>(reader->getInt32() & 0x3);
        break;
    case 347:
        material = static_cast<duint32>(reader->getHandleString());
        break;
    case 390:
        plotStyle = reader->getHandleString();
        break;
    case 440:
        transparency = reader->getInt32();
        break;
    case 1000:
    case 1001:
    case 1002:
    case 1003:
    case 1004:
    case 1005:
		extData.push_back(std::make_shared<DRW_Variant>(code, reader->getString()));
        break;
    case 1010:
    case 1011:
    case 1012:
    case 1013:
		curr =std::make_shared<DRW_Variant>(code, DRW_Coord(reader->getDouble(), 0.0, 0.0));
        extData.push_back(curr);
        break;
    case 1020:
    case 1021:
    case 1022:
    case 1023:
        if (curr)
            curr->setCoordY(reader->getDouble());
        break;
    case 1030:
    case 1031:
    case 1032:
    case 1033:
        if (curr)
            curr->setCoordZ(reader->getDouble());
		//FIXME, why do we discard curr right after setting the its Z
//        curr=NULL;
        break;
    case 1040:
    case 1041:
    case 1042:
		extData.push_back(std::make_shared<DRW_Variant>(code, reader->getDouble() ));
        break;
    case 1070:
    case 1071:
		extData.push_back(std::make_shared<DRW_Variant>(code, reader->getInt32() ));
        break;
    default:
        break;
    }
    return true;
}

//parses dxf 102 groups to read entity
bool DRW_Entity::parseDxfGroups(int code, const std::unique_ptr<dxfReader>& reader){
    std::list<DRW_Variant> ls;
    DRW_Variant curr;
    int nc;
    std::string appName= reader->getString();
    if (!appName.empty() && appName.at(0)== '{'){
        curr.addString(code, appName.substr(1, (int) appName.size()-1));
        ls.push_back(curr);
        while (code !=102 && appName.at(0)== '}'){
            reader->readRec(&nc);//RLZ curr.code = code or nc?
//            curr.code = code;
            //RLZ code == 330 || code == 360 OR nc == 330 || nc == 360 ?
            if (code == 330 || code == 360)
                curr.addInt(code, reader->getHandleString());//RLZ code or nc
            else {
                switch (reader->type) {
                case dxfReader::STRING:
                    curr.addString(code, reader->getString());//RLZ code or nc
                    break;
                case dxfReader::INT32:
                case dxfReader::INT64:
                    curr.addInt(code, reader->getInt32());//RLZ code or nc
                    break;
                case dxfReader::DOUBLE:
                    curr.addDouble(code, reader->getDouble());//RLZ code or nc
                    break;
                case dxfReader::BOOL:
                    curr.addInt(code, reader->getInt32());//RLZ code or nc
                    break;
                default:
                    break;
                }
            }
            ls.push_back(curr);
        }
    }

    appData.push_back(ls);
    return true;
}

bool DRW_Entity::parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer* strBuf, duint32 bs){
    objSize=0;
    DRW_DBG("\n***************************** parsing entity *********************************************\n");
    oType = buf->getObjType(version);
    DRW_DBG("Object type: "); DRW_DBG(oType); DRW_DBG(", "); DRW_DBGH(oType);

    if (version > DRW::AC1014 && version < DRW::AC1024) {//2000 & 2004
        objSize = buf->getRawLong32();  //RL 32bits object size in bits
        DRW_DBG(" Object size: "); DRW_DBG(objSize); DRW_DBG("\n");
    }
    if (version > DRW::AC1021) {//2010+
        duint32 ms = buf->size();
        objSize = ms*8 - bs;
        DRW_DBG(" Object size: "); DRW_DBG(objSize); DRW_DBG("\n");
    }

    if (strBuf != NULL && version > DRW::AC1018) {//2007+
        strBuf->moveBitPos(objSize-1);
        DRW_DBG(" strBuf strbit pos 2007: "); DRW_DBG(strBuf->getPosition()); DRW_DBG(" strBuf bpos 2007: "); DRW_DBG(strBuf->getBitPos()); DRW_DBG("\n");
        if (strBuf->getBit() == 1){
            DRW_DBG("DRW_TableEntry::parseDwg string bit is 1\n");
            strBuf->moveBitPos(-17);
            duint16 strDataSize = strBuf->getRawShort16();
            DRW_DBG("\nDRW_TableEntry::parseDwg string strDataSize: "); DRW_DBGH(strDataSize); DRW_DBG("\n");
            if ( (strDataSize& 0x8000) == 0x8000){
                DRW_DBG("\nDRW_TableEntry::parseDwg string 0x8000 bit is set");
                strBuf->moveBitPos(-33);//RLZ pending to verify
                duint16 hiSize = strBuf->getRawShort16();
                strDataSize = ((strDataSize&0x7fff) | (hiSize<<15));
            }
            strBuf->moveBitPos( -strDataSize -16); //-14
            DRW_DBG("strBuf start strDataSize pos 2007: "); DRW_DBG(strBuf->getPosition()); DRW_DBG(" strBuf bpos 2007: "); DRW_DBG(strBuf->getBitPos()); DRW_DBG("\n");
        } else
            DRW_DBG("\nDRW_TableEntry::parseDwg string bit is 0");
        DRW_DBG("strBuf start pos 2007: "); DRW_DBG(strBuf->getPosition()); DRW_DBG(" strBuf bpos 2007: "); DRW_DBG(strBuf->getBitPos()); DRW_DBG("\n");
    }

    dwgHandle ho = buf->getHandle();
    handle = ho.ref;
    DRW_DBG("Entity Handle: "); DRW_DBGHL(ho.code, ho.size, ho.ref);
    // ODA DWG spec §28 "Extended Entity Data". The outer loop yields one
    // BS-prefixed byte chunk per APPID-attached group; size==0 terminates.
    // Each chunk's payload is a sequence of (1-byte type code + value)
    // items; we walk it with a nested loop and push DRW_Variant entries
    // into @ref extData. Handle-typed items (type 3 layer-ref, type 5
    // entity-ref) and the per-chunk APPID handle are resolved post-hoc
    // in dwgReader::parseAttribs once the symbol tables are available.
    dint16 extDataSize = buf->getBitShort(); //BS
    DRW_DBG(" ext data size: "); DRW_DBG(extDataSize);
    while (extDataSize>0 && buf->isGood()) {
        dwgHandle ah = buf->getHandle();
        DRW_DBG("App Handle: "); DRW_DBGHL(ah.code, ah.size, ah.ref);
        duint8 *tmpExtData = new duint8[extDataSize];
        buf->getBytes(tmpExtData, extDataSize);
        dwgBuffer tmpExtDataBuf(tmpExtData, extDataSize, buf->decoder);

        // Placeholder for the APPID name (DXF group 1001). Filled in by
        // parseAttribs from appIdmap; falls back to ACAD_<hex> if unknown.
        extData.push_back(std::make_shared<DRW_Variant>(1001, std::string{}));
        pendingAppIdResolutions.push_back({extData.size() - 1, ah.ref});

        while (tmpExtDataBuf.numRemainingBytes() > 0 && tmpExtDataBuf.isGood()) {
            duint8 dxfCode = tmpExtDataBuf.getRawChar8();
            DRW_DBG(" eed type: "); DRW_DBG(dxfCode);
            switch (dxfCode){
            case 0: { //string
                std::string s;
                if (version > DRW::AC1018) { //R2007+: 2-byte char count + UTF-16LE
                    if (tmpExtDataBuf.numRemainingBytes() < 2) break;
                    duint16 nChars = tmpExtDataBuf.getRawShort16();
                    if (nChars > 0) {
                        duint64 byteLen = static_cast<duint64>(nChars) * 2;
                        if ((duint64)tmpExtDataBuf.numRemainingBytes() < byteLen) break;
                        std::vector<duint8> bytes(byteLen);
                        tmpExtDataBuf.getBytes(bytes.data(), byteLen);
                        // Inline UTF-16LE → UTF-8 conversion.
                        for (duint16 i = 0; i < nChars; ++i) {
                            duint16 c = static_cast<duint16>(bytes[2*i]) |
                                       (static_cast<duint16>(bytes[2*i+1]) << 8);
                            if (c < 0x80) {
                                s.push_back(static_cast<char>(c));
                            } else if (c < 0x800) {
                                s.push_back(static_cast<char>(0xC0 | (c >> 6)));
                                s.push_back(static_cast<char>(0x80 | (c & 0x3F)));
                            } else {
                                s.push_back(static_cast<char>(0xE0 | (c >> 12)));
                                s.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
                                s.push_back(static_cast<char>(0x80 | (c & 0x3F)));
                            }
                        }
                    }
                } else { //R13–R2004: 1-byte len + 2-byte BE codepage hint + bytes (+NUL)
                    if (tmpExtDataBuf.numRemainingBytes() < 3) break;
                    duint8 strLength = tmpExtDataBuf.getRawChar8();
                    duint16 cp = tmpExtDataBuf.getBERawShort16();
                    (void)cp; //per-string codepage hint dropped; file-level codec used
                    if (strLength > 0 && tmpExtDataBuf.numRemainingBytes() >= strLength) {
                        std::string raw(strLength, '\0');
                        tmpExtDataBuf.getBytes(reinterpret_cast<duint8*>(&raw[0]), strLength);
                        s = tmpExtDataBuf.decoder ? tmpExtDataBuf.decoder->toUtf8(raw) : raw;
                    }
                    //consume the optional trailing NUL terminator if present
                    if (tmpExtDataBuf.numRemainingBytes() > 0) {
                        tmpExtDataBuf.getRawChar8();
                    }
                }
                extData.push_back(std::make_shared<DRW_Variant>(1000, s));
                break;
            }
            case 2: { //control character: 0 = '{', 1 = '}'
                if (tmpExtDataBuf.numRemainingBytes() < 1) break;
                duint8 ctrl = tmpExtDataBuf.getRawChar8();
                extData.push_back(std::make_shared<DRW_Variant>(
                    1002, std::string(ctrl == 0 ? "{" : "}")));
                break;
            }
            case 3: { //layer-table reference (8 raw BE bytes -> handle)
                if (tmpExtDataBuf.numRemainingBytes() < 8) break;
                duint8 hb[8];
                tmpExtDataBuf.getBytes(hb, 8);
                duint64 ref = 0;
                for (int i = 0; i < 8; ++i) {
                    ref = (ref << 8) | hb[i];
                }
                // Placeholder layer-ref string; resolved post-hoc.
                extData.push_back(std::make_shared<DRW_Variant>(
                    1003, std::string{}, /*isLayerRef=*/true));
                pendingLayerRefResolutions.push_back(
                    {extData.size() - 1, static_cast<duint32>(ref)});
                break;
            }
            case 4: { //binary chunk: 1-byte length + bytes
                if (tmpExtDataBuf.numRemainingBytes() < 1) break;
                duint8 binLen = tmpExtDataBuf.getRawChar8();
                std::vector<duint8> bytes(binLen);
                if (binLen > 0 && tmpExtDataBuf.numRemainingBytes() >= binLen) {
                    tmpExtDataBuf.getBytes(bytes.data(), binLen);
                }
                extData.push_back(std::make_shared<DRW_Variant>(1004, std::move(bytes)));
                break;
            }
            case 5: { //entity-handle reference (8 raw BE bytes -> hex string)
                if (tmpExtDataBuf.numRemainingBytes() < 8) break;
                duint8 hb[8];
                tmpExtDataBuf.getBytes(hb, 8);
                duint64 ref = 0;
                for (int i = 0; i < 8; ++i) {
                    ref = (ref << 8) | hb[i];
                }
                char tmp[24];
                std::snprintf(tmp, sizeof(tmp), "%llX",
                              static_cast<unsigned long long>(ref));
                extData.push_back(std::make_shared<DRW_Variant>(1005, std::string{tmp}));
                break;
            }
            case 10: case 11: case 12: case 13: { //3-double point
                if (tmpExtDataBuf.numRemainingBytes() < 24) break;
                DRW_Coord c;
                c.x = tmpExtDataBuf.getRawDouble();
                c.y = tmpExtDataBuf.getRawDouble();
                c.z = tmpExtDataBuf.getRawDouble();
                extData.push_back(std::make_shared<DRW_Variant>(1000 + dxfCode, c));
                break;
            }
            case 40: case 41: case 42: { //real
                if (tmpExtDataBuf.numRemainingBytes() < 8) break;
                double d = tmpExtDataBuf.getRawDouble();
                extData.push_back(std::make_shared<DRW_Variant>(1000 + dxfCode, d));
                break;
            }
            case 70: { //int16
                if (tmpExtDataBuf.numRemainingBytes() < 2) break;
                dint16 i = static_cast<dint16>(tmpExtDataBuf.getRawShort16());
                extData.push_back(std::make_shared<DRW_Variant>(1070, static_cast<dint32>(i)));
                break;
            }
            case 71: { //int32
                if (tmpExtDataBuf.numRemainingBytes() < 4) break;
                dint32 i = static_cast<dint32>(tmpExtDataBuf.getRawLong32());
                extData.push_back(std::make_shared<DRW_Variant>(1071, i));
                break;
            }
            default:
                DRW_DBG(" unknown EED type: "); DRW_DBG(dxfCode); DRW_DBG("\n");
                // Unknown type — bail on this app's chunk; we cannot
                // know how many bytes the rest of the item occupies.
                tmpExtDataBuf.setPosition(tmpExtDataBuf.size());
                break;
            }
        }
        delete[]tmpExtData;
        extDataSize = buf->getBitShort(); //BS
        DRW_DBG(" ext data size: "); DRW_DBG(extDataSize);
    } //end parsing extData (EED)
    DRW_DBG(" [bidi-debug pre-graphFlag bufpos="); DRW_DBG(buf->getPosition()); DRW_DBG(" bitpos="); DRW_DBG(buf->getBitPos()); DRW_DBG("]\n");
    duint8 graphFlag = buf->getBit(); //B
    DRW_DBG(" graphFlag: "); DRW_DBG(graphFlag); DRW_DBG("\n");
    if (graphFlag) {
        DRW_DBG(" [bidi-debug pre-graphSize bufpos="); DRW_DBG(buf->getPosition()); DRW_DBG(" bitpos="); DRW_DBG(buf->getBitPos()); DRW_DBG("]\n");
        duint32 graphDataSize = buf->getRawLong32();  //RL 32bits
        DRW_DBG("graphData in bytes: "); DRW_DBG(graphDataSize); DRW_DBG("\n");
// RLZ: TODO
        //skip graphData bytes
        duint8 *tmpGraphData = new duint8[graphDataSize];
        buf->getBytes(tmpGraphData, graphDataSize);
        dwgBuffer tmpGraphDataBuf(tmpGraphData, graphDataSize, buf->decoder);
        DRW_DBG("graph data remaining bytes: "); DRW_DBG(tmpGraphDataBuf.numRemainingBytes()); DRW_DBG("\n");
        delete[]tmpGraphData;
    }
    if (version < DRW::AC1015) {//14-
        objSize = buf->getRawLong32();  //RL 32bits object size in bits
        DRW_DBG(" Object size in bits: "); DRW_DBG(objSize); DRW_DBG("\n");
    }

    duint8 entmode = buf->get2Bits(); //BB
    if (entmode == 0)
        ownerHandle= true;
//        entmode = 2;
    else if(entmode ==2)
        entmode = 0;
    space = (DRW::Space)entmode; //RLZ verify cast values
    DRW_DBG("entmode: "); DRW_DBG(entmode);
    numReactors = buf->getBitShort(); //BS
    DRW_DBG(", numReactors: "); DRW_DBG(numReactors);

    if (version < DRW::AC1015) {//14-
        if(buf->getBit()) {//is bylayer line type
            lineType = "BYLAYER";
            ltFlags = 0;
        } else {
            lineType = "";
            ltFlags = 3;
        }
        DRW_DBG(" lineType: "); DRW_DBG(lineType.c_str());
        DRW_DBG(" ltFlags: "); DRW_DBG(ltFlags);
    }
    if (version > DRW::AC1015) {//2004+
        xDictFlag = buf->getBit();
        DRW_DBG(" xDictFlag: "); DRW_DBG(xDictFlag); DRW_DBG("\n");
    }

    if (version > DRW::AC1024 || version < DRW::AC1018) {
        haveNextLinks = buf->getBit(); //aka nolinks //B
        DRW_DBG(", haveNextLinks (0 yes, 1 prev next): "); DRW_DBG(haveNextLinks); DRW_DBG("\n");
    } else {
        haveNextLinks = 1; //aka nolinks //B
        DRW_DBG(", haveNextLinks (forced): "); DRW_DBG(haveNextLinks); DRW_DBG("\n");
    }
//ENC color
    color = buf->getEnColor(version); //BS or CMC //ok for R14 or negate
    // Capture the AcDbColor side-channel BEFORE any subsequent ENC read.
    // libreDWG common_entity_data.spec:454-459 — the corresponding handle
    // is consumed at the start of the handle stream in parseDwgEntHandle.
    hasAcDbColorH = buf->lastEnColorHadDbColorRef;
    // libreDWG common_entity_data.spec:432-453 — ENC alpha_raw (DXF code
    // 440) is encoded as (alpha_type<<24) | alpha. Stored verbatim; the
    // filter (RS_FilterDXFRW::setEntityAttributes) decodes alpha_type==3
    // into a per-entity pen alpha, otherwise inherits from layer/block.
    if (buf->lastEnColorAlphaRaw != 0) {
        transparency = static_cast<int>(buf->lastEnColorAlphaRaw);
    }
    // libreDWG common_entity_data.spec:468-475 — inline TV name/book name
    // (flags 0x41/0x42) override any dbColorMap-resolved name. Captured
    // immediately; entryParse will skip the override only if colorName is
    // already populated here.
    if (!buf->lastEnColorName.empty()) {
        colorName = buf->lastEnColorBookName.empty()
            ? buf->lastEnColorName
            : (buf->lastEnColorBookName + "$" + buf->lastEnColorName);
    }
    ltypeScale = buf->getBitDouble(); //BD
    DRW_DBG(" entity color: "); DRW_DBG(color);
    DRW_DBG(" ltScale: "); DRW_DBG(ltypeScale); DRW_DBG("\n");
    if (version > DRW::AC1014) {//2000+ — §19.4.1: linetype-flags BB then plot-flags BB
        ltFlags = buf->get2Bits(); //BB
        if (ltFlags == 0)      lineType = "BYLAYER";
        else if (ltFlags == 1) lineType = "BYBLOCK";
        else if (ltFlags == 2) lineType = "CONTINUOUS";
        else                   lineType = ""; //3 → handle at end
        DRW_DBG("ltFlags: "); DRW_DBG(ltFlags);
        DRW_DBG(" lineType: "); DRW_DBG(lineType.c_str());

        plotFlags = buf->get2Bits(); //BB
        DRW_DBG(", plotFlags: "); DRW_DBG(plotFlags);
    }
    if (version > DRW::AC1018) {//2007+
        materialFlag = buf->get2Bits(); //BB
        DRW_DBG("materialFlag: "); DRW_DBG(materialFlag);
        shadowFlag = buf->getRawChar8(); //RC, low 2 bits is shadow mode 0..3
        DRW_DBG("shadowFlag: "); DRW_DBG(shadowFlag); DRW_DBG("\n");
        shadow = static_cast<DRW::ShadowMode>(shadowFlag & 0x3);
    }
    if (version > DRW::AC1021) {//2010+ — §19.4.1: three single-bit flags
        // Ground-truth: libreDWG common_entity_data.spec lines 523-528
        // and ODA spec v5.4.1 §19.4.1 both define three FIELD_B (single bit)
        // flags here, one each for full/face/edge visual style. Total bit
        // consumption (3 bits) is identical to the historical BB+B shape;
        // only the semantics differ. The corresponding handles are read
        // conditionally in parseDwgEntHandle after the plotstyle handle.
        hasFullVisualStyle = buf->getBit(); //B
        hasFaceVisualStyle = buf->getBit(); //B
        hasEdgeVisualStyle = buf->getBit(); //B
        DRW_DBG("hasFull/Face/Edge VisualStyle: ");
        DRW_DBG(hasFullVisualStyle); DRW_DBG(" ");
        DRW_DBG(hasFaceVisualStyle); DRW_DBG(" ");
        DRW_DBG(hasEdgeVisualStyle); DRW_DBG("\n");
    }
    dint16 invisibleFlag = buf->getBitShort(); //BS
    DRW_DBG(" invisibleFlag: "); DRW_DBG(invisibleFlag);
    if (version > DRW::AC1014) {//2000+
        lWeight = DRW_LW_Conv::dwgInt2lineWidth( buf->getRawChar8() ); //RC
        DRW_DBG(" lwFlag (lWeight): "); DRW_DBG(lWeight); DRW_DBG("\n");
    }
    //Only in blocks ????????
//    if (version > DRW::AC1018) {//2007+
//        duint8 unk = buf->getBit();
//        DRW_DBG("unknown bit: "); DRW_DBG(unk); DRW_DBG("\n");
//    }
    return buf->isGood();
}

bool DRW_Entity::parseDwgEntHandle(DRW::Version version, dwgBuffer *buf){
    if (version > DRW::AC1018) {//2007+ skip string area
        buf->setPosition(objSize >> 3);
        buf->setBitPos(objSize & 7);
    }

    // libreDWG common_entity_data.spec:454-459: when ENC flag 0x40 is set,
    // an AcDbColor reference handle is the FIRST item in the handle stream
    // — read before owner / reactors / xdic / etc.  Set in parseDwg via
    // dwgBuffer::lastEnColorHadDbColorRef. The dwgReader resolves this
    // handle against dbColorMap after parseDwg returns and patches
    // color24 + colorName onto the entity.
    if (hasAcDbColorH && version > DRW::AC1015 && buf->numRemainingBytes() >= 4) {
        dwgHandle dbcH = buf->getOffsetHandle(handle);
        acDbColorHandle = dbcH.ref;
        DRW_DBG(" AcDbColor Handle: ");
        DRW_DBGHL(dbcH.code, dbcH.size, dbcH.ref); DRW_DBG("\n");
    }

    if(ownerHandle){//entity are in block or in a polyline
        dwgHandle ownerH = buf->getOffsetHandle(handle);
        DRW_DBG("owner (parent) Handle: "); DRW_DBGHL(ownerH.code, ownerH.size, ownerH.ref); DRW_DBG("\n");
        DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        parentHandle = ownerH.ref;
        DRW_DBG("Block (parent) Handle: "); DRW_DBGHL(ownerH.code, ownerH.size, parentHandle); DRW_DBG("\n");
    } else
        DRW_DBG("NO Block (parent) Handle\n");

    DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    for (int i=0; i< numReactors;++i) {
        dwgHandle reactorsH = buf->getHandle();
        DRW_DBG(" reactorsH control Handle: "); DRW_DBGHL(reactorsH.code, reactorsH.size, reactorsH.ref); DRW_DBG("\n");
    }
    if (xDictFlag !=1){//linetype in 2004 seems not have XDicObjH or NULL handle
        dwgHandle XDicObjH = buf->getHandle();
        DRW_DBG(" XDicObj control Handle: "); DRW_DBGHL(XDicObjH.code, XDicObjH.size, XDicObjH.ref); DRW_DBG("\n");
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    if (version < DRW::AC1015) {//R14-
        //layer handle
        layerH = buf->getOffsetHandle(handle);
        DRW_DBG(" layer Handle: "); DRW_DBGHL(layerH.code, layerH.size, layerH.ref); DRW_DBG("\n");
        DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        //lineType handle
        if(ltFlags == 3){
            lTypeH = buf->getOffsetHandle(handle);
            DRW_DBG("linetype Handle: "); DRW_DBGHL(lTypeH.code, lTypeH.size, lTypeH.ref); DRW_DBG("\n");
            DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        }
    }
    if (version < DRW::AC1018) {//2000+
        if (haveNextLinks == 0) {
            dwgHandle nextLinkH = buf->getOffsetHandle(handle);
            DRW_DBG(" prev nextLinkers Handle: "); DRW_DBGHL(nextLinkH.code, nextLinkH.size, nextLinkH.ref); DRW_DBG("\n");
            DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
            prevEntLink = nextLinkH.ref;
            nextLinkH = buf->getOffsetHandle(handle);
            DRW_DBG(" next nextLinkers Handle: "); DRW_DBGHL(nextLinkH.code, nextLinkH.size, nextLinkH.ref); DRW_DBG("\n");
            DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
            nextEntLink = nextLinkH.ref;
        } else {
            nextEntLink = handle+1;
            prevEntLink = handle-1;
        }
    }
    if (version > DRW::AC1015) {//2004+
        //Parses Bookcolor handle
    }
    if (version > DRW::AC1014) {//2000+
        //layer handle
        layerH = buf->getOffsetHandle(handle);
        DRW_DBG(" layer Handle: "); DRW_DBGHL(layerH.code, layerH.size, layerH.ref); DRW_DBG("\n");
        DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        //lineType handle
        if(ltFlags == 3){
            lTypeH = buf->getOffsetHandle(handle);
            DRW_DBG("linetype Handle: "); DRW_DBGHL(lTypeH.code, lTypeH.size, lTypeH.ref); DRW_DBG("\n");
            DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        }
    }
    if (version > DRW::AC1014) {//2000+
        if (version > DRW::AC1018) {//2007+
            if (materialFlag == 3) {
                dwgHandle materialH = buf->getOffsetHandle(handle);
                material = materialH.ref;
                DRW_DBG(" material Handle: "); DRW_DBGHL(materialH.code, materialH.size, materialH.ref); DRW_DBG("\n");
                DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
            }
            if (shadowFlag == 3) {
                // AcDbShadow object handle (separate from entity shadow mode
                // populated from shadowFlag & 0x3 above). LibreCAD has no
                // shadow object consumer; leave discarding.
                dwgHandle shadowH = buf->getOffsetHandle(handle);
                DRW_DBG(" shadow Handle: "); DRW_DBGHL(shadowH.code, shadowH.size, shadowH.ref); DRW_DBG("\n");
                DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
            }
        }
        if (plotFlags == 3) {
            dwgHandle plotStyleH = buf->getOffsetHandle(handle);
            plotStyle = static_cast<int>(plotStyleH.ref);
            DRW_DBG(" plot style Handle: "); DRW_DBGHL(plotStyleH.code, plotStyleH.size, plotStyleH.ref); DRW_DBG("\n");
            DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        }
        if (version > DRW::AC1021) {//2010+ — §19.4.2: visual-style handles
            // Ground-truth: libreDWG common_entity_handle_data.spec lines
            // 141-150 and ODA spec v5.4.1 §19.4.2. Order matches: full,
            // face, edge — each conditional on its single-bit flag from
            // §19.4.1 (set in parseDwg above). All three are hard pointers
            // (libreDWG FIELD_HANDLE code 5), matching the existing
            // material/shadow/plotstyle handles in this block.
            if (hasFullVisualStyle) {
                dwgHandle h = buf->getOffsetHandle(handle);
                fullVisualStyleHandle = h.ref;
                DRW_DBG(" full visual-style H: ");
                DRW_DBGHL(h.code, h.size, h.ref); DRW_DBG("\n");
            }
            if (hasFaceVisualStyle) {
                dwgHandle h = buf->getOffsetHandle(handle);
                faceVisualStyleHandle = h.ref;
                DRW_DBG(" face visual-style H: ");
                DRW_DBGHL(h.code, h.size, h.ref); DRW_DBG("\n");
            }
            if (hasEdgeVisualStyle) {
                dwgHandle h = buf->getOffsetHandle(handle);
                edgeVisualStyleHandle = h.ref;
                DRW_DBG(" edge visual-style H: ");
                DRW_DBGHL(h.code, h.size, h.ref); DRW_DBG("\n");
            }
        }
    }
    const int rb = buf->numRemainingBytes();
    DRW_DBG("\n DRW_Entity::parseDwgEntHandle Remaining bytes: "); DRW_DBG(rb); DRW_DBG("\n");
    if (rb > 4) {  // 2-byte CRC + slack
        DRW_DBG("\n*** parseDwgEntHandle leftover ");
        DRW_DBG(rb);
        DRW_DBG(" bytes; entity handle ");
        DRW_DBGH(handle);
        DRW_DBG(" oType ");
        DRW_DBG(oType);
        DRW_DBG(" — possible bit-stream misalignment ***\n");
    }
    return buf->isGood();
}

bool DRW_Point::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 10:
        basePoint.x = reader->getDouble();
        break;
    case 20:
        basePoint.y = reader->getDouble();
        break;
    case 30:
        basePoint.z = reader->getDouble();
        break;
    case 39:
        thickness = reader->getDouble();
        break;
    case 210:
        haveExtrusion = true;
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

// ---------------------------------------------------------------------------
// Phase 4a (drafted 2026-05-15)
// ---------------------------------------------------------------------------
// `DRW_Entity::encodeDwgCommon` and `encodeDwgEntHandle` are R2000-only
// inverses of the corresponding parseDwg fragments above.  The version
// conditionals collapse: all `version > AC1014` branches fire (R2000 is
// AC1015 > AC1014), all `version > AC1015` and `version > AC1018` and
// `version > AC1021` branches skip.  Likewise `version < AC1015` skips.
//
// Discarded fields (Risk 4i):
//   - graphFlag B + optional graphData — we always emit graphFlag=0.
//   - haveNextLinks B — we emit 1 (no prev/next chain).
//   - acDbColorH — only fires when ENC flag 0x40 set; R2000 entity
//     encoders don't emit DBCOLOR refs yet.
//
// We always emit:
//   - entmode = 2 (modelspace, no owner-handle in stream — caller can
//     override before calling encodeDwgCommon if entity needs an owner).
//   - numReactors = 0
//   - ltFlags = 0 (BYLAYER), plotFlags = 0 (BYLAYER)
//   - invisibleFlag = 0 (visible)
//
// Caller must:
//   - Pre-populate `eType`, `handle`, `color`, `ltypeScale`, `lWeight`,
//     `layerH.ref` (handle of the layer this entity belongs to).
//   - The body emit between encodeDwgCommon and encodeDwgEntHandle is
//     per-entity (3BD basePoint for Point, etc.).

bool DRW_Entity::encodeDwgCommon(DRW::Version version, dwgBufferW *buf) {
    if (version != DRW::AC1015) return false;

    buf->putBitShort(static_cast<duint16>(oType));
    // objSize: precise value would require two-pass encode; R2000 reader
    // stores but does not validate it.  Emit 0 — see [Risk 4g, 4i].
    buf->putRawLong32(0);

    // The own handle: hard-pointer code 4.
    dwgHandle ownH;
    ownH.code = 4;
    ownH.ref  = handle;
    ownH.size = 0;
    if (handle != 0) {
        duint32 t = handle;
        while (t != 0) { t >>= 8; ++ownH.size; }
    } else {
        ownH.code = 0;
    }
    buf->putHandle(ownH);

    // No EED yet.
    buf->putBitShort(0);  // extDataSize=0

    // No graphics data.
    buf->putBit(0);  // graphFlag=0

    // entmode: 2 = modelspace, no owner-handle in stream.
    buf->put2Bits(2);

    // numReactors=0
    buf->putBitShort(0);

    // haveNextLinks=1 (no prev/next chain — entity is standalone)
    buf->putBit(1);

    // ENC color (BS for R2000 — putEnColor delegates to putBitShort).
    buf->putEnColor(version, static_cast<duint16>(color));

    // ltypeScale BD.
    buf->putBitDouble(ltypeScale);

    // ltFlags BB (0 = BYLAYER), plotFlags BB (0 = BYLAYER).
    buf->put2Bits(0);
    buf->put2Bits(0);

    // invisibleFlag BS.
    buf->putBitShort(0);

    // lWeight RC (0 = byLayer per DRW_LW_Conv).
    buf->putRawChar8(static_cast<duint8>(lWeight));

    return true;
}

bool DRW_Entity::encodeDwgEntHandle(DRW::Version version, dwgBufferW *buf) {
    if (version != DRW::AC1015) return false;

    // ownerHandle skipped — entmode=2 above.
    // No reactor handles (numReactors=0).
    // XDic handle — for R2000, xDictFlag is uninitialized in the reader,
    // so the reader's `xDictFlag != 1` branch fires and expects a null
    // XDic handle here.
    dwgHandle xDicNull;
    xDicNull.code = 3;
    xDicNull.ref  = 0;
    xDicNull.size = 0;
    buf->putHandle(xDicNull);

    // Layer handle (R2000+ unconditional).  Hard pointer.
    dwgHandle lH;
    lH.code = layerH.ref == 0 ? 0 : 5;  // 5 = hard pointer for layer ref
    lH.ref  = layerH.ref;
    lH.size = 0;
    if (lH.ref != 0) {
        duint32 t = lH.ref;
        while (t != 0) { t >>= 8; ++lH.size; }
    }
    buf->putHandle(lH);

    // ltFlags=0 → no separate lTypeH (BYLAYER).
    // plotFlags=0 → no plotStyleH (BYLAYER).
    // No 2007+ material/shadow handles, no 2010+ visualStyle handles.

    return true;
}

bool DRW_Point::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    oType = 27;  // POINT class id — see dwgreader.cpp:1111 dispatch
    if (!encodeDwgCommon(version, buf)) return false;

    // Point body — mirror of DRW_Point::parseDwg below.
    buf->putBitDouble(basePoint.x);
    buf->putBitDouble(basePoint.y);
    buf->putBitDouble(basePoint.z);
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);
    buf->putBitDouble(0.0);  // x-axis rotation; libdxfrw does not store it

    return encodeDwgEntHandle(version, buf);
}

bool DRW_Point::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing point *********************************************\n");

    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    DRW_DBG("point: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
    thickness = buf->getThickness(version > DRW::AC1014);//BD
    DRW_DBG("\nthickness: "); DRW_DBG(thickness);
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    DRW_DBG(", Extrusion: "); DRW_DBGPT(extPoint.x, extPoint.y, extPoint.z);

    double x_axis = buf->getBitDouble();//BD
    DRW_DBG("\n  x_axis: ");DRW_DBG(x_axis);DRW_DBG("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    //    RS crc;   //RS */

    return buf->isGood();
}

bool DRW_Line::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 11:
        secPoint.x = reader->getDouble();
        break;
    case 21:
        secPoint.y = reader->getDouble();
        break;
    case 31:
        secPoint.z = reader->getDouble();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

bool DRW_Line::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    oType = 19;  // LINE class id — see dwgreader.cpp:1105
    if (!encodeDwgCommon(version, buf)) return false;

    // R2000+ Line body — zIsZero shortcut: if both z's are 0, omit the
    // z fields entirely.  Reader reads `zIsZero` first, then RD x +
    // DD secX (default = x), RD y + DD secY (default = y), and
    // conditionally RD z + DD secZ.  Our putDefaultDouble always emits
    // the full RD via code 0b11; reader's getDefaultDouble with code
    // 0b11 returns the raw double.
    bool zIsZero = (basePoint.z == 0.0 && secPoint.z == 0.0);
    buf->putBit(zIsZero ? 1 : 0);
    buf->putRawDouble(basePoint.x);
    buf->putDefaultDouble(basePoint.x, secPoint.x);
    buf->putRawDouble(basePoint.y);
    buf->putDefaultDouble(basePoint.y, secPoint.y);
    if (!zIsZero) {
        buf->putRawDouble(basePoint.z);
        buf->putDefaultDouble(basePoint.z, secPoint.z);
    }
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);

    return encodeDwgEntHandle(version, buf);
}

bool DRW_Circle::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    oType = 18;  // CIRCLE class id — see dwgreader.cpp:1099
    if (!encodeDwgCommon(version, buf)) return false;

    // Circle body — mirror of DRW_Circle::parseDwg.
    buf->putBitDouble(basePoint.x);
    buf->putBitDouble(basePoint.y);
    buf->putBitDouble(basePoint.z);
    buf->putBitDouble(radious);
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);

    return encodeDwgEntHandle(version, buf);
}

bool DRW_Ray::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    // Ray = 40, Xline = 41 — derive from runtime type so DRW_Xline can
    // share this encoder (it inherits from DRW_Ray).
    oType = (eType == DRW::XLINE) ? 41 : 40;
    if (!encodeDwgCommon(version, buf)) return false;

    // 3 BD basePoint + 3 BD vector — same layout as parseDwg.
    buf->putBitDouble(basePoint.x);
    buf->putBitDouble(basePoint.y);
    buf->putBitDouble(basePoint.z);
    buf->putBitDouble(secPoint.x);
    buf->putBitDouble(secPoint.y);
    buf->putBitDouble(secPoint.z);

    return encodeDwgEntHandle(version, buf);
}

bool DRW_Trace::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    oType = 32;  // TRACE = 32 — see dwgreader.cpp:1317
    if (!encodeDwgCommon(version, buf)) return false;

    // Trace body — mirror of parseDwg.  Note the unusual layout:
    // thickness FIRST, then elevation (basePoint.z) as BD, then 4
    // corners as 2RD (z values share basePoint.z).
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putBitDouble(basePoint.z);
    buf->putRawDouble(basePoint.x);
    buf->putRawDouble(basePoint.y);
    buf->putRawDouble(secPoint.x);
    buf->putRawDouble(secPoint.y);
    buf->putRawDouble(thirdPoint.x);
    buf->putRawDouble(thirdPoint.y);
    buf->putRawDouble(fourPoint.x);
    buf->putRawDouble(fourPoint.y);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);

    return encodeDwgEntHandle(version, buf);
}

bool DRW_3Dface::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    oType = 28;  // 3DFACE class id — see dwgreader.cpp:1237
    if (!encodeDwgCommon(version, buf)) return false;

    // R2000+ 3DFACE body — mirror of parseDwg's z_is_zero / has_no_flag
    // optimization.  Reader checks `invisibleflag != NoEdge`; if NoEdge,
    // emit has_no_flag=1 to suppress the BS read.
    bool hasNoFlag = (invisibleflag == /*NoEdge*/0);
    bool zIsZero   = (basePoint.z == 0.0);
    buf->putBit(hasNoFlag ? 1 : 0);
    buf->putBit(zIsZero ? 1 : 0);
    buf->putRawDouble(basePoint.x);
    buf->putRawDouble(basePoint.y);
    if (!zIsZero) buf->putRawDouble(basePoint.z);
    buf->putDefaultDouble(basePoint.x, secPoint.x);
    buf->putDefaultDouble(basePoint.y, secPoint.y);
    buf->putDefaultDouble(basePoint.z, secPoint.z);
    buf->putDefaultDouble(secPoint.x, thirdPoint.x);
    buf->putDefaultDouble(secPoint.y, thirdPoint.y);
    buf->putDefaultDouble(secPoint.z, thirdPoint.z);
    buf->putDefaultDouble(thirdPoint.x, fourPoint.x);
    buf->putDefaultDouble(thirdPoint.y, fourPoint.y);
    buf->putDefaultDouble(thirdPoint.z, fourPoint.z);
    if (!hasNoFlag) buf->putBitShort(static_cast<duint16>(invisibleflag));

    return encodeDwgEntHandle(version, buf);
}

bool DRW_Solid::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    oType = 31;  // SOLID class id — see dwgreader.cpp:1305
    if (!encodeDwgCommon(version, buf)) return false;

    // Same body layout as TRACE (4 corners + extrusion).  Duplicated
    // here rather than delegating to DRW_Trace::encodeDwg because that
    // hardcodes oType=32.
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putBitDouble(basePoint.z);
    buf->putRawDouble(basePoint.x);
    buf->putRawDouble(basePoint.y);
    buf->putRawDouble(secPoint.x);
    buf->putRawDouble(secPoint.y);
    buf->putRawDouble(thirdPoint.x);
    buf->putRawDouble(thirdPoint.y);
    buf->putRawDouble(fourPoint.x);
    buf->putRawDouble(fourPoint.y);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);

    return encodeDwgEntHandle(version, buf);
}

bool DRW_LWPolyline::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    oType = 77;  // LWPOLYLINE class id — see dwgreader.cpp:1202
    if (!encodeDwgCommon(version, buf)) return false;

    // DRW_LWPolyline::flags carries DXF-side bits (1=closed, 128=plinegen).
    // DWG-side flags are different: they signal which optional fields are
    // present.  Per parseDwg, bit 9 (0x200) = closed, bit 8 (0x100) =
    // plinegen.  Build the DWG flags from the DXF flags plus the data.
    duint16 dwgFlags = 0;
    if (flags & 1)   dwgFlags |= 0x200;   // closed
    if (flags & 128) dwgFlags |= 0x100;   // plinegen
    if (thickness != 0.0) dwgFlags |= 0x2;
    if (width     != 0.0) dwgFlags |= 0x4;
    if (elevation != 0.0) dwgFlags |= 0x8;
    bool defaultExt = (extPoint.x == 0.0 && extPoint.y == 0.0 && extPoint.z == 1.0);
    if (!defaultExt) dwgFlags |= 0x1;
    // Detect per-vertex bulge / width data.
    bool anyBulge = false;
    bool anyWidth = false;
    for (const auto& v : vertlist) {
        if (v && v->bulge    != 0.0) anyBulge = true;
        if (v && (v->stawidth != 0.0 || v->endwidth != 0.0)) anyWidth = true;
    }
    if (anyBulge) dwgFlags |= 0x10;
    if (anyWidth) dwgFlags |= 0x20;

    buf->putBitShort(dwgFlags);
    if (dwgFlags & 0x4)  buf->putBitDouble(width);
    if (dwgFlags & 0x8)  buf->putBitDouble(elevation);
    if (dwgFlags & 0x2)  buf->putBitDouble(thickness);
    if (dwgFlags & 0x1)  buf->putExtrusion(extPoint, /*b_R2000_style=*/false);

    const dint32 numVerts = static_cast<dint32>(vertlist.size());
    buf->putBitLong(numVerts);
    if (dwgFlags & 0x10) buf->putBitLong(numVerts);  // bulgesnum
    if (dwgFlags & 0x20) buf->putBitLong(numVerts);  // widthsnum
    // (R2010+ vertexIdCount: not emitted for R2000)

    if (numVerts > 0) {
        // First vertex as 2RD.  Subsequent vertices as 2DD relative to
        // the previous, with putDefaultDouble always emitting code 0b11
        // (full RD); the reader's getDefaultDouble returns the raw value.
        buf->putRawDouble(vertlist[0]->x);
        buf->putRawDouble(vertlist[0]->y);
        for (size_t i = 1; i < vertlist.size(); ++i) {
            buf->putDefaultDouble(vertlist[i-1]->x, vertlist[i]->x);
            buf->putDefaultDouble(vertlist[i-1]->y, vertlist[i]->y);
        }
        if (dwgFlags & 0x10) {
            for (const auto& v : vertlist)
                buf->putBitDouble(v->bulge);
        }
        if (dwgFlags & 0x20) {
            for (const auto& v : vertlist) {
                buf->putBitDouble(v->stawidth);
                buf->putBitDouble(v->endwidth);
            }
        }
    }

    return encodeDwgEntHandle(version, buf);
}

bool DRW_Block::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    // BLOCK = 4, ENDBLK = 5 per DWG spec.  isEnd controls which.
    oType = isEnd ? 5 : 4;
    if (!encodeDwgCommon(version, buf)) return false;
    if (!isEnd) {
        buf->putVariableText(version, name);
    }
    // (R2007+ unknown bit skipped for R2000.)
    return encodeDwgEntHandle(version, buf);
}

bool DRW_Text::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    oType = 1;  // TEXT class id — see dwgreader.cpp:1208
    if (!encodeDwgCommon(version, buf)) return false;

    // R2000+ TEXT body — mirror of DRW_Text::parseDwg.  We emit
    // data_flags=0 so the reader sees every optional field rather than
    // substituting defaults — keeps the encoder simple, costs ~30 bytes
    // per TEXT versus the most compressed form.
    buf->putRawChar8(0);                              // data_flags=0
    buf->putRawDouble(basePoint.z);                   // elevation RD
    buf->putRawDouble(basePoint.x);                   // insertion 2RD
    buf->putRawDouble(basePoint.y);
    buf->putDefaultDouble(basePoint.x, secPoint.x);   // alignment 2DD
    buf->putDefaultDouble(basePoint.y, secPoint.y);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putRawDouble(oblique);                       // oblique angle
    // Angle: struct holds degrees; on-disk format is radians.  Reader
    // does `angle *= ARAD` (180/π) after read.  Inverse: divide here.
    buf->putRawDouble(angle / ARAD);
    buf->putRawDouble(height);                        // text height
    buf->putRawDouble(widthscale);                    // width factor
    buf->putVariableText(version, text);              // text string
    buf->putBitShort(static_cast<duint16>(textgen));
    buf->putBitShort(static_cast<duint16>(alignH));
    buf->putBitShort(static_cast<duint16>(alignV));

    if (!encodeDwgEntHandle(version, buf)) return false;

    // styleH — hard pointer to STYLE table record.  Default points at
    // the STANDARD textstyle (handle 0x13) if caller hasn't set one.
    dwgHandle sH;
    duint32 sref = (styleH.ref == 0) ? 0x13 : styleH.ref;
    sH.code = 5;  // hard pointer
    sH.ref  = sref;
    sH.size = 0;
    {
        duint32 t = sref;
        while (t != 0) { t >>= 8; ++sH.size; }
    }
    buf->putHandle(sH);
    return true;
}

bool DRW_Ellipse::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    oType = 35;  // ELLIPSE class id — see dwgreader.cpp:1117
    if (!encodeDwgCommon(version, buf)) return false;

    // Ellipse body — mirror of DRW_Ellipse::parseDwg.
    buf->put3BitDouble(basePoint);       // center
    buf->put3BitDouble(secPoint);        // major axis vector
    buf->put3BitDouble(extPoint);        // extrusion
    buf->putBitDouble(ratio);            // minor/major ratio
    buf->putBitDouble(staparam);         // start parameter
    buf->putBitDouble(endparam);         // end parameter

    return encodeDwgEntHandle(version, buf);
}

bool DRW_Arc::encodeDwg(DRW::Version version, dwgBufferW *buf, duint32 bs) {
    (void)bs;
    oType = 17;  // ARC class id — see dwgreader.cpp:1093
    if (!encodeDwgCommon(version, buf)) return false;

    // Arc body — Circle body + 2 BD angles.
    buf->putBitDouble(basePoint.x);
    buf->putBitDouble(basePoint.y);
    buf->putBitDouble(basePoint.z);
    buf->putBitDouble(radious);
    buf->putThickness(thickness, /*b_R2000_style=*/true);
    buf->putExtrusion(extPoint, /*b_R2000_style=*/true);
    buf->putBitDouble(staangle);
    buf->putBitDouble(endangle);

    return encodeDwgEntHandle(version, buf);
}

bool DRW_Line::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing line *********************************************\n");

    if (version < DRW::AC1015) {//14-
        basePoint.x = buf->getBitDouble();
        basePoint.y = buf->getBitDouble();
        basePoint.z = buf->getBitDouble();
        secPoint.x = buf->getBitDouble();
        secPoint.y = buf->getBitDouble();
        secPoint.z = buf->getBitDouble();
    }
    if (version > DRW::AC1014) {//2000+
        bool zIsZero = buf->getBit(); //B
        basePoint.x = buf->getRawDouble();//RD
        secPoint.x = buf->getDefaultDouble(basePoint.x);//DD
        basePoint.y = buf->getRawDouble();//RD
        secPoint.y = buf->getDefaultDouble(basePoint.y);//DD
        if (!zIsZero) {
            basePoint.z = buf->getRawDouble();//RD
            secPoint.z = buf->getDefaultDouble(basePoint.z);//DD
        }
    }
    DRW_DBG("start point: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
    DRW_DBG("\nend point: "); DRW_DBGPT(secPoint.x, secPoint.y, secPoint.z);
    thickness = buf->getThickness(version > DRW::AC1014);//BD
    DRW_DBG("\nthickness: "); DRW_DBG(thickness);
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    DRW_DBG(", Extrusion: "); DRW_DBGPT(extPoint.x, extPoint.y, extPoint.z);DRW_DBG("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Ray::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing ray/xline *********************************************\n");
    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    secPoint.x = buf->getBitDouble();
    secPoint.y = buf->getBitDouble();
    secPoint.z = buf->getBitDouble();
    DRW_DBG("start point: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
    DRW_DBG("\nvector: "); DRW_DBGPT(secPoint.x, secPoint.y, secPoint.z);
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Circle::applyExtrusion(){
    if (haveExtrusion) {
        //NOTE: Commenting these out causes the the arcs being tested to be located
        //on the other side of the y axis (all x dimensions are negated).
        calculateAxis(extPoint);
        extrudePoint(extPoint, &basePoint);
    }
}

bool DRW_Circle::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 40:
        radious = reader->getDouble();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

bool DRW_Circle::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    bool ret = DRW_Entity::parseDwg(version, buf, nullptr, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing circle *********************************************\n");

    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    DRW_DBG("center: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
    radious = buf->getBitDouble();
    DRW_DBG("\nradius: "); DRW_DBG(radious);

    thickness = buf->getThickness(version > DRW::AC1014);
    DRW_DBG(" thickness: "); DRW_DBG(thickness);
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    DRW_DBG("\nextrusion: "); DRW_DBGPT(extPoint.x, extPoint.y, extPoint.z); DRW_DBG("\n");

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_Arc::applyExtrusion(){
    DRW_Circle::applyExtrusion();

    if(haveExtrusion){
        // If the extrusion vector has a z value less than 0, the angles for the arc
        // have to be mirrored since DXF files use the right hand rule.
        // Note that the following code only handles the special case where there is a 2D
        // drawing with the z axis heading into the paper (or rather screen). An arbitrary
        // extrusion axis (with x and y values greater than 1/64) may still have issues.
        if (fabs(extPoint.x) < 0.015625 && fabs(extPoint.y) < 0.015625 && extPoint.z < 0.0) {
            staangle=M_PI-staangle;
            endangle=M_PI-endangle;

            double temp = staangle;
            staangle=endangle;
            endangle=temp;
        }
    }
}

bool DRW_Arc::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 50:
        staangle = reader->getDouble()/ ARAD;
        break;
    case 51:
        endangle = reader->getDouble()/ ARAD;
        break;
    default:
        return DRW_Circle::parseCode(code, reader);
    }

    return true;
}

bool DRW_Arc::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing circle arc *********************************************\n");

    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    DRW_DBG("center point: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);

    radious = buf->getBitDouble();
    DRW_DBG("\nradius: "); DRW_DBG(radious);
    thickness = buf->getThickness(version > DRW::AC1014);
    DRW_DBG(" thickness: "); DRW_DBG(thickness);
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    DRW_DBG("\nextrusion: "); DRW_DBGPT(extPoint.x, extPoint.y, extPoint.z);
    staangle = buf->getBitDouble();
    DRW_DBG("\nstart angle: "); DRW_DBG(staangle);
    endangle = buf->getBitDouble();
    DRW_DBG(" end angle: "); DRW_DBG(endangle); DRW_DBG("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    return buf->isGood();
}

bool DRW_Ellipse::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 40:
        ratio = reader->getDouble();
        break;
    case 41:
        staparam = reader->getDouble();
        break;
    case 42:
        endparam = reader->getDouble();
        break;
    default:
        return DRW_Line::parseCode(code, reader);
    }

    return true;
}

void DRW_Ellipse::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        extrudePoint(extPoint, &secPoint);
        double intialparam = staparam;
        if (extPoint.z < 0.){
            staparam = M_PIx2 - endparam;
            endparam = M_PIx2 - intialparam;
        }
    }
}

//if ratio > 1 minor axis are greather than major axis, correct it
void DRW_Ellipse::correctAxis(){
    bool complete = false;
    if (staparam == endparam) {
        staparam = 0.0;
        endparam = M_PIx2; //2*M_PI;
        complete = true;
    }
    if (ratio > 1){
        if ( fabs(endparam - staparam - M_PIx2) < 1.0e-10)
            complete = true;
        double incX = secPoint.x;
        secPoint.x = -(secPoint.y * ratio);
        secPoint.y = incX*ratio;
        ratio = 1/ratio;
        if (!complete){
            if (staparam < M_PI_2)
                staparam += M_PI *2;
            if (endparam < M_PI_2)
                endparam += M_PI *2;
            endparam -= M_PI_2;
            staparam -= M_PI_2;
        }
    }
}

bool DRW_Ellipse::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing ellipse *********************************************\n");

    basePoint =buf->get3BitDouble();
    DRW_DBG("center: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
    secPoint =buf->get3BitDouble();
    DRW_DBG(", axis: "); DRW_DBGPT(secPoint.x, secPoint.y, secPoint.z); DRW_DBG("\n");
    extPoint =buf->get3BitDouble();
    DRW_DBG("Extrusion: "); DRW_DBGPT(extPoint.x, extPoint.y, extPoint.z);
    ratio = buf->getBitDouble();//BD
    DRW_DBG("\nratio: "); DRW_DBG(ratio);
    staparam = buf->getBitDouble();//BD
    DRW_DBG(" start param: "); DRW_DBG(staparam);
    endparam = buf->getBitDouble();//BD
    DRW_DBG(" end param: "); DRW_DBG(endparam); DRW_DBG("\n");

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

//parts are the number of vertex to split polyline, default 128
void DRW_Ellipse::toPolyline(DRW_Polyline *pol, int parts){
    double radMajor, radMinor, cosRot, sinRot, incAngle, curAngle;
    double cosCurr, sinCurr;
	radMajor = hypot(secPoint.x, secPoint.y);
    radMinor = radMajor*ratio;
    //calculate sin & cos of included angle
    incAngle = atan2(secPoint.y, secPoint.x);
    cosRot = cos(incAngle);
    sinRot = sin(incAngle);
    incAngle = M_PIx2 / parts;
    curAngle = staparam;
    int i = static_cast<int>(curAngle / incAngle);
    do {
        if (curAngle > endparam) {
            curAngle = endparam;
            i = parts+2;
        }
        cosCurr = cos(curAngle);
        sinCurr = sin(curAngle);
        double x = basePoint.x + (cosCurr*cosRot*radMajor) - (sinCurr*sinRot*radMinor);
        double y = basePoint.y + (cosCurr*sinRot*radMajor) + (sinCurr*cosRot*radMinor);
        pol->addVertex( DRW_Vertex(x, y, 0.0, 0.0));
        curAngle = (++i)*incAngle;
    } while (i<parts);
    if ( fabs(endparam - staparam - M_PIx2) < 1.0e-10){
        pol->flags = 1;
    }
    pol->layer = this->layer;
    pol->lineType = this->lineType;
    pol->color = this->color;
    pol->lWeight = this->lWeight;
    pol->extPoint = this->extPoint;
}

void DRW_Trace::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        extrudePoint(extPoint, &basePoint);
        extrudePoint(extPoint, &secPoint);
        extrudePoint(extPoint, &thirdPoint);
        extrudePoint(extPoint, &fourPoint);
    }
}

bool DRW_Trace::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 12:
        thirdPoint.x = reader->getDouble();
        break;
    case 22:
        thirdPoint.y = reader->getDouble();
        break;
    case 32:
        thirdPoint.z = reader->getDouble();
        break;
    case 13:
        fourPoint.x = reader->getDouble();
        break;
    case 23:
        fourPoint.y = reader->getDouble();
        break;
    case 33:
        fourPoint.z = reader->getDouble();
        break;
    default:
        return DRW_Line::parseCode(code, reader);
    }

    return true;
}

bool DRW_Trace::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing Trace *********************************************\n");

    thickness = buf->getThickness(version>DRW::AC1014);
    basePoint.z = buf->getBitDouble();
    basePoint.x = buf->getRawDouble();
    basePoint.y = buf->getRawDouble();
    secPoint.x = buf->getRawDouble();
    secPoint.y = buf->getRawDouble();
    secPoint.z = basePoint.z;
    thirdPoint.x = buf->getRawDouble();
    thirdPoint.y = buf->getRawDouble();
    thirdPoint.z = basePoint.z;
    fourPoint.x = buf->getRawDouble();
    fourPoint.y = buf->getRawDouble();
    fourPoint.z = basePoint.z;
    extPoint = buf->getExtrusion(version>DRW::AC1014);

    DRW_DBG(" - base "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
    DRW_DBG("\n - sec "); DRW_DBGPT(secPoint.x, secPoint.y, secPoint.z);
    DRW_DBG("\n - third "); DRW_DBGPT(thirdPoint.x, thirdPoint.y, thirdPoint.z);
    DRW_DBG("\n - fourth "); DRW_DBGPT(fourPoint.x, fourPoint.y, fourPoint.z);
    DRW_DBG("\n - extrusion: "); DRW_DBGPT(extPoint.x, extPoint.y, extPoint.z);
    DRW_DBG("\n - thickness: "); DRW_DBG(thickness); DRW_DBG("\n");

    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;

    /* CRC X --- */
    return buf->isGood();
}

bool DRW_Solid::parseDwg(DRW::Version v, dwgBuffer *buf, duint32 bs){
    DRW_DBG("\n***************************** parsing Solid *********************************************\n");
    return DRW_Trace::parseDwg(v, buf, bs);
}

bool DRW_3Dface::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 70:
        invisibleflag = reader->getInt32();
        break;
    default:
        return DRW_Trace::parseCode(code, reader);
    }

    return true;
}

bool DRW_3Dface::parseDwg(DRW::Version v, dwgBuffer *buf, duint32 bs){
    bool ret = DRW_Entity::parseDwg(v, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing 3Dface *********************************************\n");

    if ( v < DRW::AC1015 ) {// R13 & R14
        basePoint.x = buf->getBitDouble();
        basePoint.y = buf->getBitDouble();
        basePoint.z = buf->getBitDouble();
        secPoint.x = buf->getBitDouble();
        secPoint.y = buf->getBitDouble();
        secPoint.z = buf->getBitDouble();
        thirdPoint.x = buf->getBitDouble();
        thirdPoint.y = buf->getBitDouble();
        thirdPoint.z = buf->getBitDouble();
        fourPoint.x = buf->getBitDouble();
        fourPoint.y = buf->getBitDouble();
        fourPoint.z = buf->getBitDouble();
        invisibleflag = buf->getBitShort();
    } else { // 2000+
        bool has_no_flag = buf->getBit();
        bool z_is_zero = buf->getBit();
        basePoint.x = buf->getRawDouble();
        basePoint.y = buf->getRawDouble();
        basePoint.z = z_is_zero ? 0.0 : buf->getRawDouble();
        secPoint.x = buf->getDefaultDouble(basePoint.x);
        secPoint.y = buf->getDefaultDouble(basePoint.y);
        secPoint.z = buf->getDefaultDouble(basePoint.z);
        thirdPoint.x = buf->getDefaultDouble(secPoint.x);
        thirdPoint.y = buf->getDefaultDouble(secPoint.y);
        thirdPoint.z = buf->getDefaultDouble(secPoint.z);
        fourPoint.x = buf->getDefaultDouble(thirdPoint.x);
        fourPoint.y = buf->getDefaultDouble(thirdPoint.y);
        fourPoint.z = buf->getDefaultDouble(thirdPoint.z);
        invisibleflag = has_no_flag ? (int)NoEdge : buf->getBitShort();
    }
    drw_assert(invisibleflag>=NoEdge);
    drw_assert(invisibleflag<=AllEdges);

    DRW_DBG(" - base "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z); DRW_DBG("\n");
    DRW_DBG(" - sec "); DRW_DBGPT(secPoint.x, secPoint.y, secPoint.z); DRW_DBG("\n");
    DRW_DBG(" - third "); DRW_DBGPT(thirdPoint.x, thirdPoint.y, thirdPoint.z); DRW_DBG("\n");
    DRW_DBG(" - fourth "); DRW_DBGPT(fourPoint.x, fourPoint.y, fourPoint.z); DRW_DBG("\n");
    DRW_DBG(" - Invisibility mask: "); DRW_DBG(invisibleflag); DRW_DBG("\n");

    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(v, buf);
    if (!ret)
        return ret;
    return buf->isGood();
}

bool DRW_Tolerance::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 1:
        text = reader->getUtf8String();
        break;
    case 3:
        dimStyleName = reader->getUtf8String();
        break;
    case 10:
        insertionPoint.x = reader->getDouble();
        break;
    case 20:
        insertionPoint.y = reader->getDouble();
        break;
    case 30:
        insertionPoint.z = reader->getDouble();
        break;
    case 11:
        xAxisDirectionVector.x = reader->getDouble();
        break;
    case 21:
        xAxisDirectionVector.y = reader->getDouble();
        break;
    case 31:
        xAxisDirectionVector.z = reader->getDouble();
        break;
    case 210:
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

bool DRW_Tolerance::parseDwg(DRW::Version v, dwgBuffer *buf, duint32 bs){
    (void) v;
    (void) buf;
    (void) bs;
    DRW_DBG("\n********************** parsing TOLERANCE from DWG is not yet implemented **************************\n");
    return true;
}

bool DRW_Block::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 1:
        xrefPath = reader->getUtf8String();
        break;
    case 2:
        name = reader->getUtf8String();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

bool DRW_Block::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    if (!isEnd){
        DRW_DBG("\n***************************** parsing block *********************************************\n");
        name = sBuf->getVariableText(version, false);
        DRW_DBG("Block name: "); DRW_DBG(name.c_str()); DRW_DBG("\n");
    } else {
        DRW_DBG("\n***************************** parsing end block *********************************************\n");
    }
    if (version > DRW::AC1018) {//2007+
        duint8 unk = buf->getBit();
        DRW_DBG("unknown bit: "); DRW_DBG(unk); DRW_DBG("\n");
    }
//    X handleAssoc;   //X
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Insert::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 2:
        name = reader->getUtf8String();
        break;
    case 41:
        xscale = reader->getDouble();
        break;
    case 42:
        yscale = reader->getDouble();
        break;
    case 43:
        zscale = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        angle = angle/ARAD; //convert to radian
        break;
    case 70:
        colcount = reader->getInt32();
        break;
    case 71:
        rowcount = reader->getInt32();
        break;
    case 44:
        colspace = reader->getDouble();
        break;
    case 45:
        rowspace = reader->getDouble();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

bool DRW_Insert::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    dint32 objCount = 0;
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n************************** parsing insert/minsert *****************************************\n");
    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    DRW_DBG("insertion point: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z); DRW_DBG("\n");
    if (version < DRW::AC1015) {//14-
        xscale = buf->getBitDouble();
        yscale = buf->getBitDouble();
        zscale = buf->getBitDouble();
    } else {
        duint8 dataFlags = buf->get2Bits();
        if (dataFlags == 3){
            //none default value 1,1,1
        } else if (dataFlags == 1){ //x default value 1, y & z can be x value
            yscale = buf->getDefaultDouble(xscale);
            zscale = buf->getDefaultDouble(xscale);
        } else if (dataFlags == 2){
            xscale = buf->getRawDouble();
            yscale = zscale = xscale;
        } else { //dataFlags == 0
            xscale = buf->getRawDouble();
            yscale = buf->getDefaultDouble(xscale);
            zscale = buf->getDefaultDouble(xscale);
        }
    }
    angle = buf->getBitDouble();
    DRW_DBG("scale : "); DRW_DBGPT(xscale, yscale, zscale); DRW_DBG(", angle: "); DRW_DBG(angle);
    extPoint = buf->getExtrusion(false); //3BD R14 style
    DRW_DBG("\nextrusion: "); DRW_DBGPT(extPoint.x, extPoint.y, extPoint.z);

    bool hasAttrib = buf->getBit();
    DRW_DBG("   has Attrib: "); DRW_DBG(hasAttrib);

    if (hasAttrib && version > DRW::AC1015) {//2004+
        objCount = buf->getBitLong();
        DRW_UNUSED(objCount);
        DRW_DBG("   objCount: "); DRW_DBG(objCount); DRW_DBG("\n");
    }
    if (oType == 8) {//entity are minsert
        colcount = buf->getBitShort();
        rowcount = buf->getBitShort();
        colspace = buf->getBitDouble();
        rowspace = buf->getBitDouble();
    }
    DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    blockRecH = buf->getHandle(); /* H 2 BLOCK HEADER (hard pointer) */
    DRW_DBG("BLOCK HEADER Handle: "); DRW_DBGHL(blockRecH.code, blockRecH.size, blockRecH.ref); DRW_DBG("\n");
    DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    /*attribs follows*/
    if (hasAttrib) {
        if (version < DRW::AC1018) {//2000-
            dwgHandle attH = buf->getHandle(); /* H 2 BLOCK HEADER (hard pointer) */
            DRW_DBG("first attrib Handle: "); DRW_DBGHL(attH.code, attH.size, attH.ref); DRW_DBG("\n");
            attribHandles.push_back(attH);
            attH = buf->getHandle(); /* H 2 BLOCK HEADER (hard pointer) */
            DRW_DBG("second attrib Handle: "); DRW_DBGHL(attH.code, attH.size, attH.ref); DRW_DBG("\n");
            attribHandles.push_back(attH);
        } else {
            for (duint8 i=0; i< objCount; ++i){
                dwgHandle attH = buf->getHandle(); /* H 2 BLOCK HEADER (hard pointer) */
                DRW_DBG("attrib Handle #"); DRW_DBG(i); DRW_DBG(": "); DRW_DBGHL(attH.code, attH.size, attH.ref); DRW_DBG("\n");
                attribHandles.push_back(attH);
            }
        }
        seqendH = buf->getHandle(); /* H 2 BLOCK HEADER (hard pointer) */
        DRW_DBG("seqendH Handle: "); DRW_DBGHL(seqendH.code, seqendH.size, seqendH.ref); DRW_DBG("\n");
    }
    DRW_DBG("   Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

void DRW_LWPolyline::applyExtrusion(){
    if (haveExtrusion) {
        calculateAxis(extPoint);
        for (unsigned int i=0; i<vertlist.size(); i++) {
			auto& vert = vertlist.at(i);
            DRW_Coord v(vert->x, vert->y, elevation);
            extrudePoint(extPoint, &v);
            vert->x = v.x;
            vert->y = v.y;
        }
    }
}

bool DRW_LWPolyline::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 10: {
		vertex = std::make_shared<DRW_Vertex2D>();
        vertlist.push_back(vertex);
        vertex->x = reader->getDouble();
        break; }
    case 20:
		if(vertex)
            vertex->y = reader->getDouble();
        break;
    case 40:
		if(vertex)
            vertex->stawidth = reader->getDouble();
        break;
    case 41:
		if(vertex)
            vertex->endwidth = reader->getDouble();
        break;
    case 42:
		if(vertex)
            vertex->bulge = reader->getDouble();
        break;
    case 38:
        elevation = reader->getDouble();
        break;
    case 39:
        thickness = reader->getDouble();
        break;
    case 43:
        width = reader->getDouble();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    case 90:
        vertexnum = reader->getInt32();
        return DRW::reserve( vertlist, vertexnum);
    case 210:
        haveExtrusion = true;
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

bool DRW_LWPolyline::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing LWPolyline *******************************************\n");

    flags = buf->getBitShort();
    DRW_DBG("flags value: "); DRW_DBG(flags);
    if (flags & 4)
        width = buf->getBitDouble();
    if (flags & 8)
        elevation = buf->getBitDouble();
    if (flags & 2)
        thickness = buf->getBitDouble();
    if (flags & 1)
        extPoint = buf->getExtrusion(false);
    vertexnum = buf->getBitLong();
    if (!DRW::reserve( vertlist, vertexnum)) {
        return false;
    }

    unsigned int bulgesnum = 0;
    if (flags & 16)
        bulgesnum = buf->getBitLong();
    int vertexIdCount = 0;
    if (version > DRW::AC1021) {//2010+
        if (flags & 1024)
            vertexIdCount = buf->getBitLong();
    }
    unsigned int widthsnum = 0;
    if (flags & 32)
        widthsnum = buf->getBitLong();
    DRW_DBG("\nvertex num: "); DRW_DBG(vertexnum); DRW_DBG(" bulges num: "); DRW_DBG(bulgesnum);
    DRW_DBG(" vertexIdCount: "); DRW_DBG(vertexIdCount); DRW_DBG(" widths num: "); DRW_DBG(widthsnum);
    // Translate DWG LWPLINE flag bits to DXF group 70 bits.
    // Per ODA spec 20.4.85 + libreDWG dwg.spec (DWG_ENTITY LWPOLYLINE):
    //   DWG bit  9 (0x200, 512) -> DXF bit 0 (closed,   value 1)
    //   DWG bit  8 (0x100, 256) -> DXF bit 7 (plinegen, value 128)
    // All other DWG flag bits indicate which optional fields are present
    // and have no DXF equivalent in group 70.
    int dxfFlags = 0;
    if (flags & 512)
        dxfFlags |= 1;
    if (flags & 256)
        dxfFlags |= 128;
    flags = dxfFlags;
    DRW_DBG("end flags value: "); DRW_DBG(flags);

    if (vertexnum > 0) { //verify if is lwpol without vertex (empty)
        // add vertexes
		vertex = std::make_shared<DRW_Vertex2D>();
        vertex->x = buf->getRawDouble();
        vertex->y = buf->getRawDouble();
        vertlist.push_back(vertex);
		auto pv = vertex;
        for (int i = 1; i< vertexnum; i++){
			vertex = std::make_shared<DRW_Vertex2D>();
			if (version < DRW::AC1015) {//14-
                vertex->x = buf->getRawDouble();
                vertex->y = buf->getRawDouble();
            } else {
//                DRW_Vertex2D *pv = vertlist.back();
                vertex->x = buf->getDefaultDouble(pv->x);
                vertex->y = buf->getDefaultDouble(pv->y);
            }
            pv = vertex;
            vertlist.push_back(vertex);
        }
        //add bulges
        for (unsigned int i = 0; i < bulgesnum; i++){
            double bulge = buf->getBitDouble();
            if (vertlist.size()> i)
                vertlist.at(i)->bulge = bulge;
        }
        //add vertexId
        if (version > DRW::AC1021) {//2010+
            for (int i = 0; i < vertexIdCount; i++){
                dint32 vertexId = buf->getBitLong();
                //TODO implement vertexId, do not exist in dxf
                DRW_UNUSED(vertexId);
//                if (vertlist.size()< i)
//                    vertlist.at(i)->vertexId = vertexId;
            }
        }
        //add widths
        for (unsigned int i = 0; i < widthsnum; i++){
            double staW = buf->getBitDouble();
            double endW = buf->getBitDouble();
            if (i < vertlist.size()) {
                vertlist.at(i)->stawidth = staW;
                vertlist.at(i)->endwidth = endW;
            }
        }
    }
    if (DRW_DBGGL == DRW_dbg::Level::Debug){
        DRW_DBG("\nVertex list: ");
		for (auto& pv: vertlist) {
            DRW_DBG("\n   x: "); DRW_DBG(pv->x); DRW_DBG(" y: "); DRW_DBG(pv->y); DRW_DBG(" bulge: "); DRW_DBG(pv->bulge);
            DRW_DBG(" stawidth: "); DRW_DBG(pv->stawidth); DRW_DBG(" endwidth: "); DRW_DBG(pv->endwidth);
        }
    }

    DRW_DBG("\n");
    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    /* CRC X --- */
    return buf->isGood();
}


// ----------------------------------------------------------------------------
// DRW_MLine — multiline entity (ODA §19.4.78, fixed type 0x2F = 47).
// ----------------------------------------------------------------------------

bool DRW_MLine::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 2:
        styleName = reader->getString();
        break;
    case 340:
        styleHandle = static_cast<duint32>(reader->getHandleString());
        break;
    case 40:
        scale = reader->getDouble();
        break;
    case 70:
        justification = static_cast<duint8>(reader->getInt32() & 0x3);
        break;
    case 71:
        openClosed = reader->getInt32();
        break;
    case 72:
        numVerts = static_cast<duint16>(reader->getInt32());
        break;
    case 73:
        numLines = static_cast<duint8>(reader->getInt32());
        break;
    case 10:
        basePoint.x = reader->getDouble();
        break;
    case 20:
        basePoint.y = reader->getDouble();
        break;
    case 30:
        basePoint.z = reader->getDouble();
        break;
    case 210:
        extPoint.x = reader->getDouble();
        break;
    case 220:
        extPoint.y = reader->getDouble();
        break;
    case 230:
        extPoint.z = reader->getDouble();
        break;
    // Per-vertex block: code 11 starts a new vertex; 12/13 follow.
    case 11:
        ++m_currentVertexIdx;
        m_currentElementIdx = 0;
        if (m_currentVertexIdx >= static_cast<int>(vertlist.size())) {
            vertlist.resize(m_currentVertexIdx + 1);
        }
        vertlist[m_currentVertexIdx].position.x = reader->getDouble();
        break;
    case 21:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].position.y = reader->getDouble();
        break;
    case 31:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].position.z = reader->getDouble();
        break;
    case 12:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].vertexDir.x = reader->getDouble();
        break;
    case 22:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].vertexDir.y = reader->getDouble();
        break;
    case 32:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].vertexDir.z = reader->getDouble();
        break;
    case 13:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].miterDir.x = reader->getDouble();
        break;
    case 23:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].miterDir.y = reader->getDouble();
        break;
    case 33:
        if (m_currentVertexIdx >= 0)
            vertlist[m_currentVertexIdx].miterDir.z = reader->getDouble();
        break;
    // 74 = segment-param count for current element. Sets up the inner
    // vector and resets the running param count. 41 reads each param.
    // 75 = fill-param count; 42 reads each. After fills are consumed,
    // advance to the next element. AutoCAD emits 74/41*/75/42* per element.
    case 74:
        if (m_currentVertexIdx >= 0) {
            auto& v = vertlist[m_currentVertexIdx];
            if (static_cast<int>(v.segParms.size()) < numLines) {
                v.segParms.resize(numLines);
                v.areaFillParms.resize(numLines);
            }
            (void)reader->getInt32();   // expected count, used only as a marker
            m_currentSegFillCount = 0;
        }
        break;
    case 41:
        if (m_currentVertexIdx >= 0
            && m_currentElementIdx < static_cast<int>(vertlist[m_currentVertexIdx].segParms.size())) {
            vertlist[m_currentVertexIdx].segParms[m_currentElementIdx]
                .push_back(reader->getDouble());
        }
        break;
    case 75:
        if (m_currentVertexIdx >= 0) {
            m_currentSegFillCount = reader->getInt32();
            // After fills are emitted (or count==0 immediate), advance element.
            if (m_currentSegFillCount == 0
                && m_currentElementIdx + 1 < numLines) {
                ++m_currentElementIdx;
            }
        }
        break;
    case 42:
        if (m_currentVertexIdx >= 0
            && m_currentElementIdx < static_cast<int>(vertlist[m_currentVertexIdx].areaFillParms.size())) {
            vertlist[m_currentVertexIdx].areaFillParms[m_currentElementIdx]
                .push_back(reader->getDouble());
            if (static_cast<int>(vertlist[m_currentVertexIdx]
                                 .areaFillParms[m_currentElementIdx].size())
                >= m_currentSegFillCount
                && m_currentElementIdx + 1 < numLines) {
                ++m_currentElementIdx;
            }
        }
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

bool DRW_MLine::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    if (!DRW_Entity::parseDwg(version, buf, nullptr, bs)) return false;
    DRW_DBG("\n***************************** parsing MLINE *********************\n");
    // Per ODA §19.4.78 / libreDWG dwg_decode_MLINE:
    //   BD scale, RC justification, 3BD basePoint, BE extrusion,
    //   BS open/closed flag, RC num_lines, BS num_verts,
    //   then per-vertex: 3BD pos, 3BD vdir, 3BD mdir,
    //     per-line: BS num_segparms × BD parm, BS num_areafillparms × BD parm.
    scale         = buf->getBitDouble();
    justification = buf->getRawChar8();
    basePoint     = buf->get3BitDouble();
    extPoint      = buf->getExtrusion(false);
    openClosed    = buf->getBitShort();
    numLines      = buf->getRawChar8();
    numVerts      = buf->getBitShort();
    DRW_DBG(" mline scale: "); DRW_DBG(scale);
    DRW_DBG(" just: "); DRW_DBG(static_cast<int>(justification));
    DRW_DBG(" openClosed: "); DRW_DBG(openClosed);
    DRW_DBG(" lines: "); DRW_DBG(static_cast<int>(numLines));
    DRW_DBG(" verts: "); DRW_DBG(numVerts); DRW_DBG("\n");
    // Sanity: numLines / numVerts are RC and BS so already small types,
    // but guard against pathological values anyway.
    if (numLines > 100) return true;
    vertlist.reserve(numVerts);
    for (int vi = 0; vi < numVerts; ++vi) {
        DRW_MLineVertex vtx;
        vtx.position  = buf->get3BitDouble();
        vtx.vertexDir = buf->get3BitDouble();
        vtx.miterDir  = buf->get3BitDouble();
        vtx.segParms.resize(numLines);
        vtx.areaFillParms.resize(numLines);
        for (int li = 0; li < numLines; ++li) {
            duint16 nSeg = buf->getBitShort();
            vtx.segParms[li].reserve(nSeg);
            for (int s = 0; s < nSeg; ++s) {
                vtx.segParms[li].push_back(buf->getBitDouble());
            }
            duint16 nFill = buf->getBitShort();
            vtx.areaFillParms[li].reserve(nFill);
            for (int f = 0; f < nFill; ++f) {
                vtx.areaFillParms[li].push_back(buf->getBitDouble());
            }
        }
        vertlist.push_back(std::move(vtx));
    }
    if (!DRW_Entity::parseDwgEntHandle(version, buf)) return false;
    // MLINE has one extra handle in the handle stream after the standard
    // entity handles: the MLINESTYLE reference. Read if available — some
    // older files (R14) store the style name inline instead.
    if (version > DRW::AC1014 && buf->numRemainingBytes() >= 2) {
        dwgHandle styleH = buf->getOffsetHandle(handle);
        styleHandle = styleH.ref;
        DRW_DBG(" MLINE style handle: ");
        DRW_DBGHL(styleH.code, styleH.size, styleH.ref); DRW_DBG("\n");
    }
    return buf->isGood();
}


// ----------------------------------------------------------------------------
// DRW_Underlay — UNDERLAY entity (PDFUNDERLAY/DGNUNDERLAY/DWFUNDERLAY).
// libreDWG UNDERLAYREFERENCE.spec field order:
//   extrusion (BE) -> position (3BD) -> angle (BD radians) -> scale (3BD)
//   -> flags (RC) -> contrast (RC) -> fade (RC) -> num_clip (BL)
//   -> clip_verts (2RD × num_clip).
// Handle stream after standard entity handles: definition_id (H).
// ----------------------------------------------------------------------------

bool DRW_Underlay::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 340:
        definitionHandle = static_cast<duint32>(reader->getHandleString());
        break;
    case 10: position.x = reader->getDouble(); break;
    case 20: position.y = reader->getDouble(); break;
    case 30: position.z = reader->getDouble(); break;
    case 41: scale.x = reader->getDouble(); break;
    case 42: scale.y = reader->getDouble(); break;
    case 43: scale.z = reader->getDouble(); break;
    case 50: rotation = reader->getDouble(); break;  // degrees in DXF
    case 210: extPoint.x = reader->getDouble(); break;
    case 220: extPoint.y = reader->getDouble(); break;
    case 230: extPoint.z = reader->getDouble(); break;
    case 280: flags    = static_cast<duint8>(reader->getInt32() & 0xFF); break;
    case 281: contrast = static_cast<duint8>(reader->getInt32() & 0xFF); break;
    case 282: fade     = static_cast<duint8>(reader->getInt32() & 0xFF); break;
    case 11: {
        ++m_currentClipVertexIdx;
        if (m_currentClipVertexIdx >= static_cast<int>(clipBoundary.size())) {
            clipBoundary.resize(m_currentClipVertexIdx + 1);
        }
        clipBoundary[m_currentClipVertexIdx].x = reader->getDouble();
        break;
    }
    case 21:
        if (m_currentClipVertexIdx >= 0
            && m_currentClipVertexIdx < static_cast<int>(clipBoundary.size())) {
            clipBoundary[m_currentClipVertexIdx].y = reader->getDouble();
        }
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

bool DRW_Underlay::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    if (!DRW_Entity::parseDwg(version, buf, nullptr, bs)) return false;
    DRW_DBG("\n***************************** parsing UNDERLAY ***************\n");
    extPoint = buf->getExtrusion(false);
    position = buf->get3BitDouble();
    rotation = buf->getBitDouble();   // angle (radians) BEFORE scale
    scale    = buf->get3BitDouble();
    flags    = buf->getRawChar8();
    contrast = buf->getRawChar8();
    fade     = buf->getRawChar8();
    duint32 nClip = buf->getBitLong();
    DRW_DBG(" UNDERLAY pos: "); DRW_DBG(position.x); DRW_DBG(",");
    DRW_DBG(position.y); DRW_DBG(" rot: "); DRW_DBG(rotation);
    DRW_DBG(" flags: "); DRW_DBGH(flags);
    DRW_DBG(" nClip: "); DRW_DBG(nClip); DRW_DBG("\n");
    if (nClip > 100000) return true;  // sanity
    clipBoundary.reserve(nClip);
    for (duint32 i = 0; i < nClip; ++i) {
        DRW_Coord p;
        p.x = buf->getRawDouble();
        p.y = buf->getRawDouble();
        p.z = 0.0;
        clipBoundary.push_back(p);
    }
    if (!DRW_Entity::parseDwgEntHandle(version, buf)) return false;
    if (version > DRW::AC1014 && buf->numRemainingBytes() >= 2) {
        dwgHandle defH = buf->getOffsetHandle(handle);
        definitionHandle = defH.ref;
        DRW_DBG(" UNDERLAY definitionHandle: ");
        DRW_DBGHL(defH.code, defH.size, defH.ref); DRW_DBG("\n");
    }
    return buf->isGood();
}


bool DRW_Text::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 40:
        height = reader->getDouble();
        break;
    case 41:
        widthscale = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        break;
    case 51:
        oblique = reader->getDouble();
        break;
    case 71:
        textgen = reader->getInt32();
        break;
    case 72:
        alignH = (HAlign)reader->getInt32();
        break;
    case 73:
        alignV = (VAlign)reader->getInt32();
        break;
    case 1:
        text = reader->getUtf8String();
        break;
    case 7:
        style = reader->getUtf8String();
        break;
    default:
        return DRW_Line::parseCode(code, reader);
    }

    return true;
}

bool DRW_Text::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing text *********************************************\n");

 // DataFlags RC Used to determine presence of subsequent data, set to 0xFF for R14-
    duint8 data_flags = 0x00;
    if (version > DRW::AC1014) {//2000+
        data_flags = buf->getRawChar8(); /* DataFlags RC Used to determine presence of subsequent data */
        DRW_DBG("data_flags: "); DRW_DBG(data_flags); DRW_DBG("\n");
        if ( !(data_flags & 0x01) ) { /* Elevation RD --- present if !(DataFlags & 0x01) */
            basePoint.z = buf->getRawDouble();
        }
    } else {//14-
        basePoint.z = buf->getBitDouble(); /* Elevation BD --- */
    }
    basePoint.x = buf->getRawDouble(); /* Insertion pt 2RD 10 */
    basePoint.y = buf->getRawDouble();
    DRW_DBG("Insert point: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z); DRW_DBG("\n");
    if (version > DRW::AC1014) {//2000+
        if ( !(data_flags & 0x02) ) { /* Alignment pt 2DD 11 present if !(DataFlags & 0x02), use 10 & 20 values for 2 default values.*/
            secPoint.x = buf->getDefaultDouble(basePoint.x);
            secPoint.y = buf->getDefaultDouble(basePoint.y);
        } else {
            secPoint = basePoint;
        }
    } else {//14-
        secPoint.x = buf->getRawDouble();  /* Alignment pt 2RD 11 */
        secPoint.y = buf->getRawDouble();
    }
    secPoint.z = basePoint.z;
    DRW_DBG("Alignment: "); DRW_DBGPT(secPoint.x, secPoint.y, basePoint.z); DRW_DBG("\n");
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    DRW_DBG("Extrusion: "); DRW_DBGPT(extPoint.x, extPoint.y, extPoint.z); DRW_DBG("\n");
    thickness = buf->getThickness(version > DRW::AC1014); /* Thickness BD 39 */

    if (version > DRW::AC1014) {//2000+
        if ( !(data_flags & 0x04) ) { /* Oblique ang RD 51 present if !(DataFlags & 0x04) */
            oblique = buf->getRawDouble();
        }
        if ( !(data_flags & 0x08) ) { /* Rotation ang RD 50 present if !(DataFlags & 0x08) */
            angle = buf->getRawDouble();
        }
        height = buf->getRawDouble(); /* Height RD 40 */
        if ( !(data_flags & 0x10) ) { /* Width factor RD 41 present if !(DataFlags & 0x10) */
            widthscale = buf->getRawDouble();
        }
    } else {//14-
        oblique = buf->getBitDouble(); /* Oblique ang BD 51 */
        angle = buf->getBitDouble(); /* Rotation ang BD 50 */
        height = buf->getBitDouble(); /* Height BD 40 */
        widthscale = buf->getBitDouble(); /* Width factor BD 41 */
    }
    angle *= ARAD;
    DRW_DBG("thickness: "); DRW_DBG(thickness); DRW_DBG(", Oblique ang: "); DRW_DBG(oblique); DRW_DBG(", Width: ");
    DRW_DBG(widthscale); DRW_DBG(", Rotation: "); DRW_DBG(angle); DRW_DBG(", height: "); DRW_DBG(height); DRW_DBG("\n");
    text = sBuf->getVariableText(version, false); /* Text value TV 1 */
    DRW_DBG("text string: "); DRW_DBG(text.c_str());DRW_DBG("\n");
    //textgen, alignH, alignV always present in R14-, data_flags set in initialisation
    if ( !(data_flags & 0x20) ) { /* Generation BS 71 present if !(DataFlags & 0x20) */
        textgen = buf->getBitShort();
        DRW_DBG("textgen: "); DRW_DBG(textgen);
    }
    if ( !(data_flags & 0x40) ) { /* Horiz align. BS 72 present if !(DataFlags & 0x40) */
        alignH = (HAlign)buf->getBitShort();
        DRW_DBG(", alignH: "); DRW_DBG(alignH);
    }
    if ( !(data_flags & 0x80) ) { /* Vert align. BS 73 present if !(DataFlags & 0x80) */
        alignV = (VAlign)buf->getBitShort();
        DRW_DBG(", alignV: "); DRW_DBG(alignV);
    }
    DRW_DBG("\n");

    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;

    styleH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    DRW_DBG("text style Handle: "); DRW_DBGHL(styleH.code, styleH.size, styleH.ref); DRW_DBG("\n");

    /* CRC X --- */
    return buf->isGood();
}

// Out-of-line special members: required because mtext is a unique_ptr<DRW_MText>
// declared with a forward-declared element type in the header.
DRW_Attrib::~DRW_Attrib() = default;
DRW_Attrib::DRW_Attrib(const DRW_Attrib& o)
    : DRW_Text(o), tag(o.tag), attribFlags(o.attribFlags),
      lockPosition(o.lockPosition), attVersion(o.attVersion),
      mtext(o.mtext ? std::make_unique<DRW_MText>(*o.mtext) : nullptr) {}
DRW_Attrib& DRW_Attrib::operator=(const DRW_Attrib& o) {
    if (this != &o) {
        DRW_Text::operator=(o);
        tag = o.tag;
        attribFlags = o.attribFlags;
        lockPosition = o.lockPosition;
        attVersion = o.attVersion;
        mtext = o.mtext ? std::make_unique<DRW_MText>(*o.mtext) : nullptr;
    }
    return *this;
}
DRW_Attrib::DRW_Attrib(DRW_Attrib&&) noexcept = default;
DRW_Attrib& DRW_Attrib::operator=(DRW_Attrib&&) noexcept = default;

bool DRW_Attrib::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // Multi-line ATTRIB (R2018+, ODA spec §20.4.4): an embedded MTEXT object
    // is introduced by the DXF subclass marker `100 / Embedded Object` (NOT
    // `AcDbMText`).  After the marker, the standard MTEXT group codes follow
    // (10/20/30 insertion, 11/21/31 X-axis, 40 height, 41 rect width, 71
    // attachment point, 72 drawing direction, 1 formatted text, etc.), then
    // the ATTRIB-specific tail (tag=2, prompt=3 for ATTDEF, flags=70,
    // lock-position=280) which we must NOT route into the embedded MText.
    if (code == 100) {
        const std::string sub = reader->getString();
        if (sub == "Embedded Object" && !mtext) {
            mtext = std::make_unique<DRW_MText>();
            if (attVersion == 0) attVersion = 1;
        }
        return true;
    }
    // Inside the embedded MText scope, route MTEXT-owned codes to mtext but
    // keep ATTRIB-specific tail codes for ATTRIB / ATTDEF handling below.
    if (mtext) {
        switch (code) {
        case 2:    // tag (ATTRIB-specific; group 1 in MText is text body)
        case 3:    // prompt (ATTDEF-specific)
        case 70:   // ATTRIB flags
        case 280:  // ATTRIB lock-position
            break; // fall through to ATTRIB handling below
        default:
            return mtext->parseCode(code, reader);
        }
    }
    switch (code) {
    case 2:
        tag = reader->getUtf8String();
        break;
    case 70:
        attribFlags = reader->getInt32();
        break;
    case 280:
        // Lock position flag (R2010+ DXF group code)
        lockPosition = reader->getInt32() != 0;
        break;
    default:
        return DRW_Text::parseCode(code, reader);
    }
    return true;
}

bool DRW_Attrib::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing attrib *********************************************\n");

    // R2010+: leading version byte (per ODA spec §20.4.65)
    if (version >= DRW::AC1024) {
        attVersion = buf->getRawChar8();
        DRW_DBG("att version: "); DRW_DBG(attVersion); DRW_DBG("\n");
    }

    // Inline TEXT subtype data (mirrors DRW_Text::parseDwg layout, sans handles)
    duint8 data_flags = 0x00;
    if (version > DRW::AC1014) {
        data_flags = buf->getRawChar8();
        if (!(data_flags & 0x01)) {
            basePoint.z = buf->getRawDouble();
        }
    } else {
        basePoint.z = buf->getBitDouble();
    }
    basePoint.x = buf->getRawDouble();
    basePoint.y = buf->getRawDouble();
    DRW_DBG("Insert point: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z); DRW_DBG("\n");
    if (version > DRW::AC1014) {
        if (!(data_flags & 0x02)) {
            secPoint.x = buf->getDefaultDouble(basePoint.x);
            secPoint.y = buf->getDefaultDouble(basePoint.y);
        } else {
            secPoint = basePoint;
        }
    } else {
        secPoint.x = buf->getRawDouble();
        secPoint.y = buf->getRawDouble();
    }
    secPoint.z = basePoint.z;
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    thickness = buf->getThickness(version > DRW::AC1014);
    if (version > DRW::AC1014) {
        if (!(data_flags & 0x04)) oblique = buf->getRawDouble();
        if (!(data_flags & 0x08)) angle = buf->getRawDouble();
        height = buf->getRawDouble();
        if (!(data_flags & 0x10)) widthscale = buf->getRawDouble();
    } else {
        oblique = buf->getBitDouble();
        angle = buf->getBitDouble();
        height = buf->getBitDouble();
        widthscale = buf->getBitDouble();
    }
    angle *= ARAD;
    text = sBuf->getVariableText(version, false);
    DRW_DBG("text value: "); DRW_DBG(text.c_str()); DRW_DBG("\n");
    if (!(data_flags & 0x20)) textgen = buf->getBitShort();
    if (!(data_flags & 0x40)) alignH = (HAlign)buf->getBitShort();
    if (!(data_flags & 0x80)) alignV = (VAlign)buf->getBitShort();

    // ATTRIB-specific fields
    tag = sBuf->getVariableText(version, false);
    DRW_DBG("attrib tag: "); DRW_DBG(tag.c_str()); DRW_DBG("\n");

    duint16 fieldLen = buf->getBitShort(); /* Field length BS (always 0) */
    DRW_UNUSED(fieldLen);

    attribFlags = buf->getRawChar8();
    DRW_DBG("attrib flags: "); DRW_DBG(attribFlags); DRW_DBG("\n");

    if (version >= DRW::AC1024) {
        lockPosition = buf->getBit();
        DRW_DBG("lock position: "); DRW_DBG(lockPosition); DRW_DBG("\n");
    }

    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;

    styleH = buf->getHandle();
    DRW_DBG("text style Handle: "); DRW_DBGHL(styleH.code, styleH.size, styleH.ref); DRW_DBG("\n");

    return buf->isGood();
}

bool DRW_Attdef::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 3:
        prompt = reader->getUtf8String();
        break;
    default:
        return DRW_Attrib::parseCode(code, reader);
    }
    return true;
}

bool DRW_Attdef::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    // ATTDEF mirrors ATTRIB layout but adds a prompt string after the tag.
    // Implementation duplicates ATTRIB::parseDwg in order to inject the
    // prompt read at the correct offset; refactor opportunity if a third
    // sibling appears.
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {
        sBuf = &sBuff;
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing attdef *********************************************\n");

    if (version >= DRW::AC1024) {
        attVersion = buf->getRawChar8();
    }

    duint8 data_flags = 0x00;
    if (version > DRW::AC1014) {
        data_flags = buf->getRawChar8();
        if (!(data_flags & 0x01)) basePoint.z = buf->getRawDouble();
    } else {
        basePoint.z = buf->getBitDouble();
    }
    basePoint.x = buf->getRawDouble();
    basePoint.y = buf->getRawDouble();
    if (version > DRW::AC1014) {
        if (!(data_flags & 0x02)) {
            secPoint.x = buf->getDefaultDouble(basePoint.x);
            secPoint.y = buf->getDefaultDouble(basePoint.y);
        } else {
            secPoint = basePoint;
        }
    } else {
        secPoint.x = buf->getRawDouble();
        secPoint.y = buf->getRawDouble();
    }
    secPoint.z = basePoint.z;
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    thickness = buf->getThickness(version > DRW::AC1014);
    if (version > DRW::AC1014) {
        if (!(data_flags & 0x04)) oblique = buf->getRawDouble();
        if (!(data_flags & 0x08)) angle = buf->getRawDouble();
        height = buf->getRawDouble();
        if (!(data_flags & 0x10)) widthscale = buf->getRawDouble();
    } else {
        oblique = buf->getBitDouble();
        angle = buf->getBitDouble();
        height = buf->getBitDouble();
        widthscale = buf->getBitDouble();
    }
    angle *= ARAD;
    text = sBuf->getVariableText(version, false);
    if (!(data_flags & 0x20)) textgen = buf->getBitShort();
    if (!(data_flags & 0x40)) alignH = (HAlign)buf->getBitShort();
    if (!(data_flags & 0x80)) alignV = (VAlign)buf->getBitShort();

    tag = sBuf->getVariableText(version, false);
    DRW_DBG("attdef tag: "); DRW_DBG(tag.c_str()); DRW_DBG("\n");

    duint16 fieldLen = buf->getBitShort();
    DRW_UNUSED(fieldLen);

    attribFlags = buf->getRawChar8();

    if (version >= DRW::AC1024) {
        lockPosition = buf->getBit();
    }

    // ATTDEF prompt follows attrib body
    prompt = sBuf->getVariableText(version, false);
    DRW_DBG("attdef prompt: "); DRW_DBG(prompt.c_str()); DRW_DBG("\n");

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;

    styleH = buf->getHandle();

    return buf->isGood();
}

bool DRW_MText::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 1:
        text += reader->getString();
        text = reader->toUtf8String(text);
        break;
    case 11:
        hasXAxisVec = true;
        return DRW_Text::parseCode(code, reader);
    case 3:
        text += reader->getString();
        break;
    case 44:
        interlin = reader->getDouble();
        break;
    case 50: // djm: per dxf docs, last of code 11 or code 50 prevails
        hasXAxisVec = false;
        angle = reader->getDouble();
        break;
    default:
        return DRW_Text::parseCode(code, reader);
    }

    return true;
}

bool DRW_MText::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing mtext *********************************************\n");

    basePoint = buf->get3BitDouble(); /* Insertion pt 3BD 10 - First picked point. */
    DRW_DBG("Insertion: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z); DRW_DBG("\n");
    extPoint = buf->get3BitDouble(); /* Extrusion 3BD 210 Undocumented; */
    secPoint = buf->get3BitDouble(); /* X-axis dir 3BD 11 */
    hasXAxisVec = true;
    updateAngle();
    widthscale = buf->getBitDouble(); /* Rect width BD 41 */
    if (version > DRW::AC1018) {//2007+
        /* Rect height BD 46 Reference rectangle height. */
        /** @todo */buf->getBitDouble();
    }
    height = buf->getBitDouble();/* Text height BD 40 Undocumented */
    textgen = buf->getBitShort(); /* Attachment BS 71 Similar to justification; */
    /* Drawing dir BS 72 Left to right, etc.; see DXF doc. Reuse the
     * inherited alignH slot — for MTEXT this field carries the DXF group 72
     * "drawing direction" code (1=LtoR, 3=TtoB, 5=ByStyle), not the TEXT
     * horizontal-alignment values the HAlign enum was named for. The integer
     * round-trips cleanly; consumers compare against the raw integer. */
    alignH = static_cast<HAlign>(buf->getBitShort());
    /* Extents ht BD Undocumented and not present in DXF or entget */
    double ext_ht = buf->getBitDouble();
    DRW_UNUSED(ext_ht);
    /* Extents wid BD Undocumented and not present in DXF or entget The extents
    rectangle, when rotated the same as the text, fits the actual text image on
    the screen (although we've seen it include an extra row of text in height). */
    double ext_wid = buf->getBitDouble();
    DRW_UNUSED(ext_wid);
    /* Text TV 1 All text in one long string (without '\n's 3 for line wrapping).
    ACAD seems to add braces ({ }) and backslash-P's to indicate paragraphs
    based on the "\r\n"'s found in the imported file. But, all the text is in
    this one long string -- not broken into 1- and 3-groups as in DXF and
    entget. ACAD's entget breaks this string into 250-char pieces (not 255 as
    doc'd) – even if it's mid-word. The 1-group always gets the tag end;
    therefore, the 3's are always 250 chars long. */
    text = sBuf->getVariableText(version, false); /* Text value TV 1 */
    if (version > DRW::AC1014) {//2000+
        buf->getBitShort();/* Linespacing Style BS 73 */
        buf->getBitDouble();/* Linespacing Factor BD 44 */
        buf->getBit();/* Unknown bit B */
    }
    if (version > DRW::AC1015) {//2004+
        /* Background flags BL 0 = no background, 1 = background fill, 2 =background
        fill with drawing fill color. */
        dint32 bk_flags = buf->getBitLong(); /** @todo add to DRW_MText */
        if ( bk_flags == 1 ) {
            /* Background scale factor BL Present if background flags = 1, default = 1.5*/
            buf->getBitLong();
            /* Background color CMC Present if background flags = 1 */
            buf->getCmColor(version); //RLZ: warning CMC or ENC
            /** @todo buf->getCMC */
            /* Background transparency BL Present if background flags = 1 */
            buf->getBitLong();
        }
    }

    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;

    styleH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    DRW_DBG("text style Handle: "); DRW_DBG(styleH.code); DRW_DBG(".");
    DRW_DBG(styleH.size); DRW_DBG("."); DRW_DBG(styleH.ref); DRW_DBG("\n");

    /* CRC X --- */
    return buf->isGood();
}

void DRW_MText::updateAngle() {
    if (hasXAxisVec) {
        angle = atan2(secPoint.y, secPoint.x) * ARAD;
    }
}

bool DRW_Polyline::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 70:
        flags = reader->getInt32();
        break;
    case 40:
        defstawidth = reader->getDouble();
        break;
    case 41:
        defendwidth = reader->getDouble();
        break;
    case 71:
        vertexcount = reader->getInt32();
        break;
    case 72:
        facecount = reader->getInt32();
        break;
    case 73:
        smoothM = reader->getInt32();
        break;
    case 74:
        smoothN = reader->getInt32();
        break;
    case 75:
        curvetype = reader->getInt32();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

//0x0F polyline 2D bit 4(8) & 5(16) NOT set
//0x10 polyline 3D bit 4(8) set
//0x1D PFACE bit 5(16) set
bool DRW_Polyline::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing polyline *********************************************\n");

    dint32 ooCount = 0;
    if (oType == 0x0F) { //pline 2D
        flags = buf->getBitShort();
        DRW_DBG("flags value: "); DRW_DBG(flags);
        curvetype = buf->getBitShort();
        defstawidth = buf->getBitDouble();
        defendwidth = buf->getBitDouble();
        thickness = buf->getThickness(version > DRW::AC1014);
        basePoint = DRW_Coord(0,0,buf->getBitDouble());
        extPoint = buf->getExtrusion(version > DRW::AC1014);
    } else if (oType == 0x10) { //pline 3D
        duint8 tmpFlag = buf->getRawChar8();
        DRW_DBG("flags 1 value: "); DRW_DBG(tmpFlag);
        if (tmpFlag & 1)
            curvetype = 5;
        else if (tmpFlag & 2)
            curvetype = 6;
        if (tmpFlag & 3) {
            curvetype = 8;
            flags |= 4;
        }
        tmpFlag = buf->getRawChar8();
        if (tmpFlag & 1)
            flags |= 1;
        flags |= 8; //indicate 3DPOL
        DRW_DBG("flags 2 value: "); DRW_DBG(tmpFlag);
    } else if (oType == 0x1D) { //PFACE
        flags = 64;
        vertexcount = buf->getBitShort();
        DRW_DBG("vertex count: "); DRW_DBG(vertexcount);
        facecount = buf->getBitShort();
        DRW_DBG("face count: "); DRW_DBG(facecount);
        DRW_DBG("flags value: "); DRW_DBG(flags);
    } else if (oType == 0x1E) { //POLYLINE_MESH per ODA spec sec 19.4.31
        flags = buf->getBitShort();
        DRW_DBG("flags value: "); DRW_DBG(flags);
        flags |= 16; //bit 4 = 3D polygon mesh
        curvetype = buf->getBitShort();
        vertexcount = buf->getBitShort(); //M-count
        DRW_DBG(" M count: "); DRW_DBG(vertexcount);
        facecount = buf->getBitShort(); //N-count
        DRW_DBG(" N count: "); DRW_DBG(facecount);
        /*dint16 mDensity =*/ buf->getBitShort();
        /*dint16 nDensity =*/ buf->getBitShort();
    }
    if (version > DRW::AC1015){ //2004+
        ooCount = buf->getBitLong();
    }

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;

    if (version < DRW::AC1018){ //2000-
        dwgHandle objectH = buf->getOffsetHandle(handle);
        firstEH = objectH.ref;
        DRW_DBG(" first Vertex Handle: "); DRW_DBGHL(objectH.code, objectH.size, objectH.ref); DRW_DBG("\n");
        objectH = buf->getOffsetHandle(handle);
        lastEH = objectH.ref;
        DRW_DBG(" last Vertex Handle: "); DRW_DBGHL(objectH.code, objectH.size, objectH.ref); DRW_DBG("\n");
        DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    } else {
        for (dint32 i = 0; i < ooCount; ++i){
                dwgHandle objectH = buf->getOffsetHandle(handle);
                hadlesList.push_back (objectH.ref);
                DRW_DBG(" Vertex Handle: "); DRW_DBGHL(objectH.code, objectH.size, objectH.ref); DRW_DBG("\n");
                DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        }
    }
    seqEndH = buf->getOffsetHandle(handle);
    DRW_DBG(" SEQEND Handle: "); DRW_DBGHL(seqEndH.code, seqEndH.size, seqEndH.ref); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

//    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Vertex::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 70:
        flags = reader->getInt32();
        break;
    case 40:
        stawidth = reader->getDouble();
        break;
    case 41:
        endwidth = reader->getDouble();
        break;
    case 42:
        bulge = reader->getDouble();
        break;
    case 50:
        tgdir = reader->getDouble();
        break;
    case 71:
        vindex1 = reader->getInt32();
        break;
    case 72:
        vindex2 = reader->getInt32();
        break;
    case 73:
        vindex3 = reader->getInt32();
        break;
    case 74:
        vindex4 = reader->getInt32();
        break;
    case 91:
        identifier = reader->getInt32();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

//0x0A vertex 2D
//0x0B vertex 3D
//0x0C MESH
//0x0D PFACE
//0x0E PFACE FACE
bool DRW_Vertex::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs, double el){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing pline Vertex *********************************************\n");

    if (oType == 0x0A) { //pline 2D, needed example
        flags = buf->getRawChar8(); //RLZ: EC  unknown type
        DRW_DBG("flags value: "); DRW_DBG(flags);
        basePoint = buf->get3BitDouble();
        basePoint.z = el;
        DRW_DBG("basePoint: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
        stawidth = buf->getBitDouble();
        if (stawidth < 0)
            endwidth = stawidth = fabs(stawidth);
        else
            endwidth = buf->getBitDouble();
        bulge = buf->getBitDouble();
        if (version > DRW::AC1021) { //2010+
            DRW_DBG("Vertex ID: "); DRW_DBG(buf->getBitLong());
        }
        tgdir = buf->getBitDouble();
    } else if (oType == 0x0B || oType == 0x0C || oType == 0x0D) { //PFACE
        flags = buf->getRawChar8(); //RLZ: EC  unknown type
        DRW_DBG("flags value: "); DRW_DBG(flags);
        basePoint = buf->get3BitDouble();
        DRW_DBG("basePoint: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
    } else if (oType == 0x0E) { //PFACE FACE
        vindex1 = buf->getBitShort();
        vindex2 = buf->getBitShort();
        vindex3 = buf->getBitShort();
        vindex4 = buf->getBitShort();
    }

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Hatch::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 2:
        name = reader->getUtf8String();
        break;
    case 70:
        solid = reader->getInt32();
        break;
    case 71:
        associative = reader->getInt32();
        break;
    case 72:        /*edge type*/
        if (ispol){ //if is polyline is a as_bulge flag
            break;
        } else if (reader->getInt32() == 1){ //line
            addLine();
        } else if (reader->getInt32() == 2){ //arc
            addArc();
        } else if (reader->getInt32() == 3){ //elliptic arc
            addEllipse();
        } else if (reader->getInt32() == 4){ //spline
            addSpline();
        }
        break;
    case 10:
        // Spline edge: 10 is a control-point x-coord.
        if (spline) {
            spline->controllist.push_back(std::make_shared<DRW_Coord>(reader->getDouble(), 0.0, 0.0));
            break;
        }
        if (pt) pt->basePoint.x = reader->getDouble();
        else if (pline) {
            plvert = pline->addVertex();
            plvert->x = reader->getDouble();
        } else {
            // After group 98 the boundary path is closed; seed-point
            // coords arrive as group-10/20 pairs.
            DRW_Coord seed;
            seed.x = reader->getDouble();
            seedPoints.push_back(seed);
        }
        break;
    case 20:
        if (spline && !spline->controllist.empty()) {
            spline->controllist.back()->y = reader->getDouble();
            break;
        }
        if (pt) pt->basePoint.y = reader->getDouble();
        else if (plvert) plvert ->y = reader->getDouble();
        else if (!seedPoints.empty())
            seedPoints.back().y = reader->getDouble();
        break;
    case 11:
        // Spline edge: 11 is a fit-point x-coord.
        if (spline) {
            spline->fitlist.push_back(std::make_shared<DRW_Coord>(reader->getDouble(), 0.0, 0.0));
            break;
        }
        if (line) line->secPoint.x = reader->getDouble();
        else if (ellipse) ellipse->secPoint.x = reader->getDouble();
        break;
    case 21:
        if (spline && !spline->fitlist.empty()) {
            spline->fitlist.back()->y = reader->getDouble();
            break;
        }
        if (line) line->secPoint.y = reader->getDouble();
        else if (ellipse) ellipse->secPoint.y = reader->getDouble();
        break;
    case 12:
        if (spline) { spline->tgStart.x = reader->getDouble(); break; }
        break;
    case 22:
        if (spline) { spline->tgStart.y = reader->getDouble(); break; }
        break;
    case 13:
        if (spline) { spline->tgEnd.x = reader->getDouble(); break; }
        break;
    case 23:
        if (spline) { spline->tgEnd.y = reader->getDouble(); break; }
        break;
    case 40:
        // Spline edge: 40 is a knot value (occurs nknots times).
        if (spline) {
            spline->knotslist.push_back(reader->getDouble());
            break;
        }
        if (arc) arc->radious = reader->getDouble();
        else if (ellipse) ellipse->ratio = reader->getDouble();
        break;
    case 41:
        scale = reader->getDouble();
        break;
    case 42:
        // Spline edge: 42 is a per-control-point weight.
        if (spline) {
            spline->weightlist.push_back(reader->getDouble());
            break;
        }
        if (plvert) plvert ->bulge = reader->getDouble();
        break;
    case 50:
        if (arc) arc->staangle = reader->getDouble()/ARAD;
        else if (ellipse) ellipse->staparam = reader->getDouble()/ARAD;
        break;
    case 51:
        if (arc) arc->endangle = reader->getDouble()/ARAD;
        else if (ellipse) ellipse->endparam = reader->getDouble()/ARAD;
        break;
    case 52:
        angle = reader->getDouble();
        break;
    case 73:
        // Spline edge: 73 is the rational flag (1 = rational).
        if (spline) {
            if (reader->getInt32()) spline->flags |= 0x4;
            break;
        }
        if (arc) arc->isccw = reader->getInt32();
        else if (pline) pline->flags = reader->getInt32();
        break;
    case 74:
        // Spline edge: 74 is the periodic flag (1 = periodic/closed).
        if (spline) {
            if (reader->getInt32()) spline->flags |= 0x2;
        }
        break;
    case 94:
        // Spline edge degree.
        if (spline) spline->degree = reader->getInt32();
        break;
    case 95:
        // Spline edge number of knots.
        if (spline) spline->nknots = reader->getInt32();
        break;
    case 96:
        // Spline edge number of control points.
        if (spline) spline->ncontrol = reader->getInt32();
        break;
    case 97:
        // Spline edge number of fit points (also used as terminator for
        // non-spline edges — only consume when spline is current).
        if (spline) spline->nfit = reader->getInt32();
        break;
    case 75:
        hstyle = reader->getInt32();
        break;
    case 76:
        hpattern = reader->getInt32();
        break;
    case 77:
        doubleflag = reader->getInt32();
        break;
    case 78:
        deflines = reader->getInt32();
        break;
    case 91:
        loopsnum = reader->getInt32();
        return DRW::reserve( looplist, loopsnum);
    case 92:
        loop = std::make_shared<DRW_HatchLoop>(reader->getInt32());
        looplist.push_back(loop);
        if (reader->getInt32() & 2) {
            ispol = true;
            clearEntities();
            pline = std::make_shared<DRW_LWPolyline>();
            loop->objlist.push_back(pline);
        } else ispol = false;
        break;
    case 93:
        if (pline) pline->vertexnum = reader->getInt32();
        else if (loop) loop->numedges = reader->getInt32();//aqui reserve
        break;
    case 98: { // seed-point count; coords follow as group-10/20 pairs
        clearEntities();
        const int count = reader->getInt32();
        if (count > 0)
            DRW::reserve(seedPoints, count);
        break;
    }
    case 450:
        isGradient = reader->getInt32();
        break;
    case 451:
        gradReserved = reader->getInt32();
        break;
    case 452:
        singleColor = reader->getInt32();
        break;
    case 453: {
        const int n = reader->getInt32();
        if (n > 0)
            DRW::reserve(gradColors, n);
        break;
    }
    case 460:
        gradAngle = reader->getDouble();
        break;
    case 461:
        gradShift = reader->getDouble();
        break;
    case 462:
        gradTint = reader->getDouble();
        break;
    case 463: { // gradient stop value
        DRW_Hatch::GradientStop stop;
        stop.value = reader->getDouble();
        gradColors.push_back(stop);
        break;
    }
    case 421:
        if (!gradColors.empty())
            gradColors.back().rgb = reader->getInt32();
        break;
    case 63:
        if (!gradColors.empty())
            gradColors.back().aciColor = reader->getInt32();
        else
            return DRW_Point::parseCode(code, reader);
        break;
    case 470:
        gradName = reader->getUtf8String();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}

bool DRW_Hatch::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    duint32 totalBoundItems = 0;
    bool havePixelSize = false;

    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing hatch *********************************************\n");

    //Gradient data, RLZ: is ok or if grad > 0 continue read ?
    if (version > DRW::AC1015) { //2004+
        isGradient = buf->getBitLong();
        DRW_DBG("is Gradient: "); DRW_DBG(isGradient);
        gradReserved = buf->getBitLong();
        DRW_DBG(" reserved: "); DRW_DBG(gradReserved);
        gradAngle = buf->getBitDouble();
        DRW_DBG(" Gradient angle: "); DRW_DBG(gradAngle);
        gradShift = buf->getBitDouble();
        DRW_DBG(" Gradient shift: "); DRW_DBG(gradShift);
        singleColor = buf->getBitLong();
        DRW_DBG("\nsingle color Grad: "); DRW_DBG(singleColor);
        gradTint = buf->getBitDouble();
        DRW_DBG(" Gradient tint: "); DRW_DBG(gradTint);
        dint32 numCol = buf->getBitLong();
        DRW_DBG(" num colors: "); DRW_DBG(numCol);
        if (numCol > 0)
            DRW::reserve(gradColors, numCol);
        for (dint32 i = 0 ; i < numCol; ++i){
            GradientStop stop;
            // First field is the stop position (per libreDWG: BD/unkDouble holds
            // the stop value in [0,1]); falls back to even spacing if missing.
            stop.value = buf->getBitDouble();
            DRW_DBG("\nstop value: "); DRW_DBG(stop.value);
            duint16 unkShort = buf->getBitShort();
            DRW_DBG(" unkShort: "); DRW_DBG(unkShort);
            stop.rgb = buf->getBitLong();
            DRW_DBG(" rgb color: "); DRW_DBG(stop.rgb);
            duint8 ignCol = buf->getRawChar8();
            DRW_DBG(" ignored color: "); DRW_DBG(ignCol);
            gradColors.push_back(stop);
        }
        gradName = sBuf->getVariableText(version, false);
        DRW_DBG("\ngradient name: "); DRW_DBG(gradName.c_str()); DRW_DBG("\n");
    }
    basePoint.z = buf->getBitDouble();
    extPoint = buf->get3BitDouble();
    DRW_DBG("base point: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
    DRW_DBG("\nextrusion: "); DRW_DBGPT(extPoint.x, extPoint.y, extPoint.z);
    name = sBuf->getVariableText(version, false);
    DRW_DBG("\nhatch pattern name: "); DRW_DBG(name.c_str()); DRW_DBG("\n");
    solid = buf->getBit();
    associative = buf->getBit();
    loopsnum = buf->getBitLong();
    DRW_DBG("solid: "); DRW_DBG(solid); DRW_DBG(" associative: "); DRW_DBG(associative);
    DRW_DBG(" loopsnum: "); DRW_DBG(loopsnum); DRW_DBG("\n");

    //read loops
    for (dint32 i = 0 ; i < loopsnum; ++i){
        loop = std::make_shared<DRW_HatchLoop>(buf->getBitLong());
        havePixelSize |= loop->type & 4;
        DRW_DBG(" loop["); DRW_DBG(i); DRW_DBG("] type: "); DRW_DBG(loop->type);
        if (!(loop->type & 2)){ //Not polyline
            dint32 numPathSeg = buf->getBitLong();
            DRW_DBG(" numPathSeg: "); DRW_DBG(numPathSeg); DRW_DBG("\n");
            for (dint32 j = 0; j<numPathSeg;++j){
                duint8 typePath = buf->getRawChar8();
                DRW_DBG("  seg["); DRW_DBG(j); DRW_DBG("] typePath: "); DRW_DBG(typePath); DRW_DBG("\n");
                if (typePath == 1){ //line
                    addLine();
                    line->basePoint = buf->get2RawDouble();
                    line->secPoint = buf->get2RawDouble();
                } else if (typePath == 2){ //circle arc
                    addArc();
                    arc->basePoint = buf->get2RawDouble();
                    arc->radious = buf->getBitDouble();
                    arc->staangle = buf->getBitDouble();
                    arc->endangle = buf->getBitDouble();
                    arc->isccw = buf->getBit();
                } else if (typePath == 3){ //ellipse arc
                    addEllipse();
                    ellipse->basePoint = buf->get2RawDouble();
                    ellipse->secPoint = buf->get2RawDouble();
                    ellipse->ratio = buf->getBitDouble();
                    ellipse->staparam = buf->getBitDouble();
                    ellipse->endparam = buf->getBitDouble();
                    ellipse->isccw = buf->getBit();
                } else if (typePath == 4){ //spline
                    addSpline();
                    spline->degree = buf->getBitLong();
                    bool isRational = buf->getBit();
                    spline->flags |= (isRational << 2); //rational
                    spline->flags |= (buf->getBit() << 1); //periodic
                    spline->nknots = buf->getBitLong();
                    if (!DRW::reserve( spline->knotslist, spline->nknots)) {
                        return false;
                    }
                    spline->ncontrol = buf->getBitLong();
                    if (!DRW::reserve( spline->controllist, spline->ncontrol)) {
                        return false;
                    }
                    for (dint32 j = 0; j < spline->nknots;++j){
                        spline->knotslist.push_back (buf->getBitDouble());
                    }
                    for (dint32 j = 0; j < spline->ncontrol;++j){
                        std::shared_ptr<DRW_Coord> crd = std::make_shared<DRW_Coord>(buf->get2RawDouble());
                        if(isRational)
                            crd->z =  buf->getBitDouble(); //RLZ: investigate how store weight
                        spline->controllist.push_back(crd);
                    }
                    if (version > DRW::AC1021) { //2010+
                        spline->nfit = buf->getBitLong();
                        if (!DRW::reserve( spline->fitlist, spline->nfit)) {
                            return false;
                        }
                        for (dint32 j = 0; j < spline->nfit;++j){
                            std::shared_ptr<DRW_Coord> crd = std::make_shared<DRW_Coord>(buf->get2RawDouble());
                            spline->fitlist.push_back(crd);
                        }
                        spline->tgStart = buf->get2RawDouble();
                        spline->tgEnd = buf->get2RawDouble();
                    }
                }
            }
        } else { //end not pline, start polyline
            pline = std::make_shared<DRW_LWPolyline>();
            bool asBulge = buf->getBit();
            pline->flags = buf->getBit();//closed bit
            dint32 numVert = buf->getBitLong();
            DRW_DBG(" asBulge: "); DRW_DBG(asBulge); DRW_DBG(" closed: "); DRW_DBG(pline->flags);
            DRW_DBG(" numVert: "); DRW_DBG(numVert); DRW_DBG("\n");
            for (dint32 j = 0; j<numVert;++j){
                DRW_Vertex2D v;
                v.x = buf->getRawDouble();
                v.y = buf->getRawDouble();
                if (asBulge)
                    v.bulge = buf->getBitDouble();
                pline->addVertex(v);
            }
            loop->objlist.push_back(pline);
        }//end polyline
        loop->update();
        looplist.push_back(loop);
        totalBoundItems += buf->getBitLong();
        DRW_DBG(" totalBoundItems: "); DRW_DBG(totalBoundItems);
    } //end read loops

    hstyle = buf->getBitShort();
    hpattern = buf->getBitShort();
    DRW_DBG("\nhatch style: "); DRW_DBG(hstyle); DRW_DBG(" pattern type"); DRW_DBG(hpattern);
    if (!solid){
        angle = buf->getBitDouble();
        scale = buf->getBitDouble();
        doubleflag = buf->getBit();
        deflines = buf->getBitShort();
        for (dint32 i = 0 ; i < deflines; ++i){
            DRW_Coord ptL, offL;
            double angleL = buf->getBitDouble();
            ptL.x = buf->getBitDouble();
            ptL.y = buf->getBitDouble();
            offL.x = buf->getBitDouble();
            offL.y = buf->getBitDouble();
            duint16 numDashL = buf->getBitShort();
            DRW_DBG("\ndef line: "); DRW_DBG(angleL); DRW_DBG(","); DRW_DBG(ptL.x); DRW_DBG(","); DRW_DBG(ptL.y);
            DRW_DBG(","); DRW_DBG(offL.x); DRW_DBG(","); DRW_DBG(offL.y); DRW_DBG(","); DRW_DBG(angleL);
            for (duint16 i = 0 ; i < numDashL; ++i){
                double lengthL = buf->getBitDouble();
                DRW_DBG(","); DRW_DBG(lengthL);
            }
        }//end deflines
    } //end not solid

    if (havePixelSize){
        ddouble64 pixsize = buf->getBitDouble();
        DRW_DBG("\npixel size: "); DRW_DBG(pixsize);
    }
    dint32 numSeedPoints = buf->getBitLong();
    DRW_DBG("\nnum Seed Points  "); DRW_DBG(numSeedPoints);
    if (numSeedPoints > 0)
        DRW::reserve(seedPoints, numSeedPoints);
    for (dint32 i = 0 ; i < numSeedPoints; ++i){
        DRW_Coord seedPt;
        seedPt.x = buf->getRawDouble();
        seedPt.y = buf->getRawDouble();
        DRW_DBG("\n  "); DRW_DBG(seedPt.x); DRW_DBG(","); DRW_DBG(seedPt.y);
        seedPoints.push_back(seedPt);
    }

    DRW_DBG("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    for (duint32 i = 0 ; i < totalBoundItems; ++i){
        dwgHandle biH = buf->getHandle();
        DRW_DBG("Boundary Items Handle: "); DRW_DBGHL(biH.code, biH.size, biH.ref);
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
//    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Spline::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 210:
        normalVec.x = reader->getDouble();
        break;
    case 220:
        normalVec.y = reader->getDouble();
        break;
    case 230:
        normalVec.z = reader->getDouble();
        break;
    case 12:
        tgStart.x = reader->getDouble();
        break;
    case 22:
        tgStart.y = reader->getDouble();
        break;
    case 32:
        tgStart.z = reader->getDouble();
        break;
    case 13:
        tgEnd.x = reader->getDouble();
        break;
    case 23:
        tgEnd.y = reader->getDouble();
        break;
    case 33:
        tgEnd.z = reader->getDouble();
        break;
    case 70:
        flags = reader->getInt32();
        break;
    case 71:
        degree = reader->getInt32();
        break;
    case 72:
        nknots = reader->getInt32();
        break;
    case 73:
        ncontrol = reader->getInt32();
        break;
    case 74:
        nfit = reader->getInt32();
        break;
    case 42:
        tolknot = reader->getDouble();
        break;
    case 43:
        tolcontrol = reader->getDouble();
        break;
    case 44:
        tolfit = reader->getDouble();
        break;
    case 10: {
        controlpoint = std::make_shared<DRW_Coord>();
        controllist.push_back(controlpoint);
        controlpoint->x = reader->getDouble();
        break; }
    case 20:
        if(controlpoint)
            controlpoint->y = reader->getDouble();
        break;
    case 30:
        if(controlpoint)
            controlpoint->z = reader->getDouble();
        break;
    case 11: {
        fitpoint = std::make_shared<DRW_Coord>();
        fitlist.push_back(fitpoint);
        fitpoint->x = reader->getDouble();
        break; }
    case 21:
        if(fitpoint)
            fitpoint->y = reader->getDouble();
        break;
    case 31:
        if(fitpoint)
            fitpoint->z = reader->getDouble();
        break;
    case 40:
        knotslist.push_back(reader->getDouble());
        break;
    case 41:
        weightlist.push_back(reader->getDouble());
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

bool DRW_Spline::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    bool ret = DRW_Entity::parseDwg(version, buf, NULL, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing spline *********************************************\n");
    duint8 weight = 0; // RLZ ??? flags, weight, code 70, bit 4 (16)

    dint32 scenario = buf->getBitLong();
    DRW_DBG("scenario: "); DRW_DBG(scenario);
    if (version > DRW::AC1024) {
        dint32 splFlag1 = buf->getBitLong();
        if (splFlag1 & 1)
            scenario = 2;
        dint32 knotParam = buf->getBitLong();
        DRW_DBG(" 2013 splFlag1: "); DRW_DBG(splFlag1);
        DRW_DBG(" 2013 knotParam: "); DRW_DBG(knotParam);
//        DRW_DBG("unk bit: "); DRW_DBG(buf->getBit());
    }
    degree = buf->getBitLong(); //RLZ: code 71, verify with dxf
    DRW_DBG(" degree: "); DRW_DBG(degree); DRW_DBG("\n");
    if (scenario == 2) {
        flags = 8;//scenario 2 = not rational & planar
        tolfit = buf->getBitDouble();//BD
        DRW_DBG("flags: "); DRW_DBG(flags); DRW_DBG(" tolfit: "); DRW_DBG(tolfit);
        tgStart =buf->get3BitDouble();
        DRW_DBG(" Start Tangent: "); DRW_DBGPT(tgStart.x, tgStart.y, tgStart.z);
        tgEnd =buf->get3BitDouble();
        DRW_DBG("\nEnd Tangent: "); DRW_DBGPT(tgEnd.x, tgEnd.y, tgEnd.z);
        nfit = buf->getBitLong();
        DRW_DBG("\nnumber of fit points: "); DRW_DBG(nfit);
    } else if (scenario == 1) {
        flags = 8;//scenario 1 = rational & planar
        flags |= buf->getBit() << 2; //flags, rational, code 70, bit 2 (4)
        flags |= buf->getBit(); //flags, closed, code 70, bit 0 (1)
        flags |= buf->getBit() << 1; //flags, periodic, code 70, bit 1 (2)
        tolknot = buf->getBitDouble();
        tolcontrol = buf->getBitDouble();
        DRW_DBG("flags: "); DRW_DBG(flags); DRW_DBG(" knot tolerance: "); DRW_DBG(tolknot);
        DRW_DBG(" control point tolerance: "); DRW_DBG(tolcontrol);
        nknots = buf->getBitLong();
        ncontrol = buf->getBitLong();
        weight = buf->getBit(); // RLZ ??? flags, weight, code 70, bit 4 (16)
        DRW_DBG("\nnum of knots: "); DRW_DBG(nknots); DRW_DBG(" num of control pt: ");
        DRW_DBG(ncontrol); DRW_DBG(" weight bit: "); DRW_DBG(weight);
    } else {
        DRW_DBG("\ndwg Spline, unknown scenario "); DRW_DBG(scenario);
        DRW_DBG(" (expected 1 or 2)\n");
        return false; //RLZ: from doc only 1 or 2 are ok ?
    }

    if (!DRW::reserve( knotslist, nknots)) {
        return false;
    }
    for (dint32 i= 0; i<nknots; ++i){
        knotslist.push_back (buf->getBitDouble());
    }
    if (!DRW::reserve( controllist, ncontrol)) {
        return false;
    }
    if (weight && !DRW::reserve(weightlist, ncontrol)) {
        return false;
    }
    for (dint32 i= 0; i<ncontrol; ++i){
        controllist.push_back(std::make_shared<DRW_Coord>(buf->get3BitDouble()));
        if (weight) {
            //per-control-point weight; required for hyperbola/parabola
            //conic detection in consumers (e.g. LibreCAD addSpline)
            double w = buf->getBitDouble(); //RLZ Warning: D (BD or RD)
            weightlist.push_back(w);
            DRW_DBG("\n w: "); DRW_DBG(w);
        }
    }
    if (!DRW::reserve( fitlist, nfit)) {
        return false;
    }
    for (dint32 i= 0; i<nfit; ++i)
        fitlist.push_back(std::make_shared<DRW_Coord>(buf->get3BitDouble()));

    if (DRW_DBGGL == DRW_dbg::Level::Debug) {
        DRW_DBG("\nknots list: ");
        for (auto const& v: knotslist) {
            DRW_DBG("\n"); DRW_DBG(v);
        }
        DRW_DBG("\ncontrol point list: ");
        for (auto const& v: controllist) {
            DRW_DBG("\n"); DRW_DBGPT(v->x, v->y, v->z);
        }
        DRW_DBG("\nfit point list: ");
        for (auto const& v: fitlist) {
            DRW_DBG("\n"); DRW_DBGPT(v->x, v->y, v->z);
        }
    }

    /* Common Entity Handle Data */
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
//    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Image::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 12:
        vVector.x = reader->getDouble();
        break;
    case 22:
        vVector.y = reader->getDouble();
        break;
    case 32:
        vVector.z = reader->getDouble();
        break;
    case 13:
        sizeu = reader->getDouble();
        break;
    case 23:
        sizev = reader->getDouble();
        break;
    case 340:
        ref = reader->getHandleString();
        break;
    case 280:
        clip = reader->getInt32();
        break;
    case 281:
        brightness = reader->getInt32();
        break;
    case 282:
        contrast = reader->getInt32();
        break;
    case 283:
        fade = reader->getInt32();
        break;
    case 91:
        // WIPEOUT: number of polygon vertices.  We don't pre-size — the count
        // is informational and the 14/24 pairs follow in order.
        clipPath.clear();
        clipPath.reserve(static_cast<size_t>(reader->getInt32()));
        break;
    case 14:
        // WIPEOUT polygon vertex x — start a new vertex.  Group 24 (y) follows.
        clipPath.emplace_back(reader->getDouble(), 0.0);
        break;
    case 24:
        // WIPEOUT polygon vertex y — complete the most recently started vertex.
        if (!clipPath.empty()) {
            clipPath.back().y = reader->getDouble();
        }
        break;
    case 290:
        // R2010+ Clip mode (IMAGE/WIPEOUT, ODA spec §20.4.80):
        // 0 = mask outside the polygon, 1 = mask inside.
        clipMode = reader->getBool();
        break;
    default:
        return DRW_Line::parseCode(code, reader);
    }

    return true;
}

bool DRW_Image::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing image *********************************************\n");

    dint32 classVersion = buf->getBitLong();
    DRW_DBG("class Version: "); DRW_DBG(classVersion);
    basePoint = buf->get3BitDouble();
    DRW_DBG("\nbase point: "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
    secPoint = buf->get3BitDouble();
    DRW_DBG("\nU vector: "); DRW_DBGPT(secPoint.x, secPoint.y, secPoint.z);
    vVector = buf->get3BitDouble();
    DRW_DBG("\nV vector: "); DRW_DBGPT(vVector.x, vVector.y, vVector.z);
    sizeu = buf->getRawDouble();
    sizev = buf->getRawDouble();
    DRW_DBG("\nsize U: "); DRW_DBG(sizeu); DRW_DBG("\nsize V: "); DRW_DBG(sizev);
    duint16 displayProps = buf->getBitShort();
    DRW_UNUSED(displayProps);//RLZ: temporary, complete API
    clip = buf->getBit();
    brightness = buf->getRawChar8();
    contrast = buf->getRawChar8();
    fade = buf->getRawChar8();
    if (version > DRW::AC1021){ //2010+
        bool clipMode = buf->getBit();
        DRW_UNUSED(clipMode);//RLZ: temporary, complete API
    }
    duint16 clipType = buf->getBitShort();
    clipPath.clear();
    if (clipType == 1){
        // rectangular clip: lower-left and upper-right corners; expand to a
        // 4-vertex polygon so downstream consumers can treat both kinds uniformly.
        DRW_Coord ll = buf->get2RawDouble();
        DRW_Coord ur = buf->get2RawDouble();
        clipPath.push_back(ll);
        clipPath.push_back(DRW_Coord(ur.x, ll.y, 0.0));
        clipPath.push_back(ur);
        clipPath.push_back(DRW_Coord(ll.x, ur.y, 0.0));
    } else { //clipType == 2
        dint32 numVerts = buf->getBitLong();
        clipPath.reserve(numVerts);
        for (int i= 0; i< numVerts;++i)
            clipPath.push_back(buf->get2RawDouble());
    }

    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    dwgHandle biH = buf->getHandle();
    DRW_DBG("ImageDef Handle: "); DRW_DBGHL(biH.code, biH.size, biH.ref);
    ref = biH.ref;
    biH = buf->getHandle();
    DRW_DBG("ImageDefReactor Handle: "); DRW_DBGHL(biH.code, biH.size, biH.ref);
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
//    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Dimension::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 1:
        text = reader->getUtf8String();
        break;
    case 2:
        name = reader->getString();
        break;
    case 3:
        style = reader->getUtf8String();
        break;
    case 70:
        type = reader->getInt32();
        break;
    case 71:
        align = reader->getInt32();
        break;
    case 72:
        linesty = reader->getInt32();
        break;
    case 10:
        defPoint.x = reader->getDouble();
        break;
    case 20:
        defPoint.y = reader->getDouble();
        break;
    case 30:
        defPoint.z = reader->getDouble();
        break;
    case 11:
        textPoint.x = reader->getDouble();
        break;
    case 21:
        textPoint.y = reader->getDouble();
        break;
    case 31:
        textPoint.z = reader->getDouble();
        break;
    case 12:
        clonePoint.x = reader->getDouble();
        break;
    case 22:
        clonePoint.y = reader->getDouble();
        break;
    case 32:
        clonePoint.z = reader->getDouble();
        break;
    case 13:
        def1.x = reader->getDouble();
        break;
    case 23:
        def1.y = reader->getDouble();
        break;
    case 33:
        def1.z = reader->getDouble();
        break;
    case 14:
        def2.x = reader->getDouble();
        break;
    case 24:
        def2.y = reader->getDouble();
        break;
    case 34:
        def2.z = reader->getDouble();
        break;
    case 15:
        circlePoint.x = reader->getDouble();
        break;
    case 25:
        circlePoint.y = reader->getDouble();
        break;
    case 35:
        circlePoint.z = reader->getDouble();
        break;
    case 16:
        arcPoint.x = reader->getDouble();
        break;
    case 26:
        arcPoint.y = reader->getDouble();
        break;
    case 36:
        arcPoint.z = reader->getDouble();
        break;
    case 41:
        linefactor = reader->getDouble();
        break;
    case 53:
        rot = reader->getDouble();
        break;
    case 50:
        angle = reader->getDouble();
        break;
    case 52:
        oblique = reader->getDouble();
        break;
    case 40:
        length = reader->getDouble();
        break;
    case 51:
        hdir = reader->getDouble();
        break;
    case 42:
        measureValue = reader->getDouble();
        break;
    case 74:
        flipArrow1 = reader->getInt32() != 0;
        break;
    case 75:
        flipArrow2 = reader->getInt32() != 0;
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

bool DRW_Dimension::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs)
{
    DRW_UNUSED( version);
    DRW_UNUSED( buf);
    DRW_UNUSED( bs);

    DRW_DBG("DRW_Dimension::parseDwg(): base class implemntation should never be called direct!\n");

    return false;
}

bool DRW_Dimension::parseDwg(DRW::Version version, dwgBuffer *buf, dwgBuffer *sBuf, duint32 bs /*= 0*/) {
    dwgBuffer sBuff = *buf;
    sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }

    if (!DRW_Entity::parseDwg( version, buf, sBuf, bs)) {
        return false;
    }

    DRW_DBG("\n***************************** parsing dimension *********************************************");
    if (version > DRW::AC1021) { //2010+
        duint8 dimVersion = buf->getRawChar8();
        DRW_DBG("\ndimVersion: "); DRW_DBG(dimVersion);
    }
    extPoint = buf->getExtrusion(version > DRW::AC1014);
    DRW_DBG("\nextPoint: "); DRW_DBGPT(extPoint.x, extPoint.y, extPoint.z);
    if (version > DRW::AC1014) { //2000+
        DRW_DBG("\nFive unknown bits: "); DRW_DBG(buf->getBit()); DRW_DBG(buf->getBit());
        DRW_DBG(buf->getBit()); DRW_DBG(buf->getBit()); DRW_DBG(buf->getBit());
    }
    textPoint.x = buf->getRawDouble();
    textPoint.y = buf->getRawDouble();
    textPoint.z = buf->getBitDouble();
    DRW_DBG("\ntextPoint: "); DRW_DBGPT(textPoint.x, textPoint.y, textPoint.z);
    type = buf->getRawChar8();
    DRW_DBG("\ntype (70) read: "); DRW_DBG(type);
    type =  (type & 1) ? type & 0x7F : type | 0x80; //set bit 7
    type =  (type & 2) ? type | 0x20 : type & 0xDF; //set bit 5
    DRW_DBG(" type (70) set: "); DRW_DBG(type);
    //clear last 3 bits to set integer dim type
    type &= 0xF8;
    text = sBuf->getVariableText(version, false);
    DRW_DBG("\nforced dim text: "); DRW_DBG(text.c_str());
    rot = buf->getBitDouble();
    hdir = buf->getBitDouble();
    DRW_Coord inspoint = buf->get3BitDouble();
    DRW_DBG("\ninspoint: "); DRW_DBGPT(inspoint.x, inspoint.y, inspoint.z);
    double insRot_code54 = buf->getBitDouble(); //RLZ: unknown, investigate
    DRW_DBG(" insRot_code54: "); DRW_DBG(insRot_code54);
    if (version > DRW::AC1014) { //2000+
        align = buf->getBitShort();
        linesty = buf->getBitShort();
        linefactor = buf->getBitDouble();
        measureValue = buf->getBitDouble();
        DRW_DBG("\n  actMeas_code42: "); DRW_DBG(measureValue);
        if (version > DRW::AC1018) { //2007+
            bool unk = buf->getBit();
            flipArrow1 = buf->getBit();
            flipArrow2 = buf->getBit();
            DRW_DBG("\n2007, unk, flip1, flip2: "); DRW_DBG(unk); DRW_DBG(flipArrow1); DRW_DBG(flipArrow2);
        }
    }
    clonePoint.x = buf->getRawDouble();
    clonePoint.y = buf->getRawDouble();
    DRW_DBG("\nclonePoint: "); DRW_DBGPT(clonePoint.x, clonePoint.y, clonePoint.z);

    return buf->isGood();
}

bool DRW_DimAligned::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    if (!DRW_Dimension::parseDwg(version, buf, nullptr, bs)) {
        return false;
    }

    if (oType == 0x15)
        DRW_DBG("\n***************************** parsing dim linear *********************************************\n");
    else
        DRW_DBG("\n***************************** parsing dim aligned *********************************************\n");
    DRW_Coord pt = buf->get3BitDouble();
    setPt3(pt); //def1
    DRW_DBG("def1: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt4(pt);
    DRW_DBG("\ndef2: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setDefPoint(pt);
    DRW_DBG("\ndefPoint: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    setOb52(buf->getBitDouble());
    if (oType == 0x15)
        setAn50(buf->getBitDouble() * ARAD);
    else
        type |= 1;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(type); DRW_DBG("\n");

    if (!DRW_Entity::parseDwgEntHandle(version, buf)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimAligned::parseDwg()\n");
        return false;
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    dimStyleH = buf->getHandle();
    DRW_DBG("dim style Handle: "); DRW_DBGHL(dimStyleH.code, dimStyleH.size, dimStyleH.ref); DRW_DBG("\n");
    blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    DRW_DBG("anon block Handle: "); DRW_DBGHL(blockH.code, blockH.size, blockH.ref); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    return buf->isGood();
 }

bool DRW_DimRadial::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    if (!DRW_Dimension::parseDwg(version, buf, nullptr, bs)) {
        return false;
    }

    DRW_DBG("\n***************************** parsing dim radial *********************************************\n");
    DRW_Coord pt = buf->get3BitDouble();
    setDefPoint(pt); //code 10
    DRW_DBG("defPoint: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt5(pt); //center pt  code 15
    DRW_DBG("\ncenter point: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    setRa40(buf->getBitDouble()); //leader length code 40
    DRW_DBG("\nleader length: "); DRW_DBG(getRa40());
    type |= 4;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(type); DRW_DBG("\n");

    if (!DRW_Entity::parseDwgEntHandle(version, buf)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimRadial::parseDwg()\n");
        return false;
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    dimStyleH = buf->getHandle();
    DRW_DBG("dim style Handle: "); DRW_DBGHL(dimStyleH.code, dimStyleH.size, dimStyleH.ref); DRW_DBG("\n");
    blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    DRW_DBG("anon block Handle: "); DRW_DBGHL(blockH.code, blockH.size, blockH.ref); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_DimDiametric::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    if (!DRW_Dimension::parseDwg(version, buf, nullptr, bs)) {
        return false;
    }

    DRW_DBG("\n***************************** parsing dim diametric *********************************************\n");
    DRW_Coord pt = buf->get3BitDouble();
    setPt5(pt); //center pt  code 15
    DRW_DBG("center point: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setDefPoint(pt); //code 10
    DRW_DBG("\ndefPoint: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    setRa40(buf->getBitDouble()); //leader length code 40
    DRW_DBG("\nleader length: "); DRW_DBG(getRa40());
    type |= 3;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(type); DRW_DBG("\n");

    if (!DRW_Entity::parseDwgEntHandle(version, buf)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimDiametric::parseDwg()\n");
        return false;
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    dimStyleH = buf->getHandle();
    DRW_DBG("dim style Handle: "); DRW_DBGHL(dimStyleH.code, dimStyleH.size, dimStyleH.ref); DRW_DBG("\n");
    blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    DRW_DBG("anon block Handle: "); DRW_DBGHL(blockH.code, blockH.size, blockH.ref); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_DimAngular::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    if (!DRW_Dimension::parseDwg(version, buf, nullptr, bs)) {
        return false;
    }

    DRW_DBG("\n***************************** parsing dim angular *********************************************\n");
    DRW_Coord pt;
    pt.x = buf->getRawDouble();
    pt.y = buf->getRawDouble();
    setPt6(pt); //code 16
    DRW_DBG("arc Point: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt3(pt); //def1  code 13
    DRW_DBG("\ndef1: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt4(pt); //def2  code 14
    DRW_DBG("\ndef2: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt5(pt); //center pt  code 15
    DRW_DBG("\ncenter point: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setDefPoint(pt); //code 10
    DRW_DBG("\ndefPoint: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    type |= 0x02;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(type); DRW_DBG("\n");

    if (!DRW_Entity::parseDwgEntHandle(version, buf)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimAngular::parseDwg()\n");
        return false;
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    dimStyleH = buf->getHandle();
    DRW_DBG("dim style Handle: "); DRW_DBGHL(dimStyleH.code, dimStyleH.size, dimStyleH.ref); DRW_DBG("\n");
    blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    DRW_DBG("anon block Handle: "); DRW_DBGHL(blockH.code, blockH.size, blockH.ref); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_DimAngular3p::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    if (!DRW_Dimension::parseDwg(version, buf, nullptr, bs)) {
        return false;
    }

    DRW_DBG("\n***************************** parsing dim angular3p *********************************************\n");
    DRW_Coord pt = buf->get3BitDouble();
    setDefPoint(pt); //code 10
    DRW_DBG("defPoint: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt3(pt); //def1  code 13
    DRW_DBG("\ndef1: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt4(pt); //def2  code 14
    DRW_DBG("\ndef2: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt5(pt); //center pt  code 15
    DRW_DBG("\ncenter point: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    type |= 0x05;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(type); DRW_DBG("\n");

    if (!DRW_Entity::parseDwgEntHandle(version, buf)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimAngular3p::parseDwg()\n");
        return false;
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    dimStyleH = buf->getHandle();
    DRW_DBG("dim style Handle: "); DRW_DBGHL(dimStyleH.code, dimStyleH.size, dimStyleH.ref); DRW_DBG("\n");
    blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    DRW_DBG("anon block Handle: "); DRW_DBGHL(blockH.code, blockH.size, blockH.ref); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_DimOrdinate::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    if (!DRW_Dimension::parseDwg(version, buf, nullptr, bs)) {
        return false;
    }

    DRW_DBG("\n***************************** parsing dim ordinate *********************************************\n");
    DRW_Coord pt = buf->get3BitDouble();
    setDefPoint(pt);
    DRW_DBG("defPoint: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt3(pt); //def1
    DRW_DBG("\ndef1: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    pt = buf->get3BitDouble();
    setPt4(pt);
    DRW_DBG("\ndef2: "); DRW_DBGPT(pt.x, pt.y, pt.z);
    duint8 type2 = buf->getRawChar8();//RLZ: correct this
    DRW_DBG("type2 (70) read: "); DRW_DBG(type2);
    type =  (type2 & 1) ? type | 0x80 : type & 0xBF; //set bit 6
    DRW_DBG(" type (70) set: "); DRW_DBG(type);
    type |= 6;
    DRW_DBG("\n  type (70) final: "); DRW_DBG(type); DRW_DBG("\n");

    if (!DRW_Entity::parseDwgEntHandle(version, buf)) {
        DRW_DBG("Failed: parseDwgEntHandle() in DRW_DimAligned::parseDwg()\n");
        return false;
    }
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    dimStyleH = buf->getHandle();
    DRW_DBG("dim style Handle: "); DRW_DBGHL(dimStyleH.code, dimStyleH.size, dimStyleH.ref); DRW_DBG("\n");
    blockH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    DRW_DBG("anon block Handle: "); DRW_DBGHL(blockH.code, blockH.size, blockH.ref); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    //    RS crc;   //RS */
    return buf->isGood();
}

bool DRW_Leader::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 3:
        style = reader->getUtf8String();
        break;
    case 71:
        arrow = reader->getInt32();
        break;
    case 72:
        leadertype = reader->getInt32();
        break;
    case 73:
        flag = reader->getInt32();
        break;
    case 74:
        hookline = reader->getInt32();
        break;
    case 75:
        hookflag = reader->getInt32();
        break;
    case 76:
        vertnum = reader->getInt32();
        break;
    case 77:
        coloruse = reader->getInt32();
        break;
    case 40:
        textheight = reader->getDouble();
        break;
    case 41:
        textwidth = reader->getDouble();
        break;
    case 10:
        vertexpoint= std::make_shared<DRW_Coord>();
        vertexlist.push_back(vertexpoint);
        vertexpoint->x = reader->getDouble();
        break;
    case 20:
        if(vertexpoint)
            vertexpoint->y = reader->getDouble();
        break;
    case 30:
        if(vertexpoint)
            vertexpoint->z = reader->getDouble();
        break;
    case 340:
        annotHandle = reader->getHandleString();
        break;
    case 210:
        extrusionPoint.x = reader->getDouble();
        break;
    case 220:
        extrusionPoint.y = reader->getDouble();
        break;
    case 230:
        extrusionPoint.z = reader->getDouble();
        break;
    case 211:
        horizdir.x = reader->getDouble();
        break;
    case 221:
        horizdir.y = reader->getDouble();
        break;
    case 231:
        horizdir.z = reader->getDouble();
        break;
    case 212:
        offsetblock.x = reader->getDouble();
        break;
    case 222:
        offsetblock.y = reader->getDouble();
        break;
    case 232:
        offsetblock.z = reader->getDouble();
        break;
    case 213:
        offsettext.x = reader->getDouble();
        break;
    case 223:
        offsettext.y = reader->getDouble();
        break;
    case 233:
        offsettext.z = reader->getDouble();
        break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }

    return true;
}

bool DRW_Leader::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing leader *********************************************\n");
    DRW_DBG("unknown bit "); DRW_DBG(buf->getBit());
    DRW_DBG(" annot type "); DRW_DBG(buf->getBitShort());
    DRW_DBG(" Path type "); DRW_DBG(buf->getBitShort());
    dint32 nPt = buf->getBitLong();
    DRW_DBG(" Num pts "); DRW_DBG(nPt);

    // add vertexes
    for (int i = 0; i< nPt; i++){
        DRW_Coord vertex = buf->get3BitDouble();
        vertexlist.push_back(std::make_shared<DRW_Coord>(vertex));
        DRW_DBG("\nvertex "); DRW_DBGPT(vertex.x, vertex.y, vertex.z);
    }
    DRW_Coord Endptproj = buf->get3BitDouble();
    DRW_DBG("\nEndptproj "); DRW_DBGPT(Endptproj.x, Endptproj.y, Endptproj.z);
    extrusionPoint = buf->getExtrusion(version > DRW::AC1014);
    DRW_DBG("\nextrusionPoint "); DRW_DBGPT(extrusionPoint.x, extrusionPoint.y, extrusionPoint.z);
    if (version > DRW::AC1014) { //2000+
        DRW_DBG("\nFive unknown bits: "); DRW_DBG(buf->getBit()); DRW_DBG(buf->getBit());
        DRW_DBG(buf->getBit()); DRW_DBG(buf->getBit()); DRW_DBG(buf->getBit());
    }
    horizdir = buf->get3BitDouble();
    DRW_DBG("\nhorizdir "); DRW_DBGPT(horizdir.x, horizdir.y, horizdir.z);
    offsetblock = buf->get3BitDouble();
    DRW_DBG("\noffsetblock "); DRW_DBGPT(offsetblock.x, offsetblock.y, offsetblock.z);
    if (version > DRW::AC1012) { //R14+
        DRW_Coord unk = buf->get3BitDouble();
        DRW_DBG("\nunknown "); DRW_DBGPT(unk.x, unk.y, unk.z);
    }
    if (version < DRW::AC1015) { //R14 -
        DRW_DBG("\ndimgap "); DRW_DBG(buf->getBitDouble());
    }
    if (version < DRW::AC1024) { //2010-
        textheight = buf->getBitDouble();
        textwidth = buf->getBitDouble();
        DRW_DBG("\ntextheight "); DRW_DBG(textheight); DRW_DBG(" textwidth "); DRW_DBG(textwidth);
    }
    hookline = buf->getBit();
    arrow = buf->getBit();
    DRW_DBG(" hookline "); DRW_DBG(hookline); DRW_DBG(" arrow flag "); DRW_DBG(arrow);

    if (version < DRW::AC1015) { //R14 -
        DRW_DBG("\nArrow head type "); DRW_DBG(buf->getBitShort());
        DRW_DBG("dimasz "); DRW_DBG(buf->getBitDouble());
        DRW_DBG("\nunk bit "); DRW_DBG(buf->getBit());
        DRW_DBG(" unk bit "); DRW_DBG(buf->getBit());
        DRW_DBG(" unk short "); DRW_DBG(buf->getBitShort());
        DRW_DBG(" byBlock color "); DRW_DBG(buf->getBitShort());
        DRW_DBG(" unk bit "); DRW_DBG(buf->getBit());
        DRW_DBG(" unk bit "); DRW_DBG(buf->getBit());
    } else { //R2000+
        DRW_DBG("\nunk short "); DRW_DBG(buf->getBitShort());
        DRW_DBG(" unk bit "); DRW_DBG(buf->getBit());
        DRW_DBG(" unk bit "); DRW_DBG(buf->getBit());
    }
    DRW_DBG("\n");
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret)
        return ret;
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
    AnnotH = buf->getHandle();
    annotHandle = AnnotH.ref;
    DRW_DBG("annot block Handle: "); DRW_DBGHL(AnnotH.code, AnnotH.size, dimStyleH.ref); DRW_DBG("\n");
    dimStyleH = buf->getHandle(); /* H 7 STYLE (hard pointer) */
    DRW_DBG("dim style Handle: "); DRW_DBGHL(dimStyleH.code, dimStyleH.size, dimStyleH.ref); DRW_DBG("\n");
    DRW_DBG("Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
//    RS crc;   //RS */
    return buf->isGood();
}

// Phase 1 placeholder: skeleton class is in place but no parser yet.
bool DRW_MLeader::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    // DXF MULTILEADER (ODA spec §20.4.48 mapped to DXF group codes).  The
    // entity-level fields read here mirror the DWG body parser; the
    // embedded CONTEXT_DATA{} block (§20.4.86) uses group 100 nested
    // subclass markers + control-flow markers (302 LEADER{, 304 LEADER_LINE{,
    // 305 }, 303 }, 301 }) and is parsed via the libdxfrw DXF state machine.
    // Phase 8 captures a minimal subset (entity-level scalars + basic
    // CONTEXT_DATA points); a full DXF round-trip is Phase 9 follow-up.
    switch (code) {
    case 170: leaderType = reader->getInt32(); break;
    case 171: leaderLineWeight = reader->getInt32(); break;
    case 172: styleContentType = reader->getInt32(); break;
    case 173: styleLeftAttach = reader->getInt32(); break;
    case 95:  styleRightAttach = reader->getInt32(); break;
    case 174: styleTextAngleType = reader->getInt32(); break;
    case 175: unknown175 = reader->getInt32(); break;
    case 176: styleAttachmentType = reader->getInt32(); break;
    case 178: ipeAlign = reader->getInt32(); break;
    case 179: justification = reader->getInt32(); break;
    case 271: attachmentDirection = reader->getInt32(); break;
    case 272: styleBottomAttach = reader->getInt32(); break;
    case 273: styleTopAttach = reader->getInt32(); break;
    case 90:  overrideFlags = reader->getInt32(); break;
    case 91:  leaderColor = reader->getInt32(); break;
    case 92:  styleTextColor = reader->getInt32(); break;
    case 93:  styleBlockColor = reader->getInt32(); break;
    case 41:  landingDistance = reader->getDouble(); break;
    case 42:  defaultArrowHeadSize = reader->getDouble(); break;
    case 43:  styleBlockRotation = reader->getDouble(); break;
    case 45:  scaleFactor = reader->getDouble(); break;
    case 290: landingEnabled = (reader->getInt32() != 0); break;
    case 291: doglegEnabled = (reader->getInt32() != 0); break;
    case 292: styleTextFrameEnabled = (reader->getInt32() != 0); break;
    case 293: isAnnotative = (reader->getInt32() != 0); break;
    case 294: isTextDirectionNegative = (reader->getInt32() != 0); break;
    case 295: leaderExtendedToText = (reader->getInt32() != 0); break;
    default:
        return DRW_Entity::parseCode(code, reader);
    }
    return true;
}

// Helper: parse one AcDbMLeaderObjectContextData::LeaderRoot entry (§20.4.86).
//
// Each root has: connection point + direction, optional break pairs, leader
// index, landing distance, then a count-and-list of leader lines.  Lines
// themselves carry: point list, break-info pairs, and (R2010+) per-line
// style overrides.  The handles inside (line-type / arrow per leader line)
// are deferred to the entity-level handle stream and not stored here.
static bool parseMLeaderRoot(DRW::Version version, dwgBuffer *buf,
                             DRW_MLeaderRoot& root) {
    // Layout per libreDWG dwg2.spec:1316-1366 (Dwg_LEADER_Node + Dwg_LEADER_Line).
    // The two 3BD coords at the head of the node are conditional on the
    // preceding B flags; reading them unconditionally drifts the bit stream
    // when either flag is 0.
    bool hasLastPt = buf->getBit();   // 290 has_lastleaderlinepoint
    bool hasDogleg = buf->getBit();   // 291 has_dogleg
    root.isContentValid = hasLastPt;
    root.unknown291     = hasDogleg;
    if (hasLastPt) root.connectionPoint = buf->get3BitDouble();
    if (hasDogleg) root.direction       = buf->get3BitDouble();

    dint32 nBreaks = buf->getBitLong();
    if (nBreaks < 0 || nBreaks > 5000) return false; // libreDWG MAX_LEADER_NUMBER
    root.breaks.reserve(static_cast<size_t>(nBreaks));
    for (dint32 i = 0; i < nBreaks; ++i) {
        DRW_Coord a = buf->get3BitDouble();
        DRW_Coord b = buf->get3BitDouble();
        root.breaks.emplace_back(a, b);
    }

    root.leaderIndex     = buf->getBitLong();   // 90 branch_index
    root.landingDistance = buf->getBitDouble(); // 40 dogleg_length

    dint32 nLines = buf->getBitLong();
    if (nLines < 0 || nLines > 5000) return false;
    root.leaderLines.reserve(static_cast<size_t>(nLines));
    for (dint32 i = 0; i < nLines; ++i) {
        DRW_MLeaderLeaderLine line;
        // Per libreDWG: BL num_points, points, BL num_breaks, breaks, BL line_index.
        // The previous 5-BL layout (brkInfoCount + segmentIndex + nPairs +
        // pairs + leaderLineIndex) inserted two spurious BL reads, drifting
        // every subsequent entity-level field (overallScale, contentType, …).
        dint32 nPts = buf->getBitLong();
        if (nPts < 0 || nPts > 5000) return false;
        line.points.reserve(static_cast<size_t>(nPts));
        for (dint32 j = 0; j < nPts; ++j) {
            line.points.push_back(buf->get3BitDouble());
        }
        dint32 nLineBreaks = buf->getBitLong();
        if (nLineBreaks < 0 || nLineBreaks > 5000) return false;
        for (dint32 j = 0; j < nLineBreaks; ++j) {
            DRW_Coord a = buf->get3BitDouble();
            DRW_Coord b = buf->get3BitDouble();
            line.breaks.emplace_back(a, b);
        }
        line.leaderLineIndex = buf->getBitLong();   // 91 line_index

        // R2010+ per-line override block.  The spec marks this block "R2010"
        // (§20.4.86 page 215); the override flags BL 93 says which fields
        // were overridden.  The handle fields (340 line-type, 341 arrow) are
        // deferred to the trailing handle stream.
        if (version >= DRW::AC1024) {
            line.leaderType = buf->getBitShort();
            line.color      = buf->getCmColor(version);
            // line type handle 340 — read from handles section later
            line.lineWeight = buf->getBitLong();
            line.arrowSize  = buf->getBitDouble();
            // arrow handle 341 — handles section
            line.overrideFlags = buf->getBitLong();
        }
        root.leaderLines.push_back(std::move(line));
    }

    if (version >= DRW::AC1024) {
        root.attachmentDirection = buf->getBitShort();
    }

    return buf->isGood();
}

// Helper: parse the AcDbMLeaderObjectContextData (§20.4.86) payload, the
// large embedded block at the start of the MLEADER body that carries the
// leader geometry plus either text or block content.
static bool parseMLeaderAnnotContext(DRW::Version version, dwgBuffer *buf,
                                     dwgBuffer *sBuf,
                                     DRW_MLeaderAnnotContext& ctx) {
    // NOTE: when AcDbMLeaderObjectContextData is embedded INSIDE the MLEADER
    // entity body (rather than serialized as a standalone object), the
    // AcDbObjectContextData base preamble (BS version, B has-file-ext-dict,
    // B default-flag) does NOT appear in the bit stream — those fields are
    // standalone-object metadata.  The embedded AnnotContext starts directly
    // with the leader-roots count.  AcDbAnnotScaleObjectContextData's scale
    // handle is deferred to the trailing handle stream.

    // Number of leader roots.
    dint32 nRoots = buf->getBitLong();
    if (nRoots < 0 || nRoots > 1000000) return false;
    ctx.roots.clear();
    ctx.roots.reserve(static_cast<size_t>(nRoots));
    for (dint32 i = 0; i < nRoots; ++i) {
        DRW_MLeaderRoot root;
        if (!parseMLeaderRoot(version, buf, root)) return false;
        ctx.roots.push_back(std::move(root));
    }

    // Common content fields.
    ctx.overallScale     = buf->getBitDouble();
    ctx.contentBasePoint = buf->get3BitDouble();
    ctx.textHeight       = buf->getBitDouble();
    ctx.arrowHeadSize    = buf->getBitDouble();
    ctx.landingGap       = buf->getBitDouble();
    ctx.styleLeftAttach  = buf->getBitShort();
    ctx.styleRightAttach = buf->getBitShort();
    ctx.textAlignType    = buf->getBitShort();
    ctx.attachmentType   = buf->getBitShort();
    ctx.hasTextContents  = buf->getBit();

    if (ctx.hasTextContents) {
        ctx.textLabel        = sBuf->getVariableText(version, false);
        ctx.textNormal       = buf->get3BitDouble();
        // text style handle 340 — handles section
        ctx.textLocation     = buf->get3BitDouble();
        ctx.textDirection    = buf->get3BitDouble();
        ctx.textRotation     = buf->getBitDouble();
        ctx.boundaryWidth    = buf->getBitDouble();
        ctx.boundaryHeight   = buf->getBitDouble();
        ctx.lineSpacingFactor = buf->getBitDouble();
        ctx.lineSpacingStyle  = buf->getBitShort();
        ctx.textColor         = buf->getCmColor(version);
        ctx.alignment         = buf->getBitShort();
        ctx.flowDirection     = buf->getBitShort();
        ctx.bgFillColor       = buf->getCmColor(version);
        ctx.bgScaleFactor     = buf->getBitDouble();
        ctx.bgTransparency    = buf->getBitLong();
        ctx.bgFillEnabled     = buf->getBit();
        ctx.bgMaskFillOn      = buf->getBit();
        ctx.columnType        = buf->getBitShort();
        ctx.textHeightAuto    = buf->getBit();
        ctx.columnWidth       = buf->getBitDouble();
        ctx.columnGutter      = buf->getBitDouble();
        ctx.columnFlowReversed = buf->getBit();
        dint32 nColSizes = buf->getBitLong();
        if (nColSizes < 0 || nColSizes > 1000000) return false;
        ctx.columnSizes.reserve(static_cast<size_t>(nColSizes));
        for (dint32 i = 0; i < nColSizes; ++i) {
            ctx.columnSizes.push_back(buf->getBitDouble());
        }
        ctx.wordBreak = buf->getBit();
        buf->getBit();  // unknown trailing bit
    } else {
        ctx.hasContentsBlock = buf->getBit();
        if (ctx.hasContentsBlock) {
            // BlockTableRecord handle 341 — deferred
            ctx.blockNormal   = buf->get3BitDouble();
            ctx.blockLocation = buf->get3BitDouble();
            ctx.blockScale    = buf->get3BitDouble();
            ctx.blockRotation = buf->getBitDouble();
            ctx.blockColor    = buf->getCmColor(version);
            for (size_t i = 0; i < 16; ++i) {
                ctx.blockTransform[i] = buf->getBitDouble();
            }
        }
    }

    // Common tail.
    ctx.basePoint     = buf->get3BitDouble();
    ctx.baseDirection = buf->get3BitDouble();
    ctx.baseVertical  = buf->get3BitDouble();
    ctx.isNormalReversed = buf->getBit();

    if (version >= DRW::AC1024) {
        ctx.styleTopAttach    = buf->getBitShort();
        ctx.styleBottomAttach = buf->getBitShort();
    }

    return buf->isGood();
}

bool DRW_MLeader::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {  // 2007+
        sBuf = &sBuff;
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret) return ret;
    DRW_DBG("\n***************************** parsing MLEADER ***************\n");

    // R2010b+ class version (BS, default 2; <=R2004 was 1). libreDWG
    // dwg2.spec:1303-1306. Absent in R2007 streams; reading it would drift.
    if (version >= DRW::AC1024) {
        classVersion = buf->getBitShort();
        if (classVersion > 10) {
            DRW_DBG("\nMLEADER: implausible classVersion=");
            DRW_DBG(static_cast<int>(classVersion));
            DRW_DBG(", aborting body\n");
            return true;
        }
    }

    // Phase 4 — embedded AcDbMLeaderObjectContextData / MLeaderAnnotContext.
    // Body misalignment is local to this entity's buffer (each entity gets a
    // fresh buffer from the object map), so on a partial-parse failure we
    // keep whatever was captured and return true.  This preserves the
    // entity-stream-continues invariant established in Phase 2.
    if (!parseMLeaderAnnotContext(version, buf, sBuf, context)) {
        DRW_DBG("\nMLEADER: AnnotContext parse drift — partial fields kept\n");
        return true;
    }

    // Phase 3 — entity-level fields per §20.4.48 (after the AnnotContext).
    // Many handle slots are deferred to the trailing handle stream and not
    // stored here yet (resolution comes in Phase 7).
    overrideFlags        = buf->getBitLong();
    leaderType           = buf->getBitShort();
    leaderColor          = buf->getCmColor(version);
    // leader line type handle 341 — handle stream
    leaderLineWeight     = buf->getBitLong();
    landingEnabled       = buf->getBit();
    doglegEnabled        = buf->getBit();
    landingDistance      = buf->getBitDouble();
    // arrow head handle 342 — handle stream
    defaultArrowHeadSize = buf->getBitDouble();
    styleContentType     = buf->getBitShort();
    // text style handle 343 — handle stream
    styleLeftAttach      = buf->getBitShort();
    styleRightAttach     = buf->getBitShort();
    styleTextAngleType   = buf->getBitShort();
    unknown175           = buf->getBitShort();
    styleTextColor       = buf->getCmColor(version);
    styleTextFrameEnabled = buf->getBit();
    // style block handle 344 — handle stream (optional)
    styleBlockColor      = buf->getCmColor(version);
    styleBlockScale      = buf->get3BitDouble();
    styleBlockRotation   = buf->getBitDouble();
    styleAttachmentType  = buf->getBitShort();
    isAnnotative         = buf->getBit();

    // R2007 arrays (pre-R2010 only): per spec §20.4.48.  Bounds-check the
    // counts; a misaligned bit stream would produce huge nonsense values.
    // On a sanity-check trip, abort the rest of the body parse but keep
    // the entity (per Phase 4 contract above).
    if (version < DRW::AC1024) {
        dint32 nArrows = buf->getBitLong();
        if (nArrows < 0 || nArrows > 1000000) return true;
        arrowHeads.reserve(static_cast<size_t>(nArrows));
        for (dint32 i = 0; i < nArrows; ++i) {
            ArrowHeadEntry e;
            e.isDefault = buf->getBit();
            arrowHeads.push_back(e);
        }
        dint32 nLabels = buf->getBitLong();
        if (nLabels < 0 || nLabels > 1000000) return true;
        blockLabels.reserve(static_cast<size_t>(nLabels));
        for (dint32 i = 0; i < nLabels; ++i) {
            BlockLabelEntry e;
            e.labelText = sBuf->getVariableText(version, false);
            e.uiIndex   = buf->getBitShort();
            e.width     = buf->getBitDouble();
            blockLabels.push_back(std::move(e));
        }
    }

    isTextDirectionNegative = buf->getBit();
    ipeAlign      = buf->getBitShort();
    justification = buf->getBitShort();
    scaleFactor   = buf->getBitDouble();

    if (version >= DRW::AC1024) {  // R2010+
        attachmentDirection = buf->getBitShort();
        styleTopAttach      = buf->getBitShort();
        styleBottomAttach   = buf->getBitShort();
    }
    if (version >= DRW::AC1027) {  // R2013+
        leaderExtendedToText = buf->getBit();
    }

    // Common entity handles first (owner/reactors/xdic/layer/ltype/...) —
    // entity-specific handles follow in declared order from libreDWG
    // dwg2.spec:1386-1453.  Read order in the trailing handle stream:
    //   1. AnnotContext content handle (text_style 340 if hasTextContents,
    //      else block_table 341 if hasContentsBlock).
    //   2. (R2010b+ only) per-leader-line ltype + arrow handles, in the
    //      same iteration order as the body block.
    //   3. mleaderstyle (340), line_ltype (341), arrow_handle (342),
    //      text_style (343), block_style (344) entity-level handles.
    //   4. (R14-R2007 only) per-arrowhead + per-blocklabel handles.
    ret = DRW_Entity::parseDwgEntHandle(version, buf);
    if (!ret) {
        DRW_DBG("\nMLEADER: parseDwgEntHandle hiccup — body fields kept\n");
        return true;
    }

    auto safeHandle = [&](dwgHandle& slot, const char* tag) {
        if (buf->numRemainingBytes() < 1) return false;
        slot = buf->getHandle();
        DRW_DBG(" "); DRW_DBG(tag); DRW_DBG(": ");
        DRW_DBGHL(slot.code, slot.size, slot.ref); DRW_DBG("\n");
        return buf->isGood();
    };

    // 1. AnnotContext content handle.
    if (context.hasTextContents) {
        if (!safeHandle(context.textStyleHandle, "ctx.text_style")) return true;
    } else if (context.hasContentsBlock) {
        if (!safeHandle(context.blockTableRecordHandle, "ctx.block_table")) return true;
    }

    // 2. R2010b+ per-line handles, in body iteration order.
    if (version >= DRW::AC1024) {
        for (auto& root : context.roots) {
            for (auto& line : root.leaderLines) {
                if (!safeHandle(line.lineTypeHandle, "line.ltype")) return true;
                if (!safeHandle(line.arrowHandle,    "line.arrow")) return true;
            }
        }
    }

    // 3. Entity-level handles.
    if (!safeHandle(styleHandle,           "mleaderstyle")) return true;
    if (!safeHandle(leaderLineTypeHandle,  "line_ltype"))   return true;
    if (!safeHandle(arrowHeadHandle,       "arrow_handle")) return true;
    if (!safeHandle(styleTextStyleHandle,  "text_style"))   return true;
    if (!safeHandle(styleBlockHandle,      "block_style"))  return true;

    // 4. R14-R2007 per-arrowhead + per-blocklabel handles (counts came
    //    from the body-side arrays read earlier).
    if (version < DRW::AC1024) {
        for (auto& a  : arrowHeads)
            if (!safeHandle(a.handle,        "arrowheads.handle"))      return true;
        for (auto& bl : blockLabels)
            if (!safeHandle(bl.attDefHandle, "blocklabels.attdef"))     return true;
    }

    const int rb = buf->numRemainingBytes();
    DRW_DBG("\nMLEADER tail rb="); DRW_DBG(rb); DRW_DBG("\n");
    if (rb > 4) {
        DRW_DBG("MLEADER: handle-stream tail "); DRW_DBG(rb);
        DRW_DBG(" bytes unconsumed (handle ");
        DRW_DBGH(handle); DRW_DBG(") — review tail handle list\n");
    }

    return true;
}

bool DRW_Viewport::parseCode(int code, const std::unique_ptr<dxfReader>& reader){
    switch (code) {
    case 40:
        pswidth = reader->getDouble();
        break;
    case 41:
        psheight = reader->getDouble();
        break;
    case 68:
        vpstatus = reader->getInt32();
        break;
    case 69:
        vpID = reader->getInt32();
        break;
    case 12: {
        centerPX = reader->getDouble();
        break; }
    case 22:
        centerPY = reader->getDouble();
        break;
    default:
        return DRW_Point::parseCode(code, reader);
    }

    return true;
}
//ex 22 dec 34
bool DRW_Viewport::parseDwg(DRW::Version version, dwgBuffer *buf, duint32 bs){
    dwgBuffer sBuff = *buf;
    dwgBuffer *sBuf = buf;
    if (version > DRW::AC1018) {//2007+
        sBuf = &sBuff; //separate buffer for strings
    }
    bool ret = DRW_Entity::parseDwg(version, buf, sBuf, bs);
    if (!ret)
        return ret;
    DRW_DBG("\n***************************** parsing viewport *****************************************\n");
    basePoint.x = buf->getBitDouble();
    basePoint.y = buf->getBitDouble();
    basePoint.z = buf->getBitDouble();
    DRW_DBG("center "); DRW_DBGPT(basePoint.x, basePoint.y, basePoint.z);
    pswidth = buf->getBitDouble();
    psheight = buf->getBitDouble();
    DRW_DBG("\nWidth: "); DRW_DBG(pswidth); DRW_DBG(", Height: "); DRW_DBG(psheight); DRW_DBG("\n");
    //RLZ TODO: complete in dxf
    if (version > DRW::AC1014) {//2000+
        viewTarget.x = buf->getBitDouble();
        viewTarget.y = buf->getBitDouble();
        viewTarget.z = buf->getBitDouble();
        DRW_DBG("view Target "); DRW_DBGPT(viewTarget.x, viewTarget.y, viewTarget.z);
        viewDir.x = buf->getBitDouble();
        viewDir.y = buf->getBitDouble();
        viewDir.z = buf->getBitDouble();
        DRW_DBG("\nview direction "); DRW_DBGPT(viewDir.x, viewDir.y, viewDir.z);
        twistAngle = buf->getBitDouble();
        DRW_DBG("\nView twist Angle: "); DRW_DBG(twistAngle);
        viewHeight = buf->getBitDouble();
        DRW_DBG("\nview Height: "); DRW_DBG(viewHeight);
        viewLength = buf->getBitDouble();
        DRW_DBG(" Lens Length: "); DRW_DBG(viewLength);
        frontClip = buf->getBitDouble();
        DRW_DBG("\nfront Clip Z: "); DRW_DBG(frontClip);
        backClip = buf->getBitDouble();
        DRW_DBG(" back Clip Z: "); DRW_DBG(backClip);
        snapAngle = buf->getBitDouble();
        DRW_DBG("\n snap Angle: "); DRW_DBG(snapAngle);
        centerPX = buf->getRawDouble();
        centerPY = buf->getRawDouble();
        DRW_DBG("\nview center X: "); DRW_DBG(centerPX); DRW_DBG(", Y: "); DRW_DBG(centerPX);
        snapPX = buf->getRawDouble();
        snapPY = buf->getRawDouble();
        DRW_DBG("\nSnap base point X: "); DRW_DBG(snapPX); DRW_DBG(", Y: "); DRW_DBG(snapPY);
        snapSpPX = buf->getRawDouble();
        snapSpPY = buf->getRawDouble();
        DRW_DBG("\nSnap spacing X: "); DRW_DBG(snapSpPX); DRW_DBG(", Y: "); DRW_DBG(snapSpPY);
        //RLZ: need to complete
        DRW_DBG("\nGrid spacing X: "); DRW_DBG(buf->getRawDouble()); DRW_DBG(", Y: "); DRW_DBG(buf->getRawDouble());DRW_DBG("\n");
        DRW_DBG("Circle zoom?: "); DRW_DBG(buf->getBitShort()); DRW_DBG("\n");
    }
    if (version > DRW::AC1018) {//2007+
        DRW_DBG("Grid major?: "); DRW_DBG(buf->getBitShort()); DRW_DBG("\n");
    }
    if (version > DRW::AC1014) {//2000+
        frozenLyCount = buf->getBitLong();
        DRW_DBG("Frozen Layer count?: "); DRW_DBG(frozenLyCount); DRW_DBG("\n");
        DRW_DBG("Status Flags?: "); DRW_DBG(buf->getBitLong()); DRW_DBG("\n");
        //RLZ: Warning needed separate string buffer
        DRW_DBG("Style sheet?: "); DRW_DBG(sBuf->getVariableText(version, false)); DRW_DBG("\n");
        DRW_DBG("Render mode?: "); DRW_DBG(buf->getRawChar8()); DRW_DBG("\n");
        DRW_DBG("UCS OMore...: "); DRW_DBG(buf->getBit()); DRW_DBG("\n");
        DRW_DBG("UCS VMore...: "); DRW_DBG(buf->getBit()); DRW_DBG("\n");
        DRW_DBG("UCS OMore...: "); DRW_DBGPT(buf->getBitDouble(), buf->getBitDouble(), buf->getBitDouble()); DRW_DBG("\n");
        DRW_DBG("ucs XAMore...: "); DRW_DBGPT(buf->getBitDouble(), buf->getBitDouble(), buf->getBitDouble()); DRW_DBG("\n");
        DRW_DBG("UCS YMore....: "); DRW_DBGPT(buf->getBitDouble(), buf->getBitDouble(), buf->getBitDouble()); DRW_DBG("\n");
        DRW_DBG("UCS EMore...: "); DRW_DBG(buf->getBitDouble()); DRW_DBG("\n");
        DRW_DBG("UCS OVMore...: "); DRW_DBG(buf->getBitShort()); DRW_DBG("\n");
    }
    if (version > DRW::AC1015) {//2004+
        DRW_DBG("ShadePlot Mode...: "); DRW_DBG(buf->getBitShort()); DRW_DBG("\n");
    }
    if (version > DRW::AC1018) {//2007+
        DRW_DBG("Use def Light...: "); DRW_DBG(buf->getBit()); DRW_DBG("\n");
        DRW_DBG("Def light type?: "); DRW_DBG(buf->getRawChar8()); DRW_DBG("\n");
        DRW_DBG("Brightness: "); DRW_DBG(buf->getBitDouble()); DRW_DBG("\n");
        DRW_DBG("Contrast: "); DRW_DBG(buf->getBitDouble()); DRW_DBG("\n");
//        DRW_DBG("Ambient Cmc or Enc: "); DRW_DBG(buf->getCmColor(version)); DRW_DBG("\n");
        DRW_DBG("Ambient (Cmc or Enc?), Enc: "); DRW_DBG(buf->getEnColor(version)); DRW_DBG("\n");
    }
    ret = DRW_Entity::parseDwgEntHandle(version, buf);

    dwgHandle someHdl;
    if (version < DRW::AC1015) {//R13 & R14 only
        DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        someHdl = buf->getHandle();
        DRW_DBG("ViewPort ent header: "); DRW_DBGHL(someHdl.code, someHdl.size, someHdl.ref); DRW_DBG("\n");
    }
    if (version > DRW::AC1014) {//2000+
        for (duint8 i=0; i < frozenLyCount; ++i){
            someHdl = buf->getHandle();
            DRW_DBG("Frozen layer handle "); DRW_DBG(i); DRW_DBG(": "); DRW_DBGHL(someHdl.code, someHdl.size, someHdl.ref); DRW_DBG("\n");
        }
        someHdl = buf->getHandle();
        DRW_DBG("Clip bpundary handle: "); DRW_DBGHL(someHdl.code, someHdl.size, someHdl.ref); DRW_DBG("\n");
        if (version == DRW::AC1015) {//2000 only
            someHdl = buf->getHandle();
            DRW_DBG("ViewPort ent header: "); DRW_DBGHL(someHdl.code, someHdl.size, someHdl.ref); DRW_DBG("\n");
        }
        someHdl = buf->getHandle();
        DRW_DBG("Named ucs handle: "); DRW_DBGHL(someHdl.code, someHdl.size, someHdl.ref); DRW_DBG("\n");
        DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        someHdl = buf->getHandle();
        DRW_DBG("base ucs handle: "); DRW_DBGHL(someHdl.code, someHdl.size, someHdl.ref); DRW_DBG("\n");
    }
    if (version > DRW::AC1018) {//2007+
        someHdl = buf->getHandle();
        DRW_DBG("background handle: "); DRW_DBGHL(someHdl.code, someHdl.size, someHdl.ref); DRW_DBG("\n");
        someHdl = buf->getHandle();
        DRW_DBG("visual style handle: "); DRW_DBGHL(someHdl.code, someHdl.size, someHdl.ref); DRW_DBG("\n");
        someHdl = buf->getHandle();
        DRW_DBG("shadeplot ID handle: "); DRW_DBGHL(someHdl.code, someHdl.size, someHdl.ref); DRW_DBG("\n");
        DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");
        someHdl = buf->getHandle();
        DRW_DBG("SUN handle: "); DRW_DBGHL(someHdl.code, someHdl.size, someHdl.ref); DRW_DBG("\n");
    }
    DRW_DBG("\n Remaining bytes: "); DRW_DBG(buf->numRemainingBytes()); DRW_DBG("\n");

    if (!ret)
        return ret;
    return buf->isGood();
}
