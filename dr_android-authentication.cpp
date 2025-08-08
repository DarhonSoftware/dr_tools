//Release 1
/*
 *   PROJECT: https://github.com/FalsinSoft/QtAndroidTools
 *
 *  Copyright (c) 2018 Fabio Falsini <falsinsoft@gmail.com>
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
*/

#include <QtCore/qglobal.h>
#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include "dr_android-authentication.h"

CAndroidAuthentication *CAndroidAuthentication::m_pInstance = nullptr;

CAndroidAuthentication::CAndroidAuthentication(QObject *parent)
    : QObject(parent)
    , m_JniJavaAuthentication("com/darhon/androidtools/AndroidAuthentication", QNativeInterface::QAndroidApplication::context())
    , m_iAuthenticators(0)
{
    m_pInstance = this;

    if (m_JniJavaAuthentication.isValid()) {
        const JNINativeMethod jniMethod[]
            = {{"authenticationError", "(ILjava/lang/String;)V", reinterpret_cast<void *>(&CAndroidAuthentication::authenticationError)},
               {"authenticationSucceeded", "()V", reinterpret_cast<void *>(&CAndroidAuthentication::authenticationSucceeded)},
               {"authenticationFailed", "()V", reinterpret_cast<void *>(&CAndroidAuthentication::authenticationFailed)},
               {"authenticationCancelled", "()V", reinterpret_cast<void *>(&CAndroidAuthentication::authenticationCancelled)}};
        QJniEnvironment jniEnv;
        jclass objectClass;

        objectClass = jniEnv->GetObjectClass(m_JniJavaAuthentication.object<jobject>());
        jniEnv->RegisterNatives(objectClass, jniMethod, sizeof(jniMethod) / sizeof(JNINativeMethod));
        jniEnv->DeleteLocalRef(objectClass);
    }
}

int CAndroidAuthentication::authenticators() const
{
    return m_iAuthenticators;
}

void CAndroidAuthentication::setAuthenticators(int authenticators)
{
    if (m_JniJavaAuthentication.isValid()) {
        m_JniJavaAuthentication.callMethod<void>("setAuthenticators", "(I)V", authenticators);
        m_iAuthenticators = authenticators;
    }
}

QString CAndroidAuthentication::title() const
{
    return m_sTitle;
}

void CAndroidAuthentication::setTitle(const QString &title)
{
    if (m_JniJavaAuthentication.isValid()) {
        m_JniJavaAuthentication.callMethod<void>("setTitle", "(Ljava/lang/String;)V", QJniObject::fromString(title).object<jstring>());
        m_sTitle = title;
    }
}

QString CAndroidAuthentication::description() const
{
    return m_sDescription;
}

void CAndroidAuthentication::setDescription(const QString &description)
{
    if (m_JniJavaAuthentication.isValid()) {
        m_JniJavaAuthentication.callMethod<void>("setDescription", "(Ljava/lang/String;)V", QJniObject::fromString(description).object<jstring>());
        m_sDescription = description;
    }
}

QString CAndroidAuthentication::negativeButton() const
{
    return m_mNegativeButton;
}

void CAndroidAuthentication::setNegativeButton(const QString &negativeButton)
{
    if (m_JniJavaAuthentication.isValid()) {
        m_JniJavaAuthentication.callMethod<void>("setNegativeButton",
                                                 "(Ljava/lang/String;)V",
                                                 QJniObject::fromString(negativeButton).object<jstring>());
        m_mNegativeButton = negativeButton;
    }
}

int CAndroidAuthentication::canAuthenticate()
{
    if (m_JniJavaAuthentication.isValid()) {
        return m_JniJavaAuthentication.callMethod<jint>("canAuthenticate");
    }

    return BIOMETRIC_STATUS_UNKNOWN;
}

bool CAndroidAuthentication::requestBiometricEnroll()
{
    if (m_JniJavaAuthentication.isValid()) {
        return m_JniJavaAuthentication.callMethod<jboolean>("requestBiometricEnroll");
    }

    return false;
}

bool CAndroidAuthentication::authenticate()
{
    if (m_JniJavaAuthentication.isValid()) {
        return m_JniJavaAuthentication.callMethod<jboolean>("authenticate");
    }

    return false;
}

void CAndroidAuthentication::cancel()
{
    if (m_JniJavaAuthentication.isValid()) {
        m_JniJavaAuthentication.callMethod<void>("cancel");
    }
}

void CAndroidAuthentication::authenticationError(JNIEnv *env, jobject thiz, jint errorCode, jstring errString)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)

    if (m_pInstance != nullptr) {
        Q_EMIT m_pInstance->error(errorCode, QJniObject(errString).toString());
    }
}

void CAndroidAuthentication::authenticationSucceeded(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)

    if (m_pInstance != nullptr) {
        Q_EMIT m_pInstance->succeeded();
    }
}

void CAndroidAuthentication::authenticationFailed(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)

    if (m_pInstance != nullptr) {
        Q_EMIT m_pInstance->failed();
    }
}

void CAndroidAuthentication::authenticationCancelled(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)

    if (m_pInstance != nullptr) {
        Q_EMIT m_pInstance->cancelled();
    }
}

#endif //Q_OS_ANDROID
