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
#include "sqlitemodel.h"
#include "db/kirjanpito.h"
#include "sqlitekysely.h"

#include "sqlitealustaja.h"

#include <QSettings>
#include <QImage>
#include <QMessageBox>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QApplication>

#include "routes/initroute.h"
#include "routes/tositeroute.h"
#include "routes/viennitroute.h"
#include "routes/kumppanitroute.h"
#include "routes/liitteetroute.h"
#include "routes/asetuksetroute.h"
#include "routes/tilikaudetroute.h"
#include "routes/saldotroute.h"
#include "routes/asiakkaatroute.h"
#include "routes/budjettiroute.h"
#include "routes/eraroute.h"
#include "routes/myyntilaskutroute.h"
#include "routes/ostolaskutroute.h"
#include "routes/toimittajatroute.h"
#include "routes/kohdennusroute.h"
#include "routes/tuotteetroute.h"
#include "routes/tilitroute.h"
#include "routes/alvroute.h"
#include "routes/ryhmatroute.h"
#include "routes/tuontitulkki.h"

#include "versio.h"

SQLiteModel::SQLiteModel(QObject *parent)
    : YhteysModel(parent)
{
    tietokanta_ = QSqlDatabase::addDatabase("QSQLITE", "KIRJANPITO");

    lisaaRoute(new TositeRoute(this));
    lisaaRoute(new ViennitRoute(this));
    lisaaRoute(new KumppanitRoute(this));
    lisaaRoute(new LiitteetRoute(this));
    lisaaRoute(new InitRoute(this));
    lisaaRoute(new SaldotRoute(this));
    lisaaRoute(new TilikaudetRoute(this));
    lisaaRoute(new AsetuksetRoute(this));
    lisaaRoute(new AsiakkaatRoute(this));
    lisaaRoute(new BudjettiRoute(this));
    lisaaRoute(new EraRoute(this));
    lisaaRoute(new MyyntilaskutRoute(this));
    lisaaRoute(new OstolaskutRoute(this));
    lisaaRoute(new ToimittajatRoute(this));
    lisaaRoute(new KohdennusRoute(this));
    lisaaRoute(new TuotteetRoute(this));
    lisaaRoute(new TilitRoute(this));
    lisaaRoute(new RyhmatRoute(this));
    lisaaRoute(new TuontiTulkki(this));
    lisaaRoute(new AlvRoute(this));
}

SQLiteModel::~SQLiteModel()
{
    for( auto route : routes_)
        delete route;
}


int SQLiteModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return viimeiset_.count();
}

QVariant SQLiteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QVariantMap map = viimeiset_.at(index.row()).toMap();
    switch (role) {
    case Qt::DisplayRole:
    case NimiRooli:
        return map.value("nimi").toString();
    case Qt::DecorationRole:
        return(  QPixmap::fromImage( map.value("logo").value<QImage>().scaled(16,16,Qt::KeepAspectRatio) ) );
    case PolkuRooli:
        {
            QString polku = map.value("polku").toString();
            // Palautetaan aina täydellinen polku
            QDir portableDir( kp()->portableDir() );
            if( !kp()->portableDir().isEmpty())
                polku = QDir::cleanPath(portableDir.absoluteFilePath(polku));
            return polku;
        }
    }

    return QVariant();
}

bool SQLiteModel::avaaTiedosto(const QString &polku, bool ilmoitavirheestaAvattaessa)
{

    tietokanta_.setDatabaseName( polku );
    tiedostoPolku_.clear();
    kp()->yhteysAvattu(nullptr);

    if( !tietokanta_.open())
    {
        if( ilmoitavirheestaAvattaessa ) {
            QMessageBox::critical(nullptr, tr("Tietokannan avaaminen epäonnistui"),
                                  tr("Tietokannan %1 avaaminen epäonnistui tietokantavirheen %2 takia")
                                  .arg( polku ).arg( tietokanta().lastError().text() ) );
        }
        qDebug() << "SQLiteYhteys: Tietokannan avaaminen epäonnistui : " << tietokanta_.lastError().text();
        return false;
    }

    // Lukitaan tietokanta, jotta käyttäminen on varmasti turvallista
#ifndef KITSAS_DEVEL
    tietokanta_.exec("PRAGMA LOCKING_MODE = EXCLUSIVE");
#endif
    tietokanta_.exec("PRAGMA JOURNAL_MODE = WAL");

    QSqlQuery query( tietokanta_ );
    query.exec("SELECT arvo FROM Asetus WHERE avain='KpVersio'");

    if( query.lastError().isValid() )
    {
        // Tietokanta on jo käytössä
        if( ilmoitavirheestaAvattaessa )
        {
            if( query.lastError().text().contains("locked"))
            {
                QMessageBox::critical(nullptr, tr("Kirjanpitoa ei voi avata"),
                                      tr("Kirjanpitotiedosto %1 on jo käytössä.\n\n"
                                         "Sulje kaikki Kitsas-ohjelman ikkunat ja yritä uudelleen.\n\n"
                                         "Ellei tämä auta, käynnistä tietokoneesi uudelleen.").arg(polku));
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Tiedostoa %1 ei voi avata").arg(polku),
                                  tr("Sql-virhe: %1").arg(query.lastError().text()));
            }
        }
        tietokanta_.close();
        return false;
    }
    // Tarkastetaan versio
    if( query.next()) {
        if( query.value(0).toInt() > Kirjanpito::TIETOKANTAVERSIO) {
            QMessageBox::critical(nullptr, tr("Kirjanpitoa %1 ei voi avata").arg(polku),
                                  tr("Kirjanpito on luotu uudemmalla Kitsaan versiolla, eikä käytössäsi oleva versio %1 pysty avaamaan sitä.\n\n"
                                     "Voidaksesi avata tiedoston, sinun on asennettava uudempi versio Kitupiikistä. Lataa ohjelma "
                                     "osoitteesta https://kitsas.fi")
                                  .arg( qApp->applicationVersion() ));
            tietokanta_.close();
            return false;
        }
    } else {
        // Tämä ei ole lainkaan kelvollinen tietokanta
        QMessageBox::critical(nullptr, tr("Tiedostoa %1 ei voi avata").arg(polku),
                              tr("Valitsemasi tiedosto ei ole Kitsaan tietokanta, tai tiedosto on vahingoittunut."));
        qDebug() << tietokanta_.lastError().text();
        tietokanta_.close();
        return false;
    }

    // Merkitään avausaika
    tietokanta_.exec("UPDATE Asetus SET arvo=CURRENT_TIMESTAMP WHERE avain='Avattu'");


    tiedostoPolku_ = polku;

    alusta();
    lisaaViimeisiin();
    return true;
}

