/*
 * Bittorrent Client using Qt and libtorrent.
 * Copyright (C) 2022-2024  Vladimir Golovnev <glassez@yandex.ru>
 * Copyright (C) 2006  Christophe Dumez <chris@qbittorrent.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link this program with the OpenSSL project's "OpenSSL" library (or with
 * modified versions of it that use the same license as the "OpenSSL" library),
 * and distribute the linked executables. You must obey the GNU General Public
 * License in all respects for all of the code used other than "OpenSSL".  If you
 * modify file(s), you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 */

#include "mainwindow.h"

#include <QtSystemDetection>

#include <algorithm>
#include <chrono>

#include <QAction>
#include <QActionGroup>
#include <QClipboard>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMetaObject>
#include <QMimeData>
#include <QProcess>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QStatusBar>
#include <QString>
#include <QTimer>

#include "base/bittorrent/session.h"
#include "base/bittorrent/sessionstatus.h"
#include "base/global.h"
#include "base/net/downloadmanager.h"
#include "base/path.h"
#include "base/preferences.h"
#include "base/utils/fs.h"
#include "base/utils/misc.h"
#include "base/utils/password.h"
#include "base/version.h"
#include "aboutdialog.h"
#include "autoexpandabledialog.h"
#include "batchoperationsdialog.h"
#include "cookiesdialog.h"
#include "desktopintegration.h"
#include "downloadfromurldialog.h"
#include "executionlogwidget.h"
#include "hidabletabwidget.h"
#include "interfaces/iguiapplication.h"
#include "lineedit.h"
#include "optionsdialog.h"
#include "portablebackupdialog.h"
#include "powermanagement/powermanagement.h"
#include "properties/peerlistwidget.h"
#include "properties/propertieswidget.h"
#include "properties/proptabbar.h"
#include "speedlimitdialog.h"
#include "statusbar.h"
#include "torrentcardswidget.h"
#include "trackerlist/trackerlistwidget.h"
#include "traypopupwidget.h"
#include "transferlistfilterswidget.h"
#include "transferlistmodel.h"
#include "transferlistwidget.h"
#include "ui_mainwindow.h"
#include "uithememanager.h"
#include "utils.h"

#ifdef Q_OS_MACOS
#include "macosdockbadge/badger.h"
#endif

using namespace std::chrono_literals;

namespace
{
#define SETTINGS_KEY(name) u"GUI/" name
#define EXECUTIONLOG_SETTINGS_KEY(name) (SETTINGS_KEY(u"Log/"_s) name)

    const std::chrono::seconds PREVENT_SUSPEND_INTERVAL {60};

}

MainWindow::MainWindow(IGUIApplication *app, const WindowState initialState, const QString &titleSuffix)
    : GUIApplicationComponent(app)
    , m_ui {new Ui::MainWindow}
    , m_downloadRate {Utils::Misc::friendlyUnit(0, true)}
    , m_uploadRate {Utils::Misc::friendlyUnit(0, true)}
    , m_storeExecutionLogEnabled {EXECUTIONLOG_SETTINGS_KEY(u"Enabled"_s)}
    , m_storeDownloadTrackerFavicon {SETTINGS_KEY(u"DownloadTrackerFavicon"_s)}
    , m_storeExecutionLogTypes {EXECUTIONLOG_SETTINGS_KEY(u"Types"_s), Log::MsgType::ALL}
#ifdef Q_OS_MACOS
    , m_badger {std::make_unique<MacUtils::Badger>()}
