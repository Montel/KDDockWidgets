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

/**
 * @file
 * @brief A widget that supports an arbitrary number of splitters (called Separators) in any
 * combination of vertical/horizontal.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */


#include "MultiSplitterWidget_p.h"
#include "MultiSplitterLayout_p.h"
#include "Logging_p.h"
#include "MainWindow.h"

#include <QResizeEvent>

using namespace KDDockWidgets;

MultiSplitterWidget::MultiSplitterWidget(QWidget *parent)
    : QWidget(parent)
    , m_layout(new MultiSplitterLayout(this))
{
    connect(m_layout, &MultiSplitterLayout::minimumSizeChanged, this, [this] (QSize sz) {
        setMinimumSize(sz);
    });

    connect(m_layout, &MultiSplitterLayout::contentsSizeChanged, this, [this] (QSize sz) {
        if (!m_inResizeEvent)
            resize(sz);
    });
}

MultiSplitterWidget::~MultiSplitterWidget()
{
}

int MultiSplitterWidget::count() const
{
    return m_layout->count();
}

void MultiSplitterWidget::resizeEvent(QResizeEvent *ev)
{
    qCDebug(sizing) << Q_FUNC_INFO << "; new=" << ev->size() << "; old=" << ev->oldSize()
                    << "; window=" << window();

    m_inResizeEvent = true;
    m_layout->setContentsSize(ev->size());
    QWidget::resizeEvent(ev);
    m_inResizeEvent = false;
}

bool MultiSplitterWidget::event(QEvent *e)
{
    if (e->type() == QEvent::LayoutRequest)
        m_layout->updateSizeConstraints();

    return QWidget::event(e);
}

bool MultiSplitterWidget::isInMainWindow() const
{
    return qobject_cast<MainWindow*>(parentWidget());
}
