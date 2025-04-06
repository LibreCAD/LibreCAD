/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2015 A. Stebich (librecad@mail.lordofbikes.de)
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/


#ifndef RS_LAYERLIST_H
#define RS_LAYERLIST_H

#include <QList>

class RS_Layer;
class RS_LayerListListener;

/**
 * A list of layers.
 *
 * @author Andrew Mustun
 */
class RS_LayerList {
public:
    RS_LayerList();
    virtual ~RS_LayerList() = default;

    void clear();

    /**
     * @return Number of layers in the list.
     */
    unsigned int count() const {
        return m_layers.count();
    }

    /**
     * @return Layer at given position or NULL if i is out of range.
     */
    RS_Layer* at(unsigned int i) {
        return m_layers.at(i);
    }
    QList<RS_Layer*>::iterator begin();
    QList<RS_Layer*>::iterator end();
    QList<RS_Layer*>::const_iterator begin()const;
    QList<RS_Layer*>::const_iterator end()const;

    void activate(const QString& name, bool notify = false);
    void fireLayerActivated();
    void activate(RS_Layer* layer, bool notify = false);
    //! @return The active layer of NULL if no layer is activated.
    RS_Layer* getActive() const {
        return m_activeLayer;
    }
    virtual void add(RS_Layer* layerToAdd);
    void fireLayerRemoved(RS_Layer* layer);
    virtual void remove(RS_Layer* layerToRemove);
    virtual void edit(RS_Layer* layer, const RS_Layer& source);
    RS_Layer* find(const QString& name);
    int getIndex(const QString& name);
    int getIndex(RS_Layer* layer);
    void toggle(const QString& name);
    void toggle(RS_Layer* layer);
    void toggleLock(RS_Layer* layer);
    void togglePrint(RS_Layer* layer);
    void toggleConstruction(RS_Layer* layer);
    void freezeAll(bool freeze);
    void lockAll(bool lock);

    void toggleLockMulti(QList<RS_Layer*> layers);
    void togglePrintMulti(QList<RS_Layer*> layers);
    void toggleConstructionMulti(QList<RS_Layer*> layers);
    void toggleFreezeMulti(QList<RS_Layer*> layers);
    void setFreezeMulti(QList<RS_Layer*> layersEnable, QList<RS_Layer*> layersDisable);
				void setLockMulti(QList<RS_Layer*> layersToUnlock, QList<RS_Layer*> layersToLock);
    void setPrintMulti(QList<RS_Layer*> layersNoPrint, QList<RS_Layer*> layersPrint);
    void setConstructionMulti(QList<RS_Layer*> layersNoConstruction, QList<RS_Layer*> layersConstruction);
    void fireEdit(RS_Layer* layer);

    void addListener(RS_LayerListListener* listener);
    void removeListener(RS_LayerListListener* listener);
    /**
     * Sets the layer lists modified status to 'm'.
     */
    void setModified(bool m);
    /**
     * @retval true The layer list has been modified.
     * @retval false The layer list has not been modified.
     */
    virtual bool isModified() const {
        return m_modified;
    }
    /**
     * @brief sort by layer names
     */
    void sort();
    void fireLayerAdded(RS_Layer* layer);
    void slotUpdateLayerList();
    friend std::ostream& operator << (std::ostream& os, RS_LayerList& l);

private:
    void fireLayerToggled();
	//! layers in the graphic
    QList<RS_Layer*> m_layers;
    //! List of registered LayerListListeners
    QList<RS_LayerListListener*> m_layerListListeners;
    RS_Layer *m_activeLayer = nullptr;
    /** Flag set if the layer list was modified and not yet saved. */
    bool m_modified = false;
};

#endif
