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

#include "DockWidget.h"
#include "DragController_p.h"
#include "Frame_p.h"
#include "FloatingWindow_p.h"
#include "Logging_p.h"
#include "TitleBar_p.h"
#include "TabWidget_p.h"
#include "Utils_p.h"
#include "DockRegistry_p.h"
#include "WidgetResizeHandler_p.h"
#include "DropArea_p.h"
#include "LastPosition_p.h"
#include "multisplitter/Item_p.h"
#include <QAction>
#include <QEvent>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QCloseEvent>

/**
 * @file
 * @brief Represents a dock widget.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

using namespace KDDockWidgets;

class DockWidget::Private
{
public:
    Private(const QString &dockName, DockWidget::Options options_, DockWidget *qq)
        : name(dockName)
        , title(dockName)
        , q(qq)
        , options(options_)
        , layout(new QVBoxLayout(q))
        , toggleAction(new QAction(q))
    {
        q->connect(q, &DockWidget::shown, q, [this] { onDockWidgetShown(); } );
        q->connect(q, &DockWidget::hidden, q, [this] { onDockWidgetHidden(); } );

        q->connect(toggleAction, &QAction::toggled, q, [this] (bool enabled) {
            toggle(enabled);
        });

        toggleAction->setCheckable(true);
    }

    void init()
    {
        titlebar = new TitleBar(q);
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(titlebar);
        updateTitleBarVisibility();
        updateTitle();
    }

    void updateTitleBarVisibility();
    void updateTitle();
    void toggle(bool enabled);
    void updateToggleAction();
    void onDockWidgetShown();
    void onDockWidgetHidden();
    TabWidget *parentTabWidget() const;
    void show();
    void close();
    void updateLayoutMargin();
    void restoreToPreviousPosition();
    int currentTabIndex() const;

    /**
     * Before floating a dock widget we save its position. So it can be restored when calling
     * DockWidget::setFloating(false)
     */
    void saveTabIndex();

    const QString name;
    QString title;
    QWidget *widget = nullptr;
    DockWidget *const q;
    const DockWidget::Options options;
    QVBoxLayout *const layout;
    TitleBar *titlebar = nullptr;
    QAction *const toggleAction;
    LastPosition m_lastPosition;
};

DockWidget::DockWidget(const QString &name, Options options, QWidget *parent, Qt::WindowFlags flags)
    : QWidget(parent, flags | Qt::Tool)
    , Draggable(this, KDDockWidgets::supportsNativeTitleBar()) // On Linux the draggable is our custom title bar, not the window itself. Because on Linux we only get mouse move events when the mouse button is released
    , d(new Private(name, options, this))
{
    d->init();
    DragController::instance();
    DockRegistry::self()->registerDockWidget(this);
    qCDebug(creation) << "DockWidget" << this;

    if (!KDDockWidgets::supportsNativeTitleBar()) {
        setWindowFlag(Qt::FramelessWindowHint, true);
        setWidgetResizeHandler(new WidgetResizeHandler(this));
    }

    if (name.isEmpty())
        qWarning() << Q_FUNC_INFO << "Name can't be null";
}

DockWidget::~DockWidget()
{
    DockRegistry::self()->unregisterDockWidget(this);
    qCDebug(creation) << "~DockWidget" << this;
    delete d;
}

void DockWidget::addDockWidgetAsTab(DockWidget *other)
{
    if (other == this) {
        qWarning() << Q_FUNC_INFO << "Refusing to add dock widget into itself" << other;
        return;
    }

    if (!other) {
        qWarning() << Q_FUNC_INFO << "dock widget is null";
        return;
    }

    if (isWindow()) {
        // Doesn't have a frame yet
        morphIntoFloatingWindow();
    }

    Frame *frame = this->frame();
    if (!frame) {
        qWarning() << Q_FUNC_INFO << "Couldn't find frame to add dock widget to";
        return;
    }

    frame->addWidget(other);
}

