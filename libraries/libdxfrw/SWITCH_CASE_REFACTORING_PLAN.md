# Switch-Case Refactoring Plan for libdxfrw

## Overview

This plan aims to improve code readability and maintainability by replacing hard-coded numerical values with descriptive enum values in switch-case blocks throughout libdxfrw.

## Phase 1: Add New Enum Definitions

### 1.1 Add `dwgType::Entity` enum to [dwgutil.h](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgutil.h)

```cpp
namespace dwgType {
    enum Entity {
        TEXT = 1,
        ATTRIB = 2,
        ATTDEF = 3,
        SEQEND = 6,
        INSERT = 7,
        MINSERT = 8,
        POLYLINE_2D = 15,
        POLYLINE_3D = 16,
        ARC = 17,
        CIRCLE = 18,
        LINE = 19,
        DIM_ORDINATE = 20,
        DIM_LINEAR = 21,
        DIM_ALIGNED = 22,
        DIM_ANGULAR3P = 23,
        DIM_ANGULAR = 24,
        DIM_RADIAL = 25,
        DIM_DIAMETRIC = 26,
        POINT = 27,
        FACE3D = 28,
        POLYLINE_PFACE = 29,
        POLYLINE_MESH = 30,
        SOLID = 31,
        TRACE = 32,
        SHAPE = 33,
        VIEWPORT = 34,
        ELLIPSE = 35,
        SPLINE = 36,
        REGION = 37,
        SOLID3D = 38,
        BODY = 39,
        RAY = 40,
        XLINE = 41,
        MTEXT = 44,
        LEADER = 45,
        TOLERANCE = 46,
        MLINE = 47,
        LWPOLYLINE = 77,
        HATCH = 78,
        OLE2FRAME = 74,
        IMAGE = 101,
    };
}
```

### 1.2 Add `dwgCP::CodePage` enum to [dwgutil.h](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgutil.h)

```cpp
namespace dwgCP {
    enum CodePage {
        ANSI_1250 = 28,   // Central/East European
        ANSI_1251 = 29,   // Cyrillic
        ANSI_1252 = 30,   // Western European (default)
        GBK_CP936 = 31,   // GB2312 / GBK Simplified Chinese
        ANSI_1253 = 32,   // Greek
        ANSI_1254 = 33,   // Turkish
        ANSI_1255 = 34,   // Hebrew
        ANSI_1256 = 35,   // Arabic
        ANSI_1257 = 36,   // Baltic
        ANSI_874 = 37,    // Thai
        SHIFT_JIS = 38,   // Japanese
        GBK = 39,         // Simplified Chinese (duplicate of 31)
        KOREAN_WANSUNG = 40,  // Korean
        BIG5 = 41,        // Traditional Chinese
        ANSI_1258 = 44,   // Vietnamese
    };
}
```

### 1.3 Add `dxfCode::Common` enum to [drw_base.h](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/drw_base.h)

> Note: This enum is placed in drw_base.h because it's needed by drw_entities.cpp
> and drw_objects.cpp, which don't include intern/dwgutil.h. drw_base.h is already
> included by both drw_entities.h and drw_objects.h.

```cpp
namespace dxfCode {
    enum Common {
        HANDLE = 5,
        LAYER = 8,
        COLOR = 62,
        OWNER_HANDLE = 330,
        INVISIBLE = 60,
        LINEWEIGHT = 370,
        PLOTSTYLE = 390,
    };
}
```

> Note: Code 6 (LINETYPE) and 39 (THICKNESS) are context-dependent and not truly universal.
> Code 6 means LINETYPE for entities but MTEXT style in other contexts.
> These are intentionally excluded from this enum to avoid misleading usage.
> COLOR(62), LAYER(8), INVISIBLE(60), LINEWEIGHT(370) are primarily entity-specific
> but are included here as they appear in common entity parsing paths.

### 1.4 Add `dwgColor::Encoding` enum to [dwgutil.h](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgutil.h)

```cpp
namespace dwgColor {
    enum Encoding {
        BYLAYER = 0xC0,
        BYBLOCK = 0xC1,
        RGB = 0xC2,
        ACIS = 0xC3,
        WINDOW_BG = 0xC8,
    };
}
```

