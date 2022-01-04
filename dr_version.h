#ifndef DR_VERSION_H
#define DR_VERSION_H

#include <QString>
#include <QtGlobal>

/**** Application Code | Types ****/

//10000000. Device specific
//01000000. Version specific
//00100000. Monthly subscription specific
//00200000. Yearly subscription specific

/***** Global constats *****/

#define TITLE_FLR   QT_TRANSLATE_NOOP("Global","Silver version Restriction")
#define TEXT_FLR    QT_TRANSLATE_NOOP("Global","Upgrade to Gold version to access all features.")

/***** Global enums *****/


/***** Global functions *****/

bool g_silverVersion(const QString& sNewActCode=QString(),const QString &sType="10000000");
void g_registerGoldVersion(const QString &sActCode);
QString g_appCode(const QString &sType="10000000", bool bCurrentDate=true);
QString g_actCodeToAppCode(const QString& sActCode);
QString g_appCodeToActCode(const QString& sAppCode);

#endif // DR_VERSION_H