void DockWidget::addDockWidgetToContainingWindow(DockWidget *other, Location location, DockWidget *relativeTo)
{
    if (qobject_cast<MainWindow*>(window())) {
        qWarning() << Q_FUNC_INFO << "Just use MainWindow::addWidget() directly. This function is for floating nested windows only.";
        return;
    }

    if (isWindow())
        morphIntoFloatingWindow();

    if (auto fw = qobject_cast<FloatingWindow *>(window())) {
        fw->dropArea()->addDockWidget(other, location, relativeTo);
    } else {
        qWarning() << Q_FUNC_INFO << "Couldn't find floating nested window";
    }
}

void DockWidget::setWidget(QWidget *w)
{
    Q_ASSERT(w && !d->widget);

    d->widget = w;
    d->layout->addWidget(w);
    setWindowTitle(name());
}

QWidget *DockWidget::widget() const
{
    return d->widget;
}

bool DockWidget::isFloating() const
{
    if (isWindow())
        return true;

    auto fw = qobject_cast<FloatingWindow *>(window());
    return fw && fw->hasSingleDockWidget();
}

void DockWidget::setFloating(bool floats)
{
    const bool alreadyFloating = isFloating();

    qCDebug(docking) << Q_FUNC_INFO << "yes=" << floats
                     << "; already floating=" << alreadyFloating;

    if ((floats && alreadyFloating) || (!floats && !alreadyFloating))
        return; // Nothing to do

    if (floats) {
        d->saveTabIndex();
        if (isTabbed()) {
            TabWidget *tabWidget= d->parentTabWidget();
            if (!tabWidget) {
                qWarning() << "DockWidget::setFloating: Tabbed but no tabbar exists"
                           << this;
                Q_ASSERT(false);
            }

            tabWidget->detachTab(this);
        } else {
            frame()->titleBar()->makeWindow();
        }
    } else {
        if (d->m_lastPosition.isValid()) {
            d->restoreToPreviousPosition();
        } else {
            qCDebug(placeholder) << Q_FUNC_INFO << "Don't have a place to restore";
            // TODO: Restore to prefered place ?
        }
    }
}

QAction *DockWidget::toggleAction() const
{
    return d->toggleAction;
}

QString DockWidget::name() const
{
    return d->name;
}

QString DockWidget::title() const
{
    return d->title;
}

void DockWidget::setTitle(const QString &title)
{
    if (title != d->title) {
        d->title = title;
        d->updateTitle();
    }
}

DockWidget::Options DockWidget::options() const
{
    return d->options;
}

bool DockWidget::isTabbed() const
{
    if (TabWidget* tabWidget = d->parentTabWidget()) {
        return frame()->alwaysShowsTabs() || tabWidget->count() > 1;
    } else {
        if (!isFloating())
            qWarning() << "DockWidget::isTabbed() Couldn't find any tab widget.";
        return false;
    }
}

bool DockWidget::event(QEvent *e)
{
    if (e->type() == QEvent::ParentChange) {
        Q_EMIT parentChanged();
        d->updateTitleBarVisibility();
        d->updateToggleAction();
    } else if (e->type() == QEvent::Show) {
        d->updateLayoutMargin();
        if (widgetResizeHandler()) {
            widgetResizeHandler()->setActive(isWindow());
        }
        Q_EMIT shown();

        if (Frame *f = frame()) {
            if (!e->spontaneous()) {
                f->onDockWidgetShown(this);
            }
        }
    } else if (e->type() == QEvent::Hide) {
        Q_EMIT hidden();

        if (Frame *f = frame()) {
            if (!e->spontaneous()) {
                f->onDockWidgetHidden(this);
            }
        }
    }

    return QWidget::event(e);
}

void DockWidget::closeEvent(QCloseEvent *e)
{
    e->accept();
    d->close();
}

std::unique_ptr<WindowBeingDragged> DockWidget::makeWindow()
{
    // No need to show(). The only way DockWidget can be dragged directly is on Windows via its native title bar
    // so it's already a window. On Linux the TitleBar is the draggable)

    if (!KDDockWidgets::supportsNativeTitleBar()) {
        qFatal("DockWidget::makeWindow() was called but native title bar isn't supported");
    }

    return std::unique_ptr<WindowBeingDragged>(new WindowBeingDragged(this));
}

void DockWidget::paintEvent(QPaintEvent *)
{
    if (isWindow())
        FloatingWindow::paintFrame(this);
}

