/****************************************************************************
** $Id: main.cpp $
**
** Copyright (C) 2011 Rallaz - rallazz@gmail.com
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
** Copyright (C) 2025 LibreCAD contributors
**
** This file is part of the ttf2lff project.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.ribbonsoft.com for further details.
**
** Contact info@ribbonsoft.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifdef __APPLE__
    #include <sys/types.h>
#endif
#ifdef __WIN32__
    #include <time.h>
#endif

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_MODULE_H
#include FT_OUTLINE_H

// RAII wrappers for FreeType resources
struct FTLibraryDeleter {
    void operator()(FT_Library lib) const {
        if (lib) FT_Done_FreeType(lib);
    }
};

struct FTFaceDeleter {
    void operator()(FT_Face f) const {
        if (f) FT_Done_Face(f);
    }
};

struct FTGlyphDeleter {
    void operator()(FT_Glyph g) const {
        if (g) FT_Done_Glyph(g);
    }
};

using FTLibraryPtr = std::unique_ptr<std::remove_pointer_t<FT_Library>, FTLibraryDeleter>;
using FTFacePtr = std::unique_ptr<std::remove_pointer_t<FT_Face>, FTFaceDeleter>;
using FTGlyphPtr = std::unique_ptr<std::remove_pointer_t<FT_Glyph>, FTGlyphDeleter>;

static std::string FT_StrError(FT_Error errnum)
{
    #undef __FTERRORS_H__
    #define FT_ERRORDEF( e, v, s )  { e, s },
    #define FT_ERROR_START_LIST     {
    #define FT_ERROR_END_LIST       { 0, 0 } };

    static const struct {
        FT_Error errnum;
        const char * errstr;
    } ft_errtab[] =
    #include FT_ERRORS_H

    const FT_Error errno_max = (FT_Error)((sizeof(ft_errtab) / sizeof(ft_errtab[0])) - 2 /* FT_ERROR_END_LIST */);
    if(errno > errno_max)
    {
        return "Internal error";
    }

    return std::string(ft_errtab[errnum].errstr);
}

// Data structures for glyph buffering
struct Vertex {
    double x;
    double y;
    double bulge;  // Arc bulge factor (0.0 if not an arc)

    Vertex() : x(0), y(0), bulge(0.0) {}
    Vertex(double x_, double y_, double bulge_ = 0.0) : x(x_), y(y_), bulge(bulge_) {}

    bool operator==(const Vertex& other) const {
        // Use tolerance for floating point comparison
        const double EPS = 1e-9;
        return std::abs(x - other.x) < EPS &&
               std::abs(y - other.y) < EPS &&
               std::abs(bulge - other.bulge) < EPS;
    }

    bool operator!=(const Vertex& other) const {
        return !(*this == other);
    }
};

struct BoundingBox {
    double xMin, yMin, xMax, yMax;
    BoundingBox() : xMin(0), yMin(0), xMax(0), yMax(0) {}
    BoundingBox(double xmin, double ymin, double xmax, double ymax)
        : xMin(xmin), yMin(ymin), xMax(xmax), yMax(ymax) {}

    bool contains(const BoundingBox& other) const {
        return xMin <= other.xMin && yMin <= other.yMin &&
               xMax >= other.xMax && yMax >= other.yMax;
    }

    bool overlaps(const BoundingBox& other) const {
        return xMin < other.xMax && xMax > other.xMin &&
               yMin < other.yMax && yMax > other.yMin;
    }

    double width() const { return xMax - xMin; }
    double height() const { return yMax - yMin; }
    double area() const { return width() * height(); }
};

struct Polyline {
    std::vector<Vertex> vertices;
    std::string comment;
    BoundingBox bbox;

    bool isEmpty() const { return vertices.empty(); }

    void updateBoundingBox() {
        if (vertices.empty()) {
            bbox = BoundingBox();
            return;
        }
        double xMin = vertices[0].x, yMin = vertices[0].y;
        double xMax = vertices[0].x, yMax = vertices[0].y;
        for (size_t i = 1; i < vertices.size(); ++i) {
            xMin = std::min(xMin, vertices[i].x);
            yMin = std::min(yMin, vertices[i].y);
            xMax = std::max(xMax, vertices[i].x);
            yMax = std::max(yMax, vertices[i].y);
        }
        bbox = BoundingBox(xMin, yMin, xMax, yMax);
    }
};

struct GlyphReference {
    unsigned int charCode = 0;
};

struct Glyph {
    unsigned int charCode;
    std::string symbol;
    std::vector<GlyphReference> references;
    std::vector<Polyline> polylines;
    std::string comment;
    BoundingBox bbox;

    bool isEmpty() const { return references.empty() && polylines.empty(); }

    void updateBoundingBox() {
        if (polylines.empty()) {
            bbox = BoundingBox();
            return;
        }
        double xMin = polylines[0].bbox.xMin, yMin = polylines[0].bbox.yMin;
        double xMax = polylines[0].bbox.xMax, yMax = polylines[0].bbox.yMax;
        for (size_t i = 1; i < polylines.size(); ++i) {
            xMin = std::min(xMin, polylines[i].bbox.xMin);
            yMin = std::min(yMin, polylines[i].bbox.yMin);
            xMax = std::max(xMax, polylines[i].bbox.xMax);
            yMax = std::max(yMax, polylines[i].bbox.yMax);
        }
        bbox = BoundingBox(xMin, yMin, xMax, yMax);
    }
};

// Encapsulated state for ttf2lff converter
class TTF2LFFConverter {
private:
    FTLibraryPtr library;
    FTFacePtr face;
    std::ofstream outputFile;
    double prevx;
    double prevy;
    bool firstpass;
    bool startcontour;
    float xMin;
    int nodes, precision;
    double factor;
    int yMax;
    std::string numFormat;
    std::vector<Glyph> glyphBuffer;

public:
    TTF2LFFConverter()
        : prevx(0), prevy(0), firstpass(false), startcontour(false),
          xMin(0), nodes(4), precision(6), factor(0), yMax(-1000) {}

    // Disable copy
    TTF2LFFConverter(const TTF2LFFConverter&) = delete;
    TTF2LFFConverter& operator=(const TTF2LFFConverter&) = delete;

    // Enable move
    TTF2LFFConverter(TTF2LFFConverter&&) = default;
    TTF2LFFConverter& operator=(TTF2LFFConverter&&) = default;

