//Release 1
#include <QtCore>
#ifdef Q_OS_ANDROID
#include <QtCore/private/qandroidextras_p.h> //This namespace is under development and is subject to change
#include "dr_android.h"

QString AndroidDR::standardPath(const QString &sFolderType, const QString &sFolderEnd)
{
    QString sDir = "";

    QJniObject JniString = QJniObject::getStaticObjectField<jstring>("android/os/Environment",
                                                                     sFolderType.toLatin1());
    QJniObject JniFile = QJniObject::callStaticObjectMethod("android/os/Environment",
                                                            "getExternalStoragePublicDirectory",
                                                            "(Ljava/lang/String;)Ljava/io/File;",
                                                            JniString.object<jstring>());
    sDir = QString("%1/%2").arg(JniFile.callObjectMethod<jstring>("getAbsolutePath").toString(),
                                sFolderEnd);

    return sDir;
}

bool AndroidDR::setPermissions(const QStringList &lsPermissions)
{
    // BEGIN This namespace is under development and is subject to change

    QFuture<QtAndroidPrivate::PermissionResult> PermissionResult;
    QStringList lsPermissionsNotGranted;

    //Select permissions not granted
    for (int i = 0; i < lsPermissions.size(); i++) {
        PermissionResult = QtAndroidPrivate::checkPermission(lsPermissions.at(i));
        if (PermissionResult.result() == QtAndroidPrivate::PermissionResult::Denied)
            lsPermissionsNotGranted.append(lsPermissions.at(i));
    }

    //Request necessary permissions
    bool bGranted = true;
    for (int i = 0; i < lsPermissionsNotGranted.size(); i++) {
        PermissionResult = QtAndroidPrivate::requestPermission(lsPermissionsNotGranted.at(i));
        bGranted &= PermissionResult.result() == QtAndroidPrivate::PermissionResult::Authorized;

        if (!bGranted)
            return false;
    }

    return true;

    // END This namespace is under development and is subject to change
}

QString AndroidDR::translatePathFromUri(const QString &sUri)
{
    QString sPath;

    QJniObject JniString = QJniObject::fromString(sUri);
    QJniObject JniUri = QJniObject::callStaticObjectMethod("java/net/URI",
                                                           "create",
                                                           "(Ljava/lang/String;)Ljava/net/URI;",
                                                           JniString.object<jstring>());

    QJniObject JniPath = QJniObject::callStaticObjectMethod("java/nio/file/spi/FileSystemProvider",
                                                            "getPath",
                                                            "(Ljava/net/URI;)Ljava/nio/file/Path;",
                                                            JniUri.object<jobject>());

    sPath = JniPath.callObjectMethod<jstring>("toString").toString();
    return sPath;
}

#endif