void SQLiteModel::lataaViimeiset()
{
    beginResetModel();
    viimeiset_ = kp()->settings()->value("ViimeTiedostot").toList();

    QDir portableDir( kp()->portableDir() );

    QMutableListIterator<QVariant> iter( viimeiset_ );
    while( iter.hasNext())
    {
        QString polku = iter.next().toMap().value("polku").toString();
        if( !kp()->portableDir().isEmpty())
            polku = QDir::cleanPath(portableDir.absoluteFilePath(polku));
        if( !QFile::exists(polku))
            iter.remove();
    }

    kp()->settings()->setValue("ViimeTiedostot", viimeiset_);
    endResetModel();

}

void SQLiteModel::poistaListalta(const QString &polku)
{

    QDir portableDir( kp()->portableDir() );
    QString poistettava = kp()->portableDir().isEmpty() ? polku : portableDir.relativeFilePath(polku);

    beginResetModel();
    QMutableListIterator<QVariant> iter( viimeiset_ );
    while( iter.hasNext())
    {
        QString tamanpolku = iter.next().toMap().value("polku").toString();
        if( poistettava == tamanpolku )
            iter.remove();
    }
    endResetModel();
    kp()->settings()->setValue("ViimeTiedostot", viimeiset_);
    emit kp()->yhteysAvattu(nullptr);

}

KpKysely *SQLiteModel::kysely(const QString &polku, KpKysely::Metodi metodi)
{
    return new SQLiteKysely(this, metodi, polku);
}

void SQLiteModel::sulje()
{
    tietokanta_.close();
}

bool SQLiteModel::onkoOikeutta(YhteysModel::Oikeus oikeus) const
{
    return oikeus != OMISTUSOIKEUS;
}

bool SQLiteModel::uusiKirjanpito(const QString &polku, const QVariantMap &initials)
{
    return SqliteAlustaja::luoKirjanpito(polku, initials);
}

void SQLiteModel::reitita(SQLiteKysely* reititettavakysely, const QVariant &data)
{
    for( SQLiteRoute* route : routes_) {
        if( reititettavakysely->polku().startsWith( route->polku() ) ) {
            reititettavakysely->vastaa(route->route( reititettavakysely, data));
            return;
        }
    }
    qDebug() << " *** Kyselyä " << reititettavakysely->polku() << " ei reititetty ***";
    emit reititettavakysely->virhe(404);
}

void SQLiteModel::reitita(SQLiteKysely *reititettavakysely, const QByteArray &ba, const QMap<QString, QString> &meta)
{
    for( SQLiteRoute* route : routes_) {
        if( reititettavakysely->polku().startsWith( route->polku() ) ) {
            reititettavakysely->vastaa( route->byteArray(reititettavakysely, ba, meta) );
            return;
        }
    }
    emit reititettavakysely->virhe(404);
}


void SQLiteModel::lisaaViimeisiin()
{

    QVariantMap map;
    // PORTABLE polut tallennetaan suhteessa portable-hakemistoon
    QDir portableDir( kp()->portableDir() );
    QString polku = tiedostopolku();

    if( !kp()->portableDir().isEmpty())
        polku = portableDir.relativeFilePath(polku);

    map.insert("polku", tiedostopolku() );
    map.insert("nimi", kp()->asetukset()->asetus("Nimi") );
    map.insert("logo", kp()->logo().scaled(16,16,Qt::KeepAspectRatio));

    beginResetModel();
    for( int i=0; i < viimeiset_.count(); i++ )
    {
        if( viimeiset_.value(i).toMap().value("polku").toString() == map.value("polku").toString()) {
            viimeiset_[i] = map;
            endResetModel();
            kp()->settings()->setValue("ViimeTiedostot", viimeiset_);
            return;
        }
    }

    viimeiset_.append(map);
    endResetModel();

    kp()->settings()->setValue("ViimeTiedostot", viimeiset_);
}

void SQLiteModel::lisaaRoute(SQLiteRoute *route)
{
    routes_.append(route);
}