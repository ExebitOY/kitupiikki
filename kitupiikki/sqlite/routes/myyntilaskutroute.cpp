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
#include "myyntilaskutroute.h"

#include "model/tositevienti.h"
#include "model/tosite.h"

#include <QJsonDocument>
#include <QDebug>

MyyntilaskutRoute::MyyntilaskutRoute(SQLiteModel *model)
    : SQLiteRoute(model, "/myyntilaskut")
{

}

QVariant MyyntilaskutRoute::get(const QString &/*polku*/, const QUrlQuery &urlquery)
{
    // Viite ja Laskutapa on json:n sisällä !

    QString kysymys("select tosite.id as tosite, vienti.pvm as pvm, vienti.erapvm as erapvm, vienti.viite, vienti.json as json, "
                        "debet as summa, ds, ks, kumppani.nimi as asiakas, kumppani.id as asiakasid, vienti.eraid as eraid, vienti.tili as tili,"
                        "tosite.tyyppi as tyyppi  from "
                        "Tosite JOIN Vienti ON vienti.tosite=tosite.id ");

    if( !urlquery.hasQueryItem("avoin") && !urlquery.hasQueryItem("eraantynyt"))
        kysymys.append("LEFT OUTER ");

    kysymys.append("JOIN (select eraid, sum(debet) as ds, sum(kredit) as ks FROM Vienti GROUP BY eraid ");
    if( urlquery.hasQueryItem("avoin"))
        kysymys.append("HAVING SUM(kredit) <> SUM(debet) OR sum(kredit) IS NULL ");

    kysymys.append(QString(") as q ON vienti.eraid=q.eraid LEFT OUTER JOIN "
            "Kumppani ON vienti.kumppani=kumppani.id WHERE vienti.tyyppi = %1"
            " AND tosite.tila ").arg( TositeVienti::MYYNTI + TositeVienti::VASTAKIRJAUS) );

    if( urlquery.hasQueryItem("luonnos"))
        kysymys.append(QString(" = %1 ").arg( Tosite::LUONNOS ));
    else if( urlquery.hasQueryItem("lahetettava"))
        kysymys.append(QString(" = %1 ").arg( Tosite::VALMISLASKU ));
    else
        kysymys.append(QString(" >= %1 ").arg( Tosite::KIRJANPIDOSSA ));

    if( urlquery.hasQueryItem("eraantynyt"))
        kysymys.append("AND vienti.erapvm < current_date ");
    if( urlquery.hasQueryItem("alkupvm"))
        kysymys.append(QString(" AND tosite.pvm >= '%1' ")
                       .arg(urlquery.queryItemValue("alkupvm")));
    if( urlquery.hasQueryItem("loppupvm"))
        kysymys.append(QString(" AND tosite.pvm <= '%1' ")
                       .arg(urlquery.queryItemValue("loppupvm")));

    kysymys.append(" ORDER BY vienti.pvm, vienti.viite");

    QSqlQuery kysely( db());
    kysely.exec(kysymys);

    QVariantList lista = resultList(kysely);
    for(int i=0; i < lista.count(); i++) {
        QVariantMap map = lista.at(i).toMap();
        double ds = map.take("ds").toDouble();
        double ks = map.take("ks").toDouble();

        QByteArray ba = map.take("json").toByteArray();
        QVariant jsonvar = QJsonDocument::fromJson(ba).toVariant();
        if( jsonvar.toMap().contains("lasku") &&
            jsonvar.toMap().value("lasku").toMap().contains("laskutapa"))
            map.insert("laskutapa", jsonvar.toMap().value("lasku").toMap().value("laskutapa"));

        map.insert("avoin", ds - ks);
        lista[i] = map;
    }

    qDebug() << kysymys;
    qDebug() << lista;

    return lista;
}