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
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
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

struct Glyph;
bool glyphContainsPoint(const Glyph& glyph, double x, double y);

struct Glyph {
    unsigned int charCode;
    std::string symbol;
    std::vector<Polyline> polylines;
    std::string comment;
    BoundingBox bbox;

    bool isEmpty() const { return polylines.empty(); }

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
int conicTo(FT_Vector* control, FT_Vector* to, void* /*fp*/);
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

int conicTo(FT_Vector* control, FT_Vector* to, void* /*fp*/) {
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

/**
 * Convert vertex to string representation
 */
std::string vertexToString(const Vertex& v) {
    std::string result = clearZeros(roundToGrid(v.x)) + "," + clearZeros(roundToGrid(v.y));
    if (v.bulge != 0.0) {
        result += ",A" + clearZeros(v.bulge);
    }
    return result;}

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

BoundingBox computeBoundingBox(const Glyph& glyph) {
    if (glyph.isEmpty()) return BoundingBox();

    double xMin = std::numeric_limits<double>::max();
    double yMin = std::numeric_limits<double>::max();
    double xMax = std::numeric_limits<double>::lowest();
    double yMax = std::numeric_limits<double>::lowest();

    for (const auto& poly : glyph.polylines) {
        for (const auto& v : poly.vertices) {
            xMin = std::min(xMin, v.x);
            yMin = std::min(yMin, v.y);
            xMax = std::max(xMax, v.x);
            yMax = std::max(yMax, v.y);
        }
    }

    return BoundingBox(xMin, yMin, xMax, yMax);
}

double computePolygonArea(const std::vector<Vertex>& vertices) {
    if (vertices.size() < 3) return 0.0;

    double area = 0.0;
    size_t n = vertices.size();

    for (size_t i = 0; i < n; ++i) {
        size_t j = (i + 1) % n;
        area += vertices[i].x * vertices[j].y;
        area -= vertices[j].x * vertices[i].y;
    }

    return std::abs(area) / 2.0;
}

double computeGlyphArea(const Glyph& glyph) {
    double totalArea = 0.0;
    for (const auto& poly : glyph.polylines) {
        totalArea += computePolygonArea(poly.vertices);
    }
    return totalArea;
}

bool pointInPolygon(double x, double y, const std::vector<Vertex>& vertices) {
    bool inside = false;
    size_t n = vertices.size();

    for (size_t i = 0, j = n - 1; i < n; j = i++) {
        double xi = vertices[i].x, yi = vertices[i].y;
        double xj = vertices[j].x, yj = vertices[j].y;

        if (((yi > y) != (yj > y)) &&
            (x < (xj - xi) * (y - yi) / (yj - yi) + xi)) {
            inside = !inside;
        }
    }

    return inside;
}

bool glyphContainsPoint(const Glyph& glyph, double x, double y) {
    if (x < glyph.bbox.xMin || x > glyph.bbox.xMax || y < glyph.bbox.yMin || y > glyph.bbox.yMax) {
        return false;
    }

    for (const auto& poly : glyph.polylines) {
        if (poly.vertices.empty()) continue;

        if (x < poly.bbox.xMin || x > poly.bbox.xMax || y < poly.bbox.yMin || y > poly.bbox.yMax) {
            continue;
        }

        if (pointInPolygon(x, y, poly.vertices)) {
            return true;
        }
    }
    return false;
}

/**
 * NestedChar: Find nested characters in glyphs
 * In LFF format, nested chars are referenced like [C0041] meaning copy char 0x0041
 *
 * NOTE: This feature is currently disabled because LFF polylines are open strokes,
 * not closed polygons. The pointInPolygon algorithm assumes closed polygons,
 * causing false positives.
 */
void applyNestedChar(Glyph& glyph) {
    // Disabled - see note above
    // if (!tuningOptions.nestedChar) return;
}

/**
 * Find duplicate glyphs and nested characters, replace with nested references
 * Uses efficient spatial partitioning to avoid O(N*N) complexity
 *
 * NOTE: This feature is currently disabled because LFF polylines are open strokes,
 * not closed polygons. The pointInPolygon algorithm assumes closed polygons,
 * causing false positives.
 */
void findAndReplaceNestedChars() {
    // Disabled - see note above
    return;

    /*
    // Original implementation kept for reference
    if (!g_converter) return;
    if (!tuningOptions.nestedChar) return;

    auto& glyphBuffer = g_converter->getGlyphBuffer();
    size_t glyphCount = glyphBuffer.size();

    if (glyphCount < 2) return;

    std::map<std::string, std::vector<size_t>> glyphFingerprints;

    for (size_t i = 0; i < glyphCount; ++i) {
        const auto& glyph = glyphBuffer[i];
        if (glyph.isEmpty()) continue;

        std::string fingerprint;
        for (const auto& poly : glyph.polylines) {
            for (const auto& v : poly.vertices) {
                fingerprint += clearZeros(v.x) + "," + clearZeros(v.y) + ",";
                if (v.bulge != 0.0) {
                    fingerprint += "A" + clearZeros(v.bulge) + ",";
                }
            }
        }

        glyphFingerprints[fingerprint].push_back(i);
    }

    for (const auto& pair : glyphFingerprints) {
        if (pair.second.size() > 1) {
            size_t originalIdx = pair.second[0];
            unsigned int originalCharCode = glyphBuffer[originalIdx].charCode;

            for (size_t i = 1; i < pair.second.size(); ++i) {
                size_t duplicateIdx = pair.second[i];
                glyphBuffer[duplicateIdx].polylines.clear();
                glyphBuffer[duplicateIdx].comment = "[C" + std::to_string(originalCharCode) + "]";
            }
        }
    }

    struct GlyphInfo {
        size_t index;
        BoundingBox bbox;
        double area;
        bool hasPolylines;
        int vertexCount;
    };

    std::vector<GlyphInfo> glyphInfos;
    glyphInfos.reserve(glyphCount);

    for (size_t i = 0; i < glyphCount; ++i) {
        auto& glyph = glyphBuffer[i];
        if (glyph.isEmpty()) {
            glyphInfos.push_back({i, BoundingBox(), 0.0, false, 0});
            continue;
        }

        bool hasPolylines = !glyph.polylines.empty();
        if (!hasPolylines) {
            glyphInfos.push_back({i, BoundingBox(), 0.0, false, 0});
            continue;
        }

        glyph.updateBoundingBox();
        double area = computeGlyphArea(glyph);
        int vertexCount = 0;
        for (const auto& poly : glyph.polylines) {
            vertexCount += poly.vertices.size();
        }
        glyphInfos.push_back({i, glyph.bbox, area, true, vertexCount});
    }

    std::sort(glyphInfos.begin(), glyphInfos.end(), [](const GlyphInfo& a, const GlyphInfo& b) {
        return a.area < b.area;
    });

    std::vector<bool> processed(glyphCount, false);
    long long containmentChecks = 0;

    for (size_t i = 0; i < glyphInfos.size(); ++i) {
        const auto& innerInfo = glyphInfos[i];
        if (!innerInfo.hasPolylines || processed[innerInfo.index]) continue;

        const auto& innerGlyph = glyphBuffer[innerInfo.index];
        if (innerGlyph.polylines.empty()) continue;

        for (size_t j = i + 1; j < glyphInfos.size(); ++j) {
            const auto& outerInfo = glyphInfos[j];
            if (!outerInfo.hasPolylines || processed[outerInfo.index]) continue;

            if (!outerInfo.bbox.contains(innerInfo.bbox)) continue;

            // Inner must be significantly smaller than outer
            if (innerInfo.area >= outerInfo.area * 0.5) continue;

            containmentChecks++;
            double sampleX = (innerInfo.bbox.xMin + innerInfo.bbox.xMax) / 2.0;
            double sampleY = (innerInfo.bbox.yMin + innerInfo.bbox.yMax) / 2.0;

            // Check center point first
            if (!glyphContainsPoint(glyphBuffer[outerInfo.index], sampleX, sampleY)) continue;

            // Check all 4 corners of inner bounding box
            bool allCornersInside = true;
            std::vector<std::pair<double, double>> corners = {
                {innerInfo.bbox.xMin, innerInfo.bbox.yMin},
                {innerInfo.bbox.xMax, innerInfo.bbox.yMin},
                {innerInfo.bbox.xMin, innerInfo.bbox.yMax},
                {innerInfo.bbox.xMax, innerInfo.bbox.yMax}
            };

            for (const auto& corner : corners) {
                if (!glyphContainsPoint(glyphBuffer[outerInfo.index], corner.first, corner.second)) {
                    allCornersInside = false;
                    break;
                }
            }

            if (!allCornersInside) continue;

            std::cerr << "Nested: inner=" << std::hex << glyphBuffer[innerInfo.index].charCode
                      << " outer=" << glyphBuffer[outerInfo.index].charCode << std::dec
                      << " innerArea=" << innerInfo.area << " outerArea=" << outerInfo.area << std::endl;

            glyphBuffer[innerInfo.index].polylines.clear();
            glyphBuffer[innerInfo.index].comment = "[C" + std::to_string(glyphBuffer[outerInfo.index].charCode) + "]";
            processed[innerInfo.index] = true;
            break;
        }
    }
    */
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
        applyNestedChar(glyph);
        applyNoComment(glyph);
    }
}

/**
 * Write a glyph to the output file
 */
void writeGlyph(std::ofstream& fp, const Glyph& glyph) {
    // Write glyph header
    if (glyph.symbol.empty()) {
        fp << "\n[#" << std::hex << std::setfill('0') << std::setw(4) << glyph.charCode << std::dec << "]\n";
    } else {
        fp << "\n[#" << std::hex << std::setfill('0') << std::setw(4) << glyph.charCode << std::dec << "] " << glyph.symbol << "\n";
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

    FTGlyphPtr glyph(glyph_raw);
    FT_OutlineGlyph og = (FT_OutlineGlyph)glyph.get();
    if (face->glyph->format != ft_glyph_format_outline) {
        std::cerr << "Not an outline font\n";
    }

    auto& glyphBuffer = converter.getGlyphBuffer();
    auto& factor = converter.getFactor();
    auto& xMin = converter.getXMin();
    auto& nodes = converter.getNodes();
    auto& firstpass = converter.getFirstPass();
    auto& startcontour = converter.getStartContour();
    auto& prevx = converter.getPrevX();
    auto& prevy = converter.getPrevY();
    auto& yMax = converter.getYMax();
    auto& outputFile = converter.getOutputFile();

    if (bufferOnly) {
        // Create new glyph entry
        Glyph newGlyph;
        newGlyph.charCode = static_cast<unsigned int>(charcode);

        // Try to get the unicode symbol
        if (charcode <= 0x10FFFF && charcode != 0xFFFD &&
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

        // Calculate xMin from outline points
        xMin = 1000.0;
        short numContours = og->outline.n_contours;
        short* contours = (short*)og->outline.contours;
        FT_Vector* points = og->outline.points;

        for (int c = 0; c < numContours; ++c) {
            int end = contours[c];
            for (int p = 0; p <= end; ++p) {
                if (points[p].x < xMin) {
                    xMin = points[p].x;
                }
            }
        }

        // Trace outline and collect vertices into polylines
        int start = 0;
        for (int c = 0; c < numContours; ++c) {
            int end = contours[c];

            Polyline poly;

            for (int p = start; p <= end; ++p) {
                Vertex v((points[p].x - xMin) * factor, points[p].y * factor, 0.0);

                // Skip duplicate vertices (ZeroVector)
                if (tuningOptions.zeroVector && !poly.vertices.empty()) {
                    const Vertex& prev = poly.vertices.back();
                    if (std::abs(prev.x - v.x) < 1e-9 && std::abs(prev.y - v.y) < 1e-9) {
                        continue;
                    }
                }

                poly.vertices.push_back(v);
            }

            if (!poly.vertices.empty()) {
                poly.updateBoundingBox();
                newGlyph.polylines.push_back(poly);
            }

            start = end + 1;
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
        findAndReplaceNestedChars();
        std::cout << "Nested character processing complete\n";
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
