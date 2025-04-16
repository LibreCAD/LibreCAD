// /****************************************************************************
//
// Utility base class for widgets that represents options for actions
//
// Copyright (C) 2025 LibreCAD.org
// Copyright (C) 2025 sand1024
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
// **********************************************************************
//

#ifndef LC_RELEASECHECKER_H
#define LC_RELEASECHECKER_H

#include <QNetworkReply>

class LC_ReleaseChecker;

class LC_TagInfo{
    friend LC_ReleaseChecker;

public:
    LC_TagInfo() = default;
    LC_TagInfo(int majorVer, int minorVer, int revisionNum, int bugfixVer, const QString &labelVer, const QString &tagNameVer);

    bool isBefore(const LC_TagInfo& other) const;
    bool isSameVersion(const LC_TagInfo& other) const;

    QString getLabel() const;
    QString getTagName() const;
protected:
    // actually, that's just a suxx in GCC world... it's weird, and previous name crashed the build.
    // https://bugzilla.redhat.com/show_bug.cgi?id=130601
    int m_major = 0;
    int m_minor = 0;
    int m_revision = 0;
    int m_bugfix = 0;
    QString m_label = "";
    QString m_tagName = "";
};

class LC_ReleaseInfo{
    friend  LC_ReleaseChecker;

 public:
    LC_ReleaseInfo() = default;

    LC_ReleaseInfo(const QString &published, bool isDraft, bool isPreRelease, const QString &url, const QString &body):
        m_isDraft{isDraft}
      , m_isPrerelease{isPreRelease}
      , m_publishedDate{!published.isEmpty() ? QDateTime::fromString(published, Qt::ISODate).toUTC() : QDateTime{}}
      , m_htmlURL{url}
      , m_releaseNotes{body}
      , m_valid{true}
    {
    }

   bool isAfter(LC_ReleaseInfo other) const {
        return m_publishedDate > other.m_publishedDate;
    }
    const LC_TagInfo &getTagInfo() const {
        return m_tagInfo;
    }
    void setTagInfo(const LC_TagInfo &tag) {
        m_tagInfo = tag;
    }
    const QDateTime &getPublishedDate() const {
        return m_publishedDate;
    }
    const QString &getHtmlUrl() const {
        return m_htmlURL;
    }
    bool isValid() const {
        return m_valid;
    }

protected:
    LC_TagInfo m_tagInfo;
    bool m_isDraft = true;
    bool m_isPrerelease = true;
    QDateTime m_publishedDate;
    QString m_htmlURL = "";
    QString m_releaseNotes = "";
    bool m_valid = false;
};


class LC_ReleaseChecker: public QObject{
    Q_OBJECT
public:
    LC_ReleaseChecker(const QString& ownTagName, bool ownPreRelease);
    void checkForNewVersion(bool forceCheck = false);
    const LC_ReleaseInfo &getLatestRelease() const;
    const LC_ReleaseInfo &getLatestPreRelease() const;

signals:
    void updatesAvailable() const;
protected:
    bool m_emitSignalIfNoNewVersion;
    QNetworkAccessManager m_WebCtrl;
    LC_ReleaseInfo getOwnReleaseInfo(const QString& tagName, bool preRelease) const;
    LC_TagInfo parseTagInfo(const QString &tagName) const;

    LC_ReleaseInfo m_ownReleaseInfo;
    LC_ReleaseInfo m_latestRelease;
    LC_ReleaseInfo m_latestPreRelease;
protected slots:
    void infoReceived(QNetworkReply* pReply);
    void processReleasesJSON(const QByteArray &responseContent);
    void sortReleasesInfo(QVector<LC_ReleaseInfo> &list) const;
};

#endif // LC_RELEASECHECKER_H
