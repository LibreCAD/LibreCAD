#
#
# CADuntu project file
# (c) Ries van Twisk (caduntu@rvt.dds.nl)
# 
#

TEMPLATE = app

DEFINES += QC_APPKEY="\"/CADuntu\""
DEFINES += QC_APPNAME="\"CADuntu\""
DEFINES += QC_COMPANYNAME="\"CADuntu\""
DEFINES += QC_VERSION="\"1.0.0alpha1\""


# Add qt3support
QT +=  qt3support

CONFIG += qt warn_on link_prl uic3

QMAKE_CXXFLAGS_DEBUG += 
QMAKE_CXXFLAGS += 

unix {
    macx {
	TARGET = Caduntu
	DEFINES += QC_APPDIR="\"Caduntu\""
	DEFINES += QINITIMAGES_CADUNTU="qInitImages_Caduntu"
	RESOURCEDIR= Caduntu.app/Contents/Resources
	RC_FILE = res/main/caduntu.icns
	DESTDIR     = .
    } else {
        TARGET = caduntu
        DEFINES += QC_APPDIR="\"caduntu\""
	DEFINES += QINITIMAGES_CADUNTU="qInitImages_caduntu"
	RESOURCEDIR= caduntu/Resources
	RC_FILE = res/main/caduntu.icns
	DESTDIR     = unix
    }
}

win32 {
    QMAKE_CFLAGS_THREAD -= -mthreads
    QMAKE_LFLAGS_THREAD -= -mthreads
    TARGET = Caduntu
    DEFINES += QC_APPDIR="\"Caduntu\""
    DEFINES += QINITIMAGES_CADUNTU="qInitImages_Caduntu"
#    RC_FILE = res/main/caduntu.rc
    DESTDIR     = .
}

# Postprocess for osx
macx:postprocess.commands = scripts/postprocess-osx.sh
macx:QMAKE_EXTRA_TARGETS += postprocess
macx:QMAKE_POST_LINK = make postprocess

unix:postprocess.commands = scripts/postprocess-unix.sh
unix:QMAKE_EXTRA_TARGETS += postprocess
unix:QMAKE_POST_LINK = make postprocess



# Additional libraries to load
#LIBS += \
#        -Ldxflib/lib -ldxf \
#        -Lfparser/lib -lfparser
                
# Store intermedia stuff somewhere else
OBJECTS_DIR 	= intermediate/obj
MOC_DIR     	= intermediate/moc
RCC_DIR     	= intermediate/rcc
UI_DIR      	= intermediate/ui
UI_HERADERS_DIR = intermediate/ui
UI_SOURCES_DIR  = intermediate/ui

INCLUDEPATH += \
                dxflib/src \
                fparser/src \
                src/cmd \
                src/lib/actions \
                src/lib/creation \
		src/lib/debug \
		src/lib/engine \
		src/lib/fileio \
		src/lib/filters \
		src/lib/gui \
		src/lib/information \
		src/lib/math \
		src/lib/modification \
		src/lib/scripting \
		src/actions \
		src/main \
		src/ui \
		src/ui/forms3 \
		res 		

