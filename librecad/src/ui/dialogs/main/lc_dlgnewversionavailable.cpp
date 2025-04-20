/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2024 LibreCAD.org
 Copyright (C) 2024 sand1024

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

#include "lc_dlgnewversionavailable.h"
#include "main.h"
#include "rs_settings.h"
#include "ui_lc_dlgnewversionavailable.h"


LC_DlgNewVersionAvailable::LC_DlgNewVersionAvailable(QWidget *parent,LC_ReleaseChecker* releaseChecker)
    : LC_Dialog(parent, "NewVersion")
    , ui(new Ui::LC_DlgNewVersionAvailable){
    ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LC_DlgNewVersionAvailable::onOk);
    LC_GROUP_GUARD("Startup");
    {
        bool checkOnStartup = LC_GET_BOOL("CheckForNewVersions", true);
        bool ignorePreRelease = LC_GET_BOOL("IgnorePreReleaseVersions");

        QString ignoredRelease = LC_GET_STR("IgnoredRelease", "");
        QString ignoredPreRelease = LC_GET_STR("IgnoredPreRelease", "");

        ui->cbCheckForNewVersion->setChecked(checkOnStartup);
        ui->cbIgnorePreReleases->setChecked(ignorePreRelease);
        if (XSTR(LC_PRERELEASE)) {
            ui->cbIgnorePreReleases->setEnabled(false);
        }

        if (ignoredRelease.isEmpty()){
            ui->cbIgnoredRelease->setVisible(false);
            ui->lIgnoredReleaseTag->setVisible(false);
        }
        else{
            ui->cbIgnoredRelease->setChecked(true);
            ui->lIgnoredReleaseTag->setText(ignoredRelease);
        }

        if (ignoredPreRelease.isEmpty()){
            ui->cbIgnoredPreRelease->setVisible(false);
            ui->lIgnoredPreReleaseTag->setVisible(false);
        }
        else{
            ui->cbIgnoredPreRelease->setChecked(true);
            ui->lIgnoredPreReleaseTag->setText(ignoredPreRelease);
        }
    }
    setup(releaseChecker);
}

LC_DlgNewVersionAvailable::~LC_DlgNewVersionAvailable(){
    delete ui;
}

void LC_DlgNewVersionAvailable::setup(LC_ReleaseChecker *releaseChecker) {
    const LC_ReleaseInfo& latestRelease = releaseChecker->getLatestRelease();
    bool hasNewVersion = false;
    if (latestRelease.isValid()){
        ui->gbRelease->setVisible(true);
        QString releaseTagName = latestRelease.getTagInfo().getTagName();
        QString labelText = QString("<a href=\"%1\">%2</a>").arg(latestRelease.getHtmlUrl(), releaseTagName);
        ui->lReleaseName->setText(labelText);
        QString date = latestRelease.getPublishedDate().toString(Qt::ISODate);
        ui->lReleasePublishDate->setText(date);
        m_currentReleaseTag = releaseTagName;
        hasNewVersion = true;
    }
    else{
        ui->gbRelease->setVisible(false);
    }
    const LC_ReleaseInfo& latestPreRelease = releaseChecker->getLatestPreRelease();
    if (latestPreRelease.isValid()){
        ui->gbPreRelease->setVisible(true);
        QString preReleaseTagName = latestPreRelease.getTagInfo().getTagName();
        QString labelText = QString("<a href=\"%1\">%2</a>").arg(latestPreRelease.getHtmlUrl(), preReleaseTagName);
        ui->lPrereleaseName->setText(labelText);
        QString date = latestPreRelease.getPublishedDate().toString(Qt::ISODate);
        ui->lPreReleasePublishDate->setText(date);
        m_currentPreReleaseTag = preReleaseTagName;
        hasNewVersion = true;
    }
    else{
        ui->gbPreRelease->setVisible(false);
    }
    if (hasNewVersion){
        ui->rbNoNewVersion->setVisible(false);
    }
    else{
        if (ui->cbIgnorePreReleases->isChecked()){
            ui->lblEnablePreReleases->setVisible(true);
        }
        ui->rbNoNewVersion->setVisible(true);
        setWindowTitle(tr("Version Check"));
    }

    ui->lblCurrentVersion->setText(XSTR(LC_VERSION));
}

void LC_DlgNewVersionAvailable::onOk(){
    LC_GROUP_GUARD("Startup");
    LC_SET("CheckForNewVersions", ui->cbCheckForNewVersion->isChecked());
    LC_SET("IgnorePreReleaseVersions", ui->cbIgnorePreReleases->isChecked());

    if (ui->cbReleaseIgnore->isChecked()){
        LC_SET("IgnoredRelease", m_currentReleaseTag);
    }
    else{
        if (!ui->cbIgnoredRelease->isChecked()){
            LC_SET("IgnoredRelease", QString(""));
        }
    }

    if (ui->cbPreReleaseIgnore->isChecked()){
        LC_SET("IgnoredPreRelease", m_currentPreReleaseTag);
    }
    else if (!ui->cbIgnoredPreRelease->isChecked()){
        LC_SET("IgnoredPreRelease", QString(""));
    }
    accept();
}
