/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/

#include <QApplication>
#include <QAtomicInt>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>
#include <QStyleOption>
#include <QSvgRenderer>

#include "lc_svgiconengine.h"


enum FileType {PlainSVG, TemplateSVG};


namespace LC_SVGIconEngineAPI {

    static const char *KEY_ICONS_OVERRIDES_DIR = "LCI_BaseDir";
    static const char *KEY_COLOR_MAIN = "LCI_ColorMain";
    static const char *KEY_COLOR_ACCENT = "LCI_ColorAccent";
    static const char *KEY_COLOR_BG = "LCI_ColorBack";

    static constexpr int ANY_STATE = 2;
    static constexpr int ANY_MODE = 4;

    enum IconMode {
        Normal, Disabled, Active, Selected, AnyMode
    };

    enum IconState {
        On, Off, AnyState
    };

    enum ColorType{
        Main,
        Accent,
        Background
    };


QString getColorAppKeyName(QString baseName, int mode, int state){
    QString result = baseName;
    switch (mode) {
    case QIcon::Mode::Normal: {
        result.append("N");
        break;
    }
    case QIcon::Mode::Active: {
        result.append("A");
        break;
    }
    case QIcon::Mode::Disabled: {
        result.append("D");
        break;
    }
    case QIcon::Mode::Selected: {
        result.append("S");
        break;
    }
    default:
        break;
    }

    switch (state) {
    case QIcon::State::On: {
        result.append("N");
        break;
    }
    case QIcon::State::Off: {
        result.append("F");
        break;
    }
    default:
        break;
    }
    return result;
}


inline void setColorAppProperty(QString baseKey, int mode, int state, QString value){
    QString key = getColorAppKeyName(baseKey, mode, state);
    qApp->setProperty(key.toStdString().c_str(),   value);
}

inline QString getColorAppProperty(QString baseKey, int mode, int state){
    QString key = getColorAppKeyName(baseKey, mode, state);
    QVariant vProperty = qApp->property(key.toStdString().c_str());
    if (vProperty.isValid()) {
        return vProperty.value<QString>();
    }
    return QString();
}
}

struct LC_SvgFileInfo {
    QString fileName;
    FileType fileType;
};

class LC_SvgIconEnginePrivate : public QSharedData {
public:
    LC_SvgIconEnginePrivate()
        : addedPixmaps(0)
    { stepSerialNum(); }

    ~LC_SvgIconEnginePrivate() { delete addedPixmaps; qDeleteAll(svgFiles);}

    static int hashKey(QIcon::Mode mode, QIcon::State state)  { return (((mode)<<4)|state); }

    QString pmcKey(const QSize &size, QIcon::Mode mode, QIcon::State state)
    { return QLatin1String("$lc_svgicon_")
               + QString::number(serialNum, 16).append(QLatin1Char('_'))
               + QString::number((((((qint64(size.width()) << 11) | size.height()) << 11) | mode) << 4) | state, 16); }

    void stepSerialNum() { serialNum = lastSerialNum.fetchAndAddRelaxed(1); }

    bool tryLoad(QSvgRenderer *renderer, QIcon::Mode mode, QIcon::State state);
    bool tryLoad(QSvgRenderer *renderer, QIcon::Mode baseMode, QIcon::State baseState, QIcon::Mode mode, QIcon::State state, bool &colorsReplaced);
    QIcon::Mode loadDataForModeAndState(QSvgRenderer *renderer, QIcon::Mode mode, QIcon::State state);
    void checkFileOverride(QIcon::Mode mode, QIcon::State state, FileType fileType, QString plainSVGFileName);
    void checkFileOverride(QString baseName, QIcon::Mode mode, QIcon::State state, FileType fileType);
    void checkFileOverrideForAnyState(QString baseName, QIcon::Mode mode, QIcon::State state, FileType fileType);
    void checkFileOverrideForAnyMode(QString baseName, QIcon::Mode mode, QIcon::State state, FileType fileType);

