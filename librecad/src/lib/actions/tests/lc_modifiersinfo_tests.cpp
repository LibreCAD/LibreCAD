/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2026 LibreCAD.org
** Copyright (C) 2026 Dongxu Li
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
******************************************************************************/

// The hints name the key that Qt's Ctrl modifier is on. On macOS that is the
// Command key, and a Control+click arrives as a right click, so they show the
// Command key's symbol.

#include <catch2/catch_test_macros.hpp>

#include <QStatusBar>

#include "lc_action_modify_move.h"
#include "lc_actiontestsupport.h"
#include "lc_cursoroverlayinfo.h"
#include "lc_modifiersinfo.h"
#include "lc_qtstatusbarmanager.h"

namespace {

#ifdef Q_OS_MACOS
const QString ctrlKey = QStringLiteral("\u2318");
#else
const QString ctrlKey = QStringLiteral("CTRL");
#endif

class MoveAction final : public LC_ActionModifyMove {
public:
    explicit MoveAction(LC_ActionContext* context) : LC_ActionModifyMove(context) {}
    QString infoCursorPrompt() const { return m_infoCursorOverlayData->getZone4(); }
};

// keeps what the action last asked for on the command line
class PromptContext final : public LC_ActionContext {
public:
    QString m_left;
    QString m_right;
    LC_ModifiersInfo m_modifiers;

    void updateActionPrompt(const QString& left, const QString& right, const LC_ModifiersInfo& modifiers) override {
        m_left = left;
        m_right = right;
        m_modifiers = modifiers;
    }
};

} // namespace

TEST_CASE("A Ctrl hint names the key the Ctrl modifier is on", "[actions][modifiers]") {
    CHECK(LC_ModifiersInfo::withNativeCtrlKey(" CTRL Key: ") == " " + ctrlKey + " Key: ");
    CHECK(LC_ModifiersInfo::withNativeCtrlKey(" SHIFT Key: ") == " SHIFT Key: ");
}

TEST_CASE("Action hints name the key the Ctrl modifier is on", "[actions][modifiers]") {
    REQUIRE(lc::test::application() != nullptr);
    RS_Graphic graphic;
    graphic.initForNewDocument();
    lc::test::TestGraphicView view;
    view.setDocument(&graphic);
    PromptContext context;
    context.setDocumentAndView(&graphic, &view);
    // the view's own preferences: the info cursor shows the prompt alone
    LC_InfoCursorOverlayPrefs* prefs = view.getInfoCursorOverlayPreferences();
    prefs->enabled = true;
    prefs->showCommandPrompt = true;
    prefs->showCurrentActionName = false;

    // nothing selected: Move asks for a selection, which Ctrl+click completes
    auto action = std::make_unique<MoveAction>(&context);
    action->init(0);

    CHECK(context.m_left == "Select to move (Enter or " + ctrlKey + "+LMB click to complete)");
    CHECK(action->infoCursorPrompt() == context.m_left + "\nSHIFT:Select contour | " + ctrlKey
                                            + ":Move immediately after selection");

    QStatusBar statusBar;
    statusBar.show();
    const auto* statusBarManager = new LC_QTStatusbarManager(&statusBar);
    statusBarManager->setActionHelp(context.m_left, context.m_right, context.m_modifiers);
    CHECK(statusBar.currentMessage() == " SHIFT Key: Select contour " + ctrlKey
                                            + " Key: Move immediately after selection");
}
