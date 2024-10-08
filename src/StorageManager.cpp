#include "StorageManager.hpp"

#include "Common.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <ranges>

StorageManager::StorageManager()
    : m_client(std::make_unique<Client>())
    , m_locale(std::make_unique<Locale>())
    , m_settings(std::make_unique<Settings>())
{
    connect(m_client.get(), SIGNAL(result(td::td_api::Object *)), this, SLOT(handleResult(td::td_api::Object *)));
}

StorageManager &StorageManager::instance()
{
    static StorageManager staticObject;
    return staticObject;
}

Client *StorageManager::client() const noexcept
{
    return m_client.get();
}

Locale *StorageManager::locale() const noexcept
{
    return m_locale.get();
}

Settings *StorageManager::settings() const noexcept
{
    return m_settings.get();
}

std::vector<int64_t> StorageManager::chatIds() const noexcept
{
    auto view = m_chats | std::views::keys;
    return std::vector<int64_t>(view.begin(), view.end());
}

const td::td_api::basicGroup *StorageManager::basicGroup(qint64 groupId) const noexcept
{
    return getPointer(m_basicGroup, groupId);
}

const td::td_api::basicGroupFullInfo *StorageManager::basicGroupFullInfo(qint64 groupId) const noexcept
{
    return getPointer(m_basicGroupFullInfo, groupId);
}

const td::td_api::chat *StorageManager::chat(qint64 chatId) const noexcept
{
    return getPointer(m_chats, chatId);
}

const td::td_api::file *StorageManager::file(qint32 fileId) const noexcept
{
    return getPointer(m_files, fileId);
}

QVariant StorageManager::option(const QString &name) const noexcept
{
    if (auto it = m_options.find(name); it != m_options.end())
        return it.value();

    return QVariant();
}

const td::td_api::supergroup *StorageManager::supergroup(qint64 groupId) const noexcept
{
    return getPointer(m_supergroup, groupId);
}

const td::td_api::supergroupFullInfo *StorageManager::supergroupFullInfo(qint64 groupId) const noexcept
{
    return getPointer(m_supergroupFullInfo, groupId);
}

const td::td_api::user *StorageManager::user(qint64 userId) const noexcept
{
    return getPointer(m_users, userId);
}

const td::td_api::userFullInfo *StorageManager::userFullInfo(qint64 userId) const noexcept
{
    return getPointer(m_userFullInfo, userId);
}

const std::vector<const td::td_api::chatFolderInfo *> &StorageManager::chatFolders() const noexcept
{
    return m_chatFolders;
}

const std::vector<const td::td_api::countryInfo *> &StorageManager::countries() const noexcept
{
    return m_countries;
}

const std::vector<const td::td_api::languagePackInfo *> &StorageManager::languagePackInfo() const noexcept
{
    return m_languagePackInfo;
}

void StorageManager::setCountries(td::td_api::object_ptr<td::td_api::countries> &&value) noexcept
{
    m_countries.reserve(value->countries_.size());
    std::ranges::transform(value->countries_, std::back_inserter(m_countries), [](const auto &countries) { return countries.get(); });

    emit countriesChanged();
}

void StorageManager::setLanguagePackInfo(td::td_api::object_ptr<td::td_api::localizationTargetInfo> &&value) noexcept
{
    m_languagePackInfo.reserve(value->language_packs_.size());
    std::ranges::transform(value->language_packs_, std::back_inserter(m_languagePackInfo), [](const auto &languagePack) { return languagePack.get(); });

    emit languagePackInfoChanged();
}

qint64 StorageManager::myId() const noexcept
{
    if (const auto value = option("my_id"); not value.isNull())
        return value.toLongLong();

    return 0;
}