#################################################################################	
# Library
HEADERS     = \
                dxflib/src/dl_attributes.h \
                dxflib/src/dl_codes.h \
                dxflib/src/dl_creationadapter.h \
                dxflib/src/dl_creationinterface.h \
                dxflib/src/dl_dxf.h \
                dxflib/src/dl_entities.h \
                dxflib/src/dl_exception.h \
                dxflib/src/dl_extrusion.h \
                dxflib/src/dl_writer_ascii.h \
                fparser/src/fparser.h \
		src/lib/actions/rs_actioninterface.h \
		src/lib/actions/rs_preview.h \
		src/lib/actions/rs_previewactioninterface.h \
		src/lib/actions/rs_snapper.h \
		src/lib/creation/rs_creation.h \
		src/lib/debug/rs_debug.h \
		src/lib/engine/rs.h \
		src/lib/engine/rs_application.h \
		src/lib/engine/rs_arc.h \
		src/lib/engine/rs_atomicentity.h \
		src/lib/engine/rs_block.h \
		src/lib/engine/rs_blocklist.h \
		src/lib/engine/rs_blocklistlistener.h \
		src/lib/engine/rs_clipboard.h \
		src/lib/engine/rs_circle.h \
		src/lib/engine/rs_color.h \
		src/lib/engine/rs_constructionline.h \
		src/lib/engine/rs_datetime.h \
		src/lib/engine/rs_dimaligned.h \
		src/lib/engine/rs_dimangular.h \
		src/lib/engine/rs_dimdiametric.h \
		src/lib/engine/rs_dimension.h \
		src/lib/engine/rs_dimlinear.h \
		src/lib/engine/rs_dimradial.h \
		src/lib/engine/rs_dir.h \
		src/lib/engine/rs_document.h \
		src/lib/engine/rs_ellipse.h \
		src/lib/engine/rs_entity.h \
		src/lib/engine/rs_entitycontainer.h \
		src/lib/engine/rs_file.h \
		src/lib/engine/rs_fileinfo.h \
		src/lib/engine/rs_flags.h \
		src/lib/engine/rs_font.h \
		src/lib/engine/rs_fontchar.h \
		src/lib/engine/rs_fontlist.h \
		src/lib/engine/rs_graphic.h \
		src/lib/engine/rs_hatch.h \
		src/lib/engine/rs_insert.h \
		src/lib/engine/rs_img.h \
		src/lib/engine/rs_image.h \
		src/lib/engine/rs_layer.h \
		src/lib/engine/rs_layerlist.h \
		src/lib/engine/rs_layerlistlistener.h \
		src/lib/engine/rs_leader.h \
		src/lib/engine/rs_line.h \
		src/lib/engine/rs_overlayline.h \
		src/lib/engine/rs_pattern.h \
		src/lib/engine/rs_patternlist.h \
		src/lib/engine/rs_pen.h \
		src/lib/engine/rs_point.h \
		src/lib/engine/rs_polyline.h \
		src/lib/engine/rs_ptrlist.h \
		src/lib/engine/rs_regexp.h \
		src/lib/engine/rs_settings.h \
		src/lib/engine/rs_solid.h \
		src/lib/engine/rs_spline.h \
		src/lib/engine/rs_string.h \
		src/lib/engine/rs_stringlist.h \
		src/lib/engine/rs_system.h \
		src/lib/engine/rs_text.h \
		src/lib/engine/rs_textstream.h \
		src/lib/engine/rs_undo.h \
		src/lib/engine/rs_undoable.h \
		src/lib/engine/rs_undocycle.h \
		src/lib/engine/rs_units.h \
		src/lib/engine/rs_utility.h \
		src/lib/engine/rs_valuelist.h \
		src/lib/engine/rs_variabledict.h \
		src/lib/engine/rs_vector.h \
		src/lib/fileio/rs_fileio.h \
		src/lib/filters/rs_filtercxf.h \
		src/lib/filters/rs_filterdxf.h \
		src/lib/filters/rs_filterdxf1.h \
		src/lib/filters/rs_filterinterface.h \
		src/lib/gui/rs_commandevent.h \
		src/lib/gui/rs_coordinateevent.h \
		src/lib/gui/rs_dialogfactory.h \
		src/lib/gui/rs_dialogfactoryinterface.h \
		src/lib/gui/rs_dialogfactoryadapter.h \
		src/lib/gui/rs_event.h \
		src/lib/gui/rs_eventhandler.h \
		src/lib/gui/rs_graphicview.h \
		src/lib/gui/rs_grid.h \
		src/lib/gui/rs_keyevent.h \
		src/lib/gui/rs_linetypepattern.h \
		src/lib/gui/rs_mainwindowinterface.h \
		src/lib/gui/rs_mouseevent.h \
		src/lib/gui/rs_painter.h \
		src/lib/gui/rs_painteradapter.h \
		src/lib/gui/rs_painterqt.h \
		src/lib/gui/rs_staticgraphicview.h \
		src/lib/information/rs_information.h \
		src/lib/information/rs_infoarea.h \
		src/lib/modification/rs_modification.h \
		src/lib/modification/rs_selection.h \
		src/lib/math/rs_math.h \
		src/lib/scripting/rs_python.h \
		src/lib/scripting/rs_simplepython.h \
		src/lib/scripting/rs_python_wrappers.h \
		src/lib/scripting/rs_script.h \
		src/lib/scripting/rs_scriptlist.h
		
