// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD contributors
#ifndef LC_CADWORKSPACE_H
#define LC_CADWORKSPACE_H

#include <QObject>
#include <QMap>
#include <QSize>
#include "rs_layerlistlistener.h"

class QC_ApplicationWindow;
class RS_GraphicView;
class RS_LayerList;
class QToolBar;
class QComboBox;
class QMenu;
class QAction;
class QWidget;
class QVBoxLayout;

// Arrangements only: drawing commands and pen widgets keep their original owners.
class LC_CadWorkspace : public QObject, public RS_LayerListListener {
public:
    explicit LC_CadWorkspace(QC_ApplicationWindow* window);
    void initialize();
    void persist();
    QString preset() const { return m_preset; }
    void activate(const QString& preset, bool reset = false);
    void prepareRestore(const QString& preset);
    void addMenuActions(QMenu* menu) const;
    void setGraphicView(RS_GraphicView* view);
    void layerListModified(bool) override;
    void layerListDestroyed() override;
private:
    QString captureLayout() const;
    bool restoreLayout(const QString& data);
    void applyDefault();
    void updateActions();
    QC_ApplicationWindow* m_window;
    QString m_preset{"classic-v1"};
    QString m_factoryClassic;
    QSize m_classicSnapIconSize;
    QString m_classicPenStyle;
    QMap<QString, QString> m_layouts;
    QToolBar* m_quickAccess;
    QToolBar* m_ribbon;
    QWidget* m_penHost;
    QVBoxLayout* m_penLayout;
    QComboBox* m_layers;
    RS_LayerList* m_layerList{nullptr};
    QAction* m_familiarAction;
    QAction* m_classicAction;
    QAction* m_resetAction;
};
#endif