void StorageManager::handleResult(td::td_api::Object *object)
{
    td::td_api::downcast_call(
        *object,
        detail::Overloaded{
            [this](td::td_api::updateNewChat &value) { m_chats.emplace(value.chat_->id_, std::move(value.chat_)); },
            [this](td::td_api::updateChatTitle &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->title_ = value.title_;
                    emit chatItemUpdated(value.chat_id_);
                }
            },
            [this](td::td_api::updateChatPhoto &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->photo_ = std::move(value.photo_);
                    emit chatItemUpdated(value.chat_id_);
                }
            },
            [this](td::td_api::updateChatPermissions &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->permissions_ = std::move(value.permissions_);
                    emit chatItemUpdated(value.chat_id_);
                }
            },
            [this](td::td_api::updateChatLastMessage &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->last_message_ = std::move(value.last_message_);
                    emit chatItemUpdated(value.chat_id_);
                }

                setChatPositions(value.chat_id_, std::move(value.positions_));
            },
            [this](td::td_api::updateChatPosition &value) {
                std::vector<td::td_api::object_ptr<td::td_api::chatPosition>> result;
                result.emplace_back(std::move(value.position_));
                setChatPositions(value.chat_id_, std::move(result));
            },
            [this](td::td_api::updateChatReadInbox &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->last_read_inbox_message_id_ = value.last_read_inbox_message_id_;
                    it->second->unread_count_ = value.unread_count_;
                    emit chatItemUpdated(value.chat_id_);
                }
            },
            [this](td::td_api::updateChatReadOutbox &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->last_read_outbox_message_id_ = value.last_read_outbox_message_id_;
                    emit chatItemUpdated(value.chat_id_);
                }
            },
            [this](td::td_api::updateChatActionBar &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->action_bar_ = std::move(value.action_bar_);
                    emit chatItemUpdated(value.chat_id_);
                }
            },
            [this](td::td_api::updateChatDraftMessage &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->draft_message_ = std::move(value.draft_message_);
                    emit chatItemUpdated(value.chat_id_);
                }

                setChatPositions(value.chat_id_, std::move(value.positions_));
            },
            [this](td::td_api::updateChatNotificationSettings &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->notification_settings_ = std::move(value.notification_settings_);
                    emit chatItemUpdated(value.chat_id_);
                }
            },
            [this](td::td_api::updateChatReplyMarkup &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->reply_markup_message_id_ = value.reply_markup_message_id_;
                    emit chatItemUpdated(value.chat_id_);
                }
            },
            [this](td::td_api::updateChatUnreadMentionCount &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->unread_mention_count_ = value.unread_mention_count_;
                    emit chatItemUpdated(value.chat_id_);
                }
            },
            [this](td::td_api::updateChatIsMarkedAsUnread &value) {
                if (auto it = m_chats.find(value.chat_id_); it != m_chats.end())
                {
                    it->second->is_marked_as_unread_ = value.is_marked_as_unread_;
                    emit chatItemUpdated(value.chat_id_);
                }
            },
            [this](td::td_api::updateUser &value) { m_users.emplace(value.user_->id_, std::move(value.user_)); },
            [this](td::td_api::updateBasicGroup &value) { m_basicGroup.emplace(value.basic_group_->id_, std::move(value.basic_group_)); },
            [this](td::td_api::updateSupergroup &value) { m_supergroup.emplace(value.supergroup_->id_, std::move(value.supergroup_)); },
            [this](td::td_api::updateUserFullInfo &value) { m_userFullInfo.emplace(value.user_id_, std::move(value.user_full_info_)); },
            [this](td::td_api::updateBasicGroupFullInfo &value) {
                m_basicGroupFullInfo.emplace(value.basic_group_id_, std::move(value.basic_group_full_info_));
            },
            [this](td::td_api::updateSupergroupFullInfo &value) { m_supergroupFullInfo.emplace(value.supergroup_id_, std::move(value.supergroup_full_info_)); },
            [this](td::td_api::updateChatFolders &value) {
                m_chatFolders.reserve(value.chat_folders_.size());
                std::ranges::transform(value.chat_folders_, std::back_inserter(m_chatFolders), [](const auto &chatFolder) { return chatFolder.get(); });

                emit chatFoldersChanged();
            },
            [this](td::td_api::updateFile &value) { m_files.emplace(value.file_->id_, std::move(value.file_)); },
            [this](td::td_api::updateOption &) {},
            [](auto &) {}});
}

void StorageManager::setChatPositions(qint64 chatId, std::vector<td::td_api::object_ptr<td::td_api::chatPosition>> &&positions) noexcept
{
    auto it = m_chats.find(chatId);
    if (it == m_chats.end())
    {
        return;  // Early return if chatId is not found
    }

    auto &currentPositions = it->second->positions_;

    // Reserve capacity if known or estimated
    currentPositions.reserve(currentPositions.size() + positions.size());

    ChatListComparator comparator;

    for (auto &&position : positions)
    {
        // Remove existing positions that match the new position
        std::erase_if(currentPositions, [&](const auto &value) { return comparator(value->list_, position->list_); });

        // Add new position
        currentPositions.emplace_back(std::move(position));
    }

    emit chatPositionUpdated(chatId);
}
