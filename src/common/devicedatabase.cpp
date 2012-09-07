/*
 * Copyright 2012 Alexander Maret-Huskinson <alex@maret.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "debug.h"

#include "devicedatabase.h"

#include <KDE/KConfigGroup>
#include <KDE/KSharedConfig>
#include <KDE/KStandardDirs>

namespace Wacom {
/**
  * Private class for the d-pointer.
  */
class DeviceDatabasePrivate {
public:
    KSharedConfig::Ptr companyConfig;        /**< Ref Pointer for the data device list with all known tablet company information */
};
}

using namespace Wacom;

DeviceDatabase::DeviceDatabase() : d_ptr( new DeviceDatabasePrivate )
{
    Q_D( DeviceDatabase );

    d->companyConfig = KSharedConfig::openConfig( KStandardDirs::locate( "data", QLatin1String( "wacomtablet/data/companylist" ) ), KConfig::SimpleConfig, "data" );

    if( d->companyConfig->groupList().isEmpty() ) {
        kError() << "Company list missing!";
    }
}

DeviceDatabase::~DeviceDatabase()
{
    delete this->d_ptr;
}



QString DeviceDatabase::lookupBackend(const QString& companyId)
{
    KConfigGroup companyGroup;

    if (!lookupCompanyGroup(companyGroup, companyId)) {
        return QString();
    }

    return companyGroup.readEntry( "driver" );
}



bool DeviceDatabase::lookupButtonMapping(QMap< QString, QString >& map, const QString& companyId, const QString& deviceId)
{
    KConfigGroup deviceGroup;

    if (!lookupDeviceGroup(deviceGroup, companyId, deviceId)) {
        kError() << "Device info not found for device ID: " << deviceId << " :: company " << companyId;
        return false;
    }

    int     i   = 1;
    QString key = QLatin1String("hwbutton1");

    while(deviceGroup.hasKey(key)) {
        map.insert( QString::number(i), deviceGroup.readEntry(key));
        key = QString::fromLatin1("hwbutton%1").arg(++i);
    }

    return true;
}


bool DeviceDatabase::lookupDevice(DeviceInformation& devinfo, const QString& deviceId)
{
    KConfigGroup companyGroup;
    QString      companyId = lookupCompanyId(deviceId);

    if (companyId.isEmpty() || !lookupCompanyGroup(companyGroup, companyId)) {
        kError() << "No company information found for device: " << deviceId;
        return false;
    }

    KConfigGroup deviceGroup;

    if (!lookupDeviceGroup(deviceGroup, companyGroup, deviceId)) {
        kDebug() << "Device info not found for device ID: " << deviceId << " :: company " << companyId;
        return false;
    }

    devinfo.tabletId    = deviceId.toUpper();
    devinfo.companyId   = companyId;
    devinfo.companyName = companyGroup.readEntry( "name" );

    devinfo.tabletModel = deviceGroup.readEntry( "model" );
    devinfo.tabletName  = deviceGroup.readEntry( "name" );

    if( deviceGroup.readEntry( "padbuttons" )  != QLatin1String( "0" ) ||
        deviceGroup.readEntry( "wheel" )       != QLatin1String( "no" ) ||
        deviceGroup.readEntry( "touchring" )   != QLatin1String( "no" ) ||
        deviceGroup.readEntry( "touchstripl" ) != QLatin1String( "no" ) ||
        deviceGroup.readEntry( "touchstripr" ) != QLatin1String( "no" ) ) {
        devinfo.hasPadButtons = true;
    }
    else {
        devinfo.hasPadButtons = false;
    }

    return true;
}



bool DeviceDatabase::lookupCompanyGroup(KConfigGroup& companyGroup, const QString& companyId)
{
    Q_D( DeviceDatabase );
    companyGroup = KConfigGroup( d->companyConfig, companyId );

    if( companyGroup.keyList().isEmpty() ) {
        return false;
    }

    return true;
}


QString DeviceDatabase::lookupCompanyId(const QString& deviceId)
{
    Q_D( DeviceDatabase );

    foreach(const QString& companyId, d->companyConfig->groupList()) {

        KConfigGroup deviceGroup;

        if (!lookupDeviceGroup(deviceGroup, companyId, deviceId)) {
            continue;
        }

        return companyId;
    }
    
    return QString();
}



bool DeviceDatabase::lookupDeviceGroup(KConfigGroup& deviceGroup, const QString& companyId, const QString& deviceId)
{
    KConfigGroup companyGroup;
    
    if (!lookupCompanyGroup(companyGroup, companyId)) {
        return false;
    }

    return lookupDeviceGroup(deviceGroup, companyGroup, deviceId);
}


bool DeviceDatabase::lookupDeviceGroup(KConfigGroup& deviceGroup, KConfigGroup& companyGroup, const QString& deviceId)
{
    // read the company name and the datafile for the device information
    KSharedConfig::Ptr deviceConfig = KSharedConfig::openConfig( KStandardDirs::locate( "data", QString::fromLatin1( "wacomtablet/data/%1" ).arg( companyGroup.readEntry( "listfile" ) ) ), KConfig::SimpleConfig, "data" );

    if( deviceConfig->groupList().isEmpty() ) {
        kDebug() << "Device list missing for company: " << companyGroup.readEntry( "name" );
        return false;
    }

    deviceGroup = KConfigGroup( deviceConfig, deviceId.toUpper() );

    if( deviceGroup.keyList().isEmpty() ) {
        return false;
    }

    return true;
}