    ~TTF2LFFConverter() {
        // Resources are automatically cleaned up by smart pointers
    }

    FT_Error initLibrary() {
        FT_Library lib = nullptr;
        FT_Error error = FT_Init_FreeType(&lib);
        if (error) {
            std::cerr << "FT_Init_FreeType: " << FT_StrError(error) << std::endl;
            return error;
        }
        library.reset(lib);
        return error;
    }

    FT_Error loadFace(const std::string& filename) {
        FT_Face f = nullptr;
        FT_Error error = FT_New_Face(library.get(), filename.c_str(), 0, &f);
        if (error) {
            std::cerr << "FT_New_Face: " << filename << ": " << FT_StrError(error) << std::endl;
            return error;
        }
        face.reset(f);
        return error;
    }

    bool openOutputFile(const std::string& filename) {
        outputFile.open(filename, std::ios::binary);
        if (!outputFile.is_open()) {
            std::cerr << "Cannot open " << filename << ": " << strerror(errno) << "\n";
            return false;
        }
        return true;
    }

    void closeOutputFile() {
        if (outputFile.is_open()) {
            outputFile.close();
        }
    }

    FT_Face getFace() const { return face.get(); }
    FT_Library getLibrary() const { return library.get(); }
    std::ofstream& getOutputFile() { return outputFile; }

    // State accessors for callbacks
    double& getPrevX() { return prevx; }
    double& getPrevY() { return prevy; }
    bool& getFirstPass() { return firstpass; }
    bool& getStartContour() { return startcontour; }
    float& getXMin() { return xMin; }
    int& getNodes() { return nodes; }
    int& getPrecision() { return precision; }
    double& getFactor() { return factor; }
    int& getYMax() { return yMax; }
    std::string& getNumFormat() { return numFormat; }
    std::vector<Glyph>& getGlyphBuffer() { return glyphBuffer; }
};

// Tuning algorithm flags (from python-lff)
struct TuningOptions {
    bool zeroVector = false;    // Remove duplicate consecutive vertices
    bool roundVertex = false;   // Round vertices to grid
    bool mergePath = false;     // Merge paths with same vertex
    bool nestedChar = false;    // Find nested characters
    bool noComment = false;     // Remove glyph comments
    double roundGrid = 0.0001;  // Grid size for rounding
};

TuningOptions tuningOptions;

// Forward declarations
int moveTo(FT_Vector* to, void* /*fp*/);
int lineTo(FT_Vector* to, void* /*fp*/);
int conicTo(FT_Vector* /*control*/, FT_Vector* to, void* /*fp*/);
int cubicTo(FT_Vector* control1, FT_Vector* control2, FT_Vector* to, void* /*fp*/);

// Global converter instance (for callbacks)
static TTF2LFFConverter* g_converter = nullptr;

static const FT_Outline_Funcs funcs
= {
      (FT_Outline_MoveTo_Func) moveTo,
      (FT_Outline_LineTo_Func) lineTo,
      (FT_Outline_ConicTo_Func)conicTo,
      (FT_Outline_CubicTo_Func)cubicTo,
      0, 0
  };

// Callback function implementations
int moveTo(FT_Vector* to, void* /*fp*/) {
    if (!g_converter) return 0;
    auto& firstpass = g_converter->getFirstPass();
    auto& xMin = g_converter->getXMin();
    auto& prevx = g_converter->getPrevX();
    auto& prevy = g_converter->getPrevY();

    if (firstpass) {
        if (to->x < xMin)
            xMin = to->x;
    } else {
        prevx = to->x;
        prevy = to->y;
    }
    return 0;
}

int lineTo(FT_Vector* to, void* /*fp*/) {
    if (!g_converter) return 0;
    auto& firstpass = g_converter->getFirstPass();
    auto& xMin = g_converter->getXMin();
    auto& prevx = g_converter->getPrevX();
    auto& prevy = g_converter->getPrevY();
    auto& yMax = g_converter->getYMax();

    if (firstpass) {
        if (to->x < xMin)
            xMin = to->x;
    } else {
        prevx = to->x;
        prevy = to->y;
        if (to->y > yMax) {
            yMax = to->y;
        }
    }
    return 0;
}

int conicTo(FT_Vector* /*control*/, FT_Vector* to, void* /*fp*/) {
    if (!g_converter) return 0;
    auto& firstpass = g_converter->getFirstPass();
    auto& xMin = g_converter->getXMin();
    auto& prevx = g_converter->getPrevX();
    auto& prevy = g_converter->getPrevY();
    auto& yMax = g_converter->getYMax();

    if (firstpass) {
        if (to->x < xMin)
            xMin = to->x;
    } else {
        prevx = to->x;
        prevy = to->y;
        if (to->y > yMax) {
            yMax = to->y;
        }
    }
    return 0;
}

int cubicTo(FT_Vector* /*control1*/, FT_Vector* /*control2*/, FT_Vector* to, void* /*fp*/) {
    if (!g_converter) return 0;
    auto& firstpass = g_converter->getFirstPass();
    auto& xMin = g_converter->getXMin();
    auto& prevx = g_converter->getPrevX();
    auto& prevy = g_converter->getPrevY();
    auto& yMax = g_converter->getYMax();

    if (firstpass) {
        if (to->x < xMin)
            xMin = to->x;
    } else {
        prevx = to->x;
        prevy = to->y;
        if (to->y > yMax) {
            yMax = to->y;
        }
    }
    return 0;
}

struct OutlineBuildContext {
    int nodes = 4;
    bool hasCurrent = false;
    FT_Vector currentPoint = {0, 0};
    Polyline currentPolyline;
    std::vector<Polyline> rawPolylines;
};

void finishCurrentPolyline(OutlineBuildContext& context)
{
    if (context.currentPolyline.vertices.size() >= 2) {
        context.rawPolylines.push_back(context.currentPolyline);
    }
    context.currentPolyline = Polyline();
}

void appendOutlinePoint(OutlineBuildContext& context, double x, double y)
{
    context.currentPolyline.vertices.emplace_back(x, y, 0.0);
}

int buildMoveTo(FT_Vector* to, void* user)
{
    auto* context = static_cast<OutlineBuildContext*>(user);
    finishCurrentPolyline(*context);
    appendOutlinePoint(*context, to->x, to->y);
    context->currentPoint = *to;
    context->hasCurrent = true;
    return 0;
}