    QString getColorForReplacement(QString baseKey, QIcon::Mode mode, QIcon::State state);
    QString replaceColor(QString content, QString baseColorKey, QIcon::Mode mode, QIcon::State state, QString originalColor);

    QHash<int, LC_SvgFileInfo*> svgFiles;
    QHash<int, QPixmap> *addedPixmaps;
    int serialNum;
    static QAtomicInt lastSerialNum;
};

QAtomicInt LC_SvgIconEnginePrivate::lastSerialNum;

LC_SVGIconEngine::LC_SVGIconEngine()
    : d(new LC_SvgIconEnginePrivate){}

LC_SVGIconEngine::LC_SVGIconEngine(const LC_SVGIconEngine &other)
    : QIconEngine(other), d(new LC_SvgIconEnginePrivate){
    d->svgFiles = other.d->svgFiles;
    if (other.d->addedPixmaps)
        d->addedPixmaps = new QHash<int, QPixmap>(*other.d->addedPixmaps);
}

LC_SVGIconEngine::~LC_SVGIconEngine(){}


QString getEnrichedFileName(QString baseName, int mode, int state, FileType type){
    QString result = baseName;
    switch (mode) {
        case QIcon::Mode::Normal: {
            result.append("_normal");
            break;
        }
        case QIcon::Mode::Active: {
            result.append("_active");
            break;
        }
        case QIcon::Mode::Disabled: {
            result.append("_disabled");
            break;
        }
        case QIcon::Mode::Selected: {
            result.append("_selected");
            break;
        }
        default:
            break;
    }

    switch (state) {
        case QIcon::State::On: {
            result.append("_on");
            break;
        }
        case QIcon::State::Off: {
            result.append("_off");
            break;
        }
        default:
            break;
    }
    switch (type) {
        case PlainSVG: {
            result.append(".svg");
            break;
        }
        case TemplateSVG: {
            result.append(".lci");
            break;
        }
        default:
            break;
    }
    return result;
}

void LC_SvgIconEnginePrivate::checkFileOverride(QIcon::Mode mode, QIcon::State state, FileType fileType, QString plainSVGFileName){
    QFile plainSVGFile = QFile(plainSVGFileName);
    if (plainSVGFile.exists()) {
        QSvgRenderer renderer(plainSVGFileName);
        if (renderer.isValid()) {
            LC_SvgFileInfo* info = new LC_SvgFileInfo();
            info->fileName = plainSVGFileName;
            info->fileType = fileType;
            stepSerialNum();
            svgFiles.insert(hashKey(mode, state), info);
        }
    }
}

void LC_SvgIconEnginePrivate::checkFileOverride(QString baseName, QIcon::Mode mode, QIcon::State state, FileType fileType){
    QString plainSVGFileName = getEnrichedFileName(baseName, mode, state, fileType);
    checkFileOverride(mode, state, fileType, plainSVGFileName);
}

void LC_SvgIconEnginePrivate::checkFileOverrideForAnyState(QString baseName, QIcon::Mode mode, QIcon::State state, FileType fileType){
    QString plainSVGFileName = getEnrichedFileName(baseName, mode, LC_SVGIconEngineAPI::ANY_STATE, fileType);
    checkFileOverride(mode, state, fileType, plainSVGFileName);
}

void LC_SvgIconEnginePrivate::checkFileOverrideForAnyMode(QString baseName, QIcon::Mode mode, QIcon::State state, FileType fileType){
    QString plainSVGFileName = getEnrichedFileName(baseName, LC_SVGIconEngineAPI::ANY_MODE, LC_SVGIconEngineAPI::ANY_STATE, fileType);
    checkFileOverride(mode, state, fileType, plainSVGFileName);
}

