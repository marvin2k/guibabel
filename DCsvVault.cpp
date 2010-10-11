#include "DCsvVault.h"

#include <QDebug>

DCsvVault::DCsvVault(QString baseName, QList<QString> &logValues)
{
	logFile.setFileName(baseName + ".csv");

	if (logFile.exists()){
		qWarning() << "DCsvVault: Warnung: " << logFile.fileName() << "exists already -- Won't overwrite!";
	}

	if (logFile.open(QFile::ReadWrite | QFile::Truncate | QIODevice::Text)) {
		// Here is where the magic happens!
		logStream.setDevice(&logFile);
	} else {
		qWarning() << "DCsvVault: Warnung: Problem handling file:" << logFile.errorString();
		logFile.unsetError();
	}

	if (logFile.isOpen()){
		qDebug() << "DCsvVault: Successfully opened logfile" << logFile.fileName();
	} else {
		qWarning() << "DCsvVault: Could'n open logfile";
	}

	m_nrOfCols = logValues.size();
	m_nrOfRows = 0;

	//logStream << "%% ";//no comment in csv needed?
	for (int i = 0; i < logValues.size()-1; ++i)
		logStream << logValues.at(i) << ", ";
	logStream << logValues.last() << endl;

}

DCsvVault::~DCsvVault()
{
	logFile.close();

	qDebug() << "DCsvVault: Successfully closed logfile" << logFile.fileName();
}

void DCsvVault::AddLogValue(const QList<double> values){

	if (values.length() != m_nrOfCols)
		qDebug() << "DCsvVault: Number of Columns changed. This is not supported an will result in some chaos, remarkable chaos. be warned!";

	for (int i = 0; i < values.size()-1; ++i)
		logStream << values.at(i) << ", ";
	logStream << values.last() << endl;

	m_nrOfRows++;
}

int DCsvVault::size(){
	logStream.flush();
	return logFile.size();
}

int DCsvVault::rows(){
	return m_nrOfRows;
}

int DCsvVault::cols(){
	return m_nrOfCols;
}

int DCsvVault::isOpen(){
	return logFile.isOpen();
}