int buildLineTo(FT_Vector* to, void* user)
{
    auto* context = static_cast<OutlineBuildContext*>(user);
    appendOutlinePoint(*context, to->x, to->y);
    context->currentPoint = *to;
    context->hasCurrent = true;
    return 0;
}

int buildConicTo(FT_Vector* control, FT_Vector* to, void* user)
{
    auto* context = static_cast<OutlineBuildContext*>(user);
    const FT_Vector from = context->currentPoint;
    const int steps = std::max(1, context->nodes);

    for (int i = 1; i <= steps; ++i) {
        const double t = static_cast<double>(i) / steps;
        const double mt = 1.0 - t;
        const double x = mt * mt * from.x + 2.0 * mt * t * control->x + t * t * to->x;
        const double y = mt * mt * from.y + 2.0 * mt * t * control->y + t * t * to->y;
        appendOutlinePoint(*context, x, y);
    }

    context->currentPoint = *to;
    context->hasCurrent = true;
    return 0;
}

int buildCubicTo(FT_Vector* control1, FT_Vector* control2, FT_Vector* to, void* user)
{
    auto* context = static_cast<OutlineBuildContext*>(user);
    const FT_Vector from = context->currentPoint;
    const int steps = std::max(1, context->nodes);

    for (int i = 1; i <= steps; ++i) {
        const double t = static_cast<double>(i) / steps;
        const double mt = 1.0 - t;
        const double x = mt * mt * mt * from.x
                       + 3.0 * mt * mt * t * control1->x
                       + 3.0 * mt * t * t * control2->x
                       + t * t * t * to->x;
        const double y = mt * mt * mt * from.y
                       + 3.0 * mt * mt * t * control1->y
                       + 3.0 * mt * t * t * control2->y
                       + t * t * t * to->y;
        appendOutlinePoint(*context, x, y);
    }

    context->currentPoint = *to;
    context->hasCurrent = true;
    return 0;
}

static const FT_Outline_Funcs glyphBuildFuncs = {
    (FT_Outline_MoveTo_Func) buildMoveTo,
    (FT_Outline_LineTo_Func) buildLineTo,
    (FT_Outline_ConicTo_Func) buildConicTo,
    (FT_Outline_CubicTo_Func) buildCubicTo,
    0,
    0
};

std::vector<Polyline> buildPolylinesFromOutline(FT_Outline& outline, int nodes, double factor, FT_Error& error)
{
    OutlineBuildContext context;
    context.nodes = nodes;

    error = FT_Outline_Decompose(&outline, &glyphBuildFuncs, &context);
    if (error) {
        return {};
    }

    finishCurrentPolyline(context);
    if (context.rawPolylines.empty()) {
        return {};
    }

    double xMin = std::numeric_limits<double>::max();
    for (const auto& polyline : context.rawPolylines) {
        for (const auto& vertex : polyline.vertices) {
            xMin = std::min(xMin, vertex.x);
        }
    }

    std::vector<Polyline> polylines;
    polylines.reserve(context.rawPolylines.size());
    for (auto polyline : context.rawPolylines) {
        for (auto& vertex : polyline.vertices) {
            vertex.x = (vertex.x - xMin) * factor;
            vertex.y *= factor;
        }
        polyline.updateBoundingBox();
        polylines.push_back(std::move(polyline));
    }

    return polylines;
}

/**
 * Format a number, removing trailing zeros
 */
std::string clearZeros(double num) {
    if (!g_converter) {
        // Fallback if converter not initialized
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "%.6f", num);
        std::string str = buffer;
        int i = static_cast<int>(str.length()) - 1;
        while (i > 1 && str.at(i) == '0') {
            --i;
        }
        if (str.at(i) != '.')
            ++i;
        return str.substr(0, i);
    }

    int precision = g_converter->getPrecision();
    const std::string& numFormat = g_converter->getNumFormat();

    std::string numLine(precision + 10, '\0');
    int len = snprintf(&numLine[0], precision + 10, numFormat.c_str(), num);
    std::string str = numLine.substr(0, len);
    int i = static_cast<int>(str.length()) - 1;
    while (i > 1 && str.at(i) == '0') {
        --i;
    }
    if (str.at(i) != '.')
        ++i;
    return str.substr(0, i);
}

/**
 * Round a value to the nearest grid point
 */
double roundToGrid(double value) {
    if (!tuningOptions.roundVertex) return value;
    return std::round(value / tuningOptions.roundGrid) * tuningOptions.roundGrid;
}

std::string formatCharCode(unsigned int charCode)
{
    std::ostringstream stream;
    stream << std::hex << std::nouppercase << std::setfill('0');
    if (charCode <= 0xffff) {
        stream << std::setw(4);
    }
    stream << charCode;
    return stream.str();
}

bool canReferenceGlyph(unsigned int charCode)
{
    // LibreCAD's LFF header discovery currently extracts four hex digits.
    return charCode <= 0xffff;
}

bool shouldWriteHeaderSymbol(FT_ULong charcode)
{
    if (charcode == 0x7f) {
        return false;
    }
    if (charcode < 0x20) {
        return false;
    }
    if (charcode >= 0x80 && charcode <= 0x9f) {
        return false;
    }
    return true;
}

std::string serializePolyline(const Polyline& polyline)
{
    std::ostringstream stream;
    for (size_t i = 0; i < polyline.vertices.size(); ++i) {
        const auto& vertex = polyline.vertices[i];
        if (i > 0) {
            stream << ';';
        }
        stream << clearZeros(vertex.x) << ',' << clearZeros(vertex.y);
        if (vertex.bulge != 0.0) {
            stream << ",A" << clearZeros(vertex.bulge);
        }
    }
    return stream.str();
}

struct PathEntry {
    uint32_t id = 0;
    int count = 0;
};

using PathMultiset = std::vector<PathEntry>;

struct GlyphPathInfo {
    PathMultiset paths;
    std::vector<uint32_t> polylinePathIds;
    int pathCount = 0;
    size_t serializedBytes = 0;
    size_t refBytes = 0;
    uint32_t rarestPathId = std::numeric_limits<uint32_t>::max();
    unsigned int charCode = 0;
    bool refable = false;
};

