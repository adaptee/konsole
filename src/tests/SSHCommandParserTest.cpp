/*
    Copyright 2008 by Robert Knight <robertknight@gmail.com>

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
#include "SSHCommandParserTest.h"

// Qt
#include <QtCore/QVector>
#include <QtCore/QHash>

// KDE
#include <qtest_kde.h>

using namespace Konsole;

void SSHCommandParserTest::init()
{
}

void SSHCommandParserTest::cleanup()
{
}

void SSHCommandParserTest::test1()
{
    QVector<QString> arguments;
    arguments << "ssh" << "someone@somehost";
    QHash<QString,QString> results = SSHProcessInfo::parseSSHCommand(arguments);

    QCOMPARE(results["user"], QString("someone"));
    QCOMPARE(results["host"], QString("somehost"));
    QCOMPARE(results["command"], QString());
}

void SSHCommandParserTest::test2()
{
    QVector<QString> arguments;
    arguments << "ssh" << "-l" << "someone" << "somehost";
    QHash<QString,QString> results = SSHProcessInfo::parseSSHCommand(arguments);

    QCOMPARE(results["user"], QString("someone"));
    QCOMPARE(results["host"], QString("somehost"));
    QCOMPARE(results["command"], QString());
}

void SSHCommandParserTest::test3()
{
    QVector<QString> arguments;
    arguments << "ssh" << "-4" << "-X" << "somehost";
    QHash<QString,QString> results = SSHProcessInfo::parseSSHCommand(arguments);

    QCOMPARE(results["user"], QString());
    QCOMPARE(results["host"], QString("somehost"));
    QCOMPARE(results["command"], QString());
}

void SSHCommandParserTest::test4()
{
    QVector<QString> arguments;
    arguments << "ssh" << "-p" << "443" << "somehost" << "ps" ;
    QHash<QString,QString> results = SSHProcessInfo::parseSSHCommand(arguments);

    QCOMPARE(results["user"], QString());
    QCOMPARE(results["host"], QString("somehost"));
    QCOMPARE(results["port"], QString("443"));
    QCOMPARE(results["command"], QString("ps"));
}

void SSHCommandParserTest::test5()
{
    QVector<QString> arguments;
    arguments << "ssh" << "-D1080" << "-oControlMaster=no" << "somehost" ;
    QHash<QString,QString> results = SSHProcessInfo::parseSSHCommand(arguments);

    QCOMPARE(results["user"], QString());
    QCOMPARE(results["host"], QString("somehost"));
}

void SSHCommandParserTest::test6()
{
    QVector<QString> arguments;
    arguments << "ssh" << "-t" << "someone@somehost" << "screen" << "-r" << "testscreen" ;
    QHash<QString,QString> results = SSHProcessInfo::parseSSHCommand(arguments);

    QCOMPARE(results["user"], QString("someone"));
    QCOMPARE(results["host"], QString("somehost"));
    QCOMPARE(results["command"], QString("screen -r testscreen"));
}

QTEST_KDEMAIN_CORE(SSHCommandParserTest)

#include "SSHCommandParserTest.moc"

