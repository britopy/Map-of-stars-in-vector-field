/*
 * Comet and asteroids importer plug-in for Stellarium
 * 
 * Copyright (C) 2010 Bogdan Marinov
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "CAImporter.hpp"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QString>

#include <stdexcept>


StelModule* CAImporterStelPluginInterface::getStelModule() const
{
	return new CAImporter();
}

StelPluginInfo CAImporterStelPluginInterface::getPluginInfo() const
{
	//Q_INIT_RESOURCE(caImporter);
	
	StelPluginInfo info;
	info.id = "CAImporter";
	info.displayedName = "Comets and Asteroids Importer";
	info.authors = "Bogdan Marinov";
	info.contact = "http://stellarium.org";
	info.description = "A plug-in that allows importing asteroid and comet data in different formats to Stellarium's ssystem.ini file. It's still a work in progress far from maturity.";
	return info;
}

Q_EXPORT_PLUGIN2(CAImporter, CAImporterStelPluginInterface)

CAImporter::CAImporter()
{
	setObjectName("CAImporter");

	solarSystemConfigurationFile = NULL;
}

CAImporter::~CAImporter()
{
	if (solarSystemConfigurationFile != NULL)
	{
		delete solarSystemConfigurationFile;
	}
}

void CAImporter::init()
{
	try
	{
		//Do something
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "init() error: " << e.what();
		return;
	}
}

void CAImporter::deinit()
{
	//
}

void CAImporter::update(double deltaTime)
{
	//
}

void CAImporter::draw(StelCore* core)
{
	//
}

double CAImporter::getCallOrder(StelModuleActionName actionName) const
{
	return 0.;
}

bool CAImporter::configureGui(bool show)
{
	if(show)
	{
		//mainWindow->setVisible(true);

		if (cloneSolarSystemConfigurationFile())
		{
			//Import Encke for a start
			//importMpcOneLineCometElements("0002P         2010 08  6.5102  0.336152  0.848265  186.5242  334.5718   11.7843  20100104  11.5  6.0  2P/Encke                                                 MPC 59600");

			//Import a list of comets on the desktop. The file is from
			//http://www.minorplanetcenter.org/iau/Ephemerides/Comets/Soft00Cmt.txt
			importMpcOneLineCometElementsFromFile(StelFileMgr::getDesktopDir() + "/Soft00Cmt.txt");
			//Results: the first lines are processed relatively fast (<1s)
			//but the speed falls progressively and around 40 starts to be
			//unbearably slow.
			//Commenting out all QSettings::setValue() calls confirms the
			//hypothesis that most of the delay is caused by configuration
			//file access. Well, I was going to rewrite it to write directly to
			//file anyway.

			//Factors increasing speed:
			// - not having to create a QSettings object on every step;
			// - using QSettings::IniFormat instead of StelIniFormat (greatly increases speed)

			//Also, some of the lines in the list are not parsed.
			/*
			  "    CJ95O010  1997 03 31.4141  0.906507  0.994945  130.5321  282.6820   89.3193  20100723  -2.0  4.0  C/1995 O1 (Hale-Bopp)                                    MPC 61436" -> minus sign, fixed
			  "    CK09K030  2011 01  9.266   3.90156   1.00000   251.413     0.032   146.680              8.5  4.0  C/2009 K3 (Beshore)                                      MPC 66205" -> lower precision than the spec, fixed
			  "    CK10F040  2010 04  6.109   0.61383   1.00000   120.718   237.294    89.143             13.5  4.0  C/2010 F4 (Machholz)                                     MPC 69906" -> lower precision than the spec, fixed
			  "    CK10M010  2012 02  7.840   2.29869   1.00000   265.318    82.150    78.373              9.0  4.0  C/2010 M1 (Gibbs)                                        MPC 70817" -> lower precision than the spec, fixed
			  "    CK10R010  2011 11 28.457   6.66247   1.00000    96.009   345.949   157.437              6.0  4.0  C/2010 R1 (LINEAR)                                       MPEC 2010-R99" -> lower precision than the spec, fixed
			*/
			//It seems that some entries in the list don't match the described format

			//This seems to work
			GETSTELMODULE(SolarSystem)->reloadPlanets();
		}
	}
	return true;
}