uint32_t internPathId(const std::string& path,
                      std::unordered_map<std::string, uint32_t>& pathIds,
                      std::vector<std::string>& pathText)
{
    const auto found = pathIds.find(path);
    if (found != pathIds.end()) {
        return found->second;
    }

    const uint32_t id = static_cast<uint32_t>(pathText.size());
    pathText.push_back(path);
    pathIds.emplace(pathText.back(), id);
    return id;
}

PathMultiset compactPathIds(std::vector<uint32_t>& ids)
{
    PathMultiset paths;
    if (ids.empty()) {
        return paths;
    }

    std::sort(ids.begin(), ids.end());
    for (const uint32_t id : ids) {
        if (!paths.empty() && paths.back().id == id) {
            ++paths.back().count;
        } else {
            paths.push_back({id, 1});
        }
    }

    return paths;
}

GlyphPathInfo buildGlyphPathInfo(const Glyph& glyph,
                                 std::unordered_map<std::string, uint32_t>& pathIds,
                                 std::vector<std::string>& pathText)
{
    GlyphPathInfo info;
    info.charCode = glyph.charCode;
    info.refable = canReferenceGlyph(glyph.charCode);
    info.refBytes = 1 + formatCharCode(glyph.charCode).size() + 1;
    info.polylinePathIds.reserve(glyph.polylines.size());

    std::vector<uint32_t> ids;
    for (const auto& polyline : glyph.polylines) {
        if (polyline.vertices.size() < 2) {
            info.polylinePathIds.push_back(std::numeric_limits<uint32_t>::max());
            continue;
        }
        const uint32_t id = internPathId(serializePolyline(polyline), pathIds, pathText);
        info.polylinePathIds.push_back(id);
        ids.push_back(id);
        ++info.pathCount;
        info.serializedBytes += pathText[id].size() + 1;
    }

    info.paths = compactPathIds(ids);
    return info;
}

int pathCount(const PathMultiset& paths)
{
    int count = 0;
    for (const auto& entry : paths) {
        count += entry.count;
    }
    return count;
}

bool containsPathMultiset(const PathMultiset& haystack, const PathMultiset& needle)
{
    size_t i = 0;
    size_t j = 0;

    while (i < haystack.size() && j < needle.size()) {
        if (haystack[i].id < needle[j].id) {
            ++i;
            continue;
        }
        if (haystack[i].id > needle[j].id) {
            return false;
        }
        if (haystack[i].count < needle[j].count) {
            return false;
        }
        ++i;
        ++j;
    }

    return j == needle.size();
}

void subtractPathMultiset(PathMultiset& haystack, const PathMultiset& needle)
{
    PathMultiset result;
    result.reserve(haystack.size());

    size_t i = 0;
    size_t j = 0;
    while (i < haystack.size()) {
        PathEntry entry = haystack[i];
        if (j < needle.size() && entry.id == needle[j].id) {
            entry.count -= needle[j].count;
            ++j;
        } else if (j < needle.size() && entry.id > needle[j].id) {
            ++j;
            continue;
        }
        if (entry.count > 0) {
            result.push_back(entry);
        }
        ++i;
    }

    haystack = std::move(result);
}

bool decrementPathEntry(PathMultiset& paths, uint32_t id)
{
    for (auto it = paths.begin(); it != paths.end(); ++it) {
        if (it->id != id) {
            continue;
        }
        --it->count;
        if (it->count == 0) {
            paths.erase(it);
        }
        return true;
    }
    return false;
}

bool removePolylineMultiset(Glyph& glyph, std::vector<uint32_t>& polylinePathIds, const PathMultiset& paths)
{
    PathMultiset remaining = paths;
    std::vector<bool> remove(glyph.polylines.size(), false);
    const uint32_t invalidPathId = std::numeric_limits<uint32_t>::max();

    if (polylinePathIds.size() != glyph.polylines.size()) {
        return false;
    }
    for (size_t i = 0; i < glyph.polylines.size(); ++i) {
        const uint32_t id = polylinePathIds[i];
        if (id != invalidPathId && decrementPathEntry(remaining, id)) {
            remove[i] = true;
        }
    }

    if (!remaining.empty()) {
        return false;
    }

    std::vector<Polyline> kept;
    std::vector<uint32_t> keptPathIds;
    kept.reserve(glyph.polylines.size());
    keptPathIds.reserve(polylinePathIds.size());
    for (size_t i = 0; i < glyph.polylines.size(); ++i) {
        if (!remove[i]) {
            kept.push_back(std::move(glyph.polylines[i]));
            keptPathIds.push_back(polylinePathIds[i]);
        }
    }

    glyph.polylines = std::move(kept);
    polylinePathIds = std::move(keptPathIds);
    glyph.updateBoundingBox();
    return true;
}

/**
 * TUNING ALGORITHMS (from python-lff)
 */

/**
 * ZeroVector: Remove two identical consecutive vertices
 * This removes duplicate points that can occur from bezier curve approximations
 */
void applyZeroVector(Glyph& glyph) {
    if (!tuningOptions.zeroVector) return;

    for (auto& polyline : glyph.polylines) {
        if (polyline.vertices.size() <= 1) continue;

        std::vector<Vertex> filtered;
        filtered.reserve(polyline.vertices.size());

        for (const auto& v : polyline.vertices) {
            if (filtered.empty()) {
                filtered.push_back(v);
            } else {
                const Vertex& prev = filtered.back();
                // Skip if identical to previous (using tolerance)
                if (std::abs(prev.x - v.x) < 1e-9 &&
                    std::abs(prev.y - v.y) < 1e-9 &&
                    std::abs(prev.bulge - v.bulge) < 1e-9) {
                    continue;  // Skip duplicate
                }
                filtered.push_back(v);
            }
        }
        polyline.vertices = std::move(filtered);
        polyline.updateBoundingBox();
    }
}

/**
 * RoundVertex: Round vertex coordinates to nearby grid
 */
void applyRoundVertex(Glyph& glyph) {
    if (!tuningOptions.roundVertex) return;

    for (auto& polyline : glyph.polylines) {
        for (auto& v : polyline.vertices) {
            v.x = roundToGrid(v.x);
            v.y = roundToGrid(v.y);
            if (v.bulge != 0.0) {
                v.bulge = roundToGrid(v.bulge);
            }
        }
        polyline.updateBoundingBox();
    }
}

