#include <QByteArray>
#include <QtDebug>
#include <QtGlobal>
#include <QDateTime>
#include <QCryptographicHash>
#include <QFile>
#include <QRandomGenerator>
#include "dr_crypt.h"

#define MARK_SIZE             8
#define SALT_SIZE             8
#define MIN_CYPHER_SIZE       59
#define MIN_PLAINTEXT_SIZE    1
#define HASH_SIZE             20
#define VERSION               0x02

CCrypt::CCrypt()
{
  m_BAPass.clear();
  m_BAMark.clear();
  m_iCompressionMode=CompressionYes;
  m_iEncryptionMode=SIM128;
  m_iLastError=ErrorNoError;
}

CCrypt::CCrypt(const QByteArray& BAPass, const QByteArray &BAMark)
{
  if (BAMark.size()==MARK_SIZE)
    m_BAMark=BAMark;
  else
    m_BAMark.clear();

  m_BAPass=BAPass;
  m_iCompressionMode=CompressionYes;
  m_iEncryptionMode=SIM128;
  m_iLastError=ErrorNoError;
}

void CCrypt::setMark(const QByteArray& BAMark)
{
  if (BAMark.size()!=MARK_SIZE) return;
  m_BAMark = BAMark;
}

QByteArray CCrypt::encryptArray(const QByteArray& BAPlainText)
{
  //Validate Pass
  if (m_BAPass.isEmpty())
  {
    m_iLastError = ErrorNoPassSet;
    return QByteArray();
  }

  //Validate Mark
  if (m_BAMark.isEmpty())
  {
    m_iLastError = ErrorNoMarkSet;
    return QByteArray();
  }

  //Validate plaintext
  if (BAPlainText.size()<MIN_PLAINTEXT_SIZE)
  {
    m_iLastError = ErrorArrayTooShort;
    return QByteArray();
  }

  //Validate whether encryption was done
  if ((BAPlainText.size()>=MIN_CYPHER_SIZE) && (BAPlainText.right(MARK_SIZE)==m_BAMark))
  {
    m_iLastError = ErrorAlreadyProcessed;
    return QByteArray();
  }

  //Store plaintext in local variable for future process
  QByteArray BA = BAPlainText;

  //Maximum compression for plaintext
  if (m_iCompressionMode == CompressionYes)
    BA = qCompress(BA, 9);

  //Integrity protection 160-bit
  QByteArray BAIntegrityHash = QCryptographicHash::hash(BA,QCryptographicHash::Sha1);

  //Prepend a random char & integrity protection to the string
  char cRandom = char(QRandomGenerator::global()->bounded(0xFF));
  BA = cRandom + BAIntegrityHash + BA;

  //Create Salt
  QByteArray BASalt="";
  for (int i=0; i<8; i++) BASalt+=QRandomGenerator::global()->bounded(0xFF);

  //ENCRYPT: SIM_128
  if (m_iEncryptionMode==SIM128)
  {
    //Create Key 128
    QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);

    //Get parameters
    char cLastChar=0;
    int iSize = BA.size();

    //Encryption algorithm
    for (int i=0; i<iSize; i++)
    {
      BA[i] = BA.at(i) ^ BAKey.at(i % 16) ^ cLastChar;
      cLastChar = BA.at(i);
    }
  }

  //ENCRYPT: SIM_256
  if (m_iEncryptionMode==SIM256)
  {
    //Create Key 256
    QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);
    BAKey.append(QCryptographicHash::hash(BAKey,QCryptographicHash::Md5));

    //Get parameters
    char cLastChar=0;
    int iSize = BA.size();

    //Encryption algorithm
    for (int i=0; i<iSize; i++)
    {
      BA[i] = BA.at(i) ^ BAKey.at(i % 32) ^ cLastChar;
      cLastChar = BA.at(i);
    }
  }

  // ENCRYPT: AES-128-CFB
  if (m_iEncryptionMode==AES128_CFB)
  {
    //Create Key 128
    QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);
    Q_UNUSED(BAKey)
    //Implementation
  }

  // ENCRYPT: AES-256-CFB
  if (m_iEncryptionMode==AES256_CFB)
  {
    //Create Key 256
    QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);
    BAKey.append(QCryptographicHash::hash(BAKey,QCryptographicHash::Md5));
    //Implementation
  }

  // ENCRYPT: AES-128-OFB
  if (m_iEncryptionMode==AES128_OFB)
  {
    //Create Key 128
    QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);
    Q_UNUSED(BAKey)
    //Implementation
  }

  // ENCRYPT: AES-256-OFB
  if (m_iEncryptionMode==AES256_OFB)
  {
    //Create Key 256
    QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);
    BAKey.append(QCryptographicHash::hash(BAKey,QCryptographicHash::Md5));
    //Implementation
  }

  //Append Password Hash
  BA.append(QCryptographicHash::hash(BASalt+m_BAPass,QCryptographicHash::Sha1));

  //Append Salt
  BA.append(BASalt);

  //Append Version
  BA.append(char(VERSION));

  //Append Mark to detect encryption was done
  BA.append(m_BAMark);

  m_iLastError = ErrorNoError;
  return BA;
}