#endif // Q_OS_MACOS
{
    m_ui->setupUi(this);

    Preferences *const pref = Preferences::instance();
    m_uiLocked = pref->isUILocked();
    m_displaySpeedInTitle = pref->speedInTitleBar();
    // Setting icons
#ifndef Q_OS_MACOS
    setWindowIcon(UIThemeManager::instance()->getIcon(u"qbittorrent"_s));
#endif // Q_OS_MACOS

    setTitleSuffix(titleSuffix);

#if (defined(Q_OS_UNIX))
    m_ui->actionOptions->setText(tr("Preferences"));
#endif

    addToolbarContextMenu();

    m_ui->actionOpen->setIcon(UIThemeManager::instance()->getIcon(u"list-add"_s));
    m_ui->actionDownloadFromURL->setIcon(UIThemeManager::instance()->getIcon(u"insert-link"_s));
    m_ui->actionSetGlobalSpeedLimits->setIcon(UIThemeManager::instance()->getIcon(u"speedometer"_s));
    m_ui->actionAbout->setIcon(UIThemeManager::instance()->getIcon(u"help-about"_s));
    m_ui->actionTopQueuePos->setIcon(UIThemeManager::instance()->getIcon(u"go-top"_s));
    m_ui->actionIncreaseQueuePos->setIcon(UIThemeManager::instance()->getIcon(u"go-up"_s));
    m_ui->actionDecreaseQueuePos->setIcon(UIThemeManager::instance()->getIcon(u"go-down"_s));
    m_ui->actionBottomQueuePos->setIcon(UIThemeManager::instance()->getIcon(u"go-bottom"_s));
    m_ui->actionDelete->setIcon(UIThemeManager::instance()->getIcon(u"list-remove"_s));
    m_ui->actionDocumentation->setIcon(UIThemeManager::instance()->getIcon(u"help-contents"_s));
    m_ui->actionDonateMoney->setIcon(UIThemeManager::instance()->getIcon(u"wallet-open"_s));
    m_ui->actionExit->setIcon(UIThemeManager::instance()->getIcon(u"application-exit"_s));
    m_ui->actionLock->setIcon(UIThemeManager::instance()->getIcon(u"object-locked"_s));
    m_ui->actionOptions->setIcon(UIThemeManager::instance()->getIcon(u"configure"_s, u"preferences-system"_s));
    m_ui->actionStart->setIcon(UIThemeManager::instance()->getIcon(u"torrent-start"_s, u"media-playback-start"_s));
    m_ui->actionStop->setIcon(UIThemeManager::instance()->getIcon(u"torrent-stop"_s, u"media-playback-pause"_s));
    m_ui->actionPauseSession->setIcon(UIThemeManager::instance()->getIcon(u"pause-session"_s, u"media-playback-pause"_s));
    m_ui->actionResumeSession->setIcon(UIThemeManager::instance()->getIcon(u"torrent-start"_s, u"media-playback-start"_s));
    m_ui->menuAutoShutdownOnDownloadsCompletion->setIcon(UIThemeManager::instance()->getIcon(u"task-complete"_s, u"application-exit"_s));
    m_ui->actionManageCookies->setIcon(UIThemeManager::instance()->getIcon(u"browser-cookies"_s, u"preferences-web-browser-cookies"_s));
    m_ui->menuLog->setIcon(UIThemeManager::instance()->getIcon(u"help-contents"_s));

    m_ui->actionPauseSession->setVisible(!BitTorrent::Session::instance()->isPaused());
    m_ui->actionResumeSession->setVisible(BitTorrent::Session::instance()->isPaused());
    connect(BitTorrent::Session::instance(), &BitTorrent::Session::paused, this, [this]
    {
        m_ui->actionPauseSession->setVisible(false);
        m_ui->actionResumeSession->setVisible(true);
        refreshWindowTitle();
        refreshTrayIconTooltip();
    });
    connect(BitTorrent::Session::instance(), &BitTorrent::Session::resumed, this, [this]
    {
        m_ui->actionPauseSession->setVisible(true);
        m_ui->actionResumeSession->setVisible(false);
        refreshWindowTitle();
        refreshTrayIconTooltip();
    });

    auto *lockMenu = new QMenu(m_ui->menuView);
    lockMenu->addAction(tr("&Set Password"), this, &MainWindow::defineUILockPassword);
    lockMenu->addAction(tr("&Clear Password"), this, &MainWindow::clearUILockPassword);
    m_ui->actionLock->setMenu(lockMenu);

    updateAltSpeedsBtn(BitTorrent::Session::instance()->isAltGlobalSpeedLimitEnabled());

    connect(BitTorrent::Session::instance(), &BitTorrent::Session::speedLimitModeChanged, this, &MainWindow::updateAltSpeedsBtn);

    qDebug("create tabWidget");
    m_tabs = new HidableTabWidget(this);
    connect(m_tabs.data(), &QTabWidget::currentChanged, this, &MainWindow::tabChanged);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    // vSplitter->setChildrenCollapsible(false);

    auto *hSplitter = new QSplitter(Qt::Vertical, this);
    hSplitter->setChildrenCollapsible(false);
    hSplitter->setFrameShape(QFrame::NoFrame);

    // Torrent filter
    m_columnFilterEdit = new LineEdit;
    m_columnFilterEdit->setPlaceholderText(tr("Filter torrents..."));
    m_columnFilterEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_columnFilterEdit->setFixedWidth(200);
    m_columnFilterEdit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_columnFilterEdit, &QWidget::customContextMenuRequested, this, &MainWindow::showFilterContextMenu);
    QHBoxLayout *columnFilterLayout = new QHBoxLayout(m_columnFilterWidget);
    columnFilterLayout->setContentsMargins(0, 0, 0, 0);
    auto *columnFilterSpacer = new QWidget(this);
    columnFilterSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    columnFilterLayout->addWidget(columnFilterSpacer);
    columnFilterLayout->addWidget(m_columnFilterEdit);
    m_columnFilterWidget = new QWidget(this);
    m_columnFilterWidget->setLayout(columnFilterLayout);
    m_ui->toolBar->removeAction(m_ui->actionLock);
    m_columnFilterAction = m_ui->toolBar->addWidget(m_columnFilterWidget);

    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_ui->toolBar->insertWidget(m_columnFilterAction, spacer);

    // Transfer List tab
    m_transferListWidget = new TransferListWidget(app, this);
    m_propertiesWidget = new PropertiesWidget(hSplitter);
    connect(m_transferListWidget, &TransferListWidget::currentTorrentChanged, m_propertiesWidget, &PropertiesWidget::loadTorrentInfos);
    hSplitter->addWidget(m_transferListWidget);
    hSplitter->addWidget(m_propertiesWidget);
    m_splitter->addWidget(hSplitter);
    m_splitter->setCollapsible(0, false);
    m_tabs->addTab(m_splitter,
#ifndef Q_OS_MACOS
        UIThemeManager::instance()->getIcon(u"folder-remote"_s),
#endif
        tr("Transfers"));
    connect(m_columnFilterEdit, &LineEdit::textChanged, this, &MainWindow::applyTransferListFilter);
    connect(hSplitter, &QSplitter::splitterMoved, this, &MainWindow::saveSettings);
    connect(m_splitter, &QSplitter::splitterMoved, this, &MainWindow::saveSplitterSettings);

    // Inline speed buttons in the properties tab bar
    const QString speedBtnStyle = u"QPushButton { font-size: 11px; padding: 2px 8px; min-height: 14px; border-radius: 3px; font-weight: normal; }"_s;
    m_inlineDlSpeedBtn = new QPushButton(this);
    m_inlineDlSpeedBtn->setStyleSheet(speedBtnStyle + u"QPushButton { color: #89b4fa; border: 1px solid #313244; background: #181825; } QPushButton:hover { background: #313244; }"_s);
    m_inlineDlSpeedBtn->setCursor(Qt::PointingHandCursor);
    m_inlineDlSpeedBtn->setToolTip(tr("Click to set download speed limit"));
    m_inlineUlSpeedBtn = new QPushButton(this);
    m_inlineUlSpeedBtn->setStyleSheet(speedBtnStyle + u"QPushButton { color: #a6e3a1; border: 1px solid #313244; background: #181825; } QPushButton:hover { background: #313244; }"_s);
    m_inlineUlSpeedBtn->setCursor(Qt::PointingHandCursor);
    m_inlineUlSpeedBtn->setToolTip(tr("Click to set upload speed limit"));

    auto createSpeedMenu = [this](bool isUpload) -> QMenu *
    {
        auto *menu = new QMenu(this);
        const QList<int> presets = {0, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000};
        for (const int kib : presets)
        {
            const QString label = (kib == 0) ? tr("Unlimited") : u"%1 KiB/s"_s.arg(kib);
            auto *action = menu->addAction(label);
            connect(action, &QAction::triggered, this, [this, kib, isUpload]()
            {
                auto *session = BitTorrent::Session::instance();
                const int bytes = kib * 1024;
                if (isUpload)
                    session->setGlobalUploadSpeedLimit(bytes);
                else
                    session->setGlobalDownloadSpeedLimit(bytes);
            });
        }
        return menu;
    };

    m_inlineDlSpeedBtn->setMenu(createSpeedMenu(false));
    m_inlineUlSpeedBtn->setMenu(createSpeedMenu(true));

    m_propertiesWidget->tabBar()->addStatusWidget(m_inlineDlSpeedBtn);
    m_propertiesWidget->tabBar()->addStatusWidget(m_inlineUlSpeedBtn);

#ifdef Q_OS_MACOS
    // Increase top spacing to avoid tab overlapping
    m_ui->centralWidgetLayout->addSpacing(8);
#endif

    m_ui->centralWidgetLayout->addWidget(m_tabs);

    m_queueSeparator = m_ui->toolBar->insertSeparator(m_ui->actionTopQueuePos);
    m_queueSeparatorMenu = m_ui->menuEdit->insertSeparator(m_ui->actionTopQueuePos);

#ifdef Q_OS_MACOS
    for (QAction *action : asConst(m_ui->toolBar->actions()))
    {
        if (action->isSeparator())
        {
            QWidget *spacer = new QWidget(this);
            spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            spacer->setMinimumWidth(16);
            m_ui->toolBar->insertWidget(action, spacer);
            m_ui->toolBar->removeAction(action);
        }
    }
    {
        QWidget *spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        spacer->setMinimumWidth(8);
        m_ui->toolBar->insertWidget(m_ui->actionDownloadFromURL, spacer);
    }
    {
        QWidget *spacer = new QWidget(this);
        spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        spacer->setMinimumWidth(8);
        m_ui->toolBar->addWidget(spacer);
    }
#endif // Q_OS_MACOS

    // Transfer list slots
    connect(m_ui->actionStart, &QAction::triggered, m_transferListWidget, &TransferListWidget::startSelectedTorrents);
    connect(m_ui->actionStop, &QAction::triggered, m_transferListWidget, &TransferListWidget::stopSelectedTorrents);
    connect(m_ui->actionPauseSession, &QAction::triggered, m_transferListWidget, &TransferListWidget::pauseSession);
    connect(m_ui->actionResumeSession, &QAction::triggered, m_transferListWidget, &TransferListWidget::resumeSession);
    connect(m_ui->actionDelete, &QAction::triggered, m_transferListWidget, &TransferListWidget::softDeleteSelectedTorrents);
    connect(m_ui->actionTopQueuePos, &QAction::triggered, m_transferListWidget, &TransferListWidget::topQueuePosSelectedTorrents);
    connect(m_ui->actionIncreaseQueuePos, &QAction::triggered, m_transferListWidget, &TransferListWidget::increaseQueuePosSelectedTorrents);
    connect(m_ui->actionDecreaseQueuePos, &QAction::triggered, m_transferListWidget, &TransferListWidget::decreaseQueuePosSelectedTorrents);
    connect(m_ui->actionBottomQueuePos, &QAction::triggered, m_transferListWidget, &TransferListWidget::bottomQueuePosSelectedTorrents);
    connect(m_ui->actionMinimize, &QAction::triggered, this, &MainWindow::minimizeWindow);
    connect(m_ui->actionUseAlternativeSpeedLimits, &QAction::triggered, this, &MainWindow::toggleAlternativeSpeeds);

    // Certain menu items should reside at specific places on macOS.
    // Qt partially does it on its own, but updates and different languages require tuning.
    m_ui->actionExit->setMenuRole(QAction::QuitRole);
    m_ui->actionAbout->setMenuRole(QAction::AboutRole);
    m_ui->actionOptions->setMenuRole(QAction::PreferencesRole);

    connect(m_ui->actionManageCookies, &QAction::triggered, this, &MainWindow::manageCookies);

    // Initialise system sleep inhibition timer
    m_pwr = new PowerManagement(this);
    m_preventTimer = new QTimer(this);
    m_preventTimer->setSingleShot(true);
    connect(m_preventTimer, &QTimer::timeout, this, &MainWindow::updatePowerManagementState);
    connect(pref, &Preferences::changed, this, &MainWindow::updatePowerManagementState);
    updatePowerManagementState();

    // Configure BT session according to options
    loadPreferences();

    connect(BitTorrent::Session::instance(), &BitTorrent::Session::statsUpdated, this, &MainWindow::loadSessionStats);
    connect(BitTorrent::Session::instance(), &BitTorrent::Session::torrentsUpdated, this, &MainWindow::reloadTorrentStats);

    createKeyboardShortcuts();

