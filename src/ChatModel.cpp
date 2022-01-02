#include "ChatModel.hpp"

#include "Common.hpp"
#include "Utils.hpp"

#include <fnv-cpp/fnv.h>

#include <QDebug>

#include <algorithm>

ChatModel::ChatModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_sortTimer(new QTimer(this))
{
    connect(TdApi::getInstance().chatStore, SIGNAL(updateChatItem(qint64)), SLOT(handleChatItem(qint64)));
    connect(TdApi::getInstance().chatStore, SIGNAL(updateChatPosition(qint64)), SLOT(handleChatPosition(qint64)));
    connect(&TdApi::getInstance(), SIGNAL(error(const QVariantMap &)), SLOT(handleError(const QVariantMap &)));
    connect(this, SIGNAL(chatListChanged()), SLOT(refresh()));

    connect(m_sortTimer, SIGNAL(timeout()), SLOT(sortChats()));
    m_sortTimer->setInterval(1000);
    m_sortTimer->setSingleShot(true);

    setRoleNames(roleNames());
    loadChats();
}

ChatModel::~ChatModel()
{
    if (m_sortTimer)
        delete m_sortTimer;
}

int ChatModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_count;
}

bool ChatModel::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid())
        return false;

    return m_count < m_chatIds.count();
}

void ChatModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid())
        return;

    if (m_loading)
        loadChats();

    const int remainder = m_chatIds.size() - m_count;
    const auto itemsToFetch = qMin(ChatSliceLimit, remainder);

    if (itemsToFetch <= 0)
        return;

    beginInsertRows(QModelIndex(), m_count, m_count + itemsToFetch - 1);

    m_count += itemsToFetch;

    endInsertRows();

    populate();

    emit countChanged();
}

QVariant ChatModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    switch (const auto chatId = m_chatIds.at(index.row()); role)
    {
        case IdRole:
            return QString::number(chatId);
        case TypeRole:
            return {};
        case TitleRole:
            return Utils::getChatTitle(chatId);
        case PhotoRole: {
            // TODO(strawberry):
            auto chat = TdApi::getInstance().chatStore->get(chatId);

            auto chatPhoto = chat.value("photo").toMap();
            if (chatPhoto.value("small").toMap().value("local").toMap().value("is_downloading_completed").toBool())
            {
                return chatPhoto.value("small").toMap().value("local").toMap().value("path").toString();
            }
            return {};
        }
        case LastMessageSenderRole: {
            auto chat = TdApi::getInstance().chatStore->get(chatId);

            return Utils::getMessageSenderName(chat.value("last_message").toMap());
        }
        case LastMessageContentRole: {
            auto chat = TdApi::getInstance().chatStore->get(chatId);

            return Utils::getContent(chat.value("last_message").toMap());
        }
        case LastMessageDateRole: {
            auto chat = TdApi::getInstance().chatStore->get(chatId);

            return Utils::getMessageDate(chat.value("last_message").toMap());
        }
        case IsPinnedRole: {
            return Utils::isChatPinned(chatId, m_list);
        }
        case UnreadCountRole: {
            auto chat = TdApi::getInstance().chatStore->get(chatId);

            return chat.value("unread_count").toInt();
        }
        case IsMutedRole: {
            return Utils::getChatMuteFor(chatId) > 0;
        }
    }

    return {};
}

QHash<int, QByteArray> ChatModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[IdRole] = "id";
    roles[TypeRole] = "type";
    roles[TitleRole] = "title";
    roles[PhotoRole] = "photo";
    roles[LastMessageSenderRole] = "lastMessageSender";
    roles[LastMessageContentRole] = "lastMessageContent";
    roles[LastMessageDateRole] = "lastMessageDate";
    roles[IsPinnedRole] = "isPinned";
    roles[UnreadCountRole] = "unreadCount";
    roles[IsMutedRole] = "isMuted";

    return roles;
}

int ChatModel::count() const
{
    return m_count;
}

bool ChatModel::loading() const
{
    return m_loading;
}

TdApi::ChatList ChatModel::chatList() const
{
    return m_chatList;
}

void ChatModel::setChatList(TdApi::ChatList value)
{
    if (m_chatList != value)
    {
        m_chatList = value;
        emit chatListChanged();
    }
}

int ChatModel::chatFilterId() const
{
    return m_chatFilterId;
}

void ChatModel::setChatFilterId(int value)
{
    if (m_chatList == TdApi::ChatListFilter && m_chatFilterId != value)
    {
        m_chatFilterId = value;
        emit chatListChanged();
    }
}