bool CAImporter::cloneSolarSystemConfigurationFile()
{
	QDir userDataDirectory(StelFileMgr::getUserDir());
	if (!userDataDirectory.exists())
	{
		return false;
	}
	if (!userDataDirectory.exists("data") && !userDataDirectory.mkdir("data"))
	{
		qDebug() << "Unable to create a \"data\" subdirectory in" << userDataDirectory.absolutePath();
		return false;
	}

	QString defaultFilePath	= StelFileMgr::getInstallationDir() + "/data/ssystem.ini";
	QString userFilePath	= StelFileMgr::getUserDir() + "/data/ssystem.ini";

	if (QFile::exists(userFilePath))
	{
		qDebug() << "Using the ssystem.ini file that already exists in the user directory...";
		return true;
	}

	if (QFile::exists(defaultFilePath))
	{
		return QFile::copy(defaultFilePath, userFilePath);
	}
	else
	{
		qDebug() << "This should be impossible.";
		return false;
	}
}

bool CAImporter::resetSolarSystemConfigurationFile()
{
	QString userFilePath	= StelFileMgr::getUserDir() + "/data/ssystem.ini";
	if (QFile::exists(userFilePath))
	{
		if (QFile::remove((userFilePath)))
		{
			return true;
		}
		else
		{
			qWarning() << "Unable to delete" << userFilePath
			         << endl << "Please remove the file manually.";
			return false;
		}
	}
	else
	{
		return true;
	}
}