#ifdef Q_OS_MACOS
    setUnifiedTitleAndToolBarOnMac(true);
#endif

    // View settings
    m_ui->actionTopToolBar->setChecked(pref->isToolbarDisplayed());
    m_ui->actionShowStatusbar->setChecked(pref->isStatusbarDisplayed());
    m_ui->actionSpeedInTitleBar->setChecked(pref->speedInTitleBar());
    m_ui->actionExecutionLogs->setChecked(isExecutionLogEnabled());

    const Log::MsgTypes flags = executionLogMsgTypes();
    m_ui->actionNormalMessages->setChecked(flags.testFlag(Log::NORMAL));
    m_ui->actionInformationMessages->setChecked(flags.testFlag(Log::INFO));
    m_ui->actionWarningMessages->setChecked(flags.testFlag(Log::WARNING));
    m_ui->actionCriticalMessages->setChecked(flags.testFlag(Log::CRITICAL));

    on_actionExecutionLogs_triggered(m_ui->actionExecutionLogs->isChecked());
    on_actionNormalMessages_triggered(m_ui->actionNormalMessages->isChecked());
    on_actionInformationMessages_triggered(m_ui->actionInformationMessages->isChecked());
    on_actionWarningMessages_triggered(m_ui->actionWarningMessages->isChecked());
    on_actionCriticalMessages_triggered(m_ui->actionCriticalMessages->isChecked());
    // Auto shutdown actions
    auto *autoShutdownGroup = new QActionGroup(this);
    autoShutdownGroup->setExclusive(true);
    autoShutdownGroup->addAction(m_ui->actionAutoShutdownDisabled);
    autoShutdownGroup->addAction(m_ui->actionAutoExit);
    autoShutdownGroup->addAction(m_ui->actionAutoShutdown);
    autoShutdownGroup->addAction(m_ui->actionAutoSuspend);
    autoShutdownGroup->addAction(m_ui->actionAutoHibernate);
#if (!defined(Q_OS_UNIX) || defined(Q_OS_MACOS)) || defined(QBT_USES_DBUS)
    m_ui->actionAutoShutdown->setChecked(pref->shutdownWhenDownloadsComplete());
    m_ui->actionAutoSuspend->setChecked(pref->suspendWhenDownloadsComplete());
    m_ui->actionAutoHibernate->setChecked(pref->hibernateWhenDownloadsComplete());
#else
    m_ui->actionAutoShutdown->setDisabled(true);
    m_ui->actionAutoSuspend->setDisabled(true);
    m_ui->actionAutoHibernate->setDisabled(true);
#endif
    m_ui->actionAutoExit->setChecked(pref->shutdownqBTWhenDownloadsComplete());

    if (!autoShutdownGroup->checkedAction())
        m_ui->actionAutoShutdownDisabled->setChecked(true);

    // Load Window state and sizes
    loadSettings();

    populateDesktopIntegrationMenu();
#ifndef Q_OS_MACOS
    m_ui->actionLock->setVisible(app->desktopIntegration()->isActive());
    connect(app->desktopIntegration(), &DesktopIntegration::stateChanged, this, [this, app]()
    {
        m_ui->actionLock->setVisible(app->desktopIntegration()->isActive());
    });
#endif
    connect(app->desktopIntegration(), &DesktopIntegration::notificationClicked, this, &MainWindow::desktopNotificationClicked);
    connect(app->desktopIntegration(), &DesktopIntegration::activationRequested, this, [this]()
    {
#ifdef Q_OS_MACOS
        if (!isVisible())
            activate();
#else
        toggleVisibility();
#endif
    });

#ifdef Q_OS_MACOS
    if (initialState == WindowState::Normal)
    {
        show();
        activateWindow();
        raise();
    }
    else
    {
        // Make sure the Window is visible if we don't have a tray icon
        showMinimized();
    }
#else
    if (app->desktopIntegration()->isActive())
    {
        if ((initialState == WindowState::Normal) && !m_uiLocked)
        {
            show();
            activateWindow();
            raise();
        }
        else if (initialState == WindowState::Minimized)
        {
            showMinimized();
            if (pref->minimizeToTray())
            {
                hide();
                if (!pref->minimizeToTrayNotified())
                {
                    app->desktopIntegration()->showNotification(tr("qBittorrent is minimized to tray"), tr("This behavior can be changed in the settings. You won't be reminded again."));
                    pref->setMinimizeToTrayNotified(true);
                }
            }
        }
    }
    else
    {
        // Make sure the Window is visible if we don't have a tray icon
        if (initialState != WindowState::Normal)
        {
            showMinimized();
        }
        else
        {
            show();
            activateWindow();
            raise();
        }
    }
#endif

    const bool isFiltersSidebarVisible = pref->isFiltersSidebarVisible();
    m_ui->actionShowFiltersSidebar->setChecked(isFiltersSidebarVisible);
    if (isFiltersSidebarVisible)
    {
        showFiltersSidebar(true);
    }
    else
    {
        m_transferListWidget->applyStatusFilter(pref->getTransSelFilter());
        m_transferListWidget->applyCategoryFilter(QString());
        m_transferListWidget->applyTagFilter(std::nullopt);
        m_transferListWidget->applyTrackerFilterAll();
    }

    // Start watching the executable for updates
    m_executableWatcher = new QFileSystemWatcher(this);
    connect(m_executableWatcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::notifyOfUpdate);
    m_executableWatcher->addPath(qApp->applicationFilePath());

    m_transferListWidget->setFocus();

    // Update the number of torrents (tab)
    updateNbTorrents();
    connect(m_transferListWidget->getSourceModel(), &QAbstractItemModel::rowsInserted, this, &MainWindow::updateNbTorrents);
    connect(m_transferListWidget->getSourceModel(), &QAbstractItemModel::rowsRemoved, this, &MainWindow::updateNbTorrents);

    connect(pref, &Preferences::changed, this, &MainWindow::optionsSaved);

    // View menu: Cards View toggle
    m_actionCardsView = new QAction(tr("&Cards View"), this);
    m_actionCardsView->setCheckable(true);
    m_actionCardsView->setChecked(false);
    connect(m_actionCardsView, &QAction::triggered, this, &MainWindow::toggleCardsView);
    m_ui->menuView->addAction(m_actionCardsView);

    // Tools menu: Batch Operations and Portable Backup
    auto *actionBatchOps = new QAction(tr("&Batch Operations..."), this);
    actionBatchOps->setIcon(UIThemeManager::instance()->getIcon(u"edit-select-all"_s, u"select-all"_s));
    connect(actionBatchOps, &QAction::triggered, this, &MainWindow::showBatchOperationsDialog);
    m_ui->menuOptions->insertAction(m_ui->actionOptions, actionBatchOps);

    auto *actionPortableBackup = new QAction(tr("&Portable Backup..."), this);
    actionPortableBackup->setIcon(UIThemeManager::instance()->getIcon(u"document-save"_s));
    connect(actionPortableBackup, &QAction::triggered, this, &MainWindow::showPortableBackupDialog);
    m_ui->menuOptions->insertAction(m_ui->actionOptions, actionPortableBackup);

    m_ui->menuOptions->insertSeparator(m_ui->actionOptions);

    // Tray popup widget
    m_trayPopup = new TrayPopupWidget(this);

    qDebug("GUI Built");
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

