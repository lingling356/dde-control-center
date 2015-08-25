#include <libdui/dthememanager.h>

#include "searchlist.h"

SearchList::SearchList(QWidget *parent) :
    DScrollArea(parent),
    m_itemWidth(-1),
    m_itemHeight(-1),
    m_layout(new QVBoxLayout),
    m_dbus(new SearchDbus(this)),
    m_searching(false),
    m_checkedItem(-1),
    m_checkable(false),
    m_mainWidget(new QWidget)
{
    D_THEME_INIT_WIDGET(SearchList);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);

    m_layout->setMargin(0);
    m_layout->setSpacing(0);
    m_layout->addStretch(1);

    m_mainWidget->setObjectName("MainWidget");
    m_mainWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_mainWidget->setLayout(m_layout);

    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlignment(Qt::AlignHCenter);
    setWidget(m_mainWidget);
}

int SearchList::addItem(SearchItem *data)
{
    insertItem(count(), data);

    return count()-1;
}

void SearchList::addItems(const QList<SearchItem*> &datas)
{
    insertItems(count(), datas);
}

void SearchList::insertItem(int index, SearchItem *data)
{
    if(data==NULL||data->widget()==NULL)
        return;

    m_itemList.insert(index, data);
    QWidget *w = data->widget();
    if(m_itemWidth>0)
        w->setFixedWidth(m_itemWidth);
    if(m_itemHeight>0){
        w->setFixedHeight(m_itemHeight);
        m_mainWidget->setFixedHeight(count()*(m_layout->spacing()+m_itemHeight)-m_layout->spacing());
        if(verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff){
            setFixedHeight(m_mainWidget->height());
        }else{
            setMaximumHeight(m_mainWidget->height());
        }
    }

    w->installEventFilter(this);
    m_layout->insertWidget(index, w, 0, Qt::AlignCenter);
    m_keyWords<<data->keyWords();
    data->setListWidget(this);

    emit countChanged();
}

void SearchList::insertItems(int index, const QList<SearchItem*> &datas)
{
    foreach (SearchItem *data, datas) {
        insertItem(index++, data);
    }
}

void SearchList::setItemData(int index, const QVariant &data)
{
    SearchItem *item = getItem(index);
    if(item&&item->getData()!=data){
        item->setData(data);
        emit itemDataChanged(index, data);
    }
}

void SearchList::setItemSize(int w, int h)
{
    m_itemWidth = w;
    m_itemHeight = h;

    m_mainWidget->setFixedWidth(w);
}

void SearchList::clear()
{
    m_keyWords.clear();

    for(int i=0;i<count();++i){
        m_layout->removeItem(m_layout->takeAt(i));
        m_itemList[i]->widget()->removeEventFilter(this);
        delete m_itemList[i];
    }

    m_itemList.clear();

    m_mainWidget->setFixedHeight(0);
    setMaximumHeight(0);
    setCheckedItem(-1);
    emit countChanged();
}

void SearchList::removeItem(int index)
{
    SearchItem *item = getItem(index);
    m_itemList.removeAt(index);

    if(item){
        foreach (const QString &str, item->keyWords()) {
            m_keyWords.removeOne(str);
        }
    }
    m_layout->removeItem(m_layout->takeAt(index));
    if(item){
        item->widget()->removeEventFilter(this);
        delete item;
    }

    if(index == m_checkedItem){
        setCheckedItem(-1);
    }

    if(m_itemHeight>0){
        m_mainWidget->setFixedHeight(count()*(m_layout->spacing()+m_itemHeight)-m_layout->spacing());
        if(verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff){
            setFixedHeight(m_mainWidget->height());
        }else{
            setMaximumHeight(m_mainWidget->height());
        }
    }

    emit countChanged();
}

void SearchList::beginSearch()
{
    if(m_searching)
        return;
    m_searching = true;
    m_dbusKey = m_dbus->NewSearchWithStrList(m_keyWords);
}

void SearchList::endSearch()
{
    if(m_searching){
        m_dbusKey = "";
        m_searching = false;

        for(int i = 0; i<count(); ++i){
            showItem(i);
        }
    }
}

bool isIntersect(const QStringList& list1, const QStringList &list2)
{
    foreach (const QString &str, list1) {
        if(list2.contains(str))
            return true;
    }

    return false;
}

void SearchList::setKeyWord(const QString &keyWord)
{
    if(m_searching){
        QStringList list = m_dbus->SearchString(keyWord, m_dbusKey);

        for (int i = 0; i<count(); ++i) {
            if(keyWord.isEmpty()
                    || isIntersect(list, m_itemList[i]->keyWords())){
                showItem(i);
            }else{
                hideItem(i);
            }
        }
    }
}

void SearchList::showItem(int index)
{
    QWidget *w = getItem(index)->widget();

    if(w&&!w->isVisible()){
        w->show();
        if(m_itemHeight>0){
            m_mainWidget->setFixedHeight(m_mainWidget->height()+m_layout->spacing()+m_itemHeight);
            if(verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff){
                setFixedHeight(m_mainWidget->height());
            }else{
                setMaximumHeight(m_mainWidget->height());
            }
        }
    }
}

void SearchList::hideItem(int index)
{
    QWidget *w = getItem(index)->widget();
    if(w&&w->isVisible()){
        w->hide();
        if(m_itemHeight>0){
            m_mainWidget->setFixedHeight(m_mainWidget->height()-m_layout->spacing()-m_itemHeight);
            if(verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff){
                setFixedHeight(m_mainWidget->height());
            }else{
                setMaximumHeight(m_mainWidget->height());
            }
        }
    }
}

void SearchList::setCheckedItem(int checkedItem)
{
    if (m_checkedItem == checkedItem)
        return;

    if(m_checkable){
        if(m_checkedItem>=0&&m_checkedItem<count()){
            QWidget *w = getItem(m_checkedItem)->widget();
            if(w){
                w->setProperty("checked", false);
            }
        }

        if(checkedItem>=0&&checkedItem<count()){
            QWidget *w = getItem(checkedItem)->widget();
            if(w){
                w->setProperty("checked", true);
            }
        }
    }

    m_checkedItem = checkedItem;
    emit checkedItemChanged(checkedItem);
}

void SearchList::setCheckable(bool checkable)
{
    if (m_checkable == checkable)
        return;

    m_checkable = checkable;
    emit checkableChanged(checkable);
}

int SearchList::count() const
{
    return m_itemList.count();
}

QVariant SearchList::getItemData(int index) const
{
    return getItem(index)->getData();
}

SearchItem *SearchList::getItem(int index) const
{
    return m_itemList[index];
}

int SearchList::indexOf(SearchItem *item) const
{
    return m_itemList.indexOf(item);
}

int SearchList::indexOf(QWidget *widget) const
{
    for (int i=0;i<count();++i) {
        const SearchItem *item = m_itemList[i];
        if(item->widget() == widget){
            return i;
        }
    }

    return -1;
}

bool SearchList::isSearching() const
{
    return m_searching;
}

int SearchList::checkedItem() const
{
    return m_checkedItem;
}

bool SearchList::checkable() const
{
    return m_checkable;
}

bool SearchList::eventFilter(QObject *obj, QEvent *e)
{
    if(!m_checkable || e->type() != QEvent::MouseButtonRelease)
        return false;

    QWidget *w = qobject_cast<QWidget*>(obj);

    if(w){
        int index = indexOf(w);
        if(index>=0){
            setCheckedItem(index);
        }
    }

    return false;
}

SearchList *SearchItem::listWidget() const
{
    return m_list;
}

void SearchItem::setListWidget(SearchList *list)
{
    m_list = list;
}