### 1.5 Add `dwgObjType::Object` enum to [dwgutil.h](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgutil.h)

```cpp
namespace dwgObjType {
    enum Object {
        DICTIONARY = 42,
        GROUP = 72,
        MLINESTYLE = 73,
        XRECORD = 79,
        ACDBPLACEHOLDER = 80,
        LAYOUT = 82,
        IMAGEDEF = 102,
    };
}
```

## Phase 2: Replace Switch-Case Blocks

### Priority 1: [dwgreader.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgreader.cpp)

#### 2.1 Line 129: Code Page Switch

**Before:**
```cpp
switch (cp) {
    case 28: return "ANSI_1250";
    case 29: return "ANSI_1251";
    case 30: return "ANSI_1252";
    case 31: return "ANSI_936";
    ...
}
```

**After:**
```cpp
switch (cp) {
    case dwgCP::ANSI_1250: return "ANSI_1250";
    case dwgCP::ANSI_1251: return "ANSI_1251";
    case dwgCP::ANSI_1252: return "ANSI_1252";
    case dwgCP::GBK_CP936: return "ANSI_936";
    ...
}
```

#### 2.2 Line 1355: Entity Type Switch (Entities Pass)

**Before:**
```cpp
switch (oType) {
    case 17: { DRW_Arc e; ... } break;
    case 18: { DRW_Circle e; ... } break;
    ...
}
```

**After:**
```cpp
switch (oType) {
    case dwgType::ARC: { DRW_Arc e; ... } break;
    case dwgType::CIRCLE: { DRW_Circle e; ... } break;
    ...
}
```

#### 2.3 Line 2072: Object Type Switch (Objects Pass)

**Before:**
```cpp
switch (oType) {
    case 42: { DRW_Dictionary e; ... } break;
    case 79: { DRW_XRecord e; ... } break;
    ...
}
```

