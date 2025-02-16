/*
  This file is part of KDDockWidgets.

  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
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

#include "utils.h"
#include "DropArea_p.h"

#include <QCloseEvent>
#include <QDebug>
#include <QPainter>
#include <QPushButton>
#include <QtTest/QtTest>

using namespace KDDockWidgets;
using namespace KDDockWidgets::Tests;

NonClosableWidget::NonClosableWidget(QWidget *parent)
    : QWidget(parent)
{
}

NonClosableWidget::~NonClosableWidget()
{
}

void NonClosableWidget::closeEvent(QCloseEvent *ev)
{
    ev->ignore(); // don't allow to close
}

std::unique_ptr<KDDockWidgets::MainWindow> KDDockWidgets::Tests::createMainWindow(QSize sz, KDDockWidgets::MainWindowOptions options)
{
    auto ptr = std::unique_ptr<MainWindow>(new MainWindow(QStringLiteral("MyMainWindow"), options));
    ptr->show();
    ptr->resize(sz);
    return ptr;
}

DockWidget *KDDockWidgets::Tests::createDockWidget(const QString &name, QWidget *w, DockWidget::Options options)
{
    auto dock = new DockWidget(name, options);
    dock->setWidget(w);
    dock->setObjectName(name);
    dock->setGeometry(0, 0, 400, 400);
    dock->show();
    dock->activateWindow();
    Q_ASSERT(dock->window());
    Q_ASSERT(dock->windowHandle());
    if (QTest::qWaitForWindowActive(dock->window()->windowHandle(), 200)) {
        qDebug() << dock->window();
        return dock;
    }
    return nullptr;
};

DockWidget *KDDockWidgets::Tests::createDockWidget(const QString &name, QColor color)
{
    return createDockWidget(name, new MyWidget(name, color));
};


std::unique_ptr<MainWindow> KDDockWidgets::Tests::createMainWindow(QVector<DockDescriptor> &docks)
{
    auto m = std::unique_ptr<MainWindow>(new MainWindow(QStringLiteral("MyMainWindow"), MainWindowOption_None));
    auto layout = m->multiSplitterLayout();
    m->show();
    m->resize(QSize(700, 700));

    int i = 0;
    for (DockDescriptor &desc : docks) {
        desc.createdDock = createDockWidget(QStringLiteral("%1").arg(i), new QPushButton(QStringLiteral("%1").arg(i)));

        DockWidget *relativeTo = nullptr;
        if (desc.relativeToIndex != -1)
            relativeTo = docks.at(desc.relativeToIndex).createdDock;

        m->addDockWidget(desc.createdDock, desc.loc, relativeTo, desc.option);
        qDebug() << "Added" <<i;
        layout->checkSanity();
        ++i;
    }

    return m;
}

MyWidget::MyWidget(const QString &, QColor c)
    : QWidget()
    , c(c)
{
    qDebug() << "MyWidget" << this;
}

MyWidget::~MyWidget()
{
    qDebug() << "~MyWidget" << this;
}

void MyWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), c);
}