bool CAImporter::importMpcOneLineCometElements(QString oneLineElements)
{
	//TODO: Fix this!
	if (solarSystemConfigurationFile == NULL)
	{
		solarSystemConfigurationFile = new QSettings(StelFileMgr::getUserDir() + "/data/ssystem.ini", QSettings::IniFormat, this);
	}
	QRegExp mpcParser("^\\s*(\\d{4})?([A-Z])(\\w{7})?\\s+(\\d{4})\\s+(\\d{2})\\s+(\\d{1,2}\\.\\d{3,4})\\s+(\\d{1,2}\\.\\d{5,6})\\s+(\\d\\.\\d{5,6})\\s+(\\d{1,3}\\.\\d{3,4})\\s+(\\d{1,3}\\.\\d{3,4})\\s+(\\d{1,3}\\.\\d{3,4})\\s+(?:(\\d{4})(\\d\\d)(\\d\\d))?\\s+(\\-?\\d{1,2}\\.\\d)\\s+(\\d{1,2}\\.\\d)\\s+(\\S.{55})\\s+(\\S.*)$");//

	int match = mpcParser.indexIn(oneLineElements);
	//qDebug() << "RegExp captured:" << match << mpcParser.capturedTexts();

	if (match < 0)
	{
		qWarning() << "No match:" << oneLineElements;
		return false;
	}

	QString name = mpcParser.cap(17).trimmed();
	QString sectionName(name);
	sectionName.remove('\\');
	sectionName.remove('/');
	sectionName.remove('#');
	sectionName.remove(' ');
	sectionName.remove('-');

	if (mpcParser.cap(1).isEmpty() && mpcParser.cap(3).isEmpty())
	{
		qWarning() << "Comet is missing both comet number AND provisional designation.";
		return false;
	}

	solarSystemConfigurationFile->beginGroup(sectionName);

	solarSystemConfigurationFile->setValue("name", name);
	solarSystemConfigurationFile->setValue("parent", "Sun");
	solarSystemConfigurationFile->setValue("coord_func","comet_orbit");

	solarSystemConfigurationFile->setValue("lighting", false);
	solarSystemConfigurationFile->setValue("color", "1.0");
	solarSystemConfigurationFile->setValue("tex_map", "nomap.png");

	bool ok = false;

	int year	= mpcParser.cap(4).toInt();
	int month	= mpcParser.cap(5).toInt();
	double dayFraction	= mpcParser.cap(6).toDouble(&ok);
	int day = (int) dayFraction;
	QDate datePerihelionPassage(year, month, day);
	int fraction = (int) ((dayFraction - day) * 24 * 60 * 60);
	int seconds = fraction % 60; fraction /= 60;
	int minutes = fraction % 60; fraction /= 60;
	int hours = fraction % 24;
	//qDebug() << hours << minutes << seconds << fraction;
	QTime timePerihelionPassage(hours, minutes, seconds, 0);
	QDateTime dtPerihelionPassage(datePerihelionPassage, timePerihelionPassage, Qt::UTC);
	double jdPerihelionPassage = StelUtils::qDateTimeToJd(dtPerihelionPassage);
	solarSystemConfigurationFile->setValue("orbit_TimeAtPericenter", jdPerihelionPassage);

	double perihelionDistance = mpcParser.cap(7).toDouble(&ok);//AU
	solarSystemConfigurationFile->setValue("orbit_PericenterDistance", perihelionDistance);

	double eccentricity = mpcParser.cap(8).toDouble(&ok);//degrees
	solarSystemConfigurationFile->setValue("orbit_Eccentricity", eccentricity);

	double argumentOfPerihelion = mpcParser.cap(9).toDouble(&ok);//J2000.0, degrees
	solarSystemConfigurationFile->setValue("orbit_ArgOfPericenter", argumentOfPerihelion);

	double longitudeOfTheAscendingNode = mpcParser.cap(10).toDouble(&ok);//J2000.0, degrees
	solarSystemConfigurationFile->setValue("orbit_AscendingNode", longitudeOfTheAscendingNode);

	double inclination = mpcParser.cap(11).toDouble(&ok);
	solarSystemConfigurationFile->setValue("orbit_Inclination", inclination);

	//Albedo doesn't work at all
	//TODO: Make sure comets don't display magnitude
	double absoluteMagnitude = mpcParser.cap(15).toDouble(&ok);
	//qDebug() << "absoluteMagnitude:" << absoluteMagnitude;
	double radius = 5; //Fictitious
	solarSystemConfigurationFile->setValue("radius", radius);
	//qDebug() << 1329 * pow(10, (absoluteMagnitude/-5));
	//double albedo = pow(( (1329 * pow(10, (absoluteMagnitude/-5))) / (2 * radius)), 2);//from http://www.physics.sfasu.edu/astro/asteroids/sizemagnitude.html
	double albedo = 1;
	solarSystemConfigurationFile->setValue("albedo", albedo);

	solarSystemConfigurationFile->endGroup();

	return true;
}

bool CAImporter::importMpcOneLineCometElementsFromFile(QString filePath)
{
	if (!QFile::exists(filePath))
	{
		qDebug() << "Can't find" << filePath;
		return false;
	}

	QFile mpcElementsFile(filePath);
	if (mpcElementsFile.open(QFile::ReadOnly | QFile::Text ))//| QFile::Unbuffered
	{
		bool atLeastOneRead = false;
		int count = 0;

		while(!mpcElementsFile.atEnd())
		{
			QString mpcOneLineElements = QString(mpcElementsFile.readLine(200));
			if(mpcOneLineElements.endsWith('\n'))
			{
				mpcOneLineElements.chop(1);
			}
			if (mpcOneLineElements.isEmpty())
			{
				qDebug() << "Empty line?";
				continue;
			}

			if(importMpcOneLineCometElements(mpcOneLineElements))
			{
				//qDebug() << ++count;//TODO: Remove debug
				atLeastOneRead = true;
			}
		}
		solarSystemConfigurationFile->sync();

		mpcElementsFile.close();
		return atLeastOneRead;
	}
	else
	{
		qDebug() << "Unable to open for reading" << filePath;
		qDebug() << "File error:" << mpcElementsFile.errorString();
		return false;
	}
}