SOURCES     = \ 
                dxflib/src/dl_dxf.cpp \
                dxflib/src/dl_writer_ascii.cpp \
                fparser/src/fparser.cpp \
		src/lib/actions/rs_actioninterface.cpp \
		src/lib/actions/rs_preview.cpp \
		src/lib/actions/rs_previewactioninterface.cpp \
		src/lib/actions/rs_snapper.cpp \
		src/lib/creation/rs_creation.cpp \
		src/lib/debug/rs_debug.cpp \
		src/lib/engine/rs_arc.cpp \
		src/lib/engine/rs_block.cpp \
		src/lib/engine/rs_blocklist.cpp \
		src/lib/engine/rs_clipboard.cpp \
		src/lib/engine/rs_circle.cpp \
		src/lib/engine/rs_constructionline.cpp \
		src/lib/engine/rs_dimaligned.cpp \
		src/lib/engine/rs_dimangular.cpp \
		src/lib/engine/rs_dimdiametric.cpp \
		src/lib/engine/rs_dimension.cpp \
		src/lib/engine/rs_dimlinear.cpp \
		src/lib/engine/rs_dimradial.cpp \
		src/lib/engine/rs_document.cpp \
		src/lib/engine/rs_ellipse.cpp \
		src/lib/engine/rs_entity.cpp \
		src/lib/engine/rs_entitycontainer.cpp \
		src/lib/engine/rs_font.cpp \
		src/lib/engine/rs_fontlist.cpp \
		src/lib/engine/rs_graphic.cpp \
		src/lib/engine/rs_hatch.cpp \
		src/lib/engine/rs_insert.cpp \
		src/lib/engine/rs_image.cpp \
		src/lib/engine/rs_layer.cpp \
		src/lib/engine/rs_layerlist.cpp \
		src/lib/engine/rs_leader.cpp \
		src/lib/engine/rs_line.cpp \
		src/lib/engine/rs_overlayline.cpp \
		src/lib/engine/rs_pattern.cpp \
		src/lib/engine/rs_patternlist.cpp \
		src/lib/engine/rs_point.cpp \
		src/lib/engine/rs_polyline.cpp \
		src/lib/engine/rs_settings.cpp \
		src/lib/engine/rs_solid.cpp \
		src/lib/engine/rs_spline.cpp \
		src/lib/engine/rs_string.cpp \
		src/lib/engine/rs_system.cpp \
		src/lib/engine/rs_text.cpp \
		src/lib/engine/rs_undo.cpp \
		src/lib/engine/rs_undoable.cpp \
		src/lib/engine/rs_units.cpp \
		src/lib/engine/rs_utility.cpp \
		src/lib/engine/rs_variabledict.cpp \
		src/lib/engine/rs_vector.cpp \
		src/lib/fileio/rs_fileio.cpp \
		src/lib/filters/rs_filtercxf.cpp \
		src/lib/filters/rs_filterdxf.cpp \
		src/lib/filters/rs_filterdxf1.cpp \
		src/lib/gui/rs_dialogfactory.cpp \
		src/lib/gui/rs_eventhandler.cpp \
		src/lib/gui/rs_graphicview.cpp \
		src/lib/gui/rs_grid.cpp \
		src/lib/gui/rs_linetypepattern.cpp \
		src/lib/gui/rs_painter.cpp \
		src/lib/gui/rs_painterqt.cpp \
		src/lib/gui/rs_staticgraphicview.cpp \
		src/lib/information/rs_information.cpp \
		src/lib/information/rs_infoarea.cpp \
		src/lib/math/rs_math.cpp \
		src/lib/modification/rs_modification.cpp \
		src/lib/modification/rs_selection.cpp \
		src/lib/scripting/rs_python.cpp \
		src/lib/scripting/rs_simplepython.cpp \
		src/lib/scripting/rs_python_wrappers.cpp \
		src/lib/scripting/rs_script.cpp \
		src/lib/scripting/rs_scriptlist.cpp
TRANSLATIONS = \
		ts/lib/qcadlib_cs.ts \
		ts/lib/qcadlib_et.ts \
		ts/lib/qcadlib_en.ts \
		ts/lib/qcadlib_da.ts \
		ts/lib/qcadlib_de.ts \
		ts/lib/qcadlib_el.ts \
		ts/lib/qcadlib_fr.ts \
		ts/lib/qcadlib_hu.ts \
		ts/lib/qcadlib_it.ts \
		ts/lib/qcadlib_nl.ts \
		ts/lib/qcadlib_no.ts \
		ts/lib/qcadlib_pl.ts \
		ts/lib/qcadlib_ru.ts \
		ts/lib/qcadlib_sk.ts \
		ts/lib/qcadlib_tr.ts


#################################################################################
# Command
HEADERS += \ 
		src/cmd/rs_commands.h
		
SOURCES += \
		src/cmd/rs_commands.cpp 

TRANSLATIONS += \
		ts/cmd/qcadcmd_cs.ts \
		ts/cmd/qcadcmd_et.ts \
		ts/cmd/qcadcmd_en.ts \
		ts/cmd/qcadcmd_da.ts \
		ts/cmd/qcadcmd_de.ts \
		ts/cmd/qcadcmd_el.ts \
		ts/cmd/qcadcmd_es.ts \
		ts/cmd/qcadcmd_fr.ts \
		ts/cmd/qcadcmd_hu.ts \
		ts/cmd/qcadcmd_it.ts \
		ts/cmd/qcadcmd_nl.ts \
		ts/cmd/qcadcmd_no.ts \
		ts/cmd/qcadcmd_pa.ts \
		ts/cmd/qcadcmd_pl.ts \
		ts/cmd/qcadcmd_ru.ts \
		ts/cmd/qcadcmd_sk.ts \
		ts/cmd/qcadcmd_tr.ts


