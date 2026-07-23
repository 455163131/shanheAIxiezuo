#pragma once

#include <QString>

// 系统凭据库封装。
//
// - Windows：用 DPAPI（crypt32.dll，随用户登录凭据加密，与 Windows 凭据管理器同机制）
//   加密 API Key，注册表只存密文 blob。不再是明文、也不再是弱混淆。
// - 非 Windows / DPAPI 不可用：available() 返回 false，调用方应回退到混淆存储。
//
// 设计取舍：
// - 用 QLibrary 运行时加载 crypt32.dll，不引入链接期依赖——非 Windows 构建零负担、零失败。
// - 不引入 QtKeychain 等第三方依赖，避免给本就无网络依赖的桌面程序增加体积与供应链面。
class WindowsCredentialStore {
public:
    // 当前平台是否可用系统凭据库（Windows + crypt32 可加载）
    static bool available();

    // 用系统凭据库加密明文，输出二进制 blob（调用方自行 base64 落盘）
    static bool protect(const QString &plaintext, QByteArray &outBlob);

    // 解密 protect 产出的 blob，恢复明文
    static bool unprotect(const QByteArray &blob, QString &outPlaintext);
};
