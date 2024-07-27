#include "SelectionModel.hpp"

#include "Localization.hpp"
#include "Serialize.hpp"

#include <QDebug>

#include <algorithm>

CountryModel::CountryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles.insert(NameRole, "name");
    roles.insert(Iso2Role, "iso2");
    roles.insert(CodeRole, "code");

    setRoleNames(roles);
}

void CountryModel::setCountries(td::td_api::object_ptr<td::td_api::countries> &&value)
{
    beginResetModel();

    m_countries = std::move(value->countries_);
    endResetModel();

    emit countChanged();
}

int CountryModel::rowCount(const QModelIndex &) const
{
    return m_countries.size();
}

QVariant CountryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || m_countries.empty())
        return QVariant();

    const auto &countryInfo = m_countries.at(index.row());
    switch (role)
    {
        case NameRole:
            return QString::fromStdString(countryInfo->name_);
        case Iso2Role:
            return QString::fromStdString(countryInfo->country_code_);
        case CodeRole: {
            QStringList result;

            for (const auto &value : countryInfo->calling_codes_)
            {
                result.append(QString::fromStdString(value));
            }

            return result;
        }
    }

    return QVariant();
}

QVariantMap CountryModel::get(int index) const noexcept
{
    QModelIndex modelIndex = createIndex(index, 0);

    QVariantMap result;
    result.insert("name", data(modelIndex, NameRole));
    result.insert("iso2", data(modelIndex, Iso2Role));
    result.insert("code", data(modelIndex, CodeRole));

    return result;
}

int CountryModel::count() const noexcept
{
    return m_countries.size();
}

int CountryModel::getDefaultIndex() const noexcept
{
    // TODO(strawberry): refactor
    auto it = std::ranges::find_if(m_countries,
                                   [](const auto &value) { return QString::fromStdString(value->country_code_).compare("NG", Qt::CaseInsensitive) == 0; });

    if (it != m_countries.end())
    {
        return int(std::distance(m_countries.begin(), it));
    }

    return {};
}

ChatFolderModel::ChatFolderModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles.insert(IdRole, "id");
    roles.insert(TitleRole, "name");

    setRoleNames(roles);
}

void ChatFolderModel::setLocale(Locale *locale)
{
    m_locale = locale;
}

void ChatFolderModel::setChatFolders(std::vector<td::td_api::object_ptr<td::td_api::chatFolderInfo>> &&value) noexcept
{
    beginResetModel();

    auto item = td::td_api::make_object<td::td_api::chatFolderInfo>();
    item->id_ = 0;
    item->title_ = m_locale->getString("FilterAllChats").toStdString();

    m_chatFolders = std::move(value);

    m_chatFolders.emplace(m_chatFolders.begin(), std::move(item));

    endResetModel();
    emit countChanged();
}

int ChatFolderModel::rowCount(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return m_chatFolders.size();
}

QVariant ChatFolderModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const auto &chatFolder = m_chatFolders.at(index.row());
    switch (role)
    {
        case IdRole:
            return chatFolder->id_;
        case TitleRole:
            return QString::fromStdString(chatFolder->title_);
        default:
            return QVariant();
    }
}

QVariant ChatFolderModel::get(int index) const noexcept
{
    QModelIndex modelIndex = createIndex(index, 0);
    QVariantMap result;
    result.insert("id", data(modelIndex, IdRole));
    result.insert("name", data(modelIndex, TitleRole));  // title
    return result;
}

int ChatFolderModel::count() const noexcept
{
    return m_chatFolders.size();
}
