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
    setIconAbout("qrc:/others/icons/about.png");
    setIconBack("qrc:/others/icons/button-back.png");
    setAppName(QString("%1 %2 (Qt - %3)")
                   .arg("mobiFinance")
                   .arg(qApp->applicationVersion())
                   .arg(QT_VERSION_STR));
    setCaption(tr("Manage your Personal Accounts"));
    setDescription(tr("mobiFinance is a simple to use and yet powerful software packed with all "
                      "the necessary tools to control your accounts."));
    setIconDarhon("qrc:/others/icons/darhon.png");
    setCopyright(tr("Copyright 2010-2014 - Darhon Software"));
    setSupport(tr("Help & Support") + " - " + QCoreApplication::organizationDomain());
    setPrivacy(QString("<a href='http://www.darhon.com/faq/284'>%1</a>").arg(tr("Privacy Policy")));
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