void LC_SVGIconEngine::addFile(const QString &fileName, const QSize &,
                               QIcon::Mode mode, QIcon::State state){
    if (!fileName.isEmpty()) {
        // first, try to check that icon override is not provided by the user
        if (fileName.startsWith(":")) {
            auto application = qApp;
            if (application != nullptr) {
                auto vBaseDir = application->property(LC_SVGIconEngineAPI::KEY_ICONS_OVERRIDES_DIR);
                if (vBaseDir.isValid()){
                    QString sBaseDir = vBaseDir.toString();
                    QDir dirFile(sBaseDir);
                    if (dirFile.exists()) {
                        QString noExtensions = fileName.mid(0, fileName.lastIndexOf('.'));
                        if (noExtensions.endsWith(".svg")) { // handle .svg.lc format of file in resources
                            noExtensions = noExtensions.mid(0, noExtensions.lastIndexOf('.'));
                        }

                        if (noExtensions.startsWith(":/")){
                            noExtensions = noExtensions.remove(":/");
                        }
                        else{
                            noExtensions = noExtensions.remove(":");
                        }

                        QString baseName = dirFile.absoluteFilePath(noExtensions);

                        // try to find all possible variants for icons overrides.

                        // first check for fullest file name of explicit icon that does not require colors substitution
                        d->checkFileOverride(baseName, mode, state, PlainSVG);
                        d->checkFileOverrideForAnyState(baseName, mode, state,PlainSVG);
                        d->checkFileOverrideForAnyMode(baseName, mode, state,PlainSVG);

                        // check whether overriden icon with colors substitution exists
                        d->checkFileOverride(baseName, mode, state, TemplateSVG);
                        d->checkFileOverrideForAnyState(baseName, mode, state,TemplateSVG);
                        d->checkFileOverrideForAnyMode(baseName, mode, state,TemplateSVG);

                        // try to check whether there are overrides for other states
                        d->checkFileOverride(baseName, QIcon::Mode::Active, QIcon::Off,PlainSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Disabled, QIcon::Off,PlainSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Normal, QIcon::Off,PlainSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Selected, QIcon::Off,PlainSVG);

                        d->checkFileOverride(baseName, QIcon::Mode::Active, QIcon::On,PlainSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Disabled, QIcon::On,PlainSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Normal, QIcon::On,PlainSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Selected, QIcon::On,PlainSVG);

                        d->checkFileOverride(baseName, QIcon::Mode::Active, QIcon::Off,TemplateSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Disabled, QIcon::Off,TemplateSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Normal, QIcon::Off,TemplateSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Selected, QIcon::Off,TemplateSVG);

                        d->checkFileOverride(baseName, QIcon::Mode::Active, QIcon::On,TemplateSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Disabled, QIcon::On,TemplateSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Normal, QIcon::On,TemplateSVG);
                        d->checkFileOverride(baseName, QIcon::Mode::Selected, QIcon::On,TemplateSVG);
                    }
                }
            }
        }

        // no icon override is provided by the user, so just check that provided file exists
        int key = d->hashKey(mode, state);
        if (!d->svgFiles.contains(key)) {
            QFile plainSVGFile = QFile(fileName);
            if (plainSVGFile.exists()) {
                QSvgRenderer renderer(fileName);
                if (renderer.isValid()) {
                    LC_SvgFileInfo *info = new LC_SvgFileInfo();
                    info->fileName = fileName;
                    info->fileType = TemplateSVG;
                    d->stepSerialNum();
                    d->svgFiles.insert(key, info);
                }
            }
        }
    }
}

QSize LC_SVGIconEngine::actualSize(const QSize &size, QIcon::Mode mode,
                                 QIcon::State state){
    if (d->addedPixmaps) {
        QPixmap pm = d->addedPixmaps->value(d->hashKey(mode, state));
        if (!pm.isNull() && pm.size() == size)
            return size;
    }

    QPixmap pm = pixmap(size, mode, state);
    if (pm.isNull()) {
        return QSize();
    }
    return pm.size();
}

namespace {
    static const char* TEMPLATE_COLOR_MAIN = "#000";
    static const char* TEMPLATE_COLOR_ACCENT = "#00ff7f";
    static const char* TEMPLATE_COLOR_BACKGROUND_FILL = "#fff";
}