TitleBar *DockWidget::titleBar() const
{
    return d->titlebar;
}

FloatingWindow *DockWidget::morphIntoFloatingWindow()
{
    qCDebug(creation) << "DockWidget::morphIntoFloatingWindow() this=" << this
                      << "; visible=" << isVisible();
    if (isWindow()) {
        QRect geo = geometry();
        auto frame = new Frame();
        frame->addWidget(this);
        auto floatingWindow = new FloatingWindow(frame);
        floatingWindow->setGeometry(geo);
        qDebug() << "DockWidget::morphIntoFloatingWindow" << geo << "; " << floatingWindow->geometry();
        floatingWindow->show();
        return floatingWindow;
    } else {
        return nullptr;
    }
}

Frame *DockWidget::frame() const
{
    QWidget *p = parentWidget();
    while (p) {
        if (auto frame = qobject_cast<Frame *>(p))
            return frame;
        p = p->parentWidget();
    }
    return nullptr;
}

void DockWidget::setLayoutItem(Item *item)
{
    qCDebug(placeholder) << Q_FUNC_INFO << this << item;
    d->m_lastPosition.setLayoutItem(item);
}

LastPosition *DockWidget::lastPosition() const
{
    return &d->m_lastPosition;
}

void DockWidget::Private::updateTitleBarVisibility()
{
    titlebar->setVisible(q->isWindow() && !KDDockWidgets::supportsNativeTitleBar());
}

void DockWidget::Private::updateTitle()
{
    if (q->isFloating())
        q->window()->setWindowTitle(title);

    titlebar->setTitle(title);
    toggleAction->setText(title);
}

void DockWidget::Private::toggle(bool enabled)
{
    if (enabled) {
        show();
    } else {
        q->close();
    }
}

void DockWidget::Private::updateToggleAction()
{
    QSignalBlocker blocker(toggleAction);
    if ((q->isVisible() || parentTabWidget()) && !toggleAction->isChecked()) {
        toggleAction->setChecked(true);
    } else if ((!q->isVisible() && !parentTabWidget()) && toggleAction->isChecked()) {
        toggleAction->setChecked(false);
    }
}

void DockWidget::Private::onDockWidgetShown()
{
    updateTitleBarVisibility();
    updateToggleAction();

    qCDebug(hiding) << Q_FUNC_INFO << "parent=" << q->parentWidget();
}

void DockWidget::Private::onDockWidgetHidden()
{
    updateToggleAction();
    qCDebug(hiding) << Q_FUNC_INFO << "parent=" << q->parentWidget();
}

TabWidget *DockWidget::Private::parentTabWidget() const
{
    QWidget *p= q->parentWidget();
    if (p && p->objectName() == QLatin1String("qt_tabwidget_stackedwidget")) {
        if (auto tw = qobject_cast<TabWidget *>(p->parentWidget()))
            return tw;
    }

    return nullptr;
}

void DockWidget::Private::close()
{
    qCDebug(hiding) << "DockWidget::close" << this;
    // Do some cleaning. Widget is hidden, but we must hide the tab containing it.
    if (auto tabWidget = parentTabWidget()) {
        tabWidget->removeDockWidget(q);
        q->setParent(nullptr);
    }
}

void DockWidget::Private::updateLayoutMargin()
{
    const int margin = (!q->isWindow() || KDDockWidgets::supportsNativeTitleBar()) ? 0 : 4;
    layout->setContentsMargins(margin, margin, margin, margin);
}

void DockWidget::Private::restoreToPreviousPosition()
{
    if (!m_lastPosition.isValid()) {
        qWarning() << Q_FUNC_INFO << "Only restoring to MainWindow supported for now";
        return;
    }

    m_lastPosition.layoutItem()->restorePlaceholder(q, m_lastPosition.m_tabIndex);
}

int DockWidget::Private::currentTabIndex() const
{
    TabWidget *tabWidget = parentTabWidget();
    return tabWidget->indexOf(q);
}

void DockWidget::Private::saveTabIndex()
{
    m_lastPosition.m_tabIndex = currentTabIndex();
}

void DockWidget::Private::show()
{
    // Only show for now
    q->show();
}
