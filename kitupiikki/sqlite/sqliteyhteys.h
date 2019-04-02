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
#ifndef SQLITEYHTEYS_H
#define SQLITEYHTEYS_H

#include "db/kpyhteys.h"
#include "sqlitekysely.h"

#include <QSqlDatabase>

class Kirjanpito;

class SQLiteYhteys : public KpYhteys
{
public:
    SQLiteYhteys(Kirjanpito* parent, const QUrl& url);

    bool avaaYhteys() override;

    SQLiteKysely* kysely(QString polku = QString(), KpKysely::Metodi metodi = KpKysely::GET) override;

    QSqlDatabase tietokanta() const { return tietokanta_;}

protected:
    QSqlDatabase tietokanta_;


};

#endif // SQLITEYHTEYS_H