QString LC_SvgIconEnginePrivate::getColorForReplacement(QString baseKey, QIcon::Mode mode, QIcon::State state){
    // first, try to find the most exact replacement color that includes mode and state

    QString result = LC_SVGIconEngineAPI::getColorAppProperty(baseKey, mode, state);
    if (!result.isEmpty()) {
       return result;
    }

    result = LC_SVGIconEngineAPI::getColorAppProperty(baseKey, -1, -1);

    return result;
}

QString LC_SvgIconEnginePrivate::replaceColor(QString content, QString baseColorKey, QIcon::Mode mode, QIcon::State state, QString originalColor){
    QString color = getColorForReplacement(baseColorKey, mode, state);
    if (!color.isEmpty()  && color != originalColor) {
        content = content.replace(originalColor, color);
    }
    return content;
}

bool LC_SvgIconEnginePrivate::tryLoad(QSvgRenderer *renderer, QIcon::Mode mode, QIcon::State state){
    bool ok;
    return tryLoad(renderer, mode, state, mode, state, ok);
}

bool LC_SvgIconEnginePrivate::tryLoad(QSvgRenderer *renderer, QIcon::Mode baseMode, QIcon::State baseState, QIcon::Mode mode, QIcon::State state, bool &colorsReplaced){
    LC_SvgFileInfo* fileInfo = svgFiles.value(hashKey(baseMode, baseState));
    if (fileInfo != nullptr) {
        switch (fileInfo->fileType) {
            case TemplateSVG: {
                QFile file(fileInfo->fileName);
                if (file.open(QFile::ReadOnly | QFile::Text)) {
                    QTextStream in(&file);
                    QString content = in.readAll();

                    content = replaceColor(content, LC_SVGIconEngineAPI::KEY_COLOR_MAIN, mode, state, TEMPLATE_COLOR_MAIN);
                    content = replaceColor(content, LC_SVGIconEngineAPI::KEY_COLOR_ACCENT, mode, state, TEMPLATE_COLOR_ACCENT);
                    content = replaceColor(content, LC_SVGIconEngineAPI::KEY_COLOR_BG, mode, state, TEMPLATE_COLOR_BACKGROUND_FILL);

                    QByteArray byteArrayContent = content.toUtf8();
                    renderer->load(byteArrayContent);
                    colorsReplaced = true;
                    return true;
                }
                break;
            }
            case PlainSVG: {
                QString fileName = fileInfo->fileName;
                renderer->load(fileName);
                return true;
            }
        }
    }
    return false;
}

QIcon::Mode LC_SvgIconEnginePrivate::loadDataForModeAndState(QSvgRenderer *renderer, QIcon::Mode mode, QIcon::State state){
    if (tryLoad(renderer, mode, state))
        return mode;

    bool colorsReplaced = false;
    const QIcon::State oppositeState = (state == QIcon::On) ? QIcon::Off : QIcon::On;
    if (mode == QIcon::Disabled || mode == QIcon::Selected) {
        const QIcon::Mode oppositeMode = (mode == QIcon::Disabled) ? QIcon::Selected : QIcon::Disabled;

        if (tryLoad(renderer, QIcon::Normal, state, mode, state,colorsReplaced))
            return colorsReplaced ? mode : QIcon::Normal;
        if (tryLoad(renderer, QIcon::Active, state, mode, state,colorsReplaced))
            return colorsReplaced ? mode : QIcon::Active;
        if (tryLoad(renderer, mode, oppositeState, mode, state,colorsReplaced))
            return mode;
        if (tryLoad(renderer, QIcon::Normal, oppositeState, mode, state,colorsReplaced))
            return colorsReplaced ? mode : QIcon::Normal;
        if (tryLoad(renderer, QIcon::Active, oppositeState, mode, state,colorsReplaced))
            return colorsReplaced ? mode : QIcon::Active;
        if (tryLoad(renderer, oppositeMode, state, mode, state,colorsReplaced))
            return colorsReplaced ? mode : oppositeMode;
        if (tryLoad(renderer, oppositeMode, oppositeState, mode, state,colorsReplaced))
            return colorsReplaced ? mode : oppositeMode;
    } else {
        const QIcon::Mode oppositeMode = (mode == QIcon::Normal) ? QIcon::Active : QIcon::Normal;
        if (tryLoad(renderer, oppositeMode, state, mode, state,colorsReplaced))
            return colorsReplaced ? mode : oppositeMode;
        if (tryLoad(renderer, mode, oppositeState, mode, state,colorsReplaced))
            return mode;
        if (tryLoad(renderer, oppositeMode, oppositeState, mode, state,colorsReplaced))
            return colorsReplaced ? mode : oppositeMode;
        if (tryLoad(renderer, QIcon::Disabled, state, mode, state,colorsReplaced))
            return colorsReplaced ? mode : QIcon::Disabled;
        if (tryLoad(renderer, QIcon::Selected, state, mode, state,colorsReplaced))
            return colorsReplaced ? mode : QIcon::Selected;
        if (tryLoad(renderer, QIcon::Disabled, oppositeState, mode, state,colorsReplaced))
            return colorsReplaced ? mode : QIcon::Disabled;
        if (tryLoad(renderer, QIcon::Selected, oppositeState, mode, state,colorsReplaced))
            return colorsReplaced ? mode : QIcon::Selected;
    }
    return QIcon::Normal;
}

