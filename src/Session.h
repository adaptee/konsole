/*
    This file is part of Konsole, an X terminal.
    Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

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

#ifndef SESSION_H
#define SESSION_H

// Qt
#include <QtCore/QStringList>
#include <QtCore/QByteRef>

// KDE
#include <KApplication>
#include <KMainWindow>

class K3ProcIO;
class K3Process;

namespace Konsole
{

class ColorScheme;
class Emulation;
class HistoryType;
class Pty;
class TerminalDisplay;
class ZModemDialog;

/**
 * Session represents a Konsole session.
 * This consists of a pseudo-teletype (or PTY) which handles I/O between the terminal
 * process and Konsole, and a terminal emulation ( Emulation and subclasses ) which
 * processes the output stream from the PTY and produces a character image which
 * is then shown on displays which are connected to the session.
 *
 * Each Session can be connected to one or more views by using the addView() method.
 * The attached views can then display output from the program running in the terminal
 * or send input to the program in the terminal in the form of keypresses and mouse
 * activity.
 */
class Session : public QObject
{ 
Q_OBJECT

public:
  Q_PROPERTY(QString sessionName READ title)
  Q_PROPERTY(QString encoding READ encoding WRITE setEncoding)
  Q_PROPERTY(int sessionPid READ sessionPid)
  Q_PROPERTY(QString keytab READ keytab WRITE setKeytab)
  Q_PROPERTY(QSize size READ size WRITE setSize)

  Session();
  ~Session();

  /** 
   * Sets the type of this session. 
   * TODO: More documentation
   */
  void setType(const QString& typeKey);
  /** 
   * Returns the type of this session. 
   * TODO: More documentation 
   */
  QString type() const;

  /** 
   * Adds a new view for this session.    
   * 
   * The viewing widget will display the output from the terminal and 
   * input from the viewing widget (key presses, mouse activity etc.) 
   * will be sent to the terminal.
   *
   * Since terminal applications assume a single terminal screen, 
   * all views of a session will display the same number of lines and 
   * columns.
   *
   */
  void addView(TerminalDisplay* widget);
  /** 
   * Removes a view from this session.  
   *
   * @p widget will no longer display output from or send input
   * to the terminal 
   */
  void removeView(TerminalDisplay* widget);

  /**
   * Returns the views connected to this session
   */
  QList<TerminalDisplay*> views() const;

  /**
   * Returns the terminal emulation instance being used to encode / decode 
   * characters to / from the process.
   */
  Emulation*  emulation() const;      // to control emulation
  bool        isSecure()  const;
  bool        isMonitorActivity() const;
  bool        isMonitorSilence()  const;
  bool        isMasterMode()      const;

  /** 
   * Returns the value of the TERM environment variable which will 
   * be used in the session's environment when it is started using 
   * the run() method.
   * 
   * Defaults to "xterm".
   */  
  const QString& terminalType() const;
  /** 
   * Sets the value of the TERM variable which will be used in the 
   * session's environment when it is started using the run() method.  
   * Changing this once the session has been started using run() has no effect.
   *
   * Defaults to "xterm" if not set explicitly
   */
  void setTerminalType(const QString& terminalType);

  int sessionId() const;
  const QString& title() const;
  const QString& iconName() const;
  const QString& iconText() const;

  /** 
   * Return the session title set by the user (ie. the program running 
   * in the terminal), or an empty string if the user has not set a custom title
   */
  QString userTitle() const;
 
  enum TabTitleContext
  {
    LocalTabTitle,
    RemoteTabTitle
  };
  void setTabTitleFormat(TabTitleContext context , const QString& format);
  QString tabTitleFormat(TabTitleContext context) const;

  QString keymap() const;

  /** Returns the arguments passed to the shell process when run() is called. */
  QStringList arguments() const;
  /** Returns the program name of the shell process started when run() is called. */
  QString program() const;

  /** 
   * Sets the command line arguments which the session's program will be passed when
   * run() is called.
   */
  void setArguments(const QStringList& arguments);
  /** Sets the program to be executed when run() is called. */
  void setProgram(const QString& program);

  /** Returns the session's current working directory. */
  QString initialWorkingDirectory() { return _initialWorkingDir; }
  
  /** 
   * Sets the initial working directory for the session when it is run 
   * This has no effect once the session has been started.
   */
  void setInitialWorkingDirectory( const QString& dir ) { _initialWorkingDir = dir; }
 
