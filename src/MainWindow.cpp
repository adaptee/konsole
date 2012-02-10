/*
    Copyright 2006-2008 by Robert Knight <robertknight@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

// Own
#include "MainWindow.h"

// Qt
#include <QtGui/QBoxLayout>

// KDE
#include <KAcceleratorManager>
#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KApplication>
#include <KCmdLineArgs>
#include <KShortcutsDialog>
#include <KLocale>
#include <KMenu>
#include <KMenuBar>
#include <KMessageBox>
#include <KService>
#include <KToggleAction>
#include <KToggleFullScreenAction>
#include <KToolInvocation>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <KWindowSystem>
#include <KXMLGUIFactory>
#include <KNotifyConfigWidget>
#include <KConfigDialog>

// Konsole
#include "BookmarkHandler.h"
#include "IncrementalSearchBar.h"
#include "SessionController.h"
#include "ProfileList.h"
#include "ManageProfilesDialog.h"
#include "Session.h"
#include "ViewManager.h"
#include "SessionManager.h"
#include "KonsoleSettings.h"
#include "settings/GeneralSettings.h"
#include "settings/TabBarSettings.h"

using namespace Konsole;

static bool useTransparency()
{
    const KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    const bool compositingAvailable = KWindowSystem::compositingActive() ||
                                      args->isSet("force-transparency");
    return compositingAvailable && args->isSet("transparency");
}

MainWindow::MainWindow()
    : KXmlGuiWindow() ,
      _bookmarkHandler(0),
      _pluggedController(0),
      _menuBarInitialVisibilityApplied(false)
{
    if (useTransparency()) {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_NoSystemBackground, false);
    }

    // create actions for menus
    setupActions();

    // create view manager
    _viewManager = new ViewManager(this, actionCollection());
    connect(_viewManager , SIGNAL(empty()) , this , SLOT(close()));
    connect(_viewManager , SIGNAL(activeViewChanged(SessionController*)) , this ,
            SLOT(activeViewChanged(SessionController*)));
    connect(_viewManager , SIGNAL(unplugController(SessionController*)) , this ,
            SLOT(disconnectController(SessionController*)));
    connect(_viewManager , SIGNAL(viewPropertiesChanged(QList<ViewProperties*>)) ,
            bookmarkHandler() , SLOT(setViews(QList<ViewProperties*>)));

    connect(_viewManager , SIGNAL(setSaveGeometryOnExitRequest(bool)) , this ,
            SLOT(setSaveGeometryOnExit(bool)));
    connect(_viewManager , SIGNAL(updateWindowIcon()) , this ,
            SLOT(updateWindowIcon()));
    connect(_viewManager , SIGNAL(newViewRequest(Profile::Ptr)) ,
            this , SLOT(newFromProfile(Profile::Ptr)));
    connect(_viewManager , SIGNAL(newViewRequest()) ,
            this , SLOT(newTab()));

    // create main window widgets
    setupWidgets();

    // disable automatically generated accelerators in top-level
    // menu items - to avoid conflicting with Alt+[Letter] shortcuts
    // in terminal applications
    KAcceleratorManager::setNoAccel(menuBar());

    // create menus
    createGUI();

    // rememebr the original menu accelerators for later use
    rememberMenuAccelerators();

    // replace standard shortcuts which cannot be used in a terminal
    // emulator (as they are reserved for use by terminal applications)
    correctShortcuts();

    // enable save and restore of window size
    setAutoSaveSettings("MainWindow", true);

    // this must come at the end
    applyKonsoleSettings();
    connect(KonsoleSettings::self(), SIGNAL(configChanged()), this, SLOT(applyKonsoleSettings()));
}

void MainWindow::rememberMenuAccelerators()
{
    foreach(QAction* menuItem, menuBar()->actions()) {
        QString itemText = menuItem->text();
        menuItem->setData(itemText);
    }
}

// remove accelerators for standard menu items (eg. &File, &View, &Edit)
// etc. which are defined in kdelibs/kdeui/xmlgui/ui_standards.rc, again,
// to avoid conflicting with Alt+[Letter] terminal shortcuts
//
// TODO - Modify XMLGUI so that it allows the text for standard actions
// defined in ui_standards.rc to be re-defined in the local application
// XMLGUI file (konsoleui.rc in this case) - the text for standard items
// can then be redefined there to exclude the standard accelerators
void MainWindow::removeMenuAccelerators()
{
    foreach(QAction* menuItem, menuBar()->actions()) {
        QString itemText = menuItem->text();
        itemText = KGlobal::locale()->removeAcceleratorMarker(itemText);
        menuItem->setText(itemText);
    }
}

void MainWindow::recoverMenuAccelerators()
{
    foreach(QAction* menuItem, menuBar()->actions()) {
        QString itemText = menuItem->data().toString();
        menuItem->setText(itemText);
    }
}

void MainWindow::setSaveGeometryOnExit(bool save)
{
    setAutoSaveSettings("MainWindow", save);
}

void MainWindow::correctShortcuts()
{
    // replace F1 shortcut for help contents
    QAction* helpAction = actionCollection()->action("help_contents");
    Q_ASSERT(helpAction);
    helpAction->setShortcut(QKeySequence());

    // replace Ctrl+B shortcut for bookmarks
    QAction* bookmarkAction = actionCollection()->action("add_bookmark");
    Q_ASSERT(bookmarkAction);
    bookmarkAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
}

ViewManager* MainWindow::viewManager() const
{
    return _viewManager;
}

void MainWindow::disconnectController(SessionController* controller)
{
    disconnect(controller , SIGNAL(titleChanged(ViewProperties*))
               , this , SLOT(activeViewTitleChanged(ViewProperties*)));

    // KXmlGuiFactory::removeClient() will try to access actions associated
    // with the controller internally, which may not be valid after the controller
    // itself is no longer valid (after the associated session and or view have
    // been destroyed)
    if (controller->isValid())
        guiFactory()->removeClient(controller);

    controller->setSearchBar(0);
}

void MainWindow::activeViewChanged(SessionController* controller)
{
    // associate bookmark menu with current session
    bookmarkHandler()->setActiveView(controller);
    disconnect(bookmarkHandler() , SIGNAL(openUrl(KUrl)) , 0 , 0);
    connect(bookmarkHandler() , SIGNAL(openUrl(KUrl)) , controller ,
            SLOT(openUrl(KUrl)));

    if (_pluggedController)
        disconnectController(_pluggedController);

    // listen for title changes from the current session
    Q_ASSERT(controller);

    connect(controller , SIGNAL(titleChanged(ViewProperties*)) ,
            this , SLOT(activeViewTitleChanged(ViewProperties*)));

    controller->setShowMenuAction(_toggleMenuBarAction);
    guiFactory()->addClient(controller);

    // set the current session's search bar
    controller->setSearchBar(searchBar());

    // update session title to match newly activated session
    activeViewTitleChanged(controller);

    _pluggedController = controller;

    // Update window icon to newly activated session's icon
    updateWindowIcon();
}

void MainWindow::activeViewTitleChanged(ViewProperties* properties)
{
    setPlainCaption(properties->title());
}

void MainWindow::updateWindowIcon()
{
    if (_pluggedController)
        setWindowIcon(_pluggedController->icon());
}

IncrementalSearchBar* MainWindow::searchBar() const
{
    return _viewManager->searchBar();
}

void MainWindow::setupActions()
{
    KActionCollection* collection = actionCollection();
    KAction* action = 0;

    // File Menu
    _newTabMenuAction = new KActionMenu(KIcon("tab-new"), i18n("&New Tab"), collection);
    _newTabMenuAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_T));
    _newTabMenuAction->setShortcutConfigurable(true);
    _newTabMenuAction->setAutoRepeat(false);
    connect(_newTabMenuAction, SIGNAL(triggered()), this, SLOT(newTab()));
    collection->addAction("new-tab", _newTabMenuAction);

    action = collection->addAction("clone-tab");
    action->setIcon( KIcon("tab-duplicate") );
    action->setText( i18n("&Clone Tab") );
    action->setShortcut( QKeySequence() );
    action->setAutoRepeat( false );
    connect( action , SIGNAL(triggered()) , this , SLOT(cloneTab()) );

    action = collection->addAction("new-window");
    action->setIcon(KIcon("window-new"));
    action->setText(i18n("New &Window"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_N));
    action->setAutoRepeat(false);
    connect(action , SIGNAL(triggered()) , this , SLOT(newWindow()));

    action = KStandardAction::quit(this , SLOT(close()) , collection);
    action->setIcon(KIcon("window-close"));
    action->setText(i18n("Close Window"));
    // the default shortcut for quit is typically Ctrl+[Some Letter, usually Q]
    // but that is reserved for use by terminal applications
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Q));

    // Bookmark Menu
    KActionMenu* bookmarkMenu = new KActionMenu(i18n("&Bookmarks") , collection);
    _bookmarkHandler = new BookmarkHandler(collection , bookmarkMenu->menu() , true , this);
    collection->addAction("bookmark" , bookmarkMenu);
    connect(_bookmarkHandler , SIGNAL(openUrls(QList<KUrl>)) , this , SLOT(openUrls(QList<KUrl>)));

    // Settings Menu
    _toggleMenuBarAction = KStandardAction::showMenubar(menuBar(), SLOT(setVisible(bool)), collection);
    _toggleMenuBarAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_M));

    // Full Screen
    action = KStandardAction::fullScreen(this, SLOT(viewFullScreen(bool)), this, collection);
    action->setShortcut(QKeySequence());

    KStandardAction::configureNotifications(this , SLOT(configureNotifications()) , collection);
    KStandardAction::keyBindings(this , SLOT(showShortcutsDialog()) , collection);
    KStandardAction::preferences(this, SLOT(showSettingsDialog()), collection);

    action = collection->addAction("manage-profiles");
    action->setText(i18n("Manage Profiles..."));
    action->setIcon(KIcon("configure"));
    connect(action, SIGNAL(triggered()) , this , SLOT(showManageProfilesDialog()));

    // Set up an shortcut-only action for activating menu bar.
    action = collection->addAction("activate-menu");
    action->setText(i18n("Activate Menu"));
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F10));
    connect(action, SIGNAL(triggered()), this, SLOT(activateMenuBar()));
}

void MainWindow::viewFullScreen(bool fullScreen)
{
    if (fullScreen)
        setWindowState(windowState() | Qt::WindowFullScreen);
    else
        setWindowState(windowState() & ~Qt::WindowFullScreen);
}

BookmarkHandler* MainWindow::bookmarkHandler() const
{
    return _bookmarkHandler;
}

void MainWindow::setProfileList(ProfileList* list)
{
    sessionListChanged(list->actions());

    connect(list , SIGNAL(profileSelected(Profile::Ptr)) , this ,
            SLOT(newFromProfile(Profile::Ptr)));

    connect(list , SIGNAL(actionsChanged(QList<QAction*>)) , this ,
            SLOT(sessionListChanged(QList<QAction*>)));
}

void MainWindow::sessionListChanged(const QList<QAction*>& actions)
{
    // If only 1 profile is to be shown in the menu, only display
    // it if it is the non-default profile.
    if (actions.size() > 2) {
        // Update the 'New Tab' KActionMenu
        KMenu* newTabMenu = _newTabMenuAction->menu();
        newTabMenu->clear();
        foreach(QAction * action, actions) {
            newTabMenu->addAction(action);

            // NOTE: defaultProfile seems to not work here, sigh.
            Profile::Ptr profile = SessionManager::instance()->defaultProfile();
            if (profile && profile->name() == action->text().remove('&')) {
                action->setIcon(KIcon(profile->icon(), NULL, QStringList("emblem-favorite")));
                newTabMenu->setDefaultAction(action);
                QFont font = action->font();
                font.setBold(true);
                action->setFont(font);
            }
        }
    } else {
        KMenu* newTabMenu = _newTabMenuAction->menu();
        newTabMenu->clear();
        Profile::Ptr profile = SessionManager::instance()->defaultProfile();

        // NOTE: Compare names w/o any '&'
        if (actions.size() == 2 &&  actions[1]->text().remove('&') != profile->name()) {
            newTabMenu->addAction(actions[1]);
        } else {
            delete newTabMenu;
        }
    }
}

QString MainWindow::activeSessionDir() const
{
    if (_pluggedController) {
        if (Session* session = _pluggedController->session()) {
            // For new tabs to get the correct working directory,
            // force the updating of the currentWorkingDirectory.
            session->getDynamicTitle();
        }
        return _pluggedController->currentDir();
    } else {
        return QString();
    }
}

void MainWindow::openUrls(const QList<KUrl>& urls)
{
    Profile::Ptr defaultProfile = SessionManager::instance()->defaultProfile();

    foreach(const KUrl& url , urls) {
        if (url.isLocalFile())
            emit newSessionRequest(defaultProfile , url.path() , _viewManager);

        else if (url.protocol() == "ssh")
            emit newSSHSessionRequest(defaultProfile , url , _viewManager);
    }
}

void MainWindow::newTab()
{
    Profile::Ptr defaultProfile = SessionManager::instance()->defaultProfile();
    emit newSessionRequest(defaultProfile , activeSessionDir() , _viewManager);
}

void MainWindow::cloneTab()
{
    Q_ASSERT(_pluggedController);

    Session* session = _pluggedController->session();
    Profile::Ptr profile = SessionManager::instance()->sessionProfile(session);
    if ( profile) {
        emit newSessionRequest( profile , activeSessionDir() , _viewManager);
    }
    else {
        // something must be wrong: every session should be associated with profile
        Q_ASSERT(false);
        newTab();
    }
}

void MainWindow::newWindow()
{
    Profile::Ptr defaultProfile = SessionManager::instance()->defaultProfile();
    emit newWindowRequest(defaultProfile , activeSessionDir());
}

bool MainWindow::queryClose()
{
    // TODO: Ideally, we should check what process is running instead
    //       of just how many sessions are running.
    // If only 1 session is running, don't ask user to confirm close.
    if (_viewManager->viewProperties().count() < 2)
        return true;

    int result = KMessageBox::warningYesNoCancel(this,
                 i18n("You have multiple tabs in this window, "
                      "are you sure you want to quit?"),
                 i18n("Confirm Close"),
                 KStandardGuiItem::quit(),
                 KGuiItem(i18n("Close Current Tab"), "tab-close"),
                 KStandardGuiItem::cancel(),
                 "CloseAllTabs");

    switch (result) {
    case KMessageBox::Yes:
        return true;
    case KMessageBox::No:
        if (_pluggedController && _pluggedController->session()) {
            disconnectController(_pluggedController);
            _pluggedController->closeSession();
        }
        return false;
    case KMessageBox::Cancel:
        return false;
    }

    return true;
}

void MainWindow::saveProperties(KConfigGroup& group)
{
    _viewManager->saveSessions(group);
}

void MainWindow::readProperties(const KConfigGroup& group)
{
    _viewManager->restoreSessions(group);
}

void MainWindow::saveGlobalProperties(KConfig* config)
{
    SessionManager::instance()->saveSessions(config);
}

void MainWindow::readGlobalProperties(KConfig* config)
{
    SessionManager::instance()->restoreSessions(config);
}

void MainWindow::syncActiveShortcuts(KActionCollection* dest, const KActionCollection* source)
{
    foreach(QAction * qAction, source->actions()) {
        if (KAction* kAction = qobject_cast<KAction*>(qAction)) {
            if (KAction* destKAction = qobject_cast<KAction*>(dest->action(kAction->objectName())))
                destKAction->setShortcut(kAction->shortcut(KAction::ActiveShortcut), KAction::ActiveShortcut);
        }
    }
}
void MainWindow::showShortcutsDialog()
{
    KShortcutsDialog dialog(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsDisallowed, this);

    // add actions from this window and the current session controller
    foreach(KXMLGUIClient * client, guiFactory()->clients()) {
        dialog.addCollection(client->actionCollection());
    }

    if (dialog.configure()) {
        // sync shortcuts for non-session actions (defined in "konsoleui.rc") in other main windows
        foreach(QWidget * widget, QApplication::topLevelWidgets()) {
            MainWindow* window = qobject_cast<MainWindow*>(widget);
            if (window && window != this)
                syncActiveShortcuts(window->actionCollection(), actionCollection());
        }
        // sync shortcuts for session actions (defined in "sessionui.rc") in other session controllers.
        // Controllers which are currently plugged in (ie. their actions are part of the current menu)
        // must be updated immediately via syncActiveShortcuts().  Other controllers will be updated
        // when they are plugged into a main window.
        foreach(SessionController * controller, SessionController::allControllers()) {
            controller->reloadXML();
            if (controller->factory() && controller != _pluggedController)
                syncActiveShortcuts(controller->actionCollection(), _pluggedController->actionCollection());
        }
    }
}

void MainWindow::newFromProfile(Profile::Ptr profile)
{
    emit newSessionRequest(profile, activeSessionDir(), _viewManager);
}
void MainWindow::showManageProfilesDialog()
{
    ManageProfilesDialog* dialog = new ManageProfilesDialog(this);
    dialog->show();
}

void MainWindow::showSettingsDialog()
{
    if (KConfigDialog::showDialog("settings"))
        return;

    KConfigDialog* settingsDialog = new KConfigDialog(this, "settings", KonsoleSettings::self());
    settingsDialog->setFaceType(KPageDialog::List);

    GeneralSettings* generalSettings = new GeneralSettings(settingsDialog);
    settingsDialog->addPage(generalSettings,
                            i18nc("@title Preferences page name", "General"),
                            "utilities-terminal");

    TabBarSettings* tabBarSettings = new TabBarSettings(settingsDialog);
    settingsDialog->addPage(tabBarSettings,
                            i18nc("@title Preferences page name", "TabBar"),
                            "system-run");

    settingsDialog->show();
}

void MainWindow::applyKonsoleSettings()
{
    if ( KonsoleSettings::allowMenuAccelerators() ) {
        recoverMenuAccelerators();
    }
    else {
        removeMenuAccelerators();
    }

    ViewManager::NavigationOptions options;
    options.visibility       = KonsoleSettings::tabBarVisibility();
    options.position         = KonsoleSettings::tabBarPosition();
    options.newTabBehavior   = KonsoleSettings::newTabBehavior();
    options.showQuickButtons = KonsoleSettings::showQuickButtons();

    _viewManager->updateNavigationOptions(options);

    // setAutoSaveSettings("MainWindow", KonsoleSettings::saveGeometryOnExit());
}

void MainWindow::activateMenuBar()
{
    const QList<QAction*> menuActions = menuBar()->actions();

    if (menuActions.isEmpty())
        return;

    // Show menubar if it is hidden at the moment
    if (menuBar()->isHidden()) {
        menuBar()->setVisible(true);
        _toggleMenuBarAction->setChecked(true);
    }

    // First menu action should be 'File'
    QAction* menuAction = menuActions.first();

    // TODO: Handle when menubar is top level (MacOS)
    menuBar()->setActiveAction(menuAction);
}

void MainWindow::setupWidgets()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout();

    layout->addWidget(_viewManager->widget());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    widget->setLayout(layout);

    setCentralWidget(widget);
}

void MainWindow::configureNotifications()
{
    KNotifyConfigWidget::configure(this);
}

void MainWindow::showEvent(QShowEvent* event)
{
    // Make sure the 'initial' visibility is applied only once.
    if (! _menuBarInitialVisibilityApplied) {

        // the initial visibility of menubar should be applied at this last
        // moment. Otherwise, the initial visibility will be determined by
        // what KMainWindow has automatically stored in konsolerc, but not by
        // what users has explicitly configured .
        menuBar()->setVisible(KonsoleSettings::showMenuBar());
        _toggleMenuBarAction->setChecked(KonsoleSettings::showMenuBar());

        _menuBarInitialVisibilityApplied = true;
    }

    // Call parent method
    KXmlGuiWindow::showEvent(event);
}

bool MainWindow::focusNextPrevChild(bool)
{
    // In stand-alone konsole, always disable implicit focus switching
    // through 'Tab' and 'Shift+Tab'
    //
    // Kpart is another different story
    return false;
}


#include "MainWindow.moc"

