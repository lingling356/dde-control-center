/*
 * Copyright (C) 2011 ~ 2019 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *             Chris Xiong <chirs241097@gmail.com>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCALINGPAGE_H
#define SCALINGPAGE_H

#include "widgets/contentwidget.h"
#include "widgets/switchwidget.h"
#include "widgets/settingsgroup.h"
#include "widgets/titledslideritem.h"
#include "widgets/labels/tipslabel.h"

#include <QVBoxLayout>

class Resolution;

namespace dcc {

namespace display {

class Monitor;
class DisplayModel;

class ScalingPage : public ContentWidget
{
    Q_OBJECT

public:
    explicit ScalingPage(QWidget *parent = 0);

    void setModel(DisplayModel *model);

Q_SIGNALS:
    void requestUiScaleChange(const double scale) const;
    void requestIndividualScaling(Monitor* m,const double scale) const;

private Q_SLOT:
    void onResolutionChanged();

private:
    void setupSliders();
    void addSlider(int monitorID);

    QStringList getScaleList(const Resolution &r);
    int convertToSlider(const double value);
    double convertToScale(const int value);

private:
    DisplayModel *m_displayModel;

    QVBoxLayout *m_centralLayout;

    QList<widgets::TitledSliderItem *> m_sliders;
    widgets::SettingsGroup *m_slidersgrp;
    widgets::TitledSliderItem *m_slider{nullptr};
    widgets::TipsLabel *tip;
};

}

}

#endif // SCALINGPAGE_H