bool MainWindow::isExecutionLogEnabled() const
{
    return m_storeExecutionLogEnabled;
}

void MainWindow::setExecutionLogEnabled(const bool value)
{
    m_storeExecutionLogEnabled = value;
}

Log::MsgTypes MainWindow::executionLogMsgTypes() const
{
    return m_storeExecutionLogTypes;
}

void MainWindow::setExecutionLogMsgTypes(const Log::MsgTypes value)
{
    m_executionLog->setMessageTypes(value);
    m_storeExecutionLogTypes = value;
}

bool MainWindow::isDownloadTrackerFavicon() const
{
    return m_storeDownloadTrackerFavicon;
}

void MainWindow::setDownloadTrackerFavicon(const bool value)
{
    if (m_transferListFiltersWidget)
        m_transferListFiltersWidget->setDownloadTrackerFavicon(value);
    m_storeDownloadTrackerFavicon = value;
}

void MainWindow::setTitleSuffix(const QString &suffix)
{
    const auto emDash = QChar(0x2014);
    const QString separator = u' ' + emDash + u' ';
    m_windowTitle = QStringLiteral("qBittorrent Vanced " QBT_VERSION)
        + (!suffix.isEmpty() ? (separator + suffix) : QString());

    refreshWindowTitle();
}

void MainWindow::addToolbarContextMenu()
{
    const Preferences *const pref = Preferences::instance();
    m_toolbarMenu = new QMenu(this);

    m_ui->toolBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ui->toolBar, &QWidget::customContextMenuRequested, this, &MainWindow::toolbarMenuRequested);

    QAction *iconsOnly = m_toolbarMenu->addAction(tr("Icons Only"), this, &MainWindow::toolbarIconsOnly);
    QAction *textOnly = m_toolbarMenu->addAction(tr("Text Only"), this, &MainWindow::toolbarTextOnly);
    QAction *textBesideIcons = m_toolbarMenu->addAction(tr("Text Alongside Icons"), this, &MainWindow::toolbarTextBeside);
    QAction *textUnderIcons = m_toolbarMenu->addAction(tr("Text Under Icons"), this, &MainWindow::toolbarTextUnder);
    QAction *followSystemStyle = m_toolbarMenu->addAction(tr("Follow System Style"), this, &MainWindow::toolbarFollowSystem);

    auto *textPositionGroup = new QActionGroup(m_toolbarMenu);
    textPositionGroup->addAction(iconsOnly);
    iconsOnly->setCheckable(true);
    textPositionGroup->addAction(textOnly);
    textOnly->setCheckable(true);
    textPositionGroup->addAction(textBesideIcons);
    textBesideIcons->setCheckable(true);
    textPositionGroup->addAction(textUnderIcons);
    textUnderIcons->setCheckable(true);
    textPositionGroup->addAction(followSystemStyle);
    followSystemStyle->setCheckable(true);

    const auto buttonStyle = static_cast<Qt::ToolButtonStyle>(pref->getToolbarTextPosition());
    if ((buttonStyle >= Qt::ToolButtonIconOnly) && (buttonStyle <= Qt::ToolButtonFollowStyle))
        m_ui->toolBar->setToolButtonStyle(buttonStyle);
    switch (buttonStyle)
    {
    case Qt::ToolButtonIconOnly:
        iconsOnly->setChecked(true);
        break;
    case Qt::ToolButtonTextOnly:
        textOnly->setChecked(true);
        break;
    case Qt::ToolButtonTextBesideIcon:
        textBesideIcons->setChecked(true);
        break;
    case Qt::ToolButtonTextUnderIcon:
        textUnderIcons->setChecked(true);
        break;
    default:
        followSystemStyle->setChecked(true);
    }
}

void MainWindow::manageCookies()
{
    auto *cookieDialog = new CookiesDialog(this);
    cookieDialog->setAttribute(Qt::WA_DeleteOnClose);
    cookieDialog->open();
}

void MainWindow::toolbarMenuRequested()
{
    m_toolbarMenu->popup(QCursor::pos());
}

void MainWindow::toolbarIconsOnly()
{
    m_ui->toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    Preferences::instance()->setToolbarTextPosition(Qt::ToolButtonIconOnly);
}

void MainWindow::toolbarTextOnly()
{
    m_ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    Preferences::instance()->setToolbarTextPosition(Qt::ToolButtonTextOnly);
}

void MainWindow::toolbarTextBeside()
{
    m_ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    Preferences::instance()->setToolbarTextPosition(Qt::ToolButtonTextBesideIcon);
}

void MainWindow::toolbarTextUnder()
{
    m_ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    Preferences::instance()->setToolbarTextPosition(Qt::ToolButtonTextUnderIcon);
}

void MainWindow::toolbarFollowSystem()
{
    m_ui->toolBar->setToolButtonStyle(Qt::ToolButtonFollowStyle);
    Preferences::instance()->setToolbarTextPosition(Qt::ToolButtonFollowStyle);
}

bool MainWindow::defineUILockPassword()
{
    bool ok = false;
    const QString newPassword = AutoExpandableDialog::getText(this, tr("UI lock password")
        , tr("Please type the UI lock password:"), QLineEdit::Password, {}, &ok);
    if (!ok)
        return false;

    if (newPassword.size() < 3)
    {
        QMessageBox::warning(this, tr("Invalid password"), tr("The password must be at least 3 characters long"));
        return false;
    }

    Preferences::instance()->setUILockPassword(Utils::Password::PBKDF2::generate(newPassword));
    return true;
}

void MainWindow::clearUILockPassword()
{
    const QMessageBox::StandardButton answer = QMessageBox::question(this, tr("Clear the password")
        , tr("Are you sure you want to clear the password?"), (QMessageBox::Yes | QMessageBox::No), QMessageBox::No);
    if (answer == QMessageBox::Yes)
        Preferences::instance()->setUILockPassword({});
}

void MainWindow::on_actionLock_triggered()
{
    Preferences *const pref = Preferences::instance();

    // Check if there is a password
    if (pref->getUILockPassword().isEmpty())
    {
        if (!defineUILockPassword())
            return;
    }

    // Lock the interface
    m_uiLocked = true;
    pref->setUILocked(true);
    app()->desktopIntegration()->menu()->setEnabled(false);
    hide();
}

void MainWindow::showFilterContextMenu()
{
    const Preferences *pref = Preferences::instance();

    QMenu *menu = m_columnFilterEdit->createStandardContextMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->addSeparator();

    QAction *useRegexAct = menu->addAction(tr("Use regular expressions"));
    useRegexAct->setCheckable(true);
    useRegexAct->setChecked(pref->getRegexAsFilteringPatternForTransferList());
    connect(useRegexAct, &QAction::toggled, pref, &Preferences::setRegexAsFilteringPatternForTransferList);
    connect(useRegexAct, &QAction::toggled, this, &MainWindow::applyTransferListFilter);

    menu->popup(QCursor::pos());
}

void MainWindow::toggleFocusBetweenLineEdits()
{
    if (m_columnFilterEdit->hasFocus() && (m_propertiesWidget->tabBar()->currentIndex() == PropTabBar::FilesTab))
    {
        m_propertiesWidget->contentFilterLine()->setFocus();
        m_propertiesWidget->contentFilterLine()->selectAll();
    }
    else
    {
        m_columnFilterEdit->setFocus();
        m_columnFilterEdit->selectAll();
    }
}

void MainWindow::updateNbTorrents()
{
    m_tabs->setTabText(0, tr("Transfers (%1)").arg(m_transferListWidget->getSourceModel()->rowCount()));
}

void MainWindow::on_actionDocumentation_triggered() const
{
    QDesktopServices::openUrl(QUrl(u"https://doc.qbittorrent.org"_s));
}

