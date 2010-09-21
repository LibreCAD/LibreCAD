/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/

void QG_BlockDialog::setBlockList(RS_BlockList* l) {
	RS_DEBUG->print("QG_BlockDialog::setBlockList");
	
    blockList = l;
    if (blockList!=NULL) {
        RS_Block* block = blockList->getActive();
        if (block!=NULL) {
            leName->setText(block->getName());
        } else {
            RS_DEBUG->print(RS_Debug::D_ERROR, 
				"QG_BlockDialog::setBlockList: No block active.");
        }
    }
}

RS_BlockData QG_BlockDialog::getBlockData() {
    /*if (blockList!=NULL) {
      RS_Block* block = blockList->getActive();
        if (block!=NULL) {
           return blockList->rename(block, leName->text().latin1());
        }
}

    return false;*/

    return RS_BlockData(leName->text(), RS_Vector(0.0,0.0), false);
}

void QG_BlockDialog::validate() {
    QString name = leName->text();

    if (!name.isEmpty()) {
        if (blockList!=NULL && blockList->find(name)==NULL) {
            accept();
        } else {
            QMessageBox::warning( this, tr("Renaming Block"),
                                  tr("Could not name block. A block named \"%1\" "
                                     "already exists.").arg(leName->text()),
                                  QMessageBox::Ok,
                                  Qt::NoButton);
        }
    }
    //else {
    //reject();
    //}
}

void QG_BlockDialog::cancel() {
    leName->setText("");
    reject();
}
