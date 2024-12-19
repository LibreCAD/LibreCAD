%module(directors="1") librecad;

%rename(__aref__)             *::operator[];
%rename(__lshift__)           *::operator<<;
%rename(__rshift__)           *::operator>>;
%{
//#include "qtconfigmacros.h"
//#include "qcolormap.h"
//#include "qcolor.h"
//#include <QPolygonF>
//#include "lc_crosshair.h"

#include <array>

#include "rs_pythongui.h"
#include "rs_pythondcl.h"

#include "rs.h"
#include "rs_flags.h"
#include "rs_undoable.h"
#include "rs_undocycle.h"
#include "rs_undo.h"
#include "rs_units.h"
#include "rs_vector.h"
#include "lc_defaults.h"
#include "lc_convert.h"
#include "lc_linemath.h"
#include "lc_quadratic.h"
#include "rs_math.h"
#include "rs_entity.h"
#include "rs_atomicentity.h"
#include "lc_rect.h"
#include "rs_entitycontainer.h"
#include "lc_cachedlengthentity.h"
#include "rs_ellipse.h"
#include "rs_hatch.h"
#include "rs_line.h"
#include "rs_constructionline.h"
#include "rs_polyline.h"
#include "rs_solid.h"
#include "rs_spline.h"
#include "rs_text.h"
#include "rs_mtext.h"
#include "rs_dimension.h"
#include "lc_dimarc.h"
#include "rs_dimaligned.h"
#include "rs_dimangular.h"
#include "rs_dimdiametric.h"
#include "rs_dimlinear.h"
#include "rs_dimradial.h"
#include "rs_circle.h"
#include "rs_arc.h"
#include "lc_hyperbola.h"
//#include "lc_splinepoints.h"             // Qt problem child X-(
//#include "lc_parabola.h"                 // Qt problem child X-(
#include "rs_image.h"
#include "rs_insert.h"
#include "rs_leader.h"
#include "rs_point.h"
#include "dxf_format.h"
#include "rs_document.h"
#include "lc_undosection.h"
#include "rs_graphic.h"
#include "rs_color.h"
#include "rs_pen.h"
#include "lc_looputils.h"
#include "rs_font.h"
#include "rs_fontchar.h"
#include "rs_fontlist.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_layerlistlistener.h"
#include "rs_pattern.h"
//#include "rs_patternlist.cpp"            // problem child X-(
//#include "rs_settings.h"                 // Qt problem child X-(
#include "rs_variable.h"
#include "rs_variabledict.h"
#include "lc_view.h"
#include "lc_viewslist.h"
#include "rs_clipboard.h"
#include "rs_modification.h"
#include "rs_selection.h"
#include "rs_block.h"
#include "rs_blocklist.h"
#include "rs_blocklistlistener.h"
#include "rs_locale.h"
#include "rs_infoarea.h"
#include "rs_information.h"
#include "rs_system.h"
#include "lc_highlight.h"
#include "rs_preview.h"
#include "lc_refarc.h"
#include "lc_refcircle.h"
#include "lc_refellipse.h"
#include "lc_refline.h"
#include "lc_refpoint.h"
#include "lc_crosshair.h"
#include "rs_overlaybox.h"
#include "rs_overlayline.h"
//#include "lc_rtree.h"                    // problem child X-(
//#include "rs_utility.h"                  // problem child X-(

%}
%feature("director") RS_Undoable;
%feature("director") RS_Entity;
%feature("director") RS_AtomicEntity;
%feature("director") LC_CachedLengthEntity;
%feature("director") RS_Line;

%include "std_array.i"

%include "lib/scripting/rs_pythondcl.h"
%include "lib/scripting/rs_pythongui.h"

