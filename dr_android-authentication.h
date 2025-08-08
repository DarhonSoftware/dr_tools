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
#include <QJniObject>
#include <QObject>
#include <qqmlintegration.h>

class CAndroidAuthentication : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int authenticators READ authenticators WRITE setAuthenticators)
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString description READ description WRITE setDescription)
    Q_PROPERTY(QString negativeButton READ negativeButton WRITE setNegativeButton)

    const QJniObject m_JniJavaAuthentication;
    static CAndroidAuthentication *m_pInstance;
    QString m_sTitle, m_sDescription, m_mNegativeButton;
    int m_iAuthenticators;

public:
    enum AUTHENTICATOR_TYPE { BIOMETRIC_STRONG = 0x01, BIOMETRIC_WEAK = 0x02, DEVICE_CREDENTIAL = 0x04 };
    Q_ENUM(AUTHENTICATOR_TYPE)

    enum BIOMETRIC_STATUS {
        BIOMETRIC_STATUS_UNKNOWN = 0,
        BIOMETRIC_SUCCESS,
        BIOMETRIC_ERROR_NO_HARDWARE,
        BIOMETRIC_ERROR_HW_UNAVAILABLE,
        BIOMETRIC_ERROR_NONE_ENROLLED,
        BIOMETRIC_ERROR_SECURITY_UPDATE_REQUIRED,
        BIOMETRIC_ERROR_UNSUPPORTED
    };
    Q_ENUM(BIOMETRIC_STATUS)

    CAndroidAuthentication(QObject *parent = nullptr);

    Q_INVOKABLE int canAuthenticate();
    Q_INVOKABLE bool requestBiometricEnroll();
    Q_INVOKABLE bool authenticate();
    Q_INVOKABLE void cancel();

    int authenticators() const;
    void setAuthenticators(int authenticators);
    QString title() const;
    void setTitle(const QString &title);
    QString description() const;
    void setDescription(const QString &description);
    QString negativeButton() const;
    void setNegativeButton(const QString &negativeButton);

Q_SIGNALS:
    void error(int errorCode, const QString &errString);
    void succeeded();
    void failed();
    void cancelled();

private:
    static void authenticationError(JNIEnv *env, jobject thiz, jint errorCode, jstring errString);
    static void authenticationSucceeded(JNIEnv *env, jobject thiz);
    static void authenticationFailed(JNIEnv *env, jobject thiz);
    static void authenticationCancelled(JNIEnv *env, jobject thiz);
};

#else //Q_OS_ANDROID
#include <QObject>
#include <qqmlintegration.h>

class CAndroidAuthentication : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int authenticators READ authenticators WRITE setAuthenticators)
    Q_PROPERTY(QString title READ title WRITE setTitle)
    Q_PROPERTY(QString description READ description WRITE setDescription)
    Q_PROPERTY(QString negativeButton READ negativeButton WRITE setNegativeButton)

public:
    enum AUTHENTICATOR_TYPE { BIOMETRIC_STRONG = 0x01, BIOMETRIC_WEAK = 0x02, DEVICE_CREDENTIAL = 0x04 };
    Q_ENUM(AUTHENTICATOR_TYPE)

    CAndroidAuthentication(QObject *parent = nullptr) {}

    Q_INVOKABLE int canAuthenticate() { return 0; }
    Q_INVOKABLE bool requestBiometricEnroll() { return true; }
    Q_INVOKABLE bool authenticate() { return true; }
    Q_INVOKABLE void cancel() {}

    int authenticators() const { return 0; }
    void setAuthenticators(int authenticators) {}
    QString title() const { return ""; }
    void setTitle(const QString &title) {}
    QString description() const { return ""; }
    void setDescription(const QString &description) {}
    QString negativeButton() const { return ""; }
    void setNegativeButton(const QString &negativeButton) {}

signals:
    void error(int errorCode, const QString &errString);
    void succeeded();
    void failed();
    void cancelled();
};

#endif
