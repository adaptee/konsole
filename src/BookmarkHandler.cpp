/*  This file was part of the KDE libraries

    Copyright 2002 Carsten Pfeiffer <pfeiffer@kde.org>
    Copyright 2007-2008 Robert Knight <robertknight@gmail.com>

    library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation, version 2
    or ( at your option ), any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

// Born as kdelibs/kio/kfile/kfilebookmarkhandler.characterpp

// Own
#include "BookmarkHandler.h"

// Qt
#include <QtCore/QFileInfo>

// KDE
#include <KDE/KShell>
#include <KDE/KBookmarkMenu>
#include <KDE/KStandardDirs>

// Konsole
#include "ViewProperties.h"

using namespace Konsole;

BookmarkHandler::BookmarkHandler(KActionCollection* collection,
                                 KMenu* menu,
                                 bool toplevel,
                                 QObject* parent)
    : QObject(parent),
      KBookmarkOwner(),
      _menu(menu),
      _toplevel(toplevel),
      _activeView(0)
{
    setObjectName(QLatin1String("BookmarkHandler"));

    _file = KStandardDirs::locate("data", "konsole/bookmarks.xml");
    if (_file.isEmpty())
        _file = KStandardDirs::locateLocal("data", "konsole/bookmarks.xml");

    KBookmarkManager* manager = KBookmarkManager::managerForFile(_file, "konsole");

    manager->setUpdate(true);

    if (toplevel)
        _bookmarkMenu = new KBookmarkMenu(manager, this, _menu, collection);
    else
        _bookmarkMenu = new KBookmarkMenu(manager, this, _menu, 0);
}

BookmarkHandler::~BookmarkHandler()
{
    delete _bookmarkMenu;
}

void BookmarkHandler::openBookmark(const KBookmark& bm, Qt::MouseButtons, Qt::KeyboardModifiers)
{
    emit openUrl(bm.url());
}
void BookmarkHandler::openFolderinTabs(const KBookmarkGroup& group)
{
    emit openUrls(group.groupUrlList());
}
bool BookmarkHandler::enableOption(BookmarkOption option) const
{
    if (option == ShowAddBookmark || option == ShowEditBookmark)
        return _toplevel;
    else
        return KBookmarkOwner::enableOption(option);
}

QString BookmarkHandler::currentUrl() const
{
    return urlForView(_activeView);
}

QString BookmarkHandler::urlForView(ViewProperties* view) const
{
    if (view)
        return view->url().prettyUrl();
    else
        return QString();
}

QString BookmarkHandler::currentTitle() const
{
    return titleForView(_activeView);
}

QString BookmarkHandler::titleForView(ViewProperties* view) const
{
    const KUrl& url = view ? view->url() : KUrl();
    if (url.isLocalFile()) {
        QString path = url.path();

        path = KShell::tildeExpand(path);
        path = QFileInfo(path).baseName();

        return path;
    } else if (url.hasHost()) {
        if (url.hasUser())
            return i18nc("@item:inmenu The user's name and host they are connected to via ssh", "%1 on %2", url.user(), url.host());
        else
            return i18nc("@item:inmenu The host the user is connected to via ssh", "%1", url.host());
    }

    return url.prettyUrl();
}

bool BookmarkHandler::supportsTabs() const
{
    return true;
}

QList<QPair<QString, QString> > BookmarkHandler::currentBookmarkList() const
{
    QList<QPair<QString, QString> > list;

    foreach(ViewProperties* view, _views) {
        list << QPair<QString, QString>(titleForView(view) , urlForView(view));
    }

    return list;
}

void BookmarkHandler::setViews(const QList<ViewProperties*>& views)
{
    _views = views;
}
QList<ViewProperties*> BookmarkHandler::views() const
{
    return _views;
}
void BookmarkHandler::setActiveView(ViewProperties* view)
{
    _activeView = view;
}
ViewProperties* BookmarkHandler::activeView() const
{
    return _activeView;
}

#include "BookmarkHandler.moc"
