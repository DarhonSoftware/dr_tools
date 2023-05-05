#include "about.h"
#include <QtGui>
#include <QtQuick>
#include "dr_profile.h"
#include "dr_version.h"
#include "global.h"
#include "globalobject.h"

CAbout::CAbout(QObject *pObject)
    : QObject(pObject)
{
    init();
}

CAbout::~CAbout() {}

void CAbout::init()
{
    setIconAbout("qrc:/qt/qml/darhon/images/about.png");
    setIconBack("qrc:/qt/qml/darhon/images/button-back.png");
    setAppName(QString("%1 %2 (Qt - %3)")
                   .arg("AppName")
                   .arg(qApp->applicationVersion())
                   .arg(QT_VERSION_STR));
    setCaption(tr("Short description"));
    setDescription(tr("Long description"));
    setIconDarhon("qrc:/qt/qml/darhon/images/darhon.png");
    setCopyright(tr("Copyright 2010-20xx - Darhon Software"));
    setSupport(tr("Help & Support") + " - " + QCoreApplication::organizationDomain());
    setPrivacy(QString("<a href='http://www.darhon.com/..'>%1</a>").arg(tr("Privacy Policy")));
    setAppCode(g_appCode());

#ifdef SILVER_VERSION
    QSettings Settings;
    Settings.beginGroup("Version");
    QString sActCode = Settings.value("key1", "X").toString();
    Settings.endGroup();
    if (g_silverVersion(sActCode)) {
        setVersion(tr("Silver version"));
        setActCode("");
    } else {
        setVersion(tr("Gold version"));
        setActCode(sActCode);
    }
    setCodesEnabled(true);
#endif

#ifdef GOLD_VERSION
    setVersion(tr("Gold version"));
    setActCode("");
    setCodesEnabled(false);
#endif
}

bool CAbout::save(const QString &sActCode)
{
    if (!g_silverVersion(sActCode)) {
        QSettings Settings;
        Settings.beginGroup("Version");
        Settings.setValue("key1", sActCode.toUpper());
        Settings.endGroup();
        setActCode(sActCode.toUpper());
        setVersion(tr("Gold version"));
        return true;
    }
    return false;
}
