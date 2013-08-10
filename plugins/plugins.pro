#-------------------------------------------------
#
# Project created by QtCreator 2011-03-22T19:33:11
#
#-------------------------------------------------

TEMPLATE = subdirs
TARGET = plugins

QT += gui

SUBDIRS     = \
        asciifile \
        align \
        list \
        sameprop \
        importshp \
        sample

TRANSLATIONS = ./ts/plugin_ca.ts \
    ./ts/plugins_cs.ts \
    ./ts/plugins_et.ts \
    ./ts/plugins_en.ts \
    ./ts/plugins_en_au.ts \
    ./ts/plugins_da.ts \
    ./ts/plugins_de.ts \
    ./ts/plugins_el.ts \
    ./ts/plugins_es.ts \
    ./ts/plugins_es_ar.ts \
    ./ts/plugins_es_bo.ts \
    ./ts/plugins_es_cl.ts \
    ./ts/plugins_es_co.ts \
    ./ts/plugins_es_cr.ts \
    ./ts/plugins_es_do.ts \
    ./ts/plugins_es_ec.ts \
    ./ts/plugins_es_gt.ts \
    ./ts/plugins_es_hn.ts \
    ./ts/plugins_es_mx.ts \
    ./ts/plugins_es_ni.ts \
    ./ts/plugins_es_pa.ts \
    ./ts/plugins_es_pe.ts \
    ./ts/plugins_es_pr.ts \
    ./ts/plugins_es_py.ts \
    ./ts/plugins_es_sv.ts \
    ./ts/plugins_es_us.ts \
    ./ts/plugins_es_uy.ts \
    ./ts/plugins_es_ve.ts \
    ./ts/plugins_fi.ts \
    ./ts/plugins_fr.ts \
    ./ts/plugins_hu.ts \
    ./ts/plugins_id_ID.ts \
    ./ts/plugins_it.ts \
    ./ts/plugins_ja.ts \
    ./ts/plugins_nl.ts \
    ./ts/plugins_no.ts \
    ./ts/plugins_pa.ts \
    ./ts/plugins_pl.ts \
    ./ts/plugins_pt_br.ts \
    ./ts/plugins_pt_pt.ts \
    ./ts/plugins_ro_ro.ts \
    ./ts/plugins_ru.ts \
    ./ts/plugins_sk.ts \
    ./ts/plugins_sq_al.ts \
    ./ts/plugins_sv.ts \
    ./ts/plugins_tr.ts \
    ./ts/plugins_uk.ts \
    ./ts/plugins_zh_cn.ts \
    ./ts/plugins_zh_tw.ts


# install
INSTALLDIR = ../unix/resources/plugins
win32 {
    INSTALLDIR = ../windows/resources/plugins
}
unix {
    macx { 
    INSTALLDIR = ../LibreCAD.app/Contents/Resources/plugins
    }
    else { 
    INSTALLDIR = ../unix/resources/plugins
    }
}