/**
 * MergePath: Merge paths that share the same start/end vertex
 * This combines adjacent polylines when they meet at the same point
 */
void applyMergePath(Glyph& glyph) {
    if (!tuningOptions.mergePath) return;
    if (glyph.polylines.size() <= 1) return;

    // Simple merging: if two polylines share start/end, merge them
    bool changed = true;
    while (changed) {
        changed = false;

        // Rebuild connections map after each merge
        std::map<std::pair<double, double>, std::vector<size_t>> connections;
        for (size_t i = 0; i < glyph.polylines.size(); ++i) {
            const auto& poly = glyph.polylines[i];
            if (poly.vertices.size() < 2) continue;

            const auto& first = poly.vertices.front();
            const auto& last = poly.vertices.back();

            connections[{roundToGrid(first.x), roundToGrid(first.y)}].push_back(i);
            connections[{roundToGrid(last.x), roundToGrid(last.y)}].push_back(i);
        }

        for (auto it = connections.begin(); it != connections.end() && !changed; ++it) {
            if (it->second.size() > 1) {
                size_t idx1 = it->second[0];
                size_t idx2 = it->second[1];

                if (idx1 == idx2) continue;

                Polyline mergedPoly;

                const auto& p1 = glyph.polylines[idx1];
                const auto& p2 = glyph.polylines[idx2];

                double eps = tuningOptions.roundVertex ? tuningOptions.roundGrid : 1e-9;

                bool p1StartsAtMatch = (std::abs(p1.vertices.front().x - it->first.first) < eps &&
                                        std::abs(p1.vertices.front().y - it->first.second) < eps);
                bool p1EndsAtMatch = (std::abs(p1.vertices.back().x - it->first.first) < eps &&
                                      std::abs(p1.vertices.back().y - it->first.second) < eps);

                if (p1StartsAtMatch && p1EndsAtMatch) {
                    mergedPoly = p1;
                } else if (p1StartsAtMatch) {
                    for (auto rit = p1.vertices.rbegin(); rit != p1.vertices.rend(); ++rit) {
                        mergedPoly.vertices.push_back(*rit);
                    }
                    mergedPoly.vertices.insert(mergedPoly.vertices.end(), p2.vertices.begin() + 1, p2.vertices.end());
                } else if (p1EndsAtMatch) {
                    mergedPoly.vertices = p1.vertices;
                    mergedPoly.vertices.insert(mergedPoly.vertices.end(), p2.vertices.begin() + 1, p2.vertices.end());
                } else {
                    mergedPoly.vertices = p1.vertices;
                    mergedPoly.vertices.insert(mergedPoly.vertices.end(), p2.vertices.begin() + 1, p2.vertices.end());
                }

                if (idx1 < idx2) {
                    mergedPoly.updateBoundingBox();
                    glyph.polylines[idx1] = mergedPoly;
                    glyph.polylines.erase(glyph.polylines.begin() + idx2);
                } else {
                    mergedPoly.updateBoundingBox();
                    glyph.polylines[idx2] = mergedPoly;
                    glyph.polylines.erase(glyph.polylines.begin() + idx1);
                }

                changed = true;
                break;
            }
        }
    }
}

/**
 * Find duplicate glyphs and nested characters, replace with nested references
 * Uses structural path-subset matching, because LFF Cxxxx references replay
 * exact glyph paths rather than filled outline containment.
 */
size_t findAndReplaceNestedChars() {
    if (!g_converter) return 0;
    if (!tuningOptions.nestedChar) return 0;
    auto& glyphBuffer = g_converter->getGlyphBuffer();
    const size_t glyphCount = glyphBuffer.size();

    if (glyphCount < 2) return 0;

    std::unordered_map<std::string, uint32_t> pathIds;
    std::vector<std::string> pathText;
    std::vector<GlyphPathInfo> glyphInfos;
    pathIds.reserve(glyphCount * 8);
    pathText.reserve(glyphCount * 8);
    glyphInfos.reserve(glyphCount);

    for (size_t i = 0; i < glyphCount; ++i) {
        glyphInfos.push_back(buildGlyphPathInfo(glyphBuffer[i], pathIds, pathText));
    }

    std::vector<int> pathFrequency(pathText.size(), 0);
    for (const auto& info : glyphInfos) {
        for (const auto& entry : info.paths) {
            ++pathFrequency[entry.id];
        }
    }

    // Exact containment filter: if candidate paths are a subset of a target,
    // the candidate's rarest path must also be one of the target's paths.
    std::vector<std::vector<size_t>> rarestPathIndex(pathText.size());
    for (size_t i = 0; i < glyphInfos.size(); ++i) {
        auto& info = glyphInfos[i];
        if (info.paths.empty()) {
            continue;
        }

        auto rarest = info.paths.front().id;
        for (const auto& entry : info.paths) {
            if (pathFrequency[entry.id] < pathFrequency[rarest] ||
                (pathFrequency[entry.id] == pathFrequency[rarest] && entry.id < rarest)) {
                rarest = entry.id;
            }
        }
        info.rarestPathId = rarest;
        rarestPathIndex[info.rarestPathId].push_back(i);
    }

    struct Candidate {
        size_t index;
        int savings;
        int pathTotal;
        unsigned int charCode;
    };

    size_t replacementCount = 0;
    std::vector<unsigned int> seen(glyphCount, 0);
    unsigned int seenStamp = 1;

    for (size_t targetIndex = 0; targetIndex < glyphCount; ++targetIndex) {
        Glyph& target = glyphBuffer[targetIndex];
        const GlyphPathInfo& targetInfo = glyphInfos[targetIndex];
        PathMultiset activePaths = targetInfo.paths;
        if (activePaths.empty()) {
            continue;
        }

        std::vector<uint32_t> activePolylinePathIds = targetInfo.polylinePathIds;
        std::vector<size_t> candidateIndexes;
        if (seenStamp == 0) {
            std::fill(seen.begin(), seen.end(), 0);
            seenStamp = 1;
        }

        for (const auto& activeEntry : activePaths) {
            for (const size_t candidateIndex : rarestPathIndex[activeEntry.id]) {
                if (candidateIndex == targetIndex || seen[candidateIndex] == seenStamp) {
                    continue;
                }
                seen[candidateIndex] = seenStamp;
                candidateIndexes.push_back(candidateIndex);
            }
        }
        ++seenStamp;

        std::vector<Candidate> candidates;
        for (const size_t candidateIndex : candidateIndexes) {
            const GlyphPathInfo& candidateInfo = glyphInfos[candidateIndex];
            if (candidateInfo.paths.empty() || !candidateInfo.refable) {
                continue;
            }
            if (candidateInfo.pathCount > targetInfo.pathCount) {
                continue;
            }
            if (candidateInfo.pathCount == targetInfo.pathCount &&
                targetInfo.charCode <= candidateInfo.charCode) {
                continue;
            }
            if (!containsPathMultiset(activePaths, candidateInfo.paths)) {
                continue;
            }
            if (candidateInfo.serializedBytes <= candidateInfo.refBytes) {
                continue;
            }

            const int savings = static_cast<int>(candidateInfo.serializedBytes - candidateInfo.refBytes);
            candidates.push_back({candidateIndex, savings, candidateInfo.pathCount, candidateInfo.charCode});
        }

        std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
            if (left.savings != right.savings) {
                return left.savings > right.savings;
            }
            if (left.pathTotal != right.pathTotal) {
                return left.pathTotal > right.pathTotal;
            }
            return left.charCode < right.charCode;
        });

        int activeCount = targetInfo.pathCount;
        for (const Candidate& selected : candidates) {
            const GlyphPathInfo& selectedInfo = glyphInfos[selected.index];
            if (!containsPathMultiset(activePaths, selectedInfo.paths)) {
                continue;
            }
            if (activeCount == selectedInfo.pathCount && target.charCode <= selectedInfo.charCode) {
                continue;
            }
            if (!removePolylineMultiset(target, activePolylinePathIds, selectedInfo.paths)) {
                continue;
            }

            target.references.insert(target.references.begin(), GlyphReference{selectedInfo.charCode});
            subtractPathMultiset(activePaths, selectedInfo.paths);
            activeCount -= selectedInfo.pathCount;
            ++replacementCount;
            if (activePaths.empty()) {
                break;
            }
        }
    }

    return replacementCount;
}

