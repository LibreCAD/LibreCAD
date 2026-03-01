/*****************************************************************************/
/*  CircleTools - plugin for LibreCAD                                        */
/*                                                                           */
/*  Copyright (C) 2026 Ivo Dörr                                              */
/*  Contact: ivo.dorr@iqkonstrukt.cz                                         */
/*                                                                           */
/*  This program is free software; you can redistribute it and/or modify     */
/*  it under the terms of the GNU General Public License as published by     */
/*  the Free Software Foundation; either version 2 of the License, or        */
/*  (at your option) any later version.                                      */
/*                                                                           */
/*  This program is distributed in the hope that it will be useful,          */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of           */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/*  GNU General Public License for more details.                             */
/*                                                                           */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

#include <algorithm>
#include <cmath>

#include <QAbstractButton>
#include <QApplication>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointF>
#include <QPushButton>
#include <QSet>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QVBoxLayout>
#include <QtAlgorithms>
#include <QtGlobal>

#include "circletools.h"

QString CircleToolsPlugin::name() const {
    return QStringLiteral("CircleTools");
}

PluginCapabilities CircleToolsPlugin::getCapabilities() const {
    PluginCapabilities cap;

    // Use ASCII only to avoid encoding issues on Windows builds
    const QString action = QStringLiteral("CircleTools - Circles (diameter / layer)");
    cap.menuEntryPoints << PluginMenuLocation(QStringLiteral("Plugins"), action);
    cap.menuEntryPoints << PluginMenuLocation(QStringLiteral("plugins_menu"), action);
    //cap.menuEntryPoints << PluginMenuLocation(QStringLiteral("menu_Plugins"), action);

    return cap;
}

static void info(QWidget* parent, const QString& title, const QString& text) {
    QMessageBox::information(parent, title, text);
}

static void warn(QWidget* parent, const QString& title, const QString& text) {
    QMessageBox::warning(parent, title, text);
}

// --- logging (debug) ---
static QString circleToolsLogFilePath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (dir.isEmpty()) dir = QDir::homePath();
    return QDir(dir).filePath(QStringLiteral("CircleTools.log"));
}

static void logLine(const QString& line) {
    QFile f(circleToolsLogFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) return;

    QTextStream ts(&f);
    ts.setCodec("UTF-8");
    ts << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
       << " " << line << "\n";
}

static void logSep() {
    logLine(QStringLiteral("-----"));
}

// --- ESC blocker (workaround) ---
// Some LibreCAD builds can crash when QC_ActionGetEnt / QC_ActionGetPoint
// are canceled by ESC. We temporarily block ESC only while those actions run.
struct EscKeyBlocker : public QObject {
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (!ev) return QObject::eventFilter(obj, ev);
        if (ev->type() == QEvent::KeyPress || ev->type() == QEvent::KeyRelease) {
            auto* ke = static_cast<QKeyEvent*>(ev);
            if (ke && ke->key() == Qt::Key_Escape) {
                return true; // swallow ESC
            }
        }
        return QObject::eventFilter(obj, ev);
    }
};

struct EscBlockerGuard {
    EscKeyBlocker blocker;
    QObject* appObj = nullptr;

    EscBlockerGuard() {
        appObj = qApp;
        if (appObj) appObj->installEventFilter(&blocker);
    }
    ~EscBlockerGuard() {
        if (appObj) appObj->removeEventFilter(&blocker);
    }
};