%template(IntegerArray2) std::array<int, 2>;
%ignore operator<<;
%ignore RS_Undo::test();
%ignore LoopSorter;
%include "lib/engine/rs.h"
%include "lib/engine/rs_flags.h"
%include "lib/engine/undo/rs_undoable.h"
%include "lib/engine/undo/rs_undocycle.h"
%include "lib/engine/undo/rs_undo.h"
%include "lib/engine/rs_units.h"
%include "lib/math/lc_convert.h"
%include "lib/math/lc_linemath.h"
%include "lib/math/lc_quadratic.h"
%include "lib/math/rs_math.h"
%include "lib/engine/rs_vector.h"
%include "lib/engine/lc_defaults.h"
%include "lib/engine/document/entities/rs_entity.h"
%include "lib/engine/document/entities/rs_atomicentity.h"
%include "lib/engine/document/entities/lc_rect.h"
%include "lib/engine/document/container/rs_entitycontainer.h"
%include "lib/engine/document/dxf_format.h"
%include "lib/engine/document/rs_document.h"
%include "lib/engine/undo/lc_undosection.h"
%include "lib/engine/document/rs_graphic.h"
%include "lib/engine/document/entities/lc_cachedlengthentity.h"
%include "lib/engine/document/entities/rs_ellipse.h"
%include "lib/engine/document/entities/rs_hatch.h"
%include "lib/engine/document/entities/rs_line.h"
%include "lib/engine/document/entities/rs_constructionline.h"
%include "lib/engine/document/entities/rs_polyline.h"
%include "lib/engine/document/entities/rs_solid.h"
%include "lib/engine/document/entities/rs_spline.h"
%include "lib/engine/document/entities/rs_text.h"
%include "lib/engine/document/entities/rs_mtext.h"
%include "lib/engine/document/entities/rs_dimension.h"
%include "lib/engine/document/entities/lc_dimarc.h"
%include "lib/engine/document/entities/rs_dimaligned.h"
%include "lib/engine/document/entities/rs_dimangular.h"
%include "lib/engine/document/entities/rs_dimdiametric.h"
%include "lib/engine/document/entities/rs_dimlinear.h"
%include "lib/engine/document/entities/rs_dimradial.h"
%include "lib/engine/document/entities/rs_circle.h"
%include "lib/engine/document/entities/rs_arc.h"
%include "lib/engine/document/entities/lc_hyperbola.h"
//%include "lib/engine/document/entities/lc_splinepoints.h"                  // Qt problem child X-(
//%include "lib/engine/document/entities/lc_parabola.h"                      // Qt problem child X-(
%include "lib/engine/document/entities/rs_image.h"
%include "lib/engine/document/entities/rs_insert.h"
%include "lib/engine/document/entities/rs_leader.h"
%include "lib/engine/document/entities/rs_point.h"
%include "lib/engine/document/blocks/rs_block.h"
%include "lib/engine/document/blocks/rs_blocklist.h"
%include "lib/engine/document/blocks/rs_blocklistlistener.h"
%include "lib/engine/document/container/lc_looputils.h"
%include "lib/engine/document/fonts/rs_font.h"
%include "lib/engine/document/fonts/rs_fontchar.h"
%include "lib/engine/document/fonts/rs_fontlist.h"
%include "lib/engine/document/layers/rs_layer.h"
%include "lib/engine/document/layers/rs_layerlist.h"
%include "lib/engine/document/layers/rs_layerlistlistener.h"
%include "lib/engine/document/patterns/rs_pattern.cpp"
//%ignore RS_PatternList::requestPattern(const QString& name);
//%ignore RS_PatternList::contains(const QString& name) const;
//%ignore operator<<(std::ostream& os, RS_PatternList& l);
//%include "lib/engine/document/patterns/rs_patternlist.cpp"                 // problem child X-(
%include "lib/engine/document/variables/rs_variable.h"
%include "lib/engine/document/variables/rs_variabledict.h"
%include "lib/engine/document/views/lc_view.h"
%include "lib/engine/document/views/lc_viewslist.h"
%include "lib/engine/clipboard/rs_clipboard.h"
%include "lib/modification/rs_modification.h"
%include "lib/modification/rs_selection.h"
%include "lib/engine/rs_color.h"
%include "lib/engine/rs_pen.h"
//%include "lib/engine/settings/rs_settings.h"                               // Qt problem child X-(
%include "lib/information/rs_locale.h"
%include "lib/information/rs_infoarea.h"
%include "lib/information/rs_information.h"
%ignore RS_System::test();
%include "lib/engine/rs_system.h"
%ignore LC_Highlight::addEntity([[maybe_unused]]RS_Entity *entity);
%include "lib/engine/overlays/highlight/lc_highlight.h"
%include "lib/engine/overlays/preview/rs_preview.h"
%include "lib/engine/overlays/references/lc_refarc.h"
%include "lib/engine/overlays/references/lc_refcircle.h"
%include "lib/engine/overlays/references/lc_refellipse.h"
%include "lib/engine/overlays/references/lc_refline.h"
%include "lib/engine/overlays/references/lc_refpoint.h"
%include "lib/engine/overlays/lc_crosshair.h"
%include "lib/engine/overlays/rs_overlaybox.h"
%include "lib/engine/overlays/rs_overlayline.h"
//%include "lib/engine/utils/lc_rtree.h"                                    // problem child X-(
//%include "lib/engine/utils/rs_utility.h"                                  // problem child X-(