**After:**
```cpp
switch (oType) {
    case dwgObjType::DICTIONARY: { DRW_Dictionary e; ... } break;
    case dwgObjType::XRECORD: { DRW_XRecord e; ... } break;
    ...
}

### Priority 2: [drw_entities.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/drw_entities.cpp)

#### 2.4 Replace common DXF group codes

**Specific switch blocks to modify:**
- Line 970: `DRW_Entity::parseCode()` - common entity DXF codes (5, 8, 62, 330, 60, 370, 390)
- Line 1216: `DRW_Entity::parseDxfGroups()` - entity group codes
- Line 1655: `DRW_LWPolyline::parseCode()` - polyline codes
- Line 1949: `DRW_Polyline::parseCode()` - polyline codes
- Line 2593: `DRW_Text::parseCode()` - text codes
- Line 2819: `DRW_MText::parseCode()` - mtext codes
- Line 3083: `DRW_Dimension::parseCode()` - dimension codes
- Line 3436: `DRW_Hatch::parseCode()` - hatch codes
- Line 5289: `DRW_Image::parseCode()` - image codes
- Line 5504: `DRW_Insert::parseCode()` - insert codes

**Example - Line 970 (DRW_Entity::parseCode):**

**Before:**
```cpp
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
    case 60:
        visible = (reader->getInt32() & 1) == 0;
        break;
    case 390:
        plotStyle = reader->getHandleString();
        break;
    ...
}
```

**After:**
```cpp
switch (code) {
    case dxfCode::HANDLE:
        handle = reader->getHandleString();
        break;
    case dxfCode::OWNER_HANDLE:
        parentHandle = reader->getHandleString();
        break;
    case dxfCode::LAYER:
        layer = reader->getUtf8String();
        break;
    case 6: // linetype (context-dependent, not in enum)
        lineType = reader->getUtf8String();
        break;
    case dxfCode::COLOR:
        color = reader->getInt32();
        break;
    case dxfCode::LINEWEIGHT:
        lWeight = DRW_LW_Conv::dxfInt2lineWidth(reader->getInt32());
        break;
    case dxfCode::INVISIBLE:
        visible = (reader->getInt32() & 1) == 0;
        break;
    case dxfCode::PLOTSTYLE:
        plotStyle = reader->getHandleString();
        break;
    ...
}
```

### Priority 3: [drw_objects.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/drw_objects.cpp)

**Specific switch blocks to modify:**
- Line 744: `DRW_TableEntry::parseCode()` - table entry codes (5, 330)
- Line 952: `DRW_Dictionary::parseCode()` - dictionary codes
- Line 1283: `DRW_DimStyle::parseCode()` - dimension style codes
- Line 1440: `DRW_Layer::parseCode()` - layer codes (5, 8, 62, 370)
- Line 1687: `DRW_LType::parseCode()` - linetype codes
- Line 2031: `DRW_Style::parseCode()` - text style codes
- Line 2107: `DRW_BlockRecord::parseCode()` - block record codes

Apply the same pattern using `dxfCode::Common` for applicable codes.

### Priority 4: Other files

- [dwgbuffer.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgbuffer.cpp):797 - Color encoding switch (0xC0, 0xC1, 0xC2, 0xC3) → use dwgColor::Encoding
- [proxygraphicdecoder.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/proxygraphicdecoder.cpp):457 - Color encoding switch (0xC0, 0xC1, 0xC2, 0xC3) → use dwgColor::Encoding
- [libdxfrw.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/libdxfrw.cpp):4006 - XDATA DXF code switch (1000-1041) → **SKIP** (XDATA-specific codes, rarely changed)
- [libdxfrw.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/libdxfrw.cpp):4087 - DXF section control codes (0, 2, 999) → **SKIP** (section-specific control codes)
- [libdxfrw.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/libdxfrw.cpp):4242 - CLASS section codes (1, 2, 3, 90, 91, 280, 281) → **SKIP** (CLASS-specific codes)
- [libdxfrw.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/libdxfrw.cpp):6643 - FIELD entity codes → **SKIP** (FIELD-specific codes)
- [drw_textcodec.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/drw_textcodec.cpp):22,259 - Version switches (already using DRW::Version enum) → **SKIP**
- [dwgwriter18.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgwriter18.cpp):256 - Version switches (already using DRW::Version enum) → **SKIP**
- [dwgreaderR11.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgreaderR11.cpp):589 - Entity type switch using R11Type enum (internal to file) → **SKIP** (uses different type codes)
- [dwgreaderR11.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgreaderR11.cpp):809 - Dimension type switch → **SKIP** (internal to R11 format)
- [libdxfrw.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/libdxfrw.cpp):103,116 - Modeler geometry switches using DRW::ETYPE enum → **SKIP**
- [libdxfrw.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/libdxfrw.cpp):2165,3373 - Entity type switches using DRW::ETYPE enum → **SKIP**
- [libdwgr.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/libdwgr.cpp):58,276,1068 - Already using enums or context-specific → **SKIP**
- [drw_header.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/drw_header.cpp):44 - DXF group codes → **SKIP** (header-specific codes)
- [drw_header.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/drw_header.cpp):128,3188 - Already using enums → **SKIP**
- [dwgwriter15.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgwriter15.cpp):31,103 - Section/definition switches → **SKIP** (already using enums)
- [dwgutil.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgutil.cpp):541 - Opcode switch → **SKIP** (internal opcode values)
- [drw_dbg.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/drw_dbg.cpp):58 - Debug level switch → **SKIP** (already using enums)

## Phase 3: Sort Case Values

### 3.1 Entity Types (Numeric Order)

**Important Notes:**
- Fall-through case groups take priority over numerical ordering and must remain contiguous as a single block
- The following groups share the same handler code:
  - INSERT (7) / MINSERT (8)
  - POLYLINE_2D (15) / POLYLINE_3D (16) / POLYLINE_PFACE (29) / POLYLINE_MESH (30) ← all four fall through together

1. TEXT (1)
2. ATTRIB (2)
3. ATTDEF (3)
4. SEQEND (6)
5. INSERT (7) / MINSERT (8) ← fall-through group
6. POLYLINE_2D (15) / POLYLINE_3D (16) / POLYLINE_PFACE (29) / POLYLINE_MESH (30) ← fall-through group
7. ARC (17)
8. CIRCLE (18)
9. LINE (19)
10. DIM_ORDINATE (20)
11. DIM_LINEAR (21)
12. DIM_ALIGNED (22)
13. DIM_ANGULAR3P (23)
14. DIM_ANGULAR (24)
15. DIM_RADIAL (25)
16. DIM_DIAMETRIC (26)
17. POINT (27)
18. FACE3D (28)
19. SOLID (31)
20. TRACE (32)
21. SHAPE (33)
22. VIEWPORT (34)
23. ELLIPSE (35)
24. SPLINE (36)
25. REGION (37)
26. SOLID3D (38)
27. BODY (39)
28. RAY (40)
29. XLINE (41)
30. MTEXT (44)
31. LEADER (45)
32. TOLERANCE (46)
33. MLINE (47)
34. OLE2FRAME (74)
35. LWPOLYLINE (77)
36. HATCH (78)
37. IMAGE (101)
38. default (custom classes >= 500)

### 3.2 Code Pages (Numeric Order)

28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 44, default

### 3.3 Object Types (Numeric Order)

For dwgreader.cpp line 2072 (objects pass):
1. DICTIONARY (42)
2. GROUP (72)
3. MLINESTYLE (73)
4. XRECORD (79)
5. ACDBPLACEHOLDER (80)
6. LAYOUT (82)
7. IMAGEDEF (102)
8. default (custom classes >= 500)

### 3.4 DXF Group Codes (Numeric Order)

Group by entity type, sort numerically within each group.

## Phase 4: Compile Verification

1. **Cross-check enum values against ODA DWG specification**:
   - Verify entity type codes (1-47, 74, 77-78, 101) match ODA spec §19
   - Verify object type codes (42, 72, 73, 79, 80, 82, 102) match ODA spec §20
   - Verify code page IDs (28-44) match DWG file header specification
   - Verify color encoding flags (0xC0-0xC3, 0xC8) match ODA spec §28

2. Run CMake build for all platforms
3. Run qmake6 build for all platforms
4. Execute existing tests
5. Verify no regressions

## Files to Modify

### New Enums
- [drw_base.h](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/drw_base.h) - Add dxfCode::Common enum (shared by drw_entities.cpp and drw_objects.cpp)
- [dwgutil.h](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgutil.h) - Add enum definitions (dwgType::Entity, dwgCP::CodePage, dwgColor::Encoding, dwgObjType::Object)

### Switch-Case Replacements
- [dwgreader.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgreader.cpp) - Code page switch (line 129), entity type switch (line 1355), object type switch (line 2072)
- [drw_entities.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/drw_entities.cpp) - Common DXF group codes in parseCode() methods (lines 970, 1216, 1655, 1949, 2593, 2819, 3083, 3436, 5289, 5504)
- [drw_objects.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/drw_objects.cpp) - Common DXF group codes in parseCode() methods (lines 744, 952, 1283, 1440, 1687, 2031, 2107)
- [dwgbuffer.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/dwgbuffer.cpp) - Color encoding switch (line 797)
- [proxygraphicdecoder.cpp](file:///D:/data/dli/LibreCAD/libraries/libdxfrw/src/intern/proxygraphicdecoder.cpp) - Color encoding switch (line 457)

## Risk Assessment

### High Risk
- Entity type switch in dwgreader.cpp (1355-1850) - largest switch block, many values
- Object type switch in dwgreader.cpp (2072-2200) - second largest switch block
- Incorrect enum values could break entity/object parsing

### Medium Risk
- Code page switch - affects text encoding
- DXF group code replacements - context-dependent
- Color encoding switches (dwgbuffer.cpp, proxygraphicdecoder.cpp) - affects color rendering

### Low Risk
- Section and table switches - already using enums (secEnum::DWGSection, DRW::TTYPE)
- Version switches - already using DRW::Version enum
- R11 entity type switches - already using internal R11Type enum

## Mitigation Strategy

1. **Incremental Changes**: Replace one switch block at a time
2. **Compile After Each Change**: Verify no syntax errors
3. **Run Tests**: Use existing test suite to verify functionality
4. **DWG File Testing**: Test with sample DWG files of various versions

## Expected Benefits

1. **Improved Readability**: Descriptive enum names instead of magic numbers
2. **Better Maintainability**: Easier to understand and modify code
3. **Type Safety**: Compiler catches invalid enum values
4. **Self-Documenting**: Enum names serve as documentation
5. **Consistency**: Uniform approach across the codebase