// Main operation chooser dialog:
// - Buttons are stacked vertically
// - Numeric input is available (focused by default)
// - ASCII-only labels (avoids garbled diacritics on some Windows builds)
static bool promptOperationDialog(QWidget* parent, int& outOp) {
    outOp = 0; // safety default
    QDialog dlg(parent);
    dlg.setWindowTitle(QStringLiteral("CircleTools"));

    QVBoxLayout* lay = new QVBoxLayout(&dlg);

    QLabel* label = new QLabel(
        QStringLiteral("Choose operation (click a button OR type 1..3 and press Enter):"),
        &dlg);
    label->setWordWrap(true);
    lay->addWidget(label);

    auto addOpButton = [&](int op, const QString& text) {
        QPushButton* b = new QPushButton(text, &dlg);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        // Left aligned labels (better readability)
        b->setStyleSheet(QStringLiteral("text-align:left; padding:6px;"));

        lay->addWidget(b);

        // IMPORTANT: capture 'op' BY VALUE, not by reference (otherwise it dangles after addOpButton returns)
        QObject::connect(b, &QPushButton::clicked, &dlg, [&, op]() {
            outOp = op;
            dlg.accept();
        });
    };

    addOpButton(1, QStringLiteral("1. Find by diameter -> MOVE to layer"));
    addOpButton(2, QStringLiteral("2. Change DIAMETER (select circles)"));
    addOpButton(3, QStringLiteral("3. Find by diameter -> change DIAMETER"));

    QLineEdit* edit = new QLineEdit(&dlg);
    edit->setPlaceholderText(QStringLiteral("Type 1..3 and press Enter"));
    edit->setMaxLength(1);
    edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    lay->addWidget(edit);

    QPushButton* cancel = new QPushButton(QStringLiteral("Cancel"), &dlg);
    cancel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    lay->addWidget(cancel);

    QObject::connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);

    QObject::connect(edit, &QLineEdit::returnPressed, &dlg, [&]() {
        bool ok = false;
        const int v = edit->text().toInt(&ok);
        if (ok && v >= 1 && v <= 3) {
            outOp = v;
            dlg.accept();
        } else {
            edit->selectAll();
        }
    });

    edit->setFocus();
    edit->selectAll();

    if (dlg.exec() != QDialog::Accepted) return false;
    return (outOp >= 1 && outOp <= 3);
}

// --- Layer combobox helpers ---


static QStringList collectLayerNamesFromDrawing(Document_Interface* doc) {
    if (!doc) return {};

    QStringList layers = doc->getAllLayer();
    layers.removeAll(QString());
    layers.sort(Qt::CaseInsensitive);

    const QString current = doc->getCurrentLayer().trimmed();
    if (!current.isEmpty()) {
        layers.removeAll(current);
        layers.prepend(current);
    }

    const QString zero = QStringLiteral("0");
    if (layers.contains(zero) && (layers.isEmpty() || layers.first() != zero)) {
        layers.removeAll(zero);
        const int insertPos = (!current.isEmpty() && !layers.isEmpty() && layers.first() == current) ? 1 : 0;
        layers.insert(insertPos, zero);
    }

    return layers;
}


static bool promptLayerNameCombo(Document_Interface* doc,
                                 QWidget* parent,
                                 const QString& title,
                                 const QString& label,
                                 QString& outName,
                                 bool editable) {
    outName.clear();

    const QStringList layers = collectLayerNamesFromDrawing(doc);

    bool ok = false;
    QString value;

    if (!layers.isEmpty()) {
        int idx = 0;
        const QString current = doc ? doc->getCurrentLayer().trimmed() : QString();
        const int i = (!current.isEmpty()) ? layers.indexOf(current) : -1;
        if (i >= 0) idx = i;

        value = QInputDialog::getItem(parent, title, label, layers, idx, editable, &ok);
        if (!ok) return false;
    } else {
        // Fallback: no layers collected -> plain string input
        if (!doc->getString(&value, label, title)) return false;
    }

    value = value.trimmed();
    if (value.isEmpty()) {
        warn(parent, title, QStringLiteral("Layer name is empty."));
        return false;
    }

    outName = value;
    return true;
}

static bool promptTargetLayerCombo(Document_Interface* doc,
                                   QWidget* parent,
                                   const QString& title,
                                   QString& outLayer) {
    return promptLayerNameCombo(
        doc,
        parent,
        title,
        QStringLiteral("Target layer (choose existing or type a new one):"),
        outLayer,
        true // editable
    );
}

namespace {


// NOTE about selection:
// LibreCAD plugin API does NOT provide "get current preselection list" directly.
// We therefore ask the user to finish selection inside the plugin selection prompt.
// Some builds treat Enter as "cancel", so we guide users to finish with RIGHT CLICK.

static bool changeCircleLayerInPlace(Plug_Entity* ent, const QString& targetLayer, bool& outAlreadyOnLayer) {
    outAlreadyOnLayer = false;
    if (!ent) return false;

    QHash<int, QVariant> data;
    ent->getData(&data);

    if (data.value(DPI::ETYPE).toInt() != static_cast<int>(DPI::CIRCLE)) return false;

    const QString curLayer = data.value(DPI::LAYER).toString();
    if (curLayer == targetLayer) {
        outAlreadyOnLayer = true;
        return true;
    }

    // Keep EID: updateData uses it to MODIFY the existing entity.
    // Removing EID can cause LibreCAD to CREATE a new entity instead (duplicates + counter drift).
    data.insert(DPI::LAYER, targetLayer);
    ent->updateData(&data);
    return true;
}
static QString circleGeomKey(const QHash<int, QVariant>& data) {
    // Robust de-dup key when EID is missing/unstable.
    // Quantize to 1e-6 drawing units (good enough for mm drawings and avoids floating noise).
    const double cx = data.value(DPI::STARTX).toDouble();
    const double cy = data.value(DPI::STARTY).toDouble();
    const double r  = data.value(DPI::RADIUS).toDouble();

    const qint64 ix = qRound64(cx * 1000000.0);
    const qint64 iy = qRound64(cy * 1000000.0);
    const qint64 ir = qRound64(r  * 1000000.0);

    return QStringLiteral("%1|%2|%3").arg(ix).arg(iy).arg(ir);
}
struct CircleStats {
    int entitiesTotal = 0;
    int circlesTotal = 0;

