/*
 * Copyright (C) 2017 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     zccrs <zccrs@live.com>
 *
 * Maintainer: zccrs <zhangjide@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "modules/accounts/accountsmodule.h"
#include "modules/bluetooth/bluetoothmodule.h"
#include "modules/commoninfo/commoninfomodule.h"
#include "modules/datetime/datetimemodule.h"
#include "modules/defapp/defaultappsmodule.h"
#include "modules/keyboard/keyboardmodule.h"
#include "modules/power/powermodule.h"
#include "modules/sound/soundmodule.h"
#include "modules/update/updatemodule.h"
#include "modules/mouse/mousemodule.h"
#include "modules/wacom/wacommodule.h"
#include "modules/display/displaymodule.h"
#include "modules/personalization/personalizationmodule.h"
#include "modules/sync/syncmodule.h"
#include "modules/systeminfo/systeminfomodule.h"
#include "modules/network/networkmodule.h"
#include "modules/defapp/defaultappsmodule.h"
#include "moduleinitthread.h"
#include "modules/update/mirrorswidget.h"

#include "mainwindow.h"
#include "constant.h"
#include "search/searchwidget.h"
#include "dtitlebar.h"
#include "utils.h"

#include <DBackgroundGroup>

#include <QHBoxLayout>
#include <QMetaEnum>
#include <QDebug>
#include <QStandardItemModel>
#include <QPushButton>
#include <QLocale>

using namespace DCC_NAMESPACE;
using namespace DCC_NAMESPACE::search;
DTK_USE_NAMESPACE

const int first_widget_min_width = 190;
const int second_widget_min_width = 250;
const int third_widget_min_width = 360;
const int widget_total_min_width = 820;

MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent)
    , m_contentLayout(nullptr)
    , m_rightContentLayout(nullptr)
    , m_navView(nullptr)
    , m_rightView(nullptr)
    , m_navModel(nullptr)
    , m_bIsFinalWidget(false)
    , m_bIsFromSecondAddWidget(false)
    , m_topWidget(nullptr)
    , m_searchWidget(nullptr)
    , m_firstCount(-1)
    , m_widgetName("")
{
    //Initialize view and layout structure
    QWidget *content = new QWidget(this);

    m_contentLayout = new QHBoxLayout(content);
    m_rightContentLayout = new QHBoxLayout();
    m_rightView = new DBackgroundGroup(m_rightContentLayout);
    m_navView = new DListView(this);
    m_navView->setFrameShape(QFrame::Shape::NoFrame);
    m_navView->setEditTriggers(QListView::NoEditTriggers);
    m_navView->setResizeMode(QListView::Adjust);
    m_navView->setAutoScroll(false);
    m_navView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_navView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_contentLayout->addWidget(m_navView);
    m_contentLayout->setSpacing(0);
    m_contentLayout->addWidget(m_rightView);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);

    m_rightContentLayout->setContentsMargins(10, 10, 10, 10);
    m_rightContentLayout->setSpacing(2);

    m_rightView->hide();
    m_rightView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setCentralWidget(content);

    //Initialize top page view and model
    m_navModel = new QStandardItemModel(m_navView);
    m_navView->setModel(m_navModel);
    connect(m_navView, &DListView::clicked, this, &MainWindow::onFirstItemClick);

    m_searchWidget = new SearchWidget(this);
    m_searchWidget->setMinimumSize(280, 36);

    DTitlebar *titlebar = this->titlebar();
    titlebar->setMinimumHeight(50);
    titlebar->addWidget(m_searchWidget, Qt::AlignCenter);
    connect(m_searchWidget, &SearchWidget::notifyModuleSearch, this, &MainWindow::onEnterSearchWidget);

    auto thlayout = new QHBoxLayout();
    thlayout->setSpacing(0);
    DBackgroundGroup *btnGroup = new DBackgroundGroup(thlayout, this);
    titlebar->addWidget(btnGroup, Qt::AlignLeft);
    QPushButton *backwardBtn = new QPushButton("<");
    thlayout->addWidget(backwardBtn);
    connect(backwardBtn, &QPushButton::clicked, this, [this] {
        popWidget();
        resetNavList(m_contentStack.isEmpty());
    });

    QPushButton *forwardBtn = new QPushButton(">");
    thlayout->addWidget(forwardBtn);

    QTimer::singleShot(0, this, &MainWindow::initAllModule);
    QTimer::singleShot(0, this, &MainWindow::modulePreInitialize);
    QTimer::singleShot(0, this, [ = ]() {
        //after initAllModule to load ts data
        m_searchWidget->setLanguage(QLocale::system().name());
    });
}

void MainWindow::initAllModule()
{
    using namespace sync;
    using namespace datetime;
    using namespace defapp;
    using namespace network;
    using namespace display;
    using namespace accounts;
    using namespace mouse;
    using namespace bluetooth;
    using namespace sound;
    using namespace personalization;
    using namespace power;
    using namespace update;
    using namespace keyboard;
    using namespace wacom;
    using namespace systeminfo;
    using namespace commoninfo;

    m_modules = {
        { new AccountsModule(this), tr("Account")},
        { new SyncModule(this), tr("Cloud Sync")},
        { new DisplayModule(this), tr("Display")},
        { new DefaultAppsModule(this), tr("Default Applications")},
        { new NetworkModule(this), tr("Network")},
        { new DatetimeModule(this), tr("Date and Time")},
        { new MouseModule(this), tr("Mouse")},
        { new KeyboardModule(this), tr("Keyboard and Language")},
        { new BluetoothModule(this), tr("Bluetooth")},
        { new SoundModule(this), tr("Sound")},
        { new PersonalizationModule(this), tr("Personalization")},
        { new PowerModule(this), tr("Power")},
        { new UpdateModule(this), tr("Update")},
        { new WacomModule(this), tr("Drawing Tablet")},
        { new SystemInfoModule(this), tr("System Info")},
        { new CommonInfoModule(this), tr("Common Info")},
    };

    bool isIcon = m_contentStack.empty();

    for (auto it = m_modules.cbegin(); it != m_modules.cend(); ++it) {
        DStandardItem *item = new DStandardItem;
        item->setIcon(it->first->icon());
        item->setText(it->second);
        item->setData(VListViewItemMargin, Dtk::MarginsRole);
        m_navModel->appendRow(item);
        m_searchWidget->addModulesName(it->first->name(), it->second);
    }

    resetNavList(isIcon);
}

void MainWindow::modulePreInitialize()
{
    for (auto it = m_modules.cbegin(); it != m_modules.cend(); ++it) {
        it->first->preInitialize();
    }
}

void MainWindow::popWidget()
{
    if (m_topWidget) {
        m_topWidget->deleteLater();
        m_topWidget = nullptr;
        return;
    }

    if (!m_contentStack.size())
        return;

    QWidget *w = m_contentStack.pop().second;

    m_rightContentLayout->removeWidget(w);
    w->setParent(nullptr);
    w->deleteLater();

    //delete replace widget : first delete replace widget(up code) , then pass pushWidget to set last widget
    if (m_lastThirdPage.second) {
        m_lastThirdPage.second->setVisible(true);
        pushWidget(m_lastThirdPage.first, m_lastThirdPage.second);
        //only set two pointer to nullptr , but not delete memory
        memset(&m_lastThirdPage, 0, sizeof(m_lastThirdPage));
    }
}

//Only used to from third page to top page can use it
void MainWindow::popAllWidgets(int place)
{
    for (int pageCount = m_contentStack.count(); pageCount > place; pageCount--) {
        popWidget();
    }
}

void MainWindow::popWidget(ModuleInterface *const inter)
{
    Q_UNUSED(inter)

    popWidget();
}

void MainWindow::showModulePage(const QString &module, const QString &page, bool animation)
{
    Q_UNUSED(animation)

    if (isMinimized())
        setWindowState(Qt::WindowActive);

    onEnterSearchWidget(module, page);
}

void MainWindow::toggle()
{
    show();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    DMainWindow::resizeEvent(event);

    if (m_topWidget) {
        m_topWidget->setFixedSize(event->size());
        m_topWidget->setFixedHeight(height() - this->titlebar()->height());
    }
}

void MainWindow::resetNavList(bool isIconMode)
{
    if (isIconMode && m_navView->viewMode() == QListView::IconMode)
        return;

    if (!isIconMode && m_navView->viewMode() == QListView::ListMode)
        return;

    if (isIconMode) {
        //Only remain 1 level page : back to top page
        m_navView->setViewMode(QListView::IconMode);
        m_navView->setDragEnabled(false);
        m_navView->setMaximumWidth(QWIDGETSIZE_MAX);
        m_navView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_navView->setIconSize(QSize(84, 84));
        m_navView->setItemSize(QSize(170, 168));
        m_navView->setSpacing(20);
        m_rightView->hide();

        DPalette pa = DPalette::get(m_navView);
        pa.setBrush(DPalette::ItemBackground, pa.base());
        DPalette::set(m_navView, pa);
    } else {
        //The second page will Covered with fill blank areas
        m_navView->setViewMode(QListView::ListMode);
        m_navView->setFixedWidth(first_widget_min_width);
        m_navView->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        m_navView->setIconSize(QSize(42, 42));
        m_navView->setItemSize(QSize(168, 48));
        m_navView->setSpacing(0);
        m_rightView->show();
        m_contentStack.top().second->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    m_navView->viewport()->setAutoFillBackground(!isIconMode);
}

void MainWindow::onEnterSearchWidget(QString moduleName, QString widget)
{
    qDebug() << Q_FUNC_INFO << " moduleName : " << moduleName << " , widget :" << widget;

    if (!m_contentStack.isEmpty()
            && (m_contentStack.top().first->name() == moduleName)
            && (m_widgetName == widget)) {
        qDebug() << Q_FUNC_INFO << " Search widget is current display widget.";
        return;
    }

    for (int firstCount = 0; firstCount < m_modules.count(); firstCount++) {
        //Compare moduleName and m_modules.second(module name)
        if (moduleName == m_modules[firstCount].first->name()) {
            //enter first level widget
            m_navView->setCurrentIndex(m_navView->model()->index(firstCount, 0));
            m_navView->clicked(m_navView->model()->index(firstCount, 0));

            m_firstCount = firstCount;
            m_widgetName = widget;

            //notify related module load widget
            QTimer::singleShot(0, this, [this] { //avoid default and load sequence in time
                m_modules[m_firstCount].first->load(m_widgetName);
            });
            break;
        }
    }
}

void MainWindow::setModuleVisible(ModuleInterface *const inter, const bool visible)
{
    auto find_it = std::find_if(m_modules.cbegin(),
                                m_modules.cend(),
    [ = ](const QPair<ModuleInterface *, QString> &pair) {
        return pair.first == inter;
    });

    if (find_it != m_modules.cend()) {
        m_navView->setRowHidden(find_it - m_modules.cbegin(), !visible);
        Q_EMIT moduleVisibleChanged(find_it->first->name(), visible);
    } else {
        qWarning() << Q_FUNC_INFO << "Not found module!";
    }
}

void MainWindow::pushWidget(ModuleInterface *const inter, QWidget *const w, PushType type)
{
    if (!inter)  {
        qWarning() << Q_FUNC_INFO << " inter is nullptr";
        return;
    }

    if (!w)  {
        qWarning() << Q_FUNC_INFO << " widget is nullptr";
        return;
    }

    switch (type) {
    case Replace:
        replaceThirdWidget(inter, w);
        break;
    case CoverTop:
        judgeTopWidgetPlace(inter, w);
        break;
    case Normal:
    default:
        pushNormalWidget(inter, w);
        break;
    }
}

//First save the third level page, Then pop the third level page
//Next set the new page as the third level page,
//Finally when in the new third level page clicked "pop" button , return to the old three level page
void MainWindow::replaceThirdWidget(ModuleInterface *const inter, QWidget *const w)
{
    if (m_contentStack.count() != 2)
        return;

    if (m_lastThirdPage.second) {
        memset(&m_lastThirdPage, 0, sizeof(m_lastThirdPage));
    }

    QPair<ModuleInterface *, QWidget *>widget = m_contentStack.pop();
    m_lastThirdPage.first = widget.first;
    m_lastThirdPage.second = widget.second;
    m_lastThirdPage.second->setVisible(false);

    w->setParent(m_lastThirdPage.second);//the replace widget follow the old third widget to delete
    m_lastThirdPage.second->setParent(m_contentStack.top().second);
    pushNormalWidget(inter, w);
}

void MainWindow::pushTopWidget(ModuleInterface *const inter, QWidget *const w)
{
    Q_UNUSED(inter)

    QWidget *topWidget = new QWidget;
    DPalette pa = DPalette::get(topWidget);
    pa.setBrush(DPalette::ItemBackground, pa.base());
    DPalette::set(topWidget, pa);
    topWidget->setFixedSize(this->width(), this->height());

    QHBoxLayout *layout = new QHBoxLayout;
    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    pa = DPalette::get(w);
    pa.setBrush(DPalette::ItemBackground, pa.base());
    DPalette::set(w, pa);

    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(w, 0, Qt::AlignRight);
    topWidget->setLayout(layout);

    topWidget->setParent(this);
    topWidget->setVisible(true);

    if (m_topWidget) {
        m_topWidget->deleteLater();
        m_topWidget = nullptr;
    }

    m_topWidget = topWidget;
    m_topWidget->move(0, this->titlebar()->height());
    m_topWidget->setFixedHeight(height() - this->titlebar()->height());
    resetNavList(m_contentStack.empty());
}

void MainWindow::pushFinalWidget(ModuleInterface *const inter, QWidget *const w)
{
    DPalette pa = DPalette::get(w);
    pa.setBrush(DPalette::ItemBackground, pa.base());
    DPalette::set(w, pa);

    w->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_contentStack.push({inter, w});
    m_rightContentLayout->addWidget(w);

    if (m_contentStack.size() == 2) {
        m_contentStack.at(0).second->setMinimumWidth(second_widget_min_width);
        m_contentStack.at(1).second->setMinimumWidth(third_widget_min_width);
    }
}

void MainWindow::pushNormalWidget(ModuleInterface *const inter, QWidget *const w)
{
    //When there is already a third-level page, first remove the previous third-level page,
    //then add a new level 3 page (guaranteed that there is only one third-level page)
    popAllWidgets(1);
    //Set the newly added page to fill the blank area
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_contentStack.push({inter, w});
    m_rightContentLayout->addWidget(w);

    if (m_contentStack.size() == 2) {
        m_contentStack.at(0).second->setMinimumWidth(second_widget_min_width);
        m_contentStack.at(1).second->setMinimumWidth(third_widget_min_width);
    }

    resetNavList(m_contentStack.empty());
}

void MainWindow::judgeTopWidgetPlace(ModuleInterface *const inter, QWidget *const w)
{
    int totalWidth = m_navView->minimumWidth() + 20;//10 is first and second widget space　, other 10 is last widget with right box space
    int contentCount = m_contentStack.count();

    if (m_bIsFinalWidget) {
        m_bIsFinalWidget = false;
    }

    for (int count = 0; count < contentCount; count++) {
        totalWidth += m_contentStack.at(count).second->minimumWidth();
    }

    //according current content widgets count to calculate use top widget or right widget
    switch (contentCount) {
    case 1: //from second widget to add top/right widget (like update setting : source list)
        if (totalWidth < widget_total_min_width) {
            m_bIsFinalWidget = true;
            m_bIsFromSecondAddWidget = true;//save from pushWidget of CoverTop type to add final(right) widget
        }
        break;
    case 2: //from third widget to add top/right widget
        if (totalWidth < widget_total_min_width) {
            m_bIsFinalWidget = true;
        }

        if (m_bIsFromSecondAddWidget) {
            popAllWidgets(1);//pop the final widget, need distinguish
            m_bIsFromSecondAddWidget = false;
        }
        break;
    case 3: //replace final widget to new top/right widget
        m_bIsFinalWidget = true;
        popAllWidgets(2);//move fourth widget(m_navView not in it , other level > 2)
        break;
    default:
        qWarning() << Q_FUNC_INFO << " error widget content conut : " << contentCount;
        return;
    }

    if (m_bIsFinalWidget) {
        pushFinalWidget(inter, w);
    } else {
        pushTopWidget(inter, w);
    }
}

void MainWindow::onFirstItemClick(const QModelIndex &index)
{
    ModuleInterface *inter = m_modules[index.row()].first;

    if (!m_contentStack.isEmpty() && m_contentStack.last().first == inter) {
        return;
    }

    popAllWidgets();

    if (!m_initList.contains(inter)) {
        inter->initialize();
        m_initList << inter;
    }
    inter->active();
}
