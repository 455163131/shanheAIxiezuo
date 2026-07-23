#include "windowscredentialstore.h"

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#include <QLibrary>
#endif

bool WindowsCredentialStore::available()
{
#ifdef Q_OS_WIN
    // 仅探测 dll 可加载即可；真正加密在 protect/unprotect 内用函数指针调用
    QLibrary lib(QStringLiteral("crypt32"));
    return lib.load();
#else
    return false;
#endif
}

bool WindowsCredentialStore::protect(const QString &plaintext, QByteArray &outBlob)
{
#ifdef Q_OS_WIN
    typedef struct {
        DWORD cbData;
        BYTE  *pbData;
    } Blob;
    typedef BOOL (WINAPI *CryptProtectDataPtr)(
        Blob *pDataIn,
        LPCWSTR szDataDescr,
        Blob *pOptionalEntropy,
        PVOID pvReserved,
        PVOID pPromptStruct,
        DWORD dwFlags,
        Blob *pDataOut);

    QLibrary lib(QStringLiteral("crypt32"));
    if (!lib.load())
        return false;
    auto cryptProtect = reinterpret_cast<CryptProtectDataPtr>(lib.resolve("CryptProtectData"));
    if (!cryptProtect)
        return false;

    const QByteArray utf8 = plaintext.toUtf8();
    Blob in{ static_cast<DWORD>(utf8.size()),
             reinterpret_cast<BYTE *>(const_cast<char *>(utf8.constData())) };
    Blob out{ 0, nullptr };

    if (!cryptProtect(&in, nullptr, nullptr, nullptr, nullptr, 0, &out))
        return false;

    outBlob = QByteArray(reinterpret_cast<char *>(out.pbData),
                         static_cast<int>(out.cbData));
    ::LocalFree(out.pbData);
    return true;
#else
    Q_UNUSED(plaintext)
    Q_UNUSED(outBlob)
    return false;
#endif
}

bool WindowsCredentialStore::unprotect(const QByteArray &blob, QString &outPlaintext)
{
#ifdef Q_OS_WIN
    typedef struct {
        DWORD cbData;
        BYTE  *pbData;
    } Blob;
    typedef BOOL (WINAPI *CryptUnprotectDataPtr)(
        Blob *pDataIn,
        LPWSTR *ppszDataDescr,
        Blob *pOptionalEntropy,
        PVOID pvReserved,
        PVOID pPromptStruct,
        DWORD dwFlags,
        Blob *pDataOut);

    QLibrary lib(QStringLiteral("crypt32"));
    if (!lib.load())
        return false;
    auto cryptUnprotect = reinterpret_cast<CryptUnprotectDataPtr>(lib.resolve("CryptUnprotectData"));
    if (!cryptUnprotect)
        return false;

    Blob in{ static_cast<DWORD>(blob.size()),
             reinterpret_cast<BYTE *>(const_cast<char *>(blob.constData())) };
    Blob out{ 0, nullptr };

    if (!cryptUnprotect(&in, nullptr, nullptr, nullptr, nullptr, 0, &out))
        return false;

    outPlaintext = QString::fromUtf8(reinterpret_cast<char *>(out.pbData),
                                     static_cast<int>(out.cbData));
    ::LocalFree(out.pbData);
    return true;
#else
    Q_UNUSED(blob)
    Q_UNUSED(outPlaintext)
    return false;
#endif
}
