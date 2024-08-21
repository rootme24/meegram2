#pragma once

#include "Client.hpp"
#include "Localization.hpp"
#include "Settings.hpp"

#include <td/telegram/td_api.h>

#include <QObject>
#include <QVariant>

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class StorageManager : public QObject
{
    Q_OBJECT
public:
    static StorageManager &instance();

    StorageManager(const StorageManager &) = delete;
    StorageManager &operator=(const StorageManager &) = delete;

    [[nodiscard]] Client *client() const noexcept;
    [[nodiscard]] Locale *locale() const noexcept;
    [[nodiscard]] Settings *settings() const noexcept;

    [[nodiscard]] std::vector<int64_t> chatIds() const noexcept;

    [[nodiscard]] const td::td_api::basicGroup *basicGroup(qint64 groupId) const noexcept;
    [[nodiscard]] const td::td_api::basicGroupFullInfo *basicGroupFullInfo(qint64 groupId) const noexcept;
    [[nodiscard]] const td::td_api::chat *chat(qint64 chatId) const noexcept;
    [[nodiscard]] const td::td_api::file *file(qint32 fileId) const noexcept;
    QVariant option(const QString &name) const noexcept;
    [[nodiscard]] const td::td_api::supergroup *supergroup(qint64 groupId) const noexcept;
    [[nodiscard]] const td::td_api::supergroupFullInfo *supergroupFullInfo(qint64 groupId) const noexcept;
    [[nodiscard]] const td::td_api::user *user(qint64 userId) const noexcept;
    [[nodiscard]] const td::td_api::userFullInfo *userFullInfo(qint64 userId) const noexcept;

    [[nodiscard]] const std::vector<const td::td_api::chatFolderInfo *> &chatFolders() const noexcept;
    [[nodiscard]] const std::vector<const td::td_api::countryInfo *> &countries() const noexcept;
    [[nodiscard]] const std::vector<const td::td_api::languagePackInfo *> &languagePackInfo() const noexcept;

    void setCountries(td::td_api::object_ptr<td::td_api::countries> &&value) noexcept;
    void setLanguagePackInfo(td::td_api::object_ptr<td::td_api::localizationTargetInfo> &&value) noexcept;

    [[nodiscard]] qint64 myId() const noexcept;

signals:
    void chatItemUpdated(qint64 chatId);
    void chatPositionUpdated(qint64 chatId);

    void chatFoldersChanged();
    void countriesChanged();
    void languagePackInfoChanged();

private slots:
    void handleResult(td::td_api::Object *object);

private:
    StorageManager();

    template <typename Map, typename Key>
    [[nodiscard]] static const typename Map::mapped_type::element_type *getPointer(const Map &map, const Key &key) noexcept
    {
        auto it = map.find(key);
        return (it != map.end()) ? it->second.get() : nullptr;
    }

    void setChatPositions(qint64 chatId, std::vector<td::td_api::object_ptr<td::td_api::chatPosition>> &&positions) noexcept;

    QVariantMap m_options;

    std::unique_ptr<Client> m_client;
    std::unique_ptr<Locale> m_locale;
    std::unique_ptr<Settings> m_settings;

    std::vector<const td::td_api::chatFolderInfo *> m_chatFolders;
    std::vector<const td::td_api::countryInfo *> m_countries;
    std::vector<const td::td_api::languagePackInfo *> m_languagePackInfo;

    std::unordered_map<int64_t, td::td_api::object_ptr<td::td_api::basicGroup>> m_basicGroup;
    std::unordered_map<int64_t, td::td_api::object_ptr<td::td_api::basicGroupFullInfo>> m_basicGroupFullInfo;
    std::unordered_map<int64_t, td::td_api::object_ptr<td::td_api::chat>> m_chats;
    std::unordered_map<int32_t, td::td_api::object_ptr<td::td_api::file>> m_files;
    std::unordered_map<int64_t, td::td_api::object_ptr<td::td_api::supergroup>> m_supergroup;
    std::unordered_map<int64_t, td::td_api::object_ptr<td::td_api::supergroupFullInfo>> m_supergroupFullInfo;
    std::unordered_map<int64_t, td::td_api::object_ptr<td::td_api::user>> m_users;
    std::unordered_map<int64_t, td::td_api::object_ptr<td::td_api::userFullInfo>> m_userFullInfo;
};
