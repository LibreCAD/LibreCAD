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

#ifndef LC_RELEASECHECKER_H
#define LC_RELEASECHECKER_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

class LC_ReleaseChecker;

class LC_TagInfo{
    friend LC_ReleaseChecker;

public:
    LC_TagInfo() = default;
    LC_TagInfo(int major, int minor, int revision, int bugfix, const QString &label, const QString &tagName);

    bool isBefore(const LC_TagInfo& other) const;
    bool isSameVersion(const LC_TagInfo& other) const;

    QString getLabel() const;
    QString getTagName() const;
protected:
    int major = 0;
    int minor = 0;
    int revision = 0;
    int bugfix = 0;
    QString label = "";
    QString tagName = "";
};

class LC_ReleaseInfo{
    friend  LC_ReleaseChecker;

 public:
    LC_ReleaseInfo() = default;

    LC_ReleaseInfo(const QString &published, bool isDraft, bool isPreRelease, const QString &url, const QString &body):
        draft{isDraft}
      , prerelease{isPreRelease}
      , publishedDate{!published.isEmpty() ? QDateTime::fromString(published, Qt::ISODate).toUTC() : QDateTime{}}
      , htmlURL{url}
      , releaseNotes{body}
      , valid{true}
    {
    }

   bool isAfter(LC_ReleaseInfo other) const {
        return publishedDate > other.publishedDate;
    }
    const LC_TagInfo &getTagInfo() const {
        return tagInfo;
    }
    void setTagInfo(const LC_TagInfo &tag) {
        tagInfo = tag;
    }
    const QDateTime &getPublishedDate() const {
        return publishedDate;
    }
    const QString &getHtmlUrl() const {
        return htmlURL;
    }
    bool isValid() const {
        return valid;
    }

protected:
    LC_TagInfo tagInfo;
    bool draft = true;
    bool prerelease = true;
    QDateTime publishedDate;
    QString htmlURL = "";
    QString releaseNotes = "";
    bool valid = false;
};


class LC_ReleaseChecker: public QObject{
    Q_OBJECT
public:
    LC_ReleaseChecker(QString ownTagName, bool ownPreRelease);
    void checkForNewVersion(bool forceCheck = false);
    const LC_ReleaseInfo &getLatestRelease() const;
    const LC_ReleaseInfo &getLatestPreRelease() const;

signals:
    void updatesAvailable() const;
protected:
    bool emitSignalIfNoNewVersion;
    QNetworkAccessManager m_WebCtrl;
    LC_ReleaseInfo getOwnReleaseInfo(QString tagName, bool preRelease) const;
    LC_TagInfo parseTagInfo(const QString &tagName) const;

    LC_ReleaseInfo ownReleaseInfo;
    LC_ReleaseInfo latestRelease;
    LC_ReleaseInfo latestPreRelease;
protected slots:
    void infoReceived(QNetworkReply* pReply);
    void processReleasesJSON(const QByteArray &responseContent);

    void sortReleasesInfo(QVector<LC_ReleaseInfo> &list) const;
};

#endif // LC_RELEASECHECKER_H