QPixmap LC_SVGIconEngine::pixmap(const QSize &size, QIcon::Mode mode,
                               QIcon::State state){
    QPixmap pm;

    QString pmckey(d->pmcKey(size, mode, state));
    if (QPixmapCache::find(pmckey, &pm)) {
        if (!pm.isNull()){
           return pm;
        }
    }

    if (d->addedPixmaps) {
        pm = d->addedPixmaps->value(d->hashKey(mode, state));
        if (!pm.isNull() && pm.size() == size) {
            return pm;
        }
    }

    QSvgRenderer renderer;
    const QIcon::Mode foundMode = d->loadDataForModeAndState(&renderer, mode, state);
    if (!renderer.isValid()) {
        return pm;
    }

    QSize actualSize = renderer.defaultSize();
    if (!actualSize.isNull()) {
        actualSize.scale(size, Qt::KeepAspectRatio);
    }

    if (actualSize.isEmpty()) {
        return QPixmap();
    }

    QImage img(actualSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(0x00000000);
    QPainter p(&img);
    renderer.render(&p);
    p.end();

    pm = QPixmap::fromImage(img);
    if (qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        if (foundMode != mode && mode != QIcon::Normal) {
            QStyleOption opt(0);
            opt.palette = QGuiApplication::palette();
            const QPixmap generated = QApplication::style()->generatedIconPixmap(mode, pm, &opt);

            if (!generated.isNull())
                pm = generated;
        }
    }

    if (!pm.isNull())
        QPixmapCache::insert(pmckey, pm);

    return pm;
}

void LC_SVGIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode,
                               QIcon::State state){
    if (!d->addedPixmaps) {
        d->addedPixmaps = new QHash<int, QPixmap>;
    }
    d->stepSerialNum();
    d->addedPixmaps->insert(d->hashKey(mode, state), pixmap);
}


void LC_SVGIconEngine::paint(QPainter *painter, const QRect &rect,
                           QIcon::Mode mode, QIcon::State state)
{
    QSize pixmapSize = rect.size();
    if (painter->device()) {
        pixmapSize *= painter->device()->devicePixelRatio();
    }
    painter->drawPixmap(rect, pixmap(pixmapSize, mode, state));
}

QString LC_SVGIconEngine::key() const{
    return QLatin1String("svg.lci");
}

QIconEngine *LC_SVGIconEngine::clone() const{
    return new LC_SVGIconEngine(*this);
}

void LC_SVGIconEngine::virtual_hook(int id, void *data)
{
    if (id == QIconEngine::IsNullHook) {
        *reinterpret_cast<bool*>(data) = d->svgFiles.isEmpty() && !d->addedPixmaps;
    }
    QIconEngine::virtual_hook(id, data);
}