void MainWindow::tabChanged([[maybe_unused]] const int newTab)
{
    // We cannot rely on the index newTab
    // because the tab order is undetermined now
    if (m_tabs->currentWidget() == m_splitter)
    {
        qDebug("Changed tab to transfer list, refreshing the list");
        m_propertiesWidget->loadDynamicData();
        m_columnFilterAction->setVisible(true);
        return;
    }
    m_columnFilterAction->setVisible(false);
}

void MainWindow::toggleCardsView()
{
    if (m_actionCardsView->isChecked())
    {
        if (!m_cardsWidget)
        {
            m_cardsWidget = new TorrentCardsWidget(this);
            connect(m_cardsWidget, &TorrentCardsWidget::torrentDoubleClicked, this, [this](BitTorrent::Torrent *torrent)
            {
                m_propertiesWidget->loadTorrentInfos(torrent);
                // Switch back to list view for detailed torrent info
                m_actionCardsView->setChecked(false);
                toggleCardsView();
            });
        }
        // Replace the transfer list with cards view in the Transfers tab
        m_splitter->hide();
        m_tabs->removeTab(0);
        m_tabs->insertTab(0, m_cardsWidget,
#ifndef Q_OS_MACOS
            UIThemeManager::instance()->getIcon(u"folder-remote"_s),
#endif
            tr("Transfers (%1)").arg(m_transferListWidget->getSourceModel()->rowCount()));
        m_tabs->setCurrentIndex(0);
        m_cardsWidget->refreshCards();
    }
    else
    {
        if (m_cardsWidget)
        {
            m_tabs->removeTab(0);
            m_tabs->insertTab(0, m_splitter,
#ifndef Q_OS_MACOS
                UIThemeManager::instance()->getIcon(u"folder-remote"_s),
#endif
                tr("Transfers (%1)").arg(m_transferListWidget->getSourceModel()->rowCount()));
            m_splitter->show();
            m_tabs->setCurrentIndex(0);
        }
    }
}

void MainWindow::showBatchOperationsDialog()
{
    const QList<BitTorrent::Torrent *> selectedTorrents = m_transferListWidget->getSelectedTorrents();
    if (selectedTorrents.isEmpty())
    {
        QMessageBox::information(this, tr("Batch Operations"), tr("Please select one or more torrents first."));
        return;
    }

    auto *dlg = new BatchOperationsDialog(selectedTorrents, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->open();
}

void MainWindow::showPortableBackupDialog()
{
    auto *dlg = new PortableBackupDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->open();
}

void MainWindow::saveSettings() const
{
    auto *pref = Preferences::instance();
    pref->setMainGeometry(saveGeometry());
    m_propertiesWidget->saveSettings();
}

void MainWindow::saveSplitterSettings() const
{
    if (!m_transferListFiltersWidget)
        return;

    auto *pref = Preferences::instance();
    pref->setFiltersSidebarWidth(m_splitter->sizes()[0]);
}

void MainWindow::cleanup()
{
    if (!m_neverShown)
    {
        saveSettings();
        saveSplitterSettings();
    }

    delete m_cardsWidget;

    delete m_executableWatcher;

    m_preventTimer->stop();

    // remove all child widgets
    while (auto *w = findChild<QWidget *>())
        delete w;
}

void MainWindow::loadSettings()
{
    const auto *pref = Preferences::instance();

    if (const QByteArray mainGeo = pref->getMainGeometry();
        !mainGeo.isEmpty() && restoreGeometry(mainGeo))
    {
        m_posInitialized = true;
    }
}

void MainWindow::desktopNotificationClicked()
{
    if (isHidden())
    {
        if (m_uiLocked)
        {
            // Ask for UI lock password
            if (!unlockUI())
                return;
        }
        show();
        if (isMinimized())
            showNormal();
    }

    raise();
    activateWindow();
}

void MainWindow::createKeyboardShortcuts()
{
    m_ui->actionOpen->setShortcut(QKeySequence::Open);
    m_ui->actionDelete->setShortcut(QKeySequence::Delete);
    m_ui->actionDelete->setShortcutContext(Qt::WidgetShortcut);  // nullify its effect: delete key event is handled by respective widgets, not here
    m_ui->actionDownloadFromURL->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_O);
    m_ui->actionExit->setShortcut(Qt::CTRL | Qt::Key_Q);
#ifdef Q_OS_MACOS
    m_ui->actionCloseWindow->setShortcut(QKeySequence::Close);
#else
    m_ui->actionCloseWindow->setVisible(false);
#endif

    const auto *switchTransferShortcut = new QShortcut((Qt::ALT | Qt::Key_1), this);
    connect(switchTransferShortcut, &QShortcut::activated, this, &MainWindow::displayTransferTab);
    const auto *switchExecutionLogShortcut = new QShortcut((Qt::ALT | Qt::Key_2), this);
    connect(switchExecutionLogShortcut, &QShortcut::activated, this, &MainWindow::displayExecutionLogTab);
    const auto *switchSearchFilterShortcut = new QShortcut(QKeySequence::Find, m_transferListWidget);
    connect(switchSearchFilterShortcut, &QShortcut::activated, this, &MainWindow::toggleFocusBetweenLineEdits);
    const auto *switchSearchFilterShortcutAlternative = new QShortcut((Qt::CTRL | Qt::Key_E), m_transferListWidget);
    connect(switchSearchFilterShortcutAlternative, &QShortcut::activated, this, &MainWindow::toggleFocusBetweenLineEdits);

    m_ui->actionDocumentation->setShortcut(QKeySequence::HelpContents);
    m_ui->actionOptions->setShortcut(Qt::ALT | Qt::Key_O);
    m_ui->actionStart->setShortcut(Qt::CTRL | Qt::Key_S);
    m_ui->actionStop->setShortcut(Qt::CTRL | Qt::Key_P);
    m_ui->actionPauseSession->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_P);
    m_ui->actionResumeSession->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
    m_ui->actionBottomQueuePos->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Minus);
    m_ui->actionDecreaseQueuePos->setShortcut(Qt::CTRL | Qt::Key_Minus);
    m_ui->actionIncreaseQueuePos->setShortcut(Qt::CTRL | Qt::Key_Plus);
    m_ui->actionTopQueuePos->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_Plus);
#ifdef Q_OS_MACOS
    m_ui->actionMinimize->setShortcut(Qt::CTRL | Qt::Key_M);
    addAction(m_ui->actionMinimize);
#endif
}

// Keyboard shortcuts slots
void MainWindow::displayTransferTab() const
{
    m_tabs->setCurrentWidget(m_splitter);
}

void MainWindow::displayExecutionLogTab()
{
    if (!m_executionLog)
    {
        m_ui->actionExecutionLogs->setChecked(true);
        on_actionExecutionLogs_triggered(true);
    }

    m_tabs->setCurrentWidget(m_executionLog);
}

// End of keyboard shortcuts slots

void MainWindow::on_actionSetGlobalSpeedLimits_triggered()
{
    auto *dialog = new SpeedLimitDialog {this};
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->open();
}

// Necessary if we want to close the window
// in one time if "close to systray" is enabled
void MainWindow::on_actionExit_triggered()
{
    // UI locking enforcement.
    if (isHidden() && m_uiLocked)
        // Ask for UI lock password
        if (!unlockUI()) return;

    m_forceExit = true;
    close();
}

#ifdef Q_OS_MACOS
void MainWindow::on_actionCloseWindow_triggered()
{
    // On macOS window close is basically equivalent to window hide.
    // If you decide to implement this functionality for other OS,
    // then you will also need ui lock checks like in actionExit.
    close();
}
#endif

QWidget *MainWindow::currentTabWidget() const
{
    if (isMinimized() || !isVisible())
        return nullptr;
    if (m_tabs->currentIndex() == 0)
        return m_transferListWidget;
    return m_tabs->currentWidget();
}

TransferListWidget *MainWindow::transferListWidget() const
{
    return m_transferListWidget;
}