QByteArray CCrypt::decryptArray(const QByteArray& BACypher)
{
  //Validate Pass
  if (m_BAPass.isEmpty())
  {
    m_iLastError = ErrorNoPassSet;
    return QByteArray();
  }

  //Validate Mark
  if (m_BAMark.isEmpty())
  {
    m_iLastError = ErrorNoMarkSet;
    return QByteArray();
  }

  //Validate cypher
  if (BACypher.size()<MIN_CYPHER_SIZE)
  {
    m_iLastError = ErrorArrayTooShort;
    return QByteArray();
  }

  //Validate whether encryption was done
  if (BACypher.right(MARK_SIZE)!=m_BAMark)
  {
    m_iLastError = ErrorAlreadyProcessed;
    return QByteArray();
  }

  //Validate version
  char cVersion = BACypher.at(BACypher.size()-1-MARK_SIZE);
  if ((cVersion==0x00) || (cVersion > VERSION))
  {
    m_iLastError = ErrorUnknownVersion;
    return QByteArray();
  }

  //Store cypher in local variable for future process and chop off version
  QByteArray BA = BACypher.left(BACypher.size()-1-MARK_SIZE);

  /*********************/
  /***** Version 1 *****/
  /*********************/

  if (cVersion==0x01)
  {
    //Get parameters
    char cLastChar=0;
    int iSize = BA.size();

    //Dencryption algorithm
    for (int i=0; i<iSize; i++)
    {
      char cCurrentChar = BA.at(i);
      BA[i] = BA.at(i) ^ cLastChar ^ m_BAOldKey.at(i % 16);
      cLastChar = cCurrentChar;
    }
  }

  /*********************/
  /***** Version 2 *****/
  /*********************/

  if (cVersion==0x02)
  {
    //Take Salt
    QByteArray BASalt=BA.right(SALT_SIZE);
    BA.chop(SALT_SIZE);

    //Take Password Hash
    QByteArray BAPassHash=BA.right(HASH_SIZE);
    BA.chop(HASH_SIZE);

    //Validate Password
    if (BAPassHash!=QCryptographicHash::hash(BASalt+m_BAPass,QCryptographicHash::Sha1))
    {
      m_iLastError = ErrorWrongPass;
      return QByteArray();
    }

    // ENCRYPT: SIM_128
    if (m_iEncryptionMode==SIM128)
    {
      //Create Key 128
      QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);

      //Get parameters
      char cLastChar=0;
      int iSize = BA.size();

      //Dencryption algorithm
      for (int i=0; i<iSize; i++)
      {
        char cCurrentChar = BA.at(i);
        BA[i] = BA.at(i) ^ cLastChar ^ BAKey.at(i % 16);
        cLastChar = cCurrentChar;
      }
    }

    // ENCRYPT: SIM_256
    if (m_iEncryptionMode==SIM256)
    {
      //Create Key 256
      QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);
      BAKey.append(QCryptographicHash::hash(BAKey,QCryptographicHash::Md5));

      //Get parameters
      char cLastChar=0;
      int iSize = BA.size();

      //Dencryption algorithm
      for (int i=0; i<iSize; i++)
      {
        char cCurrentChar = BA.at(i);
        BA[i] = BA.at(i) ^ cLastChar ^ BAKey.at(i % 32);
        cLastChar = cCurrentChar;
      }
    }

    // ENCRYPT: AES-128-CFB
    if (m_iEncryptionMode==AES128_CFB)
    {
      //Create Key 128
      QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);
      Q_UNUSED(BAKey)
      //Implementation
    }

    // ENCRYPT: AES-256-CFB
    if (m_iEncryptionMode==AES256_CFB)
    {
      //Create Key 256
      QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);
      BAKey.append(QCryptographicHash::hash(BAKey,QCryptographicHash::Md5));
      //Implementation
    }

    // ENCRYPT: AES-128-OFB
    if (m_iEncryptionMode==AES128_OFB)
    {
      //Create Key 128
      QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);
      Q_UNUSED(BAKey)
      //Implementation
    }

    // ENCRYPT: AES-256-OFB
    if (m_iEncryptionMode==AES256_OFB)
    {
      //Create Key 256
      QByteArray BAKey = QCryptographicHash::hash(m_BAPass+BASalt,QCryptographicHash::Md5);
      BAKey.append(QCryptographicHash::hash(BAKey,QCryptographicHash::Md5));
      //Implementation
    }
  }

  /*********************/
  /*********************/

  //Chop off the random number at the start
  BA = BA.mid(1);

  //Validate integrity protection 160-bit
  if (BA.length() < HASH_SIZE + MIN_PLAINTEXT_SIZE)
  {
    m_iLastError = ErrorIntegrityFailed;
    return QByteArray();
  }

  QByteArray BAIntegrityHash = BA.left(HASH_SIZE);
  BA = BA.mid(HASH_SIZE);
  if (QCryptographicHash::hash(BA,QCryptographicHash::Sha1)!=BAIntegrityHash)
  {
    m_iLastError = ErrorIntegrityFailed;
    return QByteArray();
  }

  //Uncompression of plaintext
  if (m_iCompressionMode == CompressionYes)
    BA = qUncompress(BA);

  m_iLastError = ErrorNoError;
  return BA;
}

bool CCrypt::encryptFile(const QString& sFile)
{
  return processFile(sFile,ActionEncrypt);
}

bool CCrypt::decryptFile(const QString& sFile)
{
  return processFile(sFile,ActionDecrypt);
}

bool CCrypt::processFile(const QString& sFile, Action iAction)
{
  //Validate file name
  if ((sFile.isEmpty()) || (!QFile::exists(sFile)))
  {
    m_iLastError = ErrorFileNotFound;
    return false;
  }

  //Open and process file
  QFile File(sFile);
  File.open(QIODevice::ReadOnly);
  QByteArray BA=File.readAll();

  if (iAction==ActionEncrypt) BA=encryptArray(BA);
  if (iAction==ActionDecrypt) BA=decryptArray(BA);

  //Validate operation
  if (lastError()!=ErrorNoError)
  {
    File.close();
    return false;
  }

  //Write new encrypted file
  File.remove();
  File.open(QIODevice::WriteOnly);
  File.write(BA);
  File.close();

  return true;
}

