/*
   Copyright (C) 2019 Arto Hyvättinen

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef RAPORTINMUOKKAUSDIALOGI_H
#define RAPORTINMUOKKAUSDIALOGI_H

#include <QDialog>
#include <QVariantMap>

namespace Ui {
class RaportinmuokkausDialogi;
}

class RaportinmuokkausDialogi : public QDialog
{
    Q_OBJECT

public:
    explicit RaportinmuokkausDialogi(QWidget *parent = nullptr);
    ~RaportinmuokkausDialogi();

    static QVariantMap muokkaa(const QVariantMap& data);

    void lataa(const QVariantMap& data);
    QVariantMap data() const;


private:
    Ui::RaportinmuokkausDialogi *ui;
};

#endif // RAPORTINMUOKKAUSDIALOGI_H