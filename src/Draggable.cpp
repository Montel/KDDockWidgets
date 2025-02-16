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

#include "Draggable_p.h"
#include "DragController_p.h"
#include "FloatingWindow_p.h"
#include "WidgetResizeHandler_p.h"

using namespace KDDockWidgets;

class Draggable::Private
{
public:
    explicit Private(QWidget *_thisWidget, bool _enabled)
        : thisWidget(_thisWidget)
        , enabled(_enabled)
    {
        Q_ASSERT(thisWidget);
    }

    QPointer<WidgetResizeHandler> widgetResizeHandler;
    QWidget *const thisWidget;
    const bool enabled;
};

Draggable::Draggable(QWidget *thisWidget, bool enabled)
    : d(new Private(thisWidget, enabled))
{
    if (thisWidget && d->enabled)
        DragController::instance()->registerDraggable(this);
}

Draggable::~Draggable()
{
    if (d->thisWidget && d->enabled)
        DragController::instance()->unregisterDraggable(this);
    delete d;
}

QWidget *Draggable::asWidget() const
{
    return d->thisWidget;
}

WidgetResizeHandler *Draggable::widgetResizeHandler() const
{
    return d->widgetResizeHandler;
}

void Draggable::setWidgetResizeHandler(WidgetResizeHandler *w)
{
    Q_ASSERT(!d->widgetResizeHandler);
    Q_ASSERT(w);
    d->widgetResizeHandler = w;
}

FloatingWindow *Draggable::isFloatingWindow() const
{
    QWidget *p = d->thisWidget;
    do {
        if (auto fw = qobject_cast<FloatingWindow *>(p))
            return fw;

        if (p->windowFlags() & Qt::Window)
            return nullptr;  // Some other window

        p = p->parentWidget();
    } while (p);

    return nullptr;
}
