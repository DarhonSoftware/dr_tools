//Release 4
#ifndef DR_ANDROID_H
#define DR_ANDROID_H
#include <QtCore/qglobal.h>
#ifdef Q_OS_ANDROID

namespace AndroidDR {

QString standardPath(const QString &sFolderType, const QString &sFolderEnd);
bool requestPermissions(const QStringList &lsPermissions);
QString translatePathFromUri(const QString &sUri);

} // namespace AndroidDR

#endif // Q_OS_ANDROID
#endif // DR_ANDROID_H
