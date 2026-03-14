#pragma once

#include <QString>

#include <optional>

namespace ShirohaChat
{

class Contact;

/**
 * @brief 联系人命令侧仓库接口（聚合级别操作）
 */
class IContactRepository
{
  public:
    virtual ~IContactRepository() = default;

    virtual std::optional<Contact> load(const QString& userId, const QString& friendUserId) = 0;
    virtual bool save(const Contact& contact) = 0;
    virtual bool remove(const QString& userId, const QString& friendUserId) = 0;
};

} // namespace ShirohaChat
