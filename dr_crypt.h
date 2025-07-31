//Release 2
#ifndef DR_CRYPT_H
#define DR_CRYPT_H

#include <QByteArray>

class QString;

class CCrypt
{
public:
    enum CompressionMode { CompressionYes, CompressionNo };
    enum EncryptionMode { SIM128, SIM256, AES128_CFB, AES256_CFB, AES128_OFB, AES256_OFB };
    enum Error {
        ErrorNoError,          // No error occurred.
        ErrorNoPassSet,        // No pass was set. You can not encrypt or decrypt without a valid password.
        ErrorNoMarkSet,        // No mark was set. You can not encrypt or decrypt without a valid mark.
        ErrorUnknownVersion,   // The version of this data is unknown, or the data is otherwise not valid.
        ErrorIntegrityFailed,  // The integrity check of the data failed. Perhaps the wrong key was used.
        ErrorArrayTooShort,    // Either cypher or plaintext are smaller then minimum accepted
        ErrorAlreadyProcessed, // Already encrypted or decrypted
        ErrorFileNotFound,     // None existing file to be processed
        ErrorWrongPass         // Password is wrong
    };

    CCrypt();
    CCrypt(const QByteArray &BAPass, const QByteArray &BAMark);
    void setMark(const QByteArray &BAMark);
    QByteArray mark() { return m_BAMark; }
    void setPass(const QByteArray &BAPass) { m_BAPass = BAPass; }
    QByteArray pass() { return m_BAPass; }
    void setCompressionMode(CompressionMode iMode) { m_iCompressionMode = iMode; }
    CompressionMode compressionMode() const { return m_iCompressionMode; }
    void setEncryptionMode(EncryptionMode iMode) { m_iEncryptionMode = iMode; }
    EncryptionMode encryptionMode() const { return m_iEncryptionMode; }
    Error lastError() const { return m_iLastError; }
    QByteArray encryptArray(const QByteArray &BAPlainText);
    QByteArray decryptArray(const QByteArray &BACypher);
    bool encryptFile(const QString &sFile);
    bool decryptFile(const QString &sFile);

    //Compatibility for version 0x01
    void setOldKey(const QByteArray &BAOldKey) { m_BAOldKey = BAOldKey; }

private:
    enum Action { ActionEncrypt, ActionDecrypt };

    QByteArray m_BAPass;
    QByteArray m_BAMark;
    CompressionMode m_iCompressionMode;
    EncryptionMode m_iEncryptionMode;
    Error m_iLastError;

    //Compatibility for version 0x01
    QByteArray m_BAOldKey;

    bool processFile(const QString &sFile, Action iAction);
};

/**************  File structure *******************

Version 1 (cypher size = 31)
1   Random
20  Integrity Hash
1   Plain text
1   Version
8   Mark

Version 2 (cypher size = 59)
1   Random
20  Integrity Hash
1   Plain text
20  Password hash [salt+pass]
8   Salt
1   Version
8   Mark

*******************************************************/

#endif // DR_CRYPT_H