#################################################################################
# Actions
HEADERS += \
		src/actions/rs_actionblocksadd.h \
		src/actions/rs_actionblocksattributes.h \
		src/actions/rs_actionblockscreate.h \
		src/actions/rs_actionblocksedit.h \
		src/actions/rs_actionblocksexplode.h \
		src/actions/rs_actionblocksinsert.h \
		src/actions/rs_actionblocksfreezeall.h \
		src/actions/rs_actionblocksremove.h \
		src/actions/rs_actionblockstoggleview.h \
		src/actions/rs_actiondefault.h \
		src/actions/rs_actiondimaligned.h \
		src/actions/rs_actiondimangular.h \
		src/actions/rs_actiondimdiametric.h \
		src/actions/rs_actiondimension.h \
		src/actions/rs_actiondimleader.h \
		src/actions/rs_actiondimlinear.h \
		src/actions/rs_actiondimradial.h \
		src/actions/rs_actiondrawarc.h \
		src/actions/rs_actiondrawarc3p.h \
		src/actions/rs_actiondrawarctangential.h \
		src/actions/rs_actiondrawcircle.h \
		src/actions/rs_actiondrawcircle2p.h \
		src/actions/rs_actiondrawcircle3p.h \
		src/actions/rs_actiondrawcirclecr.h \
		src/actions/rs_actiondrawellipseaxis.h \
		src/actions/rs_actiondrawhatch.h \
		src/actions/rs_actiondrawimage.h \
		src/actions/rs_actiondrawline.h \
		src/actions/rs_actiondrawlineangle.h \
		src/actions/rs_actiondrawlinebisector.h \
		src/actions/rs_actiondrawlinefree.h \
		src/actions/rs_actiondrawlinehorvert.h \
		src/actions/rs_actiondrawlineparallel.h \
		src/actions/rs_actiondrawlineparallelthrough.h \
		src/actions/rs_actiondrawlinepolygon.h \
		src/actions/rs_actiondrawlinepolygon2.h \
		src/actions/rs_actiondrawlinerectangle.h \
		src/actions/rs_actiondrawlinerelangle.h \
		src/actions/rs_actiondrawlinetangent1.h \
		src/actions/rs_actiondrawlinetangent2.h \
		src/actions/rs_actiondrawpoint.h \
		src/actions/rs_actiondrawspline.h \
		src/actions/rs_actiondrawtext.h \
		src/actions/rs_actioneditcopy.h \
		src/actions/rs_actioneditpaste.h \
		src/actions/rs_actioneditundo.h \
		src/actions/rs_actionfilenew.h \
		src/actions/rs_actionfileopen.h \
		src/actions/rs_actionfilesave.h \
		src/actions/rs_actionfilesaveas.h \
		src/actions/rs_actioninfoangle.h \
		src/actions/rs_actioninfoarea.h \
		src/actions/rs_actioninfoinside.h \
		src/actions/rs_actioninfodist.h \
		src/actions/rs_actioninfodist2.h \
		src/actions/rs_actioninfototallength.h \
		src/actions/rs_actionlayersadd.h \
		src/actions/rs_actionlayersedit.h \
		src/actions/rs_actionlayersfreezeall.h \
		src/actions/rs_actionlayersremove.h \
		src/actions/rs_actionlayerstogglelock.h \
		src/actions/rs_actionlayerstoggleview.h \
		src/actions/rs_actionlibraryinsert.h \
		src/actions/rs_actionlockrelativezero.h \
		src/actions/rs_actionmodifyattributes.h \
		src/actions/rs_actionmodifybevel.h \
		src/actions/rs_actionmodifycut.h \
		src/actions/rs_actionmodifydelete.h \
		src/actions/rs_actionmodifydeletefree.h \
		src/actions/rs_actionmodifydeletequick.h \
		src/actions/rs_actionmodifyentity.h \
		src/actions/rs_actionmodifymirror.h \
		src/actions/rs_actionmodifymove.h \
		src/actions/rs_actionmodifymoverotate.h \
		src/actions/rs_actionmodifyrotate.h \
		src/actions/rs_actionmodifyrotate2.h \
		src/actions/rs_actionmodifyround.h \
		src/actions/rs_actionmodifyscale.h \
		src/actions/rs_actionmodifystretch.h \
		src/actions/rs_actionmodifytrim.h \
		src/actions/rs_actionmodifytrimamount.h \
		src/actions/rs_actionmodifyexplodetext.h \
		src/actions/rs_actionoptionsdrawing.h \
		src/actions/rs_actionparisdebugcreatecontainer.h \
		src/actions/rs_actionprintpreview.h \
		src/actions/rs_actionselect.h \
		src/actions/rs_actionselectall.h \
		src/actions/rs_actionselectbase.h \
		src/actions/rs_actionselectcontour.h \
		src/actions/rs_actionselectintersected.h \
		src/actions/rs_actionselectinvert.h \
		src/actions/rs_actionselectsingle.h \
		src/actions/rs_actionselectwindow.h \
		src/actions/rs_actionselectlayer.h \
		src/actions/rs_actionsetrelativezero.h \
		src/actions/rs_actionsetsnapmode.h \
		src/actions/rs_actionsetsnaprestriction.h \
		src/actions/rs_actionsnapintersectionmanual.h \
		src/actions/rs_actiontoolregeneratedimensions.h \
		src/actions/rs_actionzoomauto.h \
		src/actions/rs_actionzoomautoy.h \
		src/actions/rs_actionzoomin.h \
		src/actions/rs_actionzoompan.h \
		src/actions/rs_actionzoomprevious.h \
		src/actions/rs_actionzoomredraw.h \
		src/actions/rs_actionzoomscroll.h \
		src/actions/rs_actionzoomwindow.h

