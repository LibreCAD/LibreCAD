// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD.org
// Copyright (C) 2026 Dongxu Li

// What DXF and DWG files say about the state of their layers (master #3241):
//
// - AutoCAD turns a layer off with a negative colour, apart from freezing it, and a layer can be
//   both. A layer turned off opened shown with colour ByLayer, and a save wrote it shown. Now it
//   opens hidden in its own colour, and a DXF save writes it frozen, turned off or both, as it
//   was read (RS_Layer::HiddenAs).
// - An R13/R14 DWG keeps the lock of a layer in a bit that libdxfrw took as 8, not 4.
// - A DWG header names the current layer by handle (CLAYER); it was read and dropped.
//
// The DWG fixtures were written by the ODA File Converter from DXF files made with ezdxf
// (2.2.1 reads R13 to R2013 DWG, not R2018):
// lc_layer_off_*: the layers "Plain" (colour 1), "Off" (3, turned off), "Frozen" (2, frozen)
// and "OffFrozen" (4, both), R14 and R2010; lc_layer_flags_*: "Plain", "Frozen", "Locked" and
// "FrozenLocked", R13 and R14; lc_current_layer_walls_*: "0", "Doors" and "Walls" with "Walls"
// current, R14 and R2010. R13/R14 keep names in capitals.

#include <memory>

#include <QtTest>
#include <QtWidgets>

#include "rs_debug.h"
#include "rs_filterdxfrw.h"
#include "rs_fontlist.h"
#include "rs_graphic.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_settings.h"
#include "rs_system.h"

namespace {

using HiddenAs = RS_Layer::HiddenAs;

struct ExpectedLayer {
    const char* m_name;
    int m_colorIndex;
    bool m_hidden;
    HiddenAs m_hiddenAs;
};

const ExpectedLayer expectedLayers[] = {
    {"Plain", 1, false, HiddenAs::Frozen},
    {"Off", 3, true, HiddenAs::Off},
    {"Frozen", 2, true, HiddenAs::Frozen},
    {"OffFrozen", 4, true, HiddenAs::FrozenAndOff},
};

// "Plain", "Off", "Frozen" and "OffFrozen" as AutoCAD writes them
const QByteArray dxfWithHiddenLayers =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
    "0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nLAYER\n70\n5\n"
    "0\nLAYER\n5\n10\n2\n0\n70\n0\n62\n7\n6\nCONTINUOUS\n"
    "0\nLAYER\n5\n11\n2\nPlain\n70\n0\n62\n1\n6\nCONTINUOUS\n"
    "0\nLAYER\n5\n12\n2\nOff\n70\n0\n62\n-3\n6\nCONTINUOUS\n"
    "0\nLAYER\n5\n13\n2\nFrozen\n70\n1\n62\n2\n6\nCONTINUOUS\n"
    "0\nLAYER\n5\n14\n2\nOffFrozen\n70\n1\n62\n-4\n6\nCONTINUOUS\n"
    "0\nENDTAB\n0\nENDSEC\n"
    "0\nSECTION\n2\nENTITIES\n"
    "0\nLINE\n5\n20\n8\nPlain\n10\n0\n20\n0\n11\n10\n21\n0\n"
    "0\nLINE\n5\n21\n8\nOff\n10\n0\n20\n1\n11\n10\n21\n1\n"
    "0\nENDSEC\n0\nEOF\n";

// RS_FileIO would show a message box for a DWG file
bool open(RS_Graphic& graphic, const QString& path, RS2::FormatType format)
{
    graphic.newDoc();
    RS_FilterDXFRW filter;
    const bool opened = filter.fileImport(graphic, path, format);
    return opened;
}

QString fixture(const char* name)
{
    return QString(LIBRECAD_TEST_SOURCE_DIR) + "/librecad/src/main/tests/testdata/" + name;
}

// the layer of that name, whatever its case (R13/R14 keep names in capitals)
RS_Layer* layerNamed(RS_Graphic& graphic, const QString& name)
{
    for (unsigned i = 0; i < graphic.countLayers(); ++i) {
        if (graphic.layerAt(i)->getName().compare(name, Qt::CaseInsensitive) == 0)
            return graphic.layerAt(i);
    }
    return nullptr;
}

bool checkHiddenLayers(RS_Graphic& graphic)
{
    bool ok = true;
    for (const ExpectedLayer& expected : expectedLayers) {
        RS_Layer* layer = layerNamed(graphic, QString::fromLatin1(expected.m_name));
        if (layer == nullptr) {
            qWarning() << "missing layer" << expected.m_name;
            ok = false;
            continue;
        }
        const RS_Color own = RS_FilterDXFRW::numberToColor(expected.m_colorIndex);
        const RS_Color color = layer->getPen().getColor();
        if (layer->isFrozen() != expected.m_hidden
            || (expected.m_hidden && layer->getHiddenAs() != expected.m_hiddenAs)
            || color.isByLayer() || color.red() != own.red() || color.green() != own.green()
            || color.blue() != own.blue()) {
            qWarning() << "layer" << expected.m_name << "frozen" << layer->isFrozen() << "hiddenAs"
                       << static_cast<int>(layer->getHiddenAs()) << "colour" << color.name();
            ok = false;
        }
    }
    return ok;
}

} // namespace