/**
 * NoComment: Remove comments from glyphs
 */
void applyNoComment(Glyph& glyph) {
    if (!tuningOptions.noComment) return;
    glyph.comment.clear();
    for (auto& polyline : glyph.polylines) {
        polyline.comment.clear();
    }
}

/**
 * Apply all tuning algorithms to all glyphs
 */
void applyTuning() {
    if (!g_converter) return;
    auto& glyphBuffer = g_converter->getGlyphBuffer();

    for (auto& glyph : glyphBuffer) {
        applyZeroVector(glyph);
        applyRoundVertex(glyph);
        applyMergePath(glyph);
        applyNoComment(glyph);
    }
}

/**
 * Write a glyph to the output file
 */
void writeGlyph(std::ofstream& fp, const Glyph& glyph) {
    // Write glyph header
    if (glyph.symbol.empty()) {
        fp << "\n[#" << formatCharCode(glyph.charCode) << "]\n";
    } else {
        fp << "\n[#" << formatCharCode(glyph.charCode) << "] " << glyph.symbol << "\n";
    }

    for (const auto& reference : glyph.references) {
        fp << "C" << formatCharCode(reference.charCode) << "\n";
    }

    // Write polylines
    for (const auto& polyline : glyph.polylines) {
        if (polyline.isEmpty()) continue;

        // Write comment if present and not suppressed
        if (!polyline.comment.empty() && !tuningOptions.noComment) {
            fp << "# " << polyline.comment << "\n";
        }

        // Write vertices
        for (size_t i = 0; i < polyline.vertices.size(); ++i) {
            const auto& v = polyline.vertices[i];
            if (i > 0) {
                fp << ";";
            }
            fp << clearZeros(v.x) << "," << clearZeros(v.y);
            if (v.bulge != 0.0) {
                fp << ",A" << clearZeros(v.bulge);
            }
        }
        fp << "\n";
    }
}

/**
 * Convert one single glyph (character, sign) into LFF format
 */
FT_Error convertGlyph(TTF2LFFConverter& converter, FT_ULong charcode, bool bufferOnly = false) {
    FT_Error error;
    FT_Glyph glyph_raw = nullptr;

    FT_Face face = converter.getFace();

    // load glyph
    error = FT_Load_Glyph(face,
                          FT_Get_Char_Index(face, charcode),
                          FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE);
    if (error) {
        std::cerr << "FT_Load_Glyph: " << FT_StrError(error) << std::endl;
        return error;
    }

    error = FT_Get_Glyph(face->glyph, &glyph_raw);
    if (error) {
        std::cerr << "FT_Get_Glyph: " << FT_StrError(error) << std::endl;
        return error;
    }

    if (face->glyph->format != ft_glyph_format_outline) {
        std::cerr << "Not an outline font\n";
        FT_Done_Glyph(glyph_raw);
        return 0;
    }

    FTGlyphPtr glyph(glyph_raw);
    FT_OutlineGlyph og = (FT_OutlineGlyph)glyph.get();

    auto& glyphBuffer = converter.getGlyphBuffer();
    auto& xMin = converter.getXMin();
    auto& nodes = converter.getNodes();
    auto& firstpass = converter.getFirstPass();
    auto& startcontour = converter.getStartContour();
    auto& factor = converter.getFactor();

    if (bufferOnly) {
        // Create new glyph entry
        Glyph newGlyph;
        newGlyph.charCode = static_cast<unsigned int>(charcode);

        // Try to get a printable unicode symbol for the header comment.
        if (shouldWriteHeaderSymbol(charcode) &&
            charcode <= 0x10FFFF && charcode != 0xFFFD &&
            !(charcode >= 0xD800 && charcode <= 0xDFFF)) {
            // Valid Unicode codepoint (not surrogate, not replacement char)
            std::string s;
            if (charcode < 0x80) {
                s += static_cast<char>(charcode);
            } else if (charcode < 0x800) {
                s += static_cast<char>(0xC0 | (charcode >> 6));
                s += static_cast<char>(0x80 | (charcode & 0x3F));
            } else if (charcode < 0x10000) {
                s += static_cast<char>(0xE0 | (charcode >> 12));
                s += static_cast<char>(0x80 | ((charcode >> 6) & 0x3F));
                s += static_cast<char>(0x80 | (charcode & 0x3F));
            } else {
                // 4-byte UTF-8 for characters ≥ U+10000
                s += static_cast<char>(0xF0 | (charcode >> 18));
                s += static_cast<char>(0x80 | ((charcode >> 12) & 0x3F));
                s += static_cast<char>(0x80 | ((charcode >> 6) & 0x3F));
                s += static_cast<char>(0x80 | (charcode & 0x3F));
            }
            newGlyph.symbol = s;
        }

        FT_Error buildError = 0;
        newGlyph.polylines = buildPolylinesFromOutline(og->outline, nodes, factor, buildError);
        if (buildError) {
            std::cerr << "FT_Outline_Decompose: " << FT_StrError(buildError) << std::endl;
            return buildError;
        }

        // Add to buffer
        glyphBuffer.push_back(newGlyph);
    } else {
        // Original direct-to-file output (for calibration)
        std::ofstream nullFile("/dev/null");
        if (nullFile.is_open()) {
            nullFile << "\n[#" << std::hex << std::setfill('0') << std::setw(4) << charcode << std::dec << "]\n";

            xMin = 1000.0;
            firstpass = true;
            error = FT_Outline_Decompose(&(og->outline), &funcs, nullptr);
            if (error)
                std::cerr << "FT_Outline_Decompose: first pass: " << FT_StrError(error) << std::endl;

            firstpass = false;
            startcontour = true;
            error = FT_Outline_Decompose(&(og->outline), &funcs, nullptr);
            nullFile << "\n";
        }
    }

    return error;
}

