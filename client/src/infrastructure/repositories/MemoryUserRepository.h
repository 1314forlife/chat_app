#ifndef MEMORYUSERREPOSITORY_H
#define MEMORYUSERREPOSITORY_H

#include <QMap>
#include <QList>
#include <optional>
#include "domain/repositories/IUserRepository.h"
#include "domain/entities/User.h"

class MemoryUserRepository : public IUserRepository
{
public:
    MemoryUserRepository();

    void save(const User &user) override;
    std::optional<User> findByUsername(const QString &username) override;
    std::optional<User> findById(const QString &id) override;
    QList<User> getOnlineUsers() override;
    void setOnline(const QString &id, bool online) override;
    QList<User> getAllUsers() override;

private:
    QMap<QString, User> m_users;
    QMap<QString, QString> m_usernameToId;
};

#endif