SOURCES += \
		src/actions/rs_actionblocksadd.cpp \
		src/actions/rs_actionblocksattributes.cpp \
		src/actions/rs_actionblockscreate.cpp \
		src/actions/rs_actionblocksedit.cpp \
		src/actions/rs_actionblocksexplode.cpp \
		src/actions/rs_actionblocksinsert.cpp \
		src/actions/rs_actionblocksfreezeall.cpp \
		src/actions/rs_actionblocksremove.cpp \
		src/actions/rs_actionblockstoggleview.cpp \
		src/actions/rs_actiondefault.cpp \
		src/actions/rs_actiondimaligned.cpp \
		src/actions/rs_actiondimangular.cpp \
		src/actions/rs_actiondimdiametric.cpp \
		src/actions/rs_actiondimension.cpp \
		src/actions/rs_actiondimleader.cpp \
		src/actions/rs_actiondimlinear.cpp \
		src/actions/rs_actiondimradial.cpp \
		src/actions/rs_actiondrawarc.cpp \
		src/actions/rs_actiondrawarc3p.cpp \
		src/actions/rs_actiondrawarctangential.cpp \
		src/actions/rs_actiondrawcircle.cpp \
		src/actions/rs_actiondrawcircle2p.cpp \
		src/actions/rs_actiondrawcircle3p.cpp \
		src/actions/rs_actiondrawcirclecr.cpp \
		src/actions/rs_actiondrawellipseaxis.cpp \
		src/actions/rs_actiondrawhatch.cpp \
		src/actions/rs_actiondrawimage.cpp \
		src/actions/rs_actiondrawline.cpp \
		src/actions/rs_actiondrawlineangle.cpp \
		src/actions/rs_actiondrawlinebisector.cpp \
		src/actions/rs_actiondrawlinefree.cpp \
		src/actions/rs_actiondrawlinehorvert.cpp \
		src/actions/rs_actiondrawlineparallel.cpp \
		src/actions/rs_actiondrawlineparallelthrough.cpp \
		src/actions/rs_actiondrawlinepolygon.cpp \
		src/actions/rs_actiondrawlinepolygon2.cpp \
		src/actions/rs_actiondrawlinerectangle.cpp \
		src/actions/rs_actiondrawlinerelangle.cpp \
		src/actions/rs_actiondrawlinetangent1.cpp \
		src/actions/rs_actiondrawlinetangent2.cpp \
		src/actions/rs_actiondrawpoint.cpp \
		src/actions/rs_actiondrawspline.cpp \
		src/actions/rs_actiondrawtext.cpp \
		src/actions/rs_actioneditcopy.cpp \
		src/actions/rs_actioneditpaste.cpp \
		src/actions/rs_actioneditundo.cpp \
		src/actions/rs_actionfilenew.cpp \
		src/actions/rs_actionfileopen.cpp \
		src/actions/rs_actionfilesave.cpp \
		src/actions/rs_actionfilesaveas.cpp \
		src/actions/rs_actioninfoangle.cpp \
		src/actions/rs_actioninfoarea.cpp \
		src/actions/rs_actioninfoinside.cpp \
		src/actions/rs_actioninfodist.cpp \
		src/actions/rs_actioninfodist2.cpp \
		src/actions/rs_actioninfototallength.cpp \
		src/actions/rs_actionlayersadd.cpp \
		src/actions/rs_actionlayersedit.cpp \
		src/actions/rs_actionlayersfreezeall.cpp \
		src/actions/rs_actionlayersremove.cpp \
		src/actions/rs_actionlayerstogglelock.cpp \
		src/actions/rs_actionlayerstoggleview.cpp \
		src/actions/rs_actionlibraryinsert.cpp \
		src/actions/rs_actionlockrelativezero.cpp \
		src/actions/rs_actionmodifyattributes.cpp \
		src/actions/rs_actionmodifybevel.cpp \
		src/actions/rs_actionmodifycut.cpp \
		src/actions/rs_actionmodifydelete.cpp \
		src/actions/rs_actionmodifydeletefree.cpp \
		src/actions/rs_actionmodifydeletequick.cpp \
		src/actions/rs_actionmodifyentity.cpp \
		src/actions/rs_actionmodifymirror.cpp \
		src/actions/rs_actionmodifymove.cpp \
		src/actions/rs_actionmodifymoverotate.cpp \
		src/actions/rs_actionmodifyrotate.cpp \
		src/actions/rs_actionmodifyrotate2.cpp \
		src/actions/rs_actionmodifyround.cpp \
		src/actions/rs_actionmodifyscale.cpp \
		src/actions/rs_actionmodifystretch.cpp \
		src/actions/rs_actionmodifytrim.cpp \
		src/actions/rs_actionmodifytrimamount.cpp \
		src/actions/rs_actionmodifyexplodetext.cpp \
		src/actions/rs_actionoptionsdrawing.cpp \
		src/actions/rs_actionparisdebugcreatecontainer.cpp \
		src/actions/rs_actionprintpreview.cpp \
		src/actions/rs_actionselect.cpp \
		src/actions/rs_actionselectall.cpp \
		src/actions/rs_actionselectbase.cpp \
		src/actions/rs_actionselectcontour.cpp \
		src/actions/rs_actionselectintersected.cpp \
		src/actions/rs_actionselectinvert.cpp \
		src/actions/rs_actionselectsingle.cpp \
		src/actions/rs_actionselectwindow.cpp \
		src/actions/rs_actionselectlayer.cpp \
		src/actions/rs_actionsetrelativezero.cpp \
		src/actions/rs_actionsetsnapmode.cpp \
		src/actions/rs_actionsetsnaprestriction.cpp \
		src/actions/rs_actionsnapintersectionmanual.cpp \
		src/actions/rs_actiontoolregeneratedimensions.cpp \
		src/actions/rs_actionzoomauto.cpp \
		src/actions/rs_actionzoomautoy.cpp \
		src/actions/rs_actionzoomin.cpp \
		src/actions/rs_actionzoompan.cpp \
		src/actions/rs_actionzoomprevious.cpp \
		src/actions/rs_actionzoomredraw.cpp \
		src/actions/rs_actionzoomscroll.cpp \
		src/actions/rs_actionzoomwindow.cpp