class LC_LayerStateFileIoTests : public QObject {
    Q_OBJECT

    QTemporaryDir directory;

    QString writeDxf() const
    {
        const QString path = QDir(directory.path()).filePath("hidden-layers.dxf");
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(dxfWithHiddenLayers);
            file.close();
        }
        return path;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(directory.isValid());
        QCoreApplication::setOrganizationName("LibreCAD-LayerStateFileIoTests");
        QCoreApplication::setApplicationName("layers");
        QCoreApplication::setApplicationVersion("2.2.1");
        RS_DEBUG->setLevel(RS_Debug::D_NOTHING);
        RS_SETTINGS->init(qApp->organizationName(), qApp->applicationName());
        RS_SYSTEM->init("LibreCAD", "2.2.1", "librecad", QString(LIBRECAD_TEST_SOURCE_DIR) + "/unix");
        RS_FONTLIST->init();
    }

    void dxfLayersTurnedOffOpenHidden()
    {
        RS_Graphic graphic;
        QVERIFY(open(graphic, writeDxf(), RS2::FormatDXFRW));
        QVERIFY(checkHiddenLayers(graphic));
    }

    void dwgLayersTurnedOffOpenHidden()
    {
        for (const char* name : {"lc_layer_off_ac1014.dwg", "lc_layer_off_ac1024.dwg"}) {
            QVERIFY2(QFileInfo::exists(fixture(name)), name);
            RS_Graphic graphic;
            QVERIFY2(open(graphic, fixture(name), RS2::FormatDWG), name);
            QVERIFY2(checkHiddenLayers(graphic), name);
        }
    }

    void hiddenLayersAreSavedAsTheyWereRead()
    {
        const QString source = writeDxf();
        int number = 0;
        for (const RS2::FormatType format : {RS2::FormatDXFRW12, RS2::FormatDXFRW14, RS2::FormatDXFRW2000,
                                            RS2::FormatDXFRW2004, RS2::FormatDXFRW}) {
            const QString path = QDir(directory.path()).filePath(QString("saved-%1.dxf").arg(number++));
            {
                RS_Graphic graphic;
                QVERIFY(open(graphic, source, RS2::FormatDXFRW));
                QVERIFY2(graphic.saveAs(path, format, true), qPrintable(path));
            }
            RS_Graphic reopened;
            QVERIFY2(open(reopened, path, RS2::FormatDXFRW), qPrintable(path));
            QVERIFY2(checkHiddenLayers(reopened), qPrintable(QString::number(static_cast<int>(format))));
        }
    }

    void aHiddenCurrentLayerIsSavedAsItWasHidden()
    {
        // a hidden current layer stays current, and is saved frozen or turned off
        // as it was read (#3229 lets a hidden layer be current)
        for (const HiddenAs hiddenAs : {HiddenAs::Frozen, HiddenAs::Off}) {
            const QString path = QDir(directory.path()).filePath(
                QString("current-%1.dxf").arg(static_cast<int>(hiddenAs)));
            {
                RS_Graphic graphic;
                graphic.newDoc();
                graphic.addLayer(new RS_Layer("Doors"));
                graphic.addLayer(new RS_Layer("Walls"));
                graphic.activateLayer("Walls");
                graphic.findLayer("Doors")->freeze(true);
                RS_Layer* current = graphic.findLayer("Walls");
                current->freeze(true);
                current->setHiddenAs(hiddenAs);
                QVERIFY(graphic.saveAs(path, RS2::FormatDXFRW, true));
            }
            RS_Graphic reopened;
            QVERIFY(open(reopened, path, RS2::FormatDXFRW));
            RS_Layer* walls = reopened.findLayer("Walls");
            RS_Layer* doors = reopened.findLayer("Doors");
            QVERIFY(walls != nullptr && doors != nullptr);
            QCOMPARE(reopened.getActiveLayer(), walls);
            QVERIFY(walls->isFrozen());
            QCOMPARE(walls->getHiddenAs(), hiddenAs);
            QVERIFY(doors->isFrozen());
            QCOMPARE(doors->getHiddenAs(), HiddenAs::Frozen);
        }
    }

    void aLayerKeepsHowItIsHiddenWhenShownCopiedOrAddedAgain()
    {
        RS_Layer layer("Off");
        layer.freeze(true);
        layer.setHiddenAs(HiddenAs::Off);

        layer.toggle();
        QVERIFY(!layer.isFrozen());
        layer.toggle();
        QVERIFY(layer.isFrozen());
        QCOMPARE(layer.getHiddenAs(), HiddenAs::Off);

        const RS_Layer copy(layer);
        QCOMPARE(copy.getHiddenAs(), HiddenAs::Off);
        RS_Layer assigned("Other");
        assigned = layer;
        QCOMPARE(assigned.getHiddenAs(), HiddenAs::Off);
        const std::unique_ptr<RS_Layer> clone(layer.clone());
        QCOMPARE(clone->getHiddenAs(), HiddenAs::Off);

        // added again to a drawing that has it, as a file is read into it
        RS_Graphic graphic;
        graphic.newDoc();
        graphic.addLayer(new RS_Layer("Off"));
        graphic.addLayer(new RS_Layer(layer));
        RS_Layer* merged = graphic.findLayer("Off");
        QVERIFY(merged != nullptr);
        QVERIFY(merged->isFrozen());
        QCOMPARE(merged->getHiddenAs(), HiddenAs::Off);
    }

    void r13AndR14DwgLayersKeepTheirLock()
    {
        struct Expected {
            const char* m_name;
            bool m_frozen;
            bool m_locked;
        };
        for (const char* file : {"lc_layer_flags_ac1012.dwg", "lc_layer_flags_ac1014.dwg"}) {
            QVERIFY2(QFileInfo::exists(fixture(file)), file);
            RS_Graphic graphic;
            QVERIFY2(open(graphic, fixture(file), RS2::FormatDWG), file);
            for (const Expected& expected : {Expected{"PLAIN", false, false}, Expected{"FROZEN", true, false},
                                             Expected{"LOCKED", false, true}, Expected{"FROZENLOCKED", true, true}}) {
                RS_Layer* layer = graphic.findLayer(expected.m_name);
                QVERIFY2(layer != nullptr, expected.m_name);
                QVERIFY2(layer->isFrozen() == expected.m_frozen, expected.m_name);
                QVERIFY2(layer->isLocked() == expected.m_locked, expected.m_name);
            }
        }
    }

    void dwgOpensWithItsCurrentLayer()
    {
        for (const char* file : {"lc_current_layer_walls_ac1014.dwg", "lc_current_layer_walls_ac1024.dwg"}) {
            QVERIFY2(QFileInfo::exists(fixture(file)), file);
            RS_Graphic graphic;
            QVERIFY2(open(graphic, fixture(file), RS2::FormatDWG), file);
            QVERIFY2(graphic.getActiveLayer() != nullptr, file);
            // an R14 file has its names in capitals: "WALLS"
            QCOMPARE(graphic.getActiveLayer()->getName().toUpper(), QString("WALLS"));
        }
    }
};

QTEST_MAIN(LC_LayerStateFileIoTests)
#include "lc_layer_state_file_io_tests.moc"