  void setHistory(const HistoryType&);
  const HistoryType& history();
  void clearHistory();

  void setMonitorActivity(bool);
  void setMonitorSilence(bool);
  void setMonitorSilenceSeconds(int seconds);
  void setMasterMode(bool);
  
  void setKeymap(const QString& _id);
  void setTitle(const QString& _title);
  void setIconName(const QString& _iconName);
  void setIconText(const QString& _iconText);
  void setAddToUtmp(bool);
  void setXonXoff(bool);
  bool testAndSetStateIconName (const QString& newname);
  bool sendSignal(int signal);

  void setAutoClose(bool b) { _autoClose = b; }
  void renameSession(const QString &name);

  
  void feedSession(const QString &text);
  void sendSession(const QString &text);
  
  /** 
   * Returns the process id of the terminal process. 
   * This is the id used by the system API to refer to the process.
   */
  int sessionPid() const;
 
  /**
   * Returns the process id of the terminal's foreground process.
   */
  int foregroundPid() const;

  void enableFullScripting(bool b);

  void startZModem(const QString &rz, const QString &dir, const QStringList &list);
  void cancelZModem();
  bool zmodemIsBusy() { return _zmodemBusy; }

  QString encoding();
  void setEncoding(const QString &encoding);
  QString keytab();
  void setKeytab(const QString &keytab);

  /** Returns the terminal session's window size in lines and columns. */
  QSize size();
  /** 
   * Emits a request to resize the session to accommodate
   * the specified window size.
   *
   * @param size The size in lines and columns to request.
   */
  void setSize(QSize size);
  
public slots:

  void run();
  void done();
  void done(int);
  bool closeSession();
  
  void setUserTitle( int, const QString &caption );
  void changeTabTextColor( int );
  
  void ptyError();
 
  void slotZModemDetected();
  void emitZModemDetected();

  void zmodemStatus(K3Process *, char *data, int len);
  void zmodemSendBlock(K3Process *, char *data, int len);
  void zmodemRcvBlock(const char *data, int len);
  void zmodemDone();
  void zmodemContinue();

signals:

  void processExited();
  void receivedData( const QString& text );
  void done(Session*);
  void updateTitle();
  void notifySessionState(Session* session, int state);
  /** Emitted when a bell event occurs in the session. */
  void bellRequest( const QString& message );
  void changeTabTextColor( Session*, int );

  void disableMasterModeConnections();
  void enableMasterModeConnections();
  void renameSession(Session* ses, const QString &name);

  void openUrlRequest(const QString &cwd);

  void zmodemDetected(Session *);
  void updateSessionConfig(Session*);
  void resizeSession(Session *session, QSize size);
  void setSessionEncoding(Session *session, const QString &encoding);

private slots:
  void onReceiveBlock( const char* buffer, int len );
  void monitorTimerDone();
  void notifySessionState(int state);
  void onContentSizeChange(int height, int width);

  //automatically detach views from sessions when view is destroyed
  void viewDestroyed(QObject* view);

private:

  void updateTerminalSize();

  int            _uniqueIdentifier;

  Pty*          _shellProcess;
  Emulation*    _emulation;

  QList<TerminalDisplay*> _views;

  bool           _monitorActivity;
  bool           _monitorSilence;
  bool           _notifiedActivity;
  bool           _masterMode;
  bool           _autoClose;
  bool           _wantedClose;
  QTimer*        _monitorTimer;

  int            _silenceSeconds;

  QString        _title;
  QString        _userTitle;

  QString        _localTabTitleFormat;
  QString        _remoteTabTitleFormat;

  QString        _iconName;
  QString        _iconText; // as set by: echo -en '\033]1;IconText\007
  bool           _addToUtmp;
  bool           _flowControl;
  bool           _fullScripting;

  QString	     _stateIconName;

  QString        _program;
  QStringList    _arguments;

  QString        _term;
  ulong          _winId;
  int            _sessionId;

  QString        _initialWorkingDir;

  // ZModem
  bool           _zmodemBusy;
  K3ProcIO*       _zmodemProc;
  ZModemDialog*  _zmodemProgress;

  // Color/Font Changes by ESC Sequences

  QColor         _modifiedBackground; // as set by: echo -en '\033]11;Color\007

  QString        _type;

  static int lastSessionId;
  
};

}

#endif