TRANSLATIONS += \
		ts/actions/qcadactions_cs.ts \
		ts/actions/qcadactions_en.ts \
		ts/actions/qcadactions_et.ts \
		ts/actions/qcadactions_da.ts \
		ts/actions/qcadactions_de.ts \
		ts/actions/qcadactions_el.ts \
		ts/actions/qcadactions_es.ts \
		ts/actions/qcadactions_fr.ts \
		ts/actions/qcadactions_hu.ts \
		ts/actions/qcadactions_it.ts \
		ts/actions/qcadactions_nl.ts \
		ts/actions/qcadactions_no.ts \
		ts/actions/qcadactions_pa.ts \
		ts/actions/qcadactions_pl.ts \
		ts/actions/qcadactions_ru.ts \
		ts/actions/qcadactions_sk.ts \
		ts/actions/qcadactions_tr.ts
		
#RESOURCES += \
# res/actions/actions.qrc

IMAGES += \
		res/actions/configure.png \
		res/actions/editcopy2.png \
		res/actions/editcut2.png \
		res/actions/editpaste2.png \
		res/actions/exit.png \
		res/actions/fileclose.png \
		res/actions/filenew.png \
		res/actions/fileopen2.png \
		res/actions/fileprint.png \
		res/actions/fileprintpreview.png \
		res/actions/filesave2.png \
		res/actions/filesaveas.png \
		res/actions/redo2.png \
		res/actions/undo2.png \
		res/actions/viewgrid.png \
		res/actions/viewdraft.png \
		res/actions/zoomauto.png \
		res/actions/zoomin.png \
		res/actions/zoomout.png \
		res/actions/zoompan.png \
		res/actions/zoomprevious.png \
		res/actions/zoomredraw.png \
		res/actions/zoomwindow.png	
		
		
#################################################################################
# UI
HEADERS += \
		src/ui/qg_actionfactory.h \
		src/ui/qg_actionhandler.h \
		src/ui/qg_blockwidget.h \
		src/ui/qg_colorbox.h \
		src/ui/qg_commandedit.h \
		src/ui/qg_dialogfactory.h \
		src/ui/qg_filedialog.h \
		src/ui/qg_fontbox.h \
		src/ui/qg_graphicview.h \
		src/ui/qg_layerbox.h \
		src/ui/qg_layerwidget.h \
		src/ui/qg_linetypebox.h \
		src/ui/qg_listviewitem.h \
		src/ui/qg_mainwindowinterface.h \
		src/ui/qg_patternbox.h \
		src/ui/qg_pentoolbar.h \
		src/ui/qg_qt2rs.h \
		src/ui/qg_recentfiles.h \
		src/ui/qg_scrollbar.h \
		src/ui/qg_widthbox.h \

SOURCES += \
		src/ui/qg_actionfactory.cpp \
		src/ui/qg_actionhandler.cpp \
		src/ui/qg_blockwidget.cpp \
		src/ui/qg_colorbox.cpp \
		src/ui/qg_commandedit.cpp \
		src/ui/qg_dialogfactory.cpp \
		src/ui/qg_filedialog.cpp \
		src/ui/qg_fontbox.cpp \
		src/ui/qg_graphicview.cpp \
		src/ui/qg_layerbox.cpp \
		src/ui/qg_layerwidget.cpp \
		src/ui/qg_linetypebox.cpp \
		src/ui/qg_listviewitem.cpp \
		src/ui/qg_patternbox.cpp \
		src/ui/qg_pentoolbar.cpp \
		src/ui/qg_recentfiles.cpp \
		src/ui/qg_widthbox.cpp 

FORMS3       = \
		src/ui/forms3/qg_commandwidget.ui \
		src/ui/forms3/qg_arcoptions.ui \
		src/ui/forms3/qg_arctangentialoptions.ui \
		src/ui/forms3/qg_beveloptions.ui \
		src/ui/forms3/qg_blockdialog.ui \
		src/ui/forms3/qg_cadtoolbar.ui \
		src/ui/forms3/qg_cadtoolbararcs.ui \
		src/ui/forms3/qg_cadtoolbarcircles.ui \
		src/ui/forms3/qg_cadtoolbardim.ui \
		src/ui/forms3/qg_cadtoolbarellipses.ui \
		src/ui/forms3/qg_cadtoolbarinfo.ui \
		src/ui/forms3/qg_cadtoolbarlines.ui \
		src/ui/forms3/qg_cadtoolbarmain.ui \
		src/ui/forms3/qg_cadtoolbarmodify.ui \
		src/ui/forms3/qg_cadtoolbarpoints.ui \
		src/ui/forms3/qg_cadtoolbarpolylines.ui \
		src/ui/forms3/qg_cadtoolbarselect.ui \
		src/ui/forms3/qg_cadtoolbarsnap.ui \
		src/ui/forms3/qg_cadtoolbarsplines.ui \
		src/ui/forms3/qg_circleoptions.ui \
		src/ui/forms3/qg_coordinatewidget.ui \
		src/ui/forms3/qg_dimensionlabeleditor.ui \
		src/ui/forms3/qg_dimlinearoptions.ui \
		src/ui/forms3/qg_dimoptions.ui \
		src/ui/forms3/qg_dlgattributes.ui \
		src/ui/forms3/qg_dlghatch.ui \
		src/ui/forms3/qg_dlginitial.ui \
		src/ui/forms3/qg_dlginsert.ui \
		src/ui/forms3/qg_dlgimageoptions.ui \
		src/ui/forms3/qg_dlgarc.ui \
		src/ui/forms3/qg_dlgcircle.ui \
		src/ui/forms3/qg_dlgdimension.ui \
		src/ui/forms3/qg_dlgdimlinear.ui \
		src/ui/forms3/qg_dlgline.ui \
		src/ui/forms3/qg_dlgellipse.ui \
		src/ui/forms3/qg_dlgmirror.ui \
		src/ui/forms3/qg_dlgmove.ui \
		src/ui/forms3/qg_dlgmoverotate.ui \
		src/ui/forms3/qg_dlgoptionsdrawing.ui \
		src/ui/forms3/qg_dlgoptionsgeneral.ui \
		src/ui/forms3/qg_dlgpoint.ui \
		src/ui/forms3/qg_dlgrotate.ui \
		src/ui/forms3/qg_dlgrotate2.ui \
		src/ui/forms3/qg_dlgscale.ui \
		src/ui/forms3/qg_dlgspline.ui \
		src/ui/forms3/qg_dlgtext.ui \
		src/ui/forms3/qg_exitdialog.ui \
		src/ui/forms3/qg_imageoptions.ui \
		src/ui/forms3/qg_insertoptions.ui \
		src/ui/forms3/qg_layerdialog.ui \
		src/ui/forms3/qg_libraryinsertoptions.ui \
		src/ui/forms3/qg_librarywidget.ui \
		src/ui/forms3/qg_lineangleoptions.ui \
		src/ui/forms3/qg_linebisectoroptions.ui \
		src/ui/forms3/qg_lineoptions.ui \
		src/ui/forms3/qg_lineparalleloptions.ui \
		src/ui/forms3/qg_lineparallelthroughoptions.ui \
		src/ui/forms3/qg_linepolygon2options.ui \
		src/ui/forms3/qg_linepolygonoptions.ui \
		src/ui/forms3/qg_linerelangleoptions.ui \
		src/ui/forms3/qg_mousewidget.ui \
		src/ui/forms3/qg_moverotateoptions.ui \
		src/ui/forms3/qg_printpreviewoptions.ui \
		src/ui/forms3/qg_roundoptions.ui \
		src/ui/forms3/qg_selectionwidget.ui \
		src/ui/forms3/qg_snapdistoptions.ui \
		src/ui/forms3/qg_splineoptions.ui \
		src/ui/forms3/qg_textoptions.ui \
		src/ui/forms3/qg_trimamountoptions.ui \
		src/ui/forms3/qg_widgetpen.ui \
		src/ui/forms3/qg_dlgimageoptions.ui
		
