#include "MemoryUserRepository.h"

MemoryUserRepository::MemoryUserRepository() {}

void MemoryUserRepository::save(const User &user)
{
    if (m_users.contains(user.id())) {
        QString oldUsername = m_users[user.id()].username();
        m_usernameToId.remove(oldUsername);
    }
    m_users[user.id()] = user;
    m_usernameToId[user.username()] = user.id();
}

std::optional<User> MemoryUserRepository::findByUsername(const QString &username)
{
    if (!m_usernameToId.contains(username)) {
        return std::nullopt;
    }
    return m_users.value(m_usernameToId[username]);
}

std::optional<User> MemoryUserRepository::findById(const QString &id)
{
    if (!m_users.contains(id)) {
        return std::nullopt;
    }
    return m_users.value(id);
}

QList<User> MemoryUserRepository::getOnlineUsers()
{
    QList<User> online;
    for (const User &user : m_users) {
        if (user.isOnline()) {
            online.append(user);
        }
    }
    return online;
}

void MemoryUserRepository::setOnline(const QString &id, bool online)
{
    if (!m_users.contains(id)) return;
    User user = m_users[id];
    user.setOnline(online);
    m_users[id] = user;
}

QList<User> MemoryUserRepository::getAllUsers()
{
    return m_users.values();
}