/**
 * Print usage information
 */
static void usage(int eval) {
    std::cout << "Usage: ttf2lff [options] <ttf file> <lff file>\n";
    std::cout << "\n";
    std::cout << "Convert TrueType font to LibreCAD Font Format (LFF)\n";
    std::cout << "\n";
    std::cout << "Arguments:\n";
    std::cout << "  <ttf file>              Input TrueType font file (.ttf, .otf)\n";
    std::cout << "  <lff file>             Output LFF font file\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "  -n, --nodes N          Number of nodes for quadratic/cubic splines (default: 4)\n";
    std::cout << "  -a, --author TEXT      Author name for font metadata\n";
    std::cout << "  -l, --letterspacing N  Letter spacing (default: 3.0)\n";
    std::cout << "  -w, --wordspacing N    Word spacing (default: 6.75)\n";
    std::cout << "  -f, --linespacing N    Line spacing factor (default: 1.0)\n";
    std::cout << "  -d, --precision N      Decimal precision (default: 6)\n";
    std::cout << "  -L, --license TEXT     Font license\n";
    std::cout << "\n";
    std::cout << "Tuning options (from python-lff):\n";
    std::cout << "  -z, --zerovector       Remove duplicate consecutive vertices\n";
    std::cout << "  -r, --round            Round vertices to grid\n";
    std::cout << "  -g, --grid N           Grid size for rounding (default: 0.0001)\n";
    std::cout << "  -m, --mergepath        Merge paths with same start/end vertex\n";
    std::cout << "  -e, --nestedchar       Analyze for nested characters\n";
    std::cout << "  -c, --nocomment        Remove comments from glyphs\n";
    std::cout << "  -t, --tuning           Apply all tuning options\n";
    std::cout << "\n";
    std::cout << "  -h, --help             Show this help message\n";
    std::cout << "\n";
    std::cout << "Example:\n";
    std::cout << "  ttf2lff -a \"John Doe\" -n 8 -z -r -m font.ttf output.lff\n";
    exit(eval);
}


/**
 * Main function
 */
int main(int argc, char* argv[]) {
    FT_Error error;
    std::string fTtf;
    std::string fLff;

    // Default values
    int nodes = 4;
    std::string name = "Unknown";
    double letterSpacing = 3.0;
    double wordSpacing = 6.75;
    double lineSpacingFactor = 1.0;
    std::string author = "Unknown";
    std::string license = "Unknown";
    int precision = 6;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            usage(0);
        }
        else if (arg == "-n" || arg == "--nodes") {
            if (++i >= argc) { std::cerr << "Error: -n requires an argument\n"; return 1; }
            nodes = std::atoi(argv[i]);
        }
        else if (arg == "-a" || arg == "--author") {
            if (++i >= argc) { std::cerr << "Error: -a requires an argument\n"; return 1; }
            author = argv[i];
        }
        else if (arg == "-l" || arg == "--letterspacing") {
            if (++i >= argc) { std::cerr << "Error: -l requires an argument\n"; return 1; }
            letterSpacing = std::atof(argv[i]);
        }
        else if (arg == "-w" || arg == "--wordspacing") {
            if (++i >= argc) { std::cerr << "Error: -w requires an argument\n"; return 1; }
            wordSpacing = std::atof(argv[i]);
        }
        else if (arg == "-f" || arg == "--linespacing") {
            if (++i >= argc) { std::cerr << "Error: -f requires an argument\n"; return 1; }
            lineSpacingFactor = std::atof(argv[i]);
        }
        else if (arg == "-d" || arg == "--precision") {
            if (++i >= argc) { std::cerr << "Error: -d requires an argument\n"; return 1; }
            precision = std::atoi(argv[i]);
        }
        else if (arg == "-L" || arg == "--license") {
            if (++i >= argc) { std::cerr << "Error: -L requires an argument\n"; return 1; }
            license = argv[i];
        }
        else if (arg == "-z" || arg == "--zerovector") {
            tuningOptions.zeroVector = true;
        }
        else if (arg == "-r" || arg == "--round") {
            tuningOptions.roundVertex = true;
        }
        else if (arg == "-g" || arg == "--grid") {
            if (++i >= argc) { std::cerr << "Error: -g requires an argument\n"; return 1; }
            tuningOptions.roundGrid = std::atof(argv[i]);
        }
        else if (arg == "-m" || arg == "--mergepath") {
            tuningOptions.mergePath = true;
        }
        else if (arg == "-e" || arg == "--nestedchar") {
            tuningOptions.nestedChar = true;
        }
        else if (arg == "-c" || arg == "--nocomment") {
            tuningOptions.noComment = true;
        }
        else if (arg == "-t" || arg == "--tuning") {
            // Enable all tuning options
            tuningOptions.zeroVector = true;
            tuningOptions.roundVertex = true;
            tuningOptions.mergePath = true;
            tuningOptions.nestedChar = true;
            tuningOptions.noComment = true;
        }
        else if (arg[0] != '-') {
            // Assume first non-option is TTF file, second is LFF file
            fTtf = arg;
            if (++i < argc) {
                fLff = argv[i];
            }
        }
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            usage(1);
        }
    }

    if (fTtf.empty() || fLff.empty()) {
        std::cerr << "Error: Missing required arguments\n\n";
        usage(1);
    }

    std::cout << "TTF file: " << fTtf << "\n";
    std::cout << "LFF file: " << fLff << "\n";

    if (tuningOptions.zeroVector) std::cout << "Tuning: ZeroVector enabled\n";
    if (tuningOptions.roundVertex) std::cout << "Tuning: RoundVertex enabled (grid=" << tuningOptions.roundGrid << ")\n";
    if (tuningOptions.mergePath) std::cout << "Tuning: MergePath enabled\n";
    if (tuningOptions.nestedChar) std::cout << "Tuning: NestedChar enabled\n";
    if (tuningOptions.noComment) std::cout << "Tuning: NoComment enabled\n";

    // Create converter with RAII
    TTF2LFFConverter converter;
    converter.getNodes() = nodes;
    converter.getPrecision() = precision;

    // Initialize FreeType
    error = converter.initLibrary();
    if (error) {
        return 1;
    }

    FT_Library library = converter.getLibrary();
    FT_Int major = 0, minor = 0, patch = 0;
    FT_Library_Version(library, &major, &minor, &patch);
    std::cerr << "FreeType version: " << major << '.' << minor << '.' << patch << std::endl;

    // Load font
    error = converter.loadFace(fTtf);
    if (error) {
        return 1;
    }

    FT_Face face = converter.getFace();
    std::cout << "Family:    " << face->family_name << "\n";
    std::cout << "Style:     " << face->style_name << "\n";
    std::cout << "Height:    " << face->height << "\n";
    std::cout << "Ascender:  " << face->ascender << "\n";
    std::cout << "Descender: " << face->descender << "\n";
    std::cout << "Faces:     " << face->num_faces << "\n";
    std::cout << "Glyphs:    " << face->num_glyphs << "\n";
    name = face->family_name;

    // Determine scale factor by tracing 'A'
    converter.getYMax() = -1000;
    g_converter = &converter;
    convertGlyph(converter, 65, false);  // Direct output for calibration
    converter.getFactor() = 1.0 / (1.0 / 9.0 * converter.getYMax());
    std::cout << "Factor:    " << converter.getFactor() << "\n";

    // Open output file
    if (!converter.openOutputFile(fLff)) {
        return 2;
    }

    std::ofstream& outputFile = converter.getOutputFile();

    // Set number format
    std::string numFormat = "%." + std::to_string(precision) + "f";
    converter.getNumFormat() = numFormat;

    // Write font header
    outputFile << "# Format:            LibreCAD Font 1\n";
    outputFile << "# Creator:           ttf2lff with python-lff tuning\n";
    outputFile << "# Version:           1\n";
    outputFile << "# Name:              " << name << "\n";
    outputFile << "# Encoding:          UTF-8\n";
    outputFile << "# LetterSpacing:     " << clearZeros(letterSpacing) << "\n";
    outputFile << "# WordSpacing:       " << clearZeros(wordSpacing) << "\n";
    outputFile << "# LineSpacingFactor: " << clearZeros(lineSpacingFactor) << "\n";

    time_t rawtime;
    time(&rawtime);
    struct tm* timeinfo = localtime(&rawtime);
    char buffer[12];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);

    outputFile << "# Created:           " << buffer << "\n";
    outputFile << "# Last modified:     " << buffer << "\n";
    outputFile << "# Author:            " << author << "\n";
    outputFile << "# License:           " << license << "\n";

    // Write tuning information as comments
    if (tuningOptions.zeroVector || tuningOptions.roundVertex ||
        tuningOptions.mergePath || tuningOptions.nestedChar || tuningOptions.noComment) {
        outputFile << "# Tuning:            ";
        bool first = true;
        if (tuningOptions.zeroVector) { outputFile << (first ? "" : ", ") << "ZeroVector"; first = false; }
        if (tuningOptions.roundVertex) {
            outputFile << (first ? "" : ", ") << "RoundVertex(grid=" << std::fixed << std::setprecision(6) << tuningOptions.roundGrid << ")";
            first = false;
        }
        if (tuningOptions.mergePath) { outputFile << (first ? "" : ", ") << "MergePath"; first = false; }
        if (tuningOptions.nestedChar) { outputFile << (first ? "" : ", ") << "NestedChar"; first = false; }
        if (tuningOptions.noComment) { outputFile << (first ? "" : ", ") << "NoComment"; first = false; }
        outputFile << "\n";
    }

    outputFile << "\n";

    // Convert all glyphs
    // First, collect all glyphs into buffer for tuning
    converter.getGlyphBuffer().clear();

    FT_ULong charcode;
    FT_UInt gindex;

    charcode = FT_Get_First_Char(face, &gindex);
    while (gindex != 0) {
        convertGlyph(converter, charcode, true);  // Buffer the glyph
        charcode = FT_Get_Next_Char(face, charcode, &gindex);
    }

    std::cout << "Converted " << converter.getGlyphBuffer().size() << " glyphs\n";

    // Apply tuning algorithms
    if (tuningOptions.zeroVector || tuningOptions.roundVertex ||
        tuningOptions.mergePath || tuningOptions.nestedChar || tuningOptions.noComment) {
        std::cout << "Applying tuning algorithms...\n";
        applyTuning();
        std::cout << "Tuning complete\n";
    }

    // Find and replace nested characters
    if (tuningOptions.nestedChar) {
        std::cout << "Finding nested characters...\n";
        const size_t nestedCount = findAndReplaceNestedChars();
        std::cout << "Nested character processing complete: " << nestedCount << " references\n";
    }

    // Write buffered glyphs to file
    for (const auto& glyph : converter.getGlyphBuffer()) {
        writeGlyph(outputFile, glyph);
    }

    converter.closeOutputFile();
    g_converter = nullptr;

    std::cout << "Conversion complete: " << fLff << "\n";
    return 0;
}
