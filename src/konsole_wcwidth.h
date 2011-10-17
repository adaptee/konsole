/* $XFree86: xc/programs/xterm/wcwidth.h,v 1.2 2001/06/18 19:09:27 dickey Exp $ */

/* Markus Kuhn -- 2001-01-12 -- public domain */
/* Adaptions for KDE by Waldo Bastian <bastian@kde.org> */

#ifndef KONSOLE_WCWIDTH_H
#define KONSOLE_WCWIDTH_H

// Qt
#include <QtCore/QBool>
#include <QtCore/QString>

int konsole_wcwidth(quint16 ucs, bool CJKAmbiguousWide=false);

int string_width( const QString& text, bool CJKAmbiguousWide=false );

#endif
