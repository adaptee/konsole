/*
    Copyright (C) 2007 by Robert Knight <robertknight@gmail.com>

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

#ifndef PART_H
#define PART_H

// KDE
#include <KParts/Factory>
#include <KParts/Part>
#include <kde_terminal_interface.h>


class QStringList;

namespace Konsole
{
class SessionController;
class ViewManager;

/**
 * A factory which creates Konsole parts.
 */
class PartFactory : public KParts::Factory
{
protected:
    /** Reimplemented to create Konsole parts. */
    virtual KParts::Part* createPartObject(QWidget* parentWidget = 0,
                                           QObject* parent = 0,
                                           const char* classname = "KParts::Part",
                                           const QStringList& args = QStringList());
};

/**
 * A re-usable terminal emulator component using the KParts framework which can
 * be used to embed terminal emulators into other applications.
 */
class Part : public KParts::ReadOnlyPart , public TerminalInterface
{
Q_OBJECT
 
public:
    /** Constructs a new Konsole part with the specified parent. */
    Part(QObject* parent = 0);
    virtual ~Part();

    /** Reimplemented from TerminalInterface. */
    virtual void startProgram( const QString& program,
                               const QStringList& arguments );
    /** Reimplemented from TerminalInterface. */
    virtual void showShellInDir( const QString& dir );
    /** Reimplemented from TerminalInterface. */
    virtual void sendInput( const QString& text );

protected:
    /** Reimplemented from KParts::PartBase. */
    virtual bool openFile();

private slots:
    void createSession(const QString& key);
    void activeViewChanged(SessionController* controller);
private:
    ViewManager* _viewManager;
    SessionController* _pluggedController;
};

};

#endif // PART_H