QVariant ChatModel::get(int index) const noexcept
{
    QModelIndex modelIndex = createIndex(index, 0);

    QVariantMap result;
    result.insert("id", data(modelIndex, IdRole));
    result.insert("type", data(modelIndex, TypeRole));
    result.insert("title", data(modelIndex, TitleRole));
    result.insert("photo", data(modelIndex, PhotoRole));
    result.insert("lastMessageSender", data(modelIndex, LastMessageSenderRole));
    result.insert("lastMessageContent", data(modelIndex, LastMessageContentRole));
    result.insert("lastMessageDate", data(modelIndex, LastMessageDateRole));
    result.insert("isPinned", data(modelIndex, IsPinnedRole));
    result.insert("unreadCount", data(modelIndex, UnreadCountRole));
    result.insert("isMuted", data(modelIndex, IsMutedRole));

    return result;
}

void ChatModel::toggleChatIsPinned(qint64 chatId, bool isPinned)
{
    QVariantMap result;
    result.insert("@type", "toggleChatIsPinned");
    result.insert("chat_list", m_list);
    result.insert("chat_id", chatId);
    result.insert("is_pinned", isPinned);

    TdApi::getInstance().sendRequest(result);
}

void ChatModel::populate()
{
    m_chatIds.clear();

    for (auto id : TdApi::getInstance().chatStore->getIds())
    {
        for (auto chat = TdApi::getInstance().chatStore->get(id); const auto &position : chat.value("positions").toList())
        {
            if (Utils::chatListEquals(position.toMap().value("list").toMap(), m_list))
                m_chatIds.append(id);

            auto chatPhoto = chat.value("photo").toMap();
            if (!chat.value("photo").isNull() &&
                !chatPhoto.value("small").toMap().value("local").toMap().value("is_downloading_completed").toBool())
            {
                TdApi::getInstance().downloadFile(chatPhoto.value("small").toMap().value("id").toInt(), 1, 0, 0, false);
            }
        }
    }

    sortChats();

    if (m_count == 0 && !m_chatIds.isEmpty())
        fetchMore();
}

void ChatModel::clear()
{
    beginResetModel();
    m_chatIds.clear();
    m_count = 0;
    m_list.clear();
    endResetModel();

    emit countChanged();
}

void ChatModel::refresh()
{
    m_loading = true;

    clear();
    loadChats();

    emit loadingChanged();
}

void ChatModel::sortChats()
{
    emit layoutAboutToBeChanged();

    std::ranges::sort(m_chatIds, [&](auto a, auto b) { return Utils::getChatOrder(a, m_list) > Utils::getChatOrder(b, m_list); });

    emit layoutChanged();
}

void ChatModel::handleChatItem(qint64 chatId)
{
    auto it = std::ranges::find_if(m_chatIds, [chatId](qint64 id) { return id == chatId; });

    if (it != m_chatIds.end())
    {
        auto index = std::distance(m_chatIds.begin(), it);
        QModelIndex modelIndex = createIndex(static_cast<int>(index), 0);

        qDebug() << chatId;

        emit dataChanged(modelIndex, modelIndex);
    }
}

void ChatModel::handleChatPhoto(int fileId)
{
}

void ChatModel::handleChatPosition(qint64 chatId)
{
    auto it = std::ranges::find_if(m_chatIds, [chatId](qint64 id) { return id == chatId; });

    if (it != m_chatIds.end())
    {
        // emit delayed event
        if (not m_sortTimer->isActive())
            m_sortTimer->start();

        qDebug() << chatId;
    }
}

void ChatModel::handleError(const QVariantMap &error)
{
    if (error.value("@extra").toString() == "load_chats_error" && error.value("@extra").toInt() == 404)
    {
        m_loading = false;
        emit loadingChanged();
    }
}

void ChatModel::loadChats()
{
    switch (m_chatList)
    {
        case TdApi::ChatListMain:
            m_list.insert("@type", "chatListMain");
            break;
        case TdApi::ChatListArchive:
            m_list.insert("@type", "chatListArchive");
            break;
        case TdApi::ChatListFilter:
            m_list.insert("@type", "chatListFilter");
            m_list.insert("chat_filter_id", m_chatFilterId);
            break;
    }

    QVariantMap result;
    result.insert("@type", "loadChats");
    result.insert("chat_list", m_list);
    result.insert("limit", ChatSliceLimit);
    result.insert("@extra", "load_chats_error");

    TdApi::getInstance().sendRequest(result);
}