bool MainWindow::unlockUI()
{
    if (m_unlockDlgShowing)
        return false;

    bool ok = false;
    const QString password = AutoExpandableDialog::getText(this, tr("UI lock password")
        , tr("Please type the UI lock password:"), QLineEdit::Password, {}, &ok);
    if (!ok) return false;

    Preferences *const pref = Preferences::instance();

    const QByteArray secret = pref->getUILockPassword();
    if (!Utils::Password::PBKDF2::verify(secret, password))
    {
        QMessageBox::warning(this, tr("Invalid password"), tr("The password is invalid"));
        return false;
    }

    m_uiLocked = false;
    pref->setUILocked(false);
    app()->desktopIntegration()->menu()->setEnabled(true);
    return true;
}

void MainWindow::notifyOfUpdate(const QString &)
{
    // Show restart message
    m_statusBar->showRestartRequired();
    LogMsg(tr("qBittorrent was just updated and needs to be restarted for the changes to be effective.")
                                   , Log::CRITICAL);
    // Delete the executable watcher
    delete m_executableWatcher;
    m_executableWatcher = nullptr;
}

#ifndef Q_OS_MACOS
// Toggle Main window visibility
void MainWindow::toggleVisibility()
{
    if (isHidden())
    {
        if (m_uiLocked && !unlockUI())  // Ask for UI lock password
            return;

        // Make sure the window is not minimized
        setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);

        // Then show it
        show();
        raise();
        activateWindow();
    }
    else
    {
        hide();
    }
}
#endif // Q_OS_MACOS

// Display About Dialog
void MainWindow::on_actionAbout_triggered()
{
    // About dialog
    if (m_aboutDlg)
    {
        m_aboutDlg->activateWindow();
    }
    else
    {
        m_aboutDlg = new AboutDialog(this);
        m_aboutDlg->setAttribute(Qt::WA_DeleteOnClose);
        m_aboutDlg->show();
    }
}


void MainWindow::showEvent(QShowEvent *e)
{
    qDebug("** Show Event **");
    e->accept();

    if (isVisible())
    {
        // preparations before showing the window

        if (m_neverShown)
        {
            m_propertiesWidget->readSettings();
            m_neverShown = false;
        }

        if (currentTabWidget() == m_transferListWidget)
            m_propertiesWidget->loadDynamicData();

        // Make sure the window is initially centered
        if (!m_posInitialized)
        {
            move(Utils::Gui::screenCenter(this));
            m_posInitialized = true;
        }
    }
    else
    {
        // to avoid blank screen when restoring from tray icon
        show();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Paste))
    {
        const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData();
        if (mimeData->hasText())
        {
            const QStringList lines = mimeData->text().split(u'\n', Qt::SkipEmptyParts);
            for (QString line : lines)
            {
                line = line.trimmed();
                if (!Utils::Misc::isTorrentLink(line))
                    continue;

                app()->addTorrentManager()->addTorrent(line);
            }

            return;
        }
    }

    QMainWindow::keyPressEvent(event);
}

// Called when we close the program
void MainWindow::closeEvent(QCloseEvent *e)
{
    Preferences *const pref = Preferences::instance();
#ifdef Q_OS_MACOS
    if (!m_forceExit)
    {
        hide();
        e->ignore();
        return;
    }
#else
    const bool goToSystrayOnExit = pref->closeToTray();
    if (!m_forceExit && app()->desktopIntegration()->isActive() && goToSystrayOnExit && !this->isHidden())
    {
        e->ignore();
        QMetaObject::invokeMethod(this, &QWidget::hide, Qt::QueuedConnection);
        if (!pref->closeToTrayNotified())
        {
            app()->desktopIntegration()->showNotification(tr("qBittorrent is closed to tray"), tr("This behavior can be changed in the settings. You won't be reminded again."));
            pref->setCloseToTrayNotified(true);
        }
        return;
    }
#endif // Q_OS_MACOS

    const QList<BitTorrent::Torrent *> allTorrents = BitTorrent::Session::instance()->torrents();
    const bool hasActiveTorrents = std::any_of(allTorrents.cbegin(), allTorrents.cend(), [](BitTorrent::Torrent *torrent)
    {
        return torrent->isActive();
    });
    if (pref->confirmOnExit() && hasActiveTorrents)
    {
        if (e->spontaneous() || m_forceExit)
        {
            if (!isVisible())
                show();
            QMessageBox confirmBox(QMessageBox::Question, tr("Exiting qBittorrent"),
                                   // Split it because the last sentence is used in the WebUI
                                   tr("Some files are currently transferring.") + u'\n' + tr("Are you sure you want to quit qBittorrent?"),
                                   QMessageBox::NoButton, this);
            QPushButton *noBtn = confirmBox.addButton(tr("&No"), QMessageBox::NoRole);
            confirmBox.addButton(tr("&Yes"), QMessageBox::YesRole);
            QPushButton *alwaysBtn = confirmBox.addButton(tr("&Always Yes"), QMessageBox::YesRole);
            confirmBox.setDefaultButton(noBtn);
            confirmBox.exec();
            if (!confirmBox.clickedButton() || (confirmBox.clickedButton() == noBtn))
            {
                // Cancel exit
                e->ignore();
                m_forceExit = false;
                return;
            }
            if (confirmBox.clickedButton() == alwaysBtn)
                // Remember choice
                Preferences::instance()->setConfirmOnExit(false);
        }
    }

    // Accept exit
    e->accept();
    qApp->exit();
}

bool MainWindow::event(QEvent *e)
{
#ifndef Q_OS_MACOS
    switch (e->type())
    {
    case QEvent::WindowStateChange:
        qDebug("Window change event");
        // Now check to see if the window is minimised
        if (isMinimized())
        {
            qDebug("minimisation");
            Preferences *const pref = Preferences::instance();
            if (app()->desktopIntegration()->isActive() && pref->minimizeToTray())
            {
                qDebug() << "Has active window:" << (qApp->activeWindow() != nullptr);
                // Check if there is a modal window
                const QWidgetList allWidgets = QApplication::allWidgets();
                const bool hasModalWindow = std::any_of(allWidgets.cbegin(), allWidgets.cend()
                    , [](const QWidget *widget) { return widget->isModal(); });
                // Iconify if there is no modal window
                if (!hasModalWindow)
                {
                    qDebug("Minimize to Tray enabled, hiding!");
                    e->ignore();
                    QMetaObject::invokeMethod(this, &QWidget::hide, Qt::QueuedConnection);
                    if (!pref->minimizeToTrayNotified())
                    {
                        app()->desktopIntegration()->showNotification(tr("qBittorrent is minimized to tray"), tr("This behavior can be changed in the settings. You won't be reminded again."));
                        pref->setMinimizeToTrayNotified(true);
                    }
                    return true;
                }
            }
        }
        break;
    case QEvent::ToolBarChange:
        {
            qDebug("MAC: Received a toolbar change event!");
            const bool ret = QMainWindow::event(e);

            qDebug("MAC: new toolbar visibility is %d", !m_ui->actionTopToolBar->isChecked());
            m_ui->actionTopToolBar->toggle();
            Preferences::instance()->setToolbarDisplayed(m_ui->actionTopToolBar->isChecked());
            return ret;
        }
    default:
        break;
    }
#endif // Q_OS_MACOS

    return QMainWindow::event(e);
}

// Display a dialog to allow user to add
// torrents to download list
void MainWindow::on_actionOpen_triggered()
{
    Preferences *const pref = Preferences::instance();
    // Open File Open Dialog
    // Note: it is possible to select more than one file
    const QStringList pathsList = QFileDialog::getOpenFileNames(this, tr("Open Torrent Files")
            , pref->getMainLastDir().data(),  tr("Torrent Files") + u" (*" + TORRENT_FILE_EXTENSION + u')');

    if (pathsList.isEmpty())
        return;

    for (const QString &file : pathsList)
        app()->addTorrentManager()->addTorrent(file);

    // Save last dir to remember it
    const Path topDir {pathsList.at(0)};
    const Path parentDir = topDir.parentPath();
    pref->setMainLastDir(parentDir.isEmpty() ? topDir : parentDir);
}

void MainWindow::activate()
{
    if (!m_uiLocked || unlockUI())
    {
        show();
        activateWindow();
        raise();
    }
}

