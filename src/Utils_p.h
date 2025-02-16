/*
  This file is part of KDDockWidgets.

  Copyright (C) 2018-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KD_UTILS_P_H
#define KD_UTILS_P_H

#include <QApplication>
#ifdef QT_X11EXTRAS_LIB
# include <QtX11Extras/QX11Info>
#endif

namespace KDDockWidgets {

inline bool isLeftButtonPressed()
{
    return qApp->mouseButtons() & Qt::LeftButton;
}

inline bool supportsNativeTitleBar()
{
#ifdef Q_OS_LINUX
    // On Linux, dragging the title bar of a window doesn't generate NonClientMouseEvents
    return false;
#else
    // But let's return false for Windows and macOS too, since our fake resizing seems to be working fine
    return false;
#endif
}

inline bool windowManagerSupportsTranslucency()
{
#ifdef QT_X11EXTRAS_LIB
    if (qApp->platformName() == QLatin1String("xcb"))
        return QX11Info::isCompositingManagerRunning();
#endif

    // macOS and Windows are fine
    return true;
}

};

#endif
