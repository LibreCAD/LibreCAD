/*
 * This file is part of LibreCAD, a 2D CAD program.
 * Copyright (C) 2026 LibreCAD.org
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef LC_PROPERTIESPROVIDER_OFFSETCURVE_H
#define LC_PROPERTIESPROVIDER_OFFSETCURVE_H

#include "lc_entity_type_propertiesprovider.h"

struct LC_OffsetCurvePropertySession;

class LC_PropertiesProviderOffsetCurve final : public LC_EntityTypePropertiesProvider {
public:
    LC_PropertiesProviderOffsetCurve(LC_ActionContext* context, LC_PropertySheetWidget* widget);
    static void createSourceActionLinks(LC_PropertyContainer* container, LC_PropertySheetWidget* widget,
        LC_ActionContext* context, const QList<RS_Entity*>& entities, bool single);
protected:
    void fillGenericAttributes(LC_PropertyContainer*, const QList<RS_Entity*>&) override;
    void doCreateEntitySpecificProperties(LC_PropertyContainer*, const QList<RS_Entity*>&) override;
    void fillComputedProperites(LC_PropertyContainer*, const QList<RS_Entity*>&) override;
    void fillSingleEntityCommands(LC_PropertyContainer*, const QList<RS_Entity*>&) override;
    void doCreateSelectedSetCommands(LC_PropertyContainer*, const QList<RS_Entity*>&) override;
    void fillSelectedSetCommands(LC_PropertyContainer*, const QList<RS_Entity*>&) override;
private:
    std::shared_ptr<LC_OffsetCurvePropertySession> m_session;
};
#endif