void MainWindow::optionsSaved()
{
    LogMsg(tr("Options saved."));
    loadPreferences();
}

void MainWindow::showStatusBar(bool show)
{
    if (!show)
    {
        // Remove status bar
        setStatusBar(nullptr);
    }
    else if (!m_statusBar)
    {
        // Create status bar
        m_statusBar = new StatusBar;
        connect(m_statusBar.data(), &StatusBar::connectionButtonClicked, this, &MainWindow::showConnectionSettings);
        connect(m_statusBar.data(), &StatusBar::alternativeSpeedsButtonClicked, this, &MainWindow::toggleAlternativeSpeeds);
        setStatusBar(m_statusBar);
    }
}

void MainWindow::showFiltersSidebar(const bool show)
{
    if (show && !m_transferListFiltersWidget)
    {
        m_transferListFiltersWidget = new TransferListFiltersWidget(m_splitter, m_transferListWidget, isDownloadTrackerFavicon());
        connect(BitTorrent::Session::instance(), &BitTorrent::Session::trackersAdded, m_transferListFiltersWidget, &TransferListFiltersWidget::addTrackers);
        connect(BitTorrent::Session::instance(), &BitTorrent::Session::trackersRemoved, m_transferListFiltersWidget, &TransferListFiltersWidget::removeTrackers);
        connect(BitTorrent::Session::instance(), &BitTorrent::Session::trackersChanged, m_transferListFiltersWidget, &TransferListFiltersWidget::refreshTrackers);
        connect(BitTorrent::Session::instance(), &BitTorrent::Session::trackerEntryStatusesUpdated, m_transferListFiltersWidget, &TransferListFiltersWidget::trackerEntryStatusesUpdated);

        m_splitter->insertWidget(0, m_transferListFiltersWidget);
        m_splitter->setCollapsible(0, true);
        // From https://doc.qt.io/qt-5/qsplitter.html#setSizes:
        // Instead, any additional/missing space is distributed amongst the widgets
        // according to the relative weight of the sizes.
        m_splitter->setStretchFactor(0, 0);
        m_splitter->setStretchFactor(1, 1);
        m_splitter->setSizes({Preferences::instance()->getFiltersSidebarWidth()});
    }
    else if (!show && m_transferListFiltersWidget)
    {
        saveSplitterSettings();
        delete m_transferListFiltersWidget;
        m_transferListFiltersWidget = nullptr;
    }
}

void MainWindow::loadPreferences()
{
    const Preferences *pref = Preferences::instance();

    // General
    if (pref->isToolbarDisplayed())
    {
        m_ui->toolBar->setVisible(true);
    }
    else
    {
        // Clear search filter before hiding the top toolbar
        m_columnFilterEdit->clear();
        m_ui->toolBar->setVisible(false);
    }

    showStatusBar(pref->isStatusbarDisplayed());

    m_transferListWidget->setAlternatingRowColors(pref->useAlternatingRowColors());
    m_propertiesWidget->getFilesList()->setAlternatingRowColors(pref->useAlternatingRowColors());
    m_propertiesWidget->getTrackerList()->setAlternatingRowColors(pref->useAlternatingRowColors());
    m_propertiesWidget->getPeerList()->setAlternatingRowColors(pref->useAlternatingRowColors());

    // Queueing System
    if (BitTorrent::Session::instance()->isQueueingSystemEnabled())
    {
        if (!m_ui->actionDecreaseQueuePos->isVisible())
        {
            m_transferListWidget->hideQueuePosColumn(false);
            m_ui->actionDecreaseQueuePos->setVisible(true);
            m_ui->actionIncreaseQueuePos->setVisible(true);
            m_ui->actionTopQueuePos->setVisible(true);
            m_ui->actionBottomQueuePos->setVisible(true);
#ifndef Q_OS_MACOS
            m_queueSeparator->setVisible(true);
#endif
            m_queueSeparatorMenu->setVisible(true);
        }
    }
    else
    {
        if (m_ui->actionDecreaseQueuePos->isVisible())
        {
            m_transferListWidget->hideQueuePosColumn(true);
            m_ui->actionDecreaseQueuePos->setVisible(false);
            m_ui->actionIncreaseQueuePos->setVisible(false);
            m_ui->actionTopQueuePos->setVisible(false);
            m_ui->actionBottomQueuePos->setVisible(false);
#ifndef Q_OS_MACOS
            m_queueSeparator->setVisible(false);
#endif
            m_queueSeparatorMenu->setVisible(false);
        }
    }

    // Torrent properties
    m_propertiesWidget->reloadPreferences();

    qDebug("GUI settings loaded");
}

void MainWindow::loadSessionStats()
{
    const auto *btSession = BitTorrent::Session::instance();
    const BitTorrent::SessionStatus &status = btSession->status();
    m_downloadRate = Utils::Misc::friendlyUnit(status.payloadDownloadRate, true);
    m_uploadRate = Utils::Misc::friendlyUnit(status.payloadUploadRate, true);

    // Update inline speed buttons
    if (m_inlineDlSpeedBtn)
    {
        const int dlLimit = btSession->globalDownloadSpeedLimit();
        const QString dlLimitStr = (dlLimit <= 0) ? tr("Unlimited") : Utils::Misc::friendlyUnit(dlLimit, true);
        m_inlineDlSpeedBtn->setText(u"D: %1 [%2]"_s.arg(m_downloadRate, dlLimitStr));
    }
    if (m_inlineUlSpeedBtn)
    {
        const int ulLimit = btSession->globalUploadSpeedLimit();
        const QString ulLimitStr = (ulLimit <= 0) ? tr("Unlimited") : Utils::Misc::friendlyUnit(ulLimit, true);
        m_inlineUlSpeedBtn->setText(u"U: %1 [%2]"_s.arg(m_uploadRate, ulLimitStr));
    }

    // update global information
#ifdef Q_OS_MACOS
    m_badger->updateSpeed(status.payloadDownloadRate, status.payloadUploadRate);
#else
    refreshTrayIconTooltip();
#endif  // Q_OS_MACOS

    refreshWindowTitle();
}

void MainWindow::reloadTorrentStats(const QList<BitTorrent::Torrent *> &torrents)
{
    if (currentTabWidget() == m_transferListWidget)
    {
        if (torrents.contains(m_propertiesWidget->getCurrentTorrent()))
            m_propertiesWidget->loadDynamicData();
    }
}

void MainWindow::downloadFromURLList(const QStringList &urlList)
{
    for (const QString &url : urlList)
        app()->addTorrentManager()->addTorrent(url);
}

void MainWindow::populateDesktopIntegrationMenu()
{
    auto *menu = app()->desktopIntegration()->menu();
    menu->clear();

#ifndef Q_OS_MACOS
    connect(menu, &QMenu::aboutToShow, this, [this]()
    {
        m_ui->actionToggleVisibility->setText(isVisible() ? tr("Hide") : tr("Show"));
    });
    connect(m_ui->actionToggleVisibility, &QAction::triggered, this, &MainWindow::toggleVisibility);

    menu->addAction(m_ui->actionToggleVisibility);
    menu->addSeparator();
#endif

    menu->addAction(m_ui->actionOpen);
    menu->addAction(m_ui->actionDownloadFromURL);
    menu->addSeparator();

    menu->addAction(m_ui->actionUseAlternativeSpeedLimits);
    menu->addAction(m_ui->actionSetGlobalSpeedLimits);
    menu->addSeparator();

    menu->addAction(m_ui->actionResumeSession);
    menu->addAction(m_ui->actionPauseSession);
    menu->addSeparator();

    auto *actionStatusPopup = new QAction(tr("Status &Popup"), menu);
    connect(actionStatusPopup, &QAction::triggered, this, [this]()
    {
        if (m_trayPopup)
            m_trayPopup->showAtTrayIcon();
    });
    menu->addAction(actionStatusPopup);

#ifndef Q_OS_MACOS
    menu->addSeparator();
    menu->addAction(m_ui->actionExit);
#endif

    if (m_uiLocked)
        menu->setEnabled(false);
}

void MainWindow::updateAltSpeedsBtn(const bool alternative)
{
    m_ui->actionUseAlternativeSpeedLimits->setChecked(alternative);
}

PropertiesWidget *MainWindow::propertiesWidget() const
{
    return m_propertiesWidget;
}

// Display Program Options
void MainWindow::on_actionOptions_triggered()
{
    if (m_options)
    {
        m_options->activateWindow();
    }
    else
    {
        m_options = new OptionsDialog(app(), this);
        m_options->setAttribute(Qt::WA_DeleteOnClose);
        m_options->open();
    }
}

void MainWindow::on_actionTopToolBar_triggered()
{
    const bool isVisible = static_cast<QAction *>(sender())->isChecked();
    m_ui->toolBar->setVisible(isVisible);
    Preferences::instance()->setToolbarDisplayed(isVisible);
}

void MainWindow::on_actionShowStatusbar_triggered()
{
    const bool isVisible = static_cast<QAction *>(sender())->isChecked();
    Preferences::instance()->setStatusbarDisplayed(isVisible);
    showStatusBar(isVisible);
}

void MainWindow::on_actionShowFiltersSidebar_triggered(const bool checked)
{
    Preferences *const pref = Preferences::instance();
    pref->setFiltersSidebarVisible(checked);
    showFiltersSidebar(checked);
}

void MainWindow::on_actionSpeedInTitleBar_triggered()
{
    m_displaySpeedInTitle = static_cast<QAction *>(sender())->isChecked();
    Preferences::instance()->showSpeedInTitleBar(m_displaySpeedInTitle);
    refreshWindowTitle();
}

// Display an input dialog to prompt user for
// an url
void MainWindow::on_actionDownloadFromURL_triggered()
{
    if (!m_downloadFromURLDialog)
    {
        m_downloadFromURLDialog = new DownloadFromURLDialog(this);
        m_downloadFromURLDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_downloadFromURLDialog.data(), &DownloadFromURLDialog::urlsReadyToBeDownloaded, this, &MainWindow::downloadFromURLList);
        m_downloadFromURLDialog->open();
    }
}

void MainWindow::toggleAlternativeSpeeds()
{
    BitTorrent::Session *const session = BitTorrent::Session::instance();
    session->setAltGlobalSpeedLimitEnabled(!session->isAltGlobalSpeedLimitEnabled());
}

void MainWindow::on_actionDonateMoney_triggered()
{
}

void MainWindow::showConnectionSettings()
{
    on_actionOptions_triggered();
    m_options->showConnectionTab();
}

void MainWindow::minimizeWindow()
{
    setWindowState(windowState() | Qt::WindowMinimized);
}

void MainWindow::on_actionExecutionLogs_triggered(bool checked)
{
    if (checked)
    {
        Q_ASSERT(!m_executionLog);
        m_executionLog = new ExecutionLogWidget(executionLogMsgTypes(), m_tabs);
#ifdef Q_OS_MACOS
        m_tabs->addTab(m_executionLog, tr("Execution Log"));
#else
        const int indexTab = m_tabs->addTab(m_executionLog, tr("Execution Log"));
        m_tabs->setTabIcon(indexTab, UIThemeManager::instance()->getIcon(u"help-contents"_s));
#endif
    }
    else
    {
        delete m_executionLog;
    }

    m_ui->actionNormalMessages->setEnabled(checked);
    m_ui->actionInformationMessages->setEnabled(checked);
    m_ui->actionWarningMessages->setEnabled(checked);
    m_ui->actionCriticalMessages->setEnabled(checked);
    setExecutionLogEnabled(checked);
}

void MainWindow::on_actionNormalMessages_triggered(const bool checked)
{
    if (!m_executionLog)
        return;

    const Log::MsgTypes flags = executionLogMsgTypes().setFlag(Log::NORMAL, checked);
    setExecutionLogMsgTypes(flags);
}

void MainWindow::on_actionInformationMessages_triggered(const bool checked)
{
    if (!m_executionLog)
        return;

    const Log::MsgTypes flags = executionLogMsgTypes().setFlag(Log::INFO, checked);
    setExecutionLogMsgTypes(flags);
}

void MainWindow::on_actionWarningMessages_triggered(const bool checked)
{
    if (!m_executionLog)
        return;

    const Log::MsgTypes flags = executionLogMsgTypes().setFlag(Log::WARNING, checked);
    setExecutionLogMsgTypes(flags);
}

void MainWindow::on_actionCriticalMessages_triggered(const bool checked)
{
    if (!m_executionLog)
        return;

    const Log::MsgTypes flags = executionLogMsgTypes().setFlag(Log::CRITICAL, checked);
    setExecutionLogMsgTypes(flags);
}

void MainWindow::on_actionAutoExit_toggled(bool enabled)
{
    qDebug() << Q_FUNC_INFO << enabled;
    Preferences::instance()->setShutdownqBTWhenDownloadsComplete(enabled);
}

void MainWindow::on_actionAutoSuspend_toggled(bool enabled)
{
    qDebug() << Q_FUNC_INFO << enabled;
    Preferences::instance()->setSuspendWhenDownloadsComplete(enabled);
}

void MainWindow::on_actionAutoHibernate_toggled(bool enabled)
{
    qDebug() << Q_FUNC_INFO << enabled;
    Preferences::instance()->setHibernateWhenDownloadsComplete(enabled);
}

void MainWindow::on_actionAutoShutdown_toggled(bool enabled)
{
    qDebug() << Q_FUNC_INFO << enabled;
    Preferences::instance()->setShutdownWhenDownloadsComplete(enabled);
}

void MainWindow::updatePowerManagementState() const
{
    const auto *pref = Preferences::instance();
    const bool preventFromSuspendWhenDownloading = pref->preventFromSuspendWhenDownloading();
    const bool preventFromSuspendWhenSeeding = pref->preventFromSuspendWhenSeeding();

    const QList<BitTorrent::Torrent *> allTorrents = BitTorrent::Session::instance()->torrents();
    const bool inhibitSuspend = std::any_of(allTorrents.cbegin(), allTorrents.cend(), [&](const BitTorrent::Torrent *torrent)
    {
        if (preventFromSuspendWhenDownloading && (!torrent->isFinished() && !torrent->isStopped() && !torrent->isErrored() && torrent->hasMetadata()))
            return true;

        if (preventFromSuspendWhenSeeding && (torrent->isFinished() && !torrent->isStopped()))
            return true;

        return torrent->isMoving();
    });
    m_pwr->setActivityState(inhibitSuspend);

    m_preventTimer->start(PREVENT_SUSPEND_INTERVAL);
}

void MainWindow::applyTransferListFilter()
{
    m_transferListWidget->applyFilter(m_columnFilterEdit->text(), TransferListModel::Column::TR_NAME);
}

void MainWindow::refreshWindowTitle()
{
    const auto *btSession = BitTorrent::Session::instance();
    if (btSession->isPaused())
    {
        const QString title = tr("[PAUSED] %1", "%1 is the rest of the window title").arg(m_windowTitle);
        setWindowTitle(title);
    }
    else
    {
        if (m_displaySpeedInTitle)
        {
            const QString title = tr("[D: %1, U: %2] %3", "D = Download; U = Upload; %3 is the rest of the window title")
                    .arg(m_downloadRate, m_uploadRate, m_windowTitle);
            setWindowTitle(title);
        }
        else
        {
            setWindowTitle(m_windowTitle);
        }
    }
}

void MainWindow::refreshTrayIconTooltip()
{
    const auto *btSession = BitTorrent::Session::instance();
    if (!btSession->isPaused())
    {
        const auto toolTip = u"%1\n%2"_s.arg(
                tr("DL speed: %1", "e.g: Download speed: 10 KiB/s").arg(m_downloadRate)
                , tr("UP speed: %1", "e.g: Upload speed: 10 KiB/s").arg(m_uploadRate));
        app()->desktopIntegration()->setToolTip(toolTip);
    }
    else
    {
        app()->desktopIntegration()->setToolTip(tr("Paused"));
    }
}