TRANSLATIONS += \
		ts/ui/qcadguiqt_cs.ts \
		ts/ui/qcadguiqt_et.ts \
		ts/ui/qcadguiqt_en.ts \
		ts/ui/qcadguiqt_da.ts \
		ts/ui/qcadguiqt_de.ts \
		ts/ui/qcadguiqt_el.ts \
		ts/ui/qcadguiqt_es.ts \
		ts/ui/qcadguiqt_fr.ts \
		ts/ui/qcadguiqt_hu.ts \
		ts/ui/qcadguiqt_it.ts \
		ts/ui/qcadguiqt_nl.ts \
		ts/ui/qcadguiqt_no.ts \
		ts/ui/qcadguiqt_pa.ts \
		ts/ui/qcadguiqt_pl.ts \
		ts/ui/qcadguiqt_ru.ts \
		ts/ui/qcadguiqt_sk.ts \
		ts/ui/qcadguiqt_tr.ts

#RESOURCES += \
#		res/ui/ui.qrc
		
		
#################################################################################
# Main
HEADERS += \ 
		src/main/qc_applicationwindow.h \
		src/main/qc_dialogfactory.h \
		src/main/qc_graphicview.h \
		src/main/qc_mdiwindow.h \
		src/main/main.h
		
SOURCES += \
		src/main/qc_applicationwindow.cpp \
		src/main/qc_dialogfactory.cpp \
		src/main/qc_graphicview.cpp \
		src/main/qc_mdiwindow.cpp \
		src/main/main.cpp 
		
TRANSLATIONS += \
		ts/main/qcad_cs.ts \
		ts/main/qcad_et.ts \
		ts/main/qcad_en.ts \
		ts/main/qcad_da.ts \
		ts/main/qcad_de.ts \
		ts/main/qcad_el.ts \
		ts/main/qcad_es.ts \
		ts/main/qcad_fr.ts \
		ts/main/qcad_hu.ts \
		ts/main/qcad_it.ts \
		ts/main/qcad_nl.ts \
		ts/main/qcad_no.ts \
		ts/main/qcad_pa.ts \
		ts/main/qcad_pl.ts \
		ts/main/qcad_ru.ts \
		ts/main/qcad_sk.ts \
		ts/main/qcad_tr.ts

IMAGES += \
		res/main/contents.png \
		res/main/document.png \
		res/main/editclear.png \
		res/main/caduntu16.png \
		res/main/caduntu.png

#RESOURCES += \
#		res/main/main.qrc
	
        
# Include any custom.pro files for personal/special builds
exists( custom.pro ) {
	include( custom.pro )
}