    int circlesVisible = 0;
    int circlesInvisible = 0;

    int uniqueEid = 0;

    int uniqueGeom = 0;
    int dupGeom = 0;
};

static CircleStats scanCircleStats(Document_Interface* doc) {
    CircleStats s;
    if (!doc) return s;

    QList<Plug_Entity*> all;
    if (!doc->getAllEntities(&all, false)) return s;

    s.entitiesTotal = all.size();

    QSet<QString> geoms;
    QSet<qulonglong> eids;

    for (Plug_Entity* e : all) {
        if (!e) continue;
        QHash<int, QVariant> data;
        e->getData(&data);

        if (data.value(DPI::ETYPE).toInt() != static_cast<int>(DPI::CIRCLE)) continue;

        s.circlesTotal++;

        // VISIBLE: plugin API uses DPI::VISIBLE (note: doc says it's reversed vs DXF spec).
        // We'll interpret 1 = visible, 0 = invisible. Default to visible if missing.
        const int vis = data.contains(DPI::VISIBLE) ? data.value(DPI::VISIBLE).toInt() : 1;
        if (vis == 1) s.circlesVisible++;
        else s.circlesInvisible++;

        const qulonglong eid = static_cast<qulonglong>(data.value(DPI::EID).toULongLong());
        if (eid != 0) eids.insert(eid);

        const QString k = circleGeomKey(data);
        if (geoms.contains(k)) s.dupGeom++;
        else geoms.insert(k);
    }

    s.uniqueGeom = geoms.size();
    s.uniqueEid = eids.size();

    qDeleteAll(all);
    return s;
}

static QString statsText(const CircleStats& s) {
    return QStringLiteral("entities=%1 circles=%2 visible=%3 invisible=%4 uniqueEID=%5 uniqueGeom=%6 dupGeom=%7")
        .arg(s.entitiesTotal)
        .arg(s.circlesTotal)
        .arg(s.circlesVisible)
        .arg(s.circlesInvisible)
        .arg(s.uniqueEid)
        .arg(s.uniqueGeom)
        .arg(s.dupGeom);
}

static int countMatchedOnTargetLayer(Document_Interface* doc,
                                    const QSet<QString>& matchedGeomKeys,
                                    const QString& targetLayer) {
    if (!doc || matchedGeomKeys.isEmpty()) return 0;

    QList<Plug_Entity*> all;
    if (!doc->getAllEntities(&all, false)) return 0;

    int count = 0;
    const QString tgt = targetLayer.trimmed();

    for (Plug_Entity* e : all) {
        if (!e) continue;

        QHash<int, QVariant> data;
        e->getData(&data);

        if (data.value(DPI::ETYPE).toInt() != static_cast<int>(DPI::CIRCLE)) continue;

        // ignore invisible entities (undo/redo history)
        const int vis = data.contains(DPI::VISIBLE) ? data.value(DPI::VISIBLE).toInt() : 1;
        if (vis != 1) continue;

        const QString gk = circleGeomKey(data);
        if (!matchedGeomKeys.contains(gk)) continue;

        const QString lay = data.value(DPI::LAYER).toString().trimmed();
        if (lay == tgt) count++;
    }

    qDeleteAll(all);
    return count;
}


static bool windowSelectCirclesBy2Points(Document_Interface* doc, QWidget* parent, QList<Plug_Entity*>& outSel) {
    outSel.clear();
    if (!doc) return false;

    QPointF p1;
    {
        EscBlockerGuard escGuard; // prevents ESC-crash during QC_ActionGetPoint
        if (!doc->getPoint(&p1, QStringLiteral("Window corner 1 (finish by clicking point)"))) return false;
    }

    QPointF p2;
    {
        EscBlockerGuard escGuard; // prevents ESC-crash during QC_ActionGetPoint
        if (!doc->getPoint(&p2, QStringLiteral("Window corner 2 (finish by clicking point)"), &p1)) return false;
    }

    const double xmin = std::min(p1.x(), p2.x());
    const double xmax = std::max(p1.x(), p2.x());
    const double ymin = std::min(p1.y(), p2.y());
    const double ymax = std::max(p1.y(), p2.y());

    QList<Plug_Entity*> all;
    if (!doc->getAllEntities(&all, false)) return false;

    QList<Plug_Entity*> kept;
    kept.reserve(all.size());

    QSet<qulonglong> seenIds;
    QSet<QString> seenGeom;

    for (Plug_Entity* e : all) {
        if (!e) continue;

        QHash<int, QVariant> data;
        e->getData(&data);

        if (data.value(DPI::ETYPE).toInt() != static_cast<int>(DPI::CIRCLE)) {
            delete e;
            continue;
        }

        // IMPORTANT: ignore invisible entities (undo/redo history, not shown in the drawing)
        const int vis = data.contains(DPI::VISIBLE) ? data.value(DPI::VISIBLE).toInt() : 1;
        if (vis != 1) { delete e; continue; }

        // Circle center is stored in STARTX/STARTY
        const double cx = data.value(DPI::STARTX).toDouble();
        const double cy = data.value(DPI::STARTY).toDouble();

        const bool inside = (cx >= xmin && cx <= xmax && cy >= ymin && cy <= ymax);
        if (!inside) {
            delete e;
            continue;
        }

        // Dedupe: prevent duplicate wrappers / geometry duplicates from being returned as "selected"
        const qulonglong eid = static_cast<qulonglong>(data.value(DPI::EID).toULongLong());
        const QString gk = circleGeomKey(data);

        if (seenGeom.contains(gk) || (eid != 0 && seenIds.contains(eid))) {
            delete e;
            continue;
        }
        seenGeom.insert(gk);
        if (eid != 0) seenIds.insert(eid);

        kept.push_back(e);
    }

    outSel = kept;

    if (outSel.isEmpty()) {
        QMessageBox::information(parent,
                                 QStringLiteral("CircleTools"),
                                 QStringLiteral("No circles found inside the window."));
        return false;
    }

    return true;
}
static bool pickCirclesByClick(Document_Interface* doc, QWidget* parent, QList<Plug_Entity*>& outSel, const QString& purpose) {
    outSel.clear();
    if (!doc) return false;

    const QString title = QStringLiteral("CircleTools");

    QSet<qulonglong> seenIds;
    QSet<QString> seenGeom;

    for (;;) {
        // IMPORTANT: QC_ActionGetEnt completes on LEFT click.
        // Right-click behavior is inconsistent across builds (can crash in some setups),
        // therefore we finish picking with ESC / cancel, or with the dialog below.
        Plug_Entity* ent = doc->getEnt(
            QStringLiteral("Click a CIRCLE to %1.\nPress ESC to finish picking.")
                .arg(purpose)
        );

        if (!ent) break; // ESC / cancel => finish

        QHash<int, QVariant> data;
        ent->getData(&data);

        if (data.value(DPI::ETYPE).toInt() != static_cast<int>(DPI::CIRCLE)) {
            delete ent;
            continue;
        }

        const qulonglong eid = static_cast<qulonglong>(data.value(DPI::EID).toULongLong());
        if (eid != 0) {
            if (seenIds.contains(eid)) {
                delete ent;
                continue;
            }
            seenIds.insert(eid);
        } else {
            const QString k = circleGeomKey(data);
            if (seenGeom.contains(k)) {
                delete ent;
                continue;
            }
            seenGeom.insert(k);
        }

        outSel.append(ent);

        const auto ret = QMessageBox::question(
            parent,
            title,
            QStringLiteral("Add another circle?\n\nYes = pick more\nNo = finish"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes
        );
        if (ret != QMessageBox::Yes) break;
    }

    if (outSel.isEmpty()) {
        QMessageBox::information(parent,
                                 title,
                                 QStringLiteral("No circles selected."));
        return false;
    }

    return true;
}



static bool collectCirclesForBatchOps(Document_Interface* doc,
                                      QWidget* parent,
                                      QList<Plug_Entity*>& outSel,
                                      const QString& purpose) {
    outSel.clear();
    if (!doc) return false;

    QMessageBox box(parent);
    box.setWindowTitle(QStringLiteral("CircleTools"));
    box.setText(QStringLiteral("Select circles to %1:\n\n"
                               "NOTE: In your LibreCAD build, plugin multi-select is unreliable.\n"
                               "Use one of these methods:")
                    .arg(purpose));
    box.setTextFormat(Qt::PlainText);

    QPushButton* bWin = box.addButton(QStringLiteral("Window (2 points)"), QMessageBox::ActionRole);
    QPushButton* bPick = box.addButton(QStringLiteral("Pick one-by-one"), QMessageBox::ActionRole);
    box.addButton(QMessageBox::Cancel);

    box.exec();

    if (box.clickedButton() == bWin) {
        return windowSelectCirclesBy2Points(doc, parent, outSel);
    }
    if (box.clickedButton() == bPick) {
        return pickCirclesByClick(doc, parent, outSel, purpose);
    }
    return false;
}

} // namespace



bool CircleToolsPlugin::isCircle(const QHash<int, QVariant>& data) {
    return data.value(DPI::ETYPE).toInt() == static_cast<int>(DPI::CIRCLE);
}

double CircleToolsPlugin::circleDiameter(const QHash<int, QVariant>& data) {
    const double r = data.value(DPI::RADIUS).toDouble();
    return 2.0 * r;
}

void CircleToolsPlugin::ensureLayerExists(Document_Interface* doc, const QString& layerName) {
    if (!doc || layerName.trimmed().isEmpty()) return;
    const QString prev = doc->getCurrentLayer();
    doc->setLayer(layerName);   // creates if missing (per interface contract)
    doc->setLayer(prev);
}

bool CircleToolsPlugin::promptReferenceCircle(Document_Interface* doc, QWidget* parent, RefCircle& outRef) {
    if (!doc) return false;

    const QString title = QStringLiteral("CircleTools");

    // Give user a cancel-safe option (no CAD action needed).
    QMessageBox box(parent);
    box.setWindowTitle(title);
    box.setText(QStringLiteral("Provide reference diameter:"));
    QPushButton* bPick  = box.addButton(QStringLiteral("Pick from drawing"), QMessageBox::ActionRole);
    QPushButton* bEnter = box.addButton(QStringLiteral("Enter diameter"),   QMessageBox::ActionRole);
    box.addButton(QMessageBox::Cancel);
    box.exec();

    if (box.clickedButton() == bEnter) {
        qreal dia = 0.0;
        if (!doc->getReal(&dia, QStringLiteral("Reference diameter (drawing units):"), title)) return false;
        if (dia <= 0.0) {
            warn(parent, title, QStringLiteral("Diameter must be > 0."));
            return false;
        }
        outRef.diameter = static_cast<double>(dia);
        outRef.layer = doc->getCurrentLayer().trimmed();
        return true;
    }

    if (box.clickedButton() != bPick) return false;

    Plug_Entity* ent = nullptr;
    {
        EscBlockerGuard escGuard; // prevents ESC-crash during QC_ActionGetEnt
        ent = doc->getEnt(QStringLiteral("Pick a reference CIRCLE (left click)."));
    }
    if (!ent) return false;

    QHash<int, QVariant> data;
    ent->getData(&data);
    delete ent;

    if (!isCircle(data)) {
        warn(parent, title, QStringLiteral("Selected entity is not a CIRCLE."));
        return false;
    }

    outRef.diameter = circleDiameter(data);
    outRef.layer = data.value(DPI::LAYER).toString().trimmed();
    return outRef.diameter > 0.0;
}

bool CircleToolsPlugin::promptLayerFilter(Document_Interface* doc, QWidget* parent, int& outMode, QString& outLayerName) {
    int m = 0;
    const QString title = QStringLiteral("CircleTools");
    const QString msg =
        QStringLiteral("Filter by layer?\n"
                       "0 = no (all layers)\n"
                       "1 = yes (only the reference circle layer)\n"
                       "2 = yes (choose / enter a layer name)");
    if (!doc->getInt(&m, msg, title)) return false;
    if (m < 0 || m > 2) m = 0;

    outMode = m;
    outLayerName.clear();

    if (m == 2) {
        QString s;
        if (!promptLayerNameCombo(doc, parent, title, QStringLiteral("Layer name:"), s, true)) return false;
        outLayerName = s;
    }
    return true;
}


bool CircleToolsPlugin::promptTolerance(Document_Interface* doc, QWidget* parent, double& outTol) {
    Q_UNUSED(doc);

    // Recommended default tolerance: 0.001 mm
    bool ok = false;
    const double tol = QInputDialog::getDouble(
        parent,
        QStringLiteral("CircleTools"),
        QStringLiteral("Diameter tolerance [mm]:"),
        0.001,   // default
        0.0,     // min
        1e9,     // max
        6,       // decimals
        &ok
    );
    if (!ok) return false;

    outTol = tol;
    return true;
}


void CircleToolsPlugin::opFindMoveToLayer(Document_Interface* doc, QWidget* parent) {
    const QString title = QStringLiteral("CircleTools");

    RefCircle ref;
    if (!promptReferenceCircle(doc, parent, ref)) return;

    double tol = 0.0;
    if (!promptTolerance(doc, parent, tol)) return;

    int layerMode = 0;
    QString layerName;
    if (!promptLayerFilter(doc, parent, layerMode, layerName)) return;

    QString targetLayer;
    if (!promptTargetLayerCombo(doc, parent, title, targetLayer)) return;

    ensureLayerExists(doc, targetLayer);

    const CircleStats statsBefore = scanCircleStats(doc);
    logSep();
    logLine(QStringLiteral("opFindMoveToLayer START refDia=%1 tol=%2 layerMode=%3 layerName='%4' target='%5'")
            .arg(ref.diameter).arg(tol).arg(layerMode).arg(layerName).arg(targetLayer));
    logLine(QStringLiteral("BEFORE %1").arg(statsText(statsBefore)));

    QList<Plug_Entity*> all;
    if (!doc->getAllEntities(&all, false)) return;

    int matched = 0;
    int checked = 0;

    QSet<qulonglong> seenIds;
    QSet<QString> seenGeom;
    QSet<QString> matchedGeomKeys;
    QList<Plug_Entity*> toMove;
    toMove.reserve(all.size());

    // Pass 1: scan + count using a stable de-dup (prevents "imaginary" extra circles)
    for (Plug_Entity* e : all) {
        if (!e) continue;
        QHash<int, QVariant> data;
        e->getData(&data);

        if (!isCircle(data)) continue;

        // IMPORTANT: ignore invisible entities (undo/redo history, not shown in the drawing)
        const int vis = data.contains(DPI::VISIBLE) ? data.value(DPI::VISIBLE).toInt() : 1;
        if (vis != 1) continue;

        const QString lay = data.value(DPI::LAYER).toString();
        if (layerMode == 1 && lay != ref.layer) continue;
        if (layerMode == 2 && lay != layerName) continue;

        // Dedupe ALWAYS also by geometry:
        // fixes cases where the same real circle appears as multiple wrappers (even with different EID)
        const QString gk = circleGeomKey(data);
        if (seenGeom.contains(gk)) continue;
        seenGeom.insert(gk);

        const qulonglong eid = static_cast<qulonglong>(data.value(DPI::EID).toULongLong());
        if (eid != 0) {
            if (seenIds.contains(eid)) continue;
            seenIds.insert(eid);
        }

        checked++;

        const double d = circleDiameter(data);
        if (std::fabs(d - ref.diameter) <= tol) {
            matched++;
            matchedGeomKeys.insert(gk);
            toMove.append(e);
        }
    }

    // Pass 2: apply modifications AFTER scanning.
    // NOTE: DPI::LAYER reported by wrappers can be stale -> do NOT report "already" here.
    for (Plug_Entity* e : toMove) {
        if (!e) continue;

        QHash<int, QVariant> d;
        e->getData(&d);

        const qulonglong eid = static_cast<qulonglong>(d.value(DPI::EID).toULongLong());
        const QString layerBefore = d.value(DPI::LAYER).toString().trimmed();

        d.insert(DPI::LAYER, targetLayer);
        e->updateData(&d);

        QHash<int, QVariant> d2;
        e->getData(&d2);
        const QString layerAfter = d2.value(DPI::LAYER).toString().trimmed();

        logLine(QStringLiteral("MOVE eid=%1 '%2' -> '%3'").arg(eid).arg(layerBefore).arg(layerAfter));
    }


    qDeleteAll(all);
    doc->updateView();

    const int matchedUnique = matchedGeomKeys.size();
    const int onTargetAfter = countMatchedOnTargetLayer(doc, matchedGeomKeys, targetLayer);
    const int failedToMove  = std::max(0, matchedUnique - onTargetAfter);

    const CircleStats statsAfter = scanCircleStats(doc);
    logLine(QStringLiteral("RESULT checked=%1 matched=%2 onTargetAfter=%3 failed=%4")
            .arg(checked).arg(matchedUnique).arg(onTargetAfter).arg(failedToMove));
    logLine(QStringLiteral("AFTER  %1").arg(statsText(statsAfter)));

    info(parent, title,
         QStringLiteral("Done (this run).\n"
                        "Reference diameter: %1\n"
                        "Tolerance: %2\n"
                        "Checked circles: %3\n"
                        "Matched circles: %4\n"
                        "On target layer after: %5\n"
                        "Failed to move: %6\n"
                        "Target layer: %7\n\n"
                        "Circles in drawing: before %8, after %9\n"
                        "Geometry-duplicates after: %10\n"
                        "Log: %11")
             .arg(ref.diameter)
             .arg(tol)
             .arg(checked)
             .arg(matchedUnique)
             .arg(onTargetAfter)
             .arg(failedToMove)
             .arg(targetLayer)
             .arg(statsBefore.circlesTotal)
             .arg(statsAfter.circlesTotal)
             .arg(statsAfter.dupGeom)
             .arg(circleToolsLogFilePath()));
}


void CircleToolsPlugin::opResizeSelected(Document_Interface* doc, QWidget* parent) {
    const QString title = QStringLiteral("CircleTools");

    const CircleStats statsBefore = scanCircleStats(doc);
    logSep();
    logLine(QStringLiteral("opResizeSelected START"));
    logLine(QStringLiteral("BEFORE %1").arg(statsText(statsBefore)));

	QList<Plug_Entity*> sel;
	if (!collectCirclesForBatchOps(doc, parent, sel, QStringLiteral("change DIAMETER"))) return;

    qreal newDia = 0.0;
    if (!doc->getReal(&newDia, QStringLiteral("New diameter (drawing units):"), title)) {
        qDeleteAll(sel);
        return;
    }
    if (newDia <= 0.0) {
        qDeleteAll(sel);
        warn(parent, title, QStringLiteral("Diameter must be > 0."));
        return;
    }
    const double newR = static_cast<double>(newDia) / 2.0;

    int changed = 0;
    int unchanged = 0;
    int ignored = 0;
    int skippedInvisible = 0;
    int skippedDuplicate = 0;

    QSet<qulonglong> seenIds;
    QSet<QString> seenGeom;

    for (Plug_Entity* e : sel) {
        if (!e) continue;
        QHash<int, QVariant> data;
        e->getData(&data);

        if (!isCircle(data)) { ignored++; continue; }

        // IMPORTANT: ignore invisible entities (undo/redo history, not shown in the drawing)
        const int vis = data.contains(DPI::VISIBLE) ? data.value(DPI::VISIBLE).toInt() : 1;
        if (vis != 1) { skippedInvisible++; continue; }

        // Dedupe (robust): avoid processing the same circle multiple times via duplicate wrappers
        const qulonglong eid = static_cast<qulonglong>(data.value(DPI::EID).toULongLong());
        const QString gk = circleGeomKey(data);
        if (seenGeom.contains(gk) || (eid != 0 && seenIds.contains(eid))) {
            skippedDuplicate++;
            continue;
        }
        seenGeom.insert(gk);
        if (eid != 0) seenIds.insert(eid);

        const double oldR = data.value(DPI::RADIUS).toDouble();
        if (oldR <= 0.0) { ignored++; continue; }

        if (std::fabs(newR - oldR) <= 1e-12) {
            unchanged++;
            continue;
        }

        // Modify in place: update radius (keep EID so LibreCAD updates the existing entity).
        data.insert(DPI::RADIUS, newR);
        e->updateData(&data);
        changed++;
    }

    qDeleteAll(sel);
    doc->updateView();
	
	const CircleStats statsAfter = scanCircleStats(doc);
    logLine(QStringLiteral("RESULT changed=%1 unchanged=%2 ignored=%3 skippedInvisible=%4 skippedDuplicate=%5")
            .arg(changed).arg(unchanged).arg(ignored).arg(skippedInvisible).arg(skippedDuplicate));
    logLine(QStringLiteral("AFTER  %1").arg(statsText(statsAfter)));

    info(parent, title,
         QStringLiteral("Done (this run).\n"
                        "Circles changed: %1\n"
                        "Circles unchanged: %2\n"
                        "Ignored (not circles / invalid): %3\n"
                        "Skipped invisible (undo/history): %4\n"
                        "Skipped duplicates (wrappers): %5\n\n"
                        "Circles in drawing: before %6, after %7\n"
                        "Geometry-duplicates after: %8\n"
                        "Log: %9")
             .arg(changed)
             .arg(unchanged)
             .arg(ignored)
             .arg(skippedInvisible)
             .arg(skippedDuplicate)
             .arg(statsBefore.circlesTotal)
             .arg(statsAfter.circlesTotal)
             .arg(statsAfter.dupGeom)
             .arg(circleToolsLogFilePath()));

}

void CircleToolsPlugin::opFindResize(Document_Interface* doc, QWidget* parent) {
    const QString title = QStringLiteral("CircleTools");
	
	const CircleStats statsBefore = scanCircleStats(doc);
    logSep();
    logLine(QStringLiteral("opFindResize START"));
    logLine(QStringLiteral("BEFORE %1").arg(statsText(statsBefore)));

    RefCircle ref;
    if (!promptReferenceCircle(doc, parent, ref)) return;

    double tol = 0.0;
    if (!promptTolerance(doc, parent, tol)) return;

    int layerMode = 0;
    QString layerName;
    if (!promptLayerFilter(doc, parent, layerMode, layerName)) return;

    qreal newDia = 0.0;
    if (!doc->getReal(&newDia, QStringLiteral("New diameter for FOUND circles (drawing units):"), title)) return;
    if (newDia <= 0.0) {
        warn(parent, title, QStringLiteral("Diameter must be > 0."));
        return;
    }
    const double newR = static_cast<double>(newDia) / 2.0;

    QList<Plug_Entity*> all;
    if (!doc->getAllEntities(&all, false)) return;

    int changed = 0;
    int checked = 0;

    QSet<qulonglong> seenIds;
    QSet<QString> seenGeom;

    for (Plug_Entity* e : all) {
        if (!e) continue;
        QHash<int, QVariant> data;
        e->getData(&data);

        if (!isCircle(data)) continue;

        // IMPORTANT: ignore invisible entities (undo/redo history, not shown in the drawing)
        const int vis = data.contains(DPI::VISIBLE) ? data.value(DPI::VISIBLE).toInt() : 1;
        if (vis != 1) continue;

        const QString lay = data.value(DPI::LAYER).toString();
        if (layerMode == 1 && lay != ref.layer) continue;
        if (layerMode == 2 && lay != layerName) continue;

        // Dedupe: avoid processing the same circle multiple times via wrappers/history
        const QString gk = circleGeomKey(data);
        if (seenGeom.contains(gk)) continue;
        seenGeom.insert(gk);

        const qulonglong eid = static_cast<qulonglong>(data.value(DPI::EID).toULongLong());
        if (eid != 0) {
            if (seenIds.contains(eid)) continue;
            seenIds.insert(eid);
        }

        const double d = circleDiameter(data);
        checked++;

        if (std::fabs(d - ref.diameter) <= tol) {
            const double oldR = data.value(DPI::RADIUS).toDouble();
            if (oldR > 0.0) {
                if (std::fabs(newR - oldR) > 1e-12) {
                    // Modify in place: update radius (keep EID).
                    data.insert(DPI::RADIUS, newR);
                    e->updateData(&data);
                    changed++;
                }
            }
        }
    }

    qDeleteAll(all);
    doc->updateView();
	
	const CircleStats statsAfter = scanCircleStats(doc);
    logLine(QStringLiteral("RESULT checked=%1 changed=%2").arg(checked).arg(changed));
    logLine(QStringLiteral("AFTER  %1").arg(statsText(statsAfter)));

    info(parent, title,
         QStringLiteral("Done (this run).\n"
                        "Reference diameter: %1\n"
                        "Tolerance: %2\n"
                        "Checked circles: %3\n"
                        "Circles changed: %4\n"
                        "New diameter: %5\n\n"
                        "Circles in drawing: before %6, after %7\n"
                        "Geometry-duplicates after: %8\n"
                        "Log: %9")
             .arg(ref.diameter)
             .arg(tol)
             .arg(checked)
             .arg(changed)
             .arg(newDia)
             .arg(statsBefore.circlesTotal)
             .arg(statsAfter.circlesTotal)
             .arg(statsAfter.dupGeom)
             .arg(circleToolsLogFilePath()));
}

void CircleToolsPlugin::execComm(Document_Interface* doc, QWidget* parent, QString cmd) {
    Q_UNUSED(cmd);
    if (!doc) return;

    int op = 1;
    if (!promptOperationDialog(parent, op)) {
        return; // Cancel
    }

    switch (op) {
        case 1: opFindMoveToLayer(doc, parent); break;
        case 2: opResizeSelected(doc, parent); break;
        case 3: opFindResize(doc, parent); break;
        default:
            warn(parent, QStringLiteral("CircleTools"), QStringLiteral("Invalid choice."));
            break;
    }
}


