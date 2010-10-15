#include "DOctaveVault.h"

#include <QTime>
#include <QDebug>
#include <QHostInfo>//to write <username@hostname> as a String into mat-file

DOctaveVault::DOctaveVault(QString baseName, QList<QString> &logValues)
{
	logFile.setFileName(baseName + ".mat");

	if (logFile.exists()){
		qWarning() << "DOctaveVault: Warnung: File exists already -- Won't overwrite!";
	}

	if (logFile.open(QFile::ReadWrite | QFile::Truncate | QIODevice::Text)) {
		// Here is where the magic happens!
		logStream.setDevice(&logFile);
	} else {
		qWarning() << "DOctaveVault: Warnung: Problem handling file:" << logFile.errorString();
		logFile.unsetError();
	}

	if (logFile.isOpen()){
		qDebug() << "DOctaveVault: Successfully opened logfile" << logFile.fileName();
	} else {
		qWarning() << "DOctaveVault: Could'n open logfile";
	}

	m_nrOfCols = logValues.size();
	m_nrOfRows = 0;
		
	writeHeader();
}

DOctaveVault::~DOctaveVault()
{
	finishMatrixToOctaveFile();

	logFile.close();

	qDebug() << "DOctaveVault: Successfully closed logfile" << logFile.fileName();
}

/**
 * The overloaded << operator
 */
void DOctaveVault::AddLogValue(const QList<double> logdata){

	if (logdata.length() != m_nrOfCols)
		qDebug() << "DOctaveVault: Number of Columns changed. This is not supported an will result in some chaos, remarkable chaos. be warned!";

	for (int i = 0; i < logdata.size()-1; ++i)
		logStream << logdata.at(i) << " ";
	logStream << logdata.last() << endl;

	m_nrOfRows++;

}

int DOctaveVault::size(){
	logStream.flush();
	return logFile.size();
}

int DOctaveVault::rows(){
	return m_nrOfRows;
}

int DOctaveVault::cols(){
	return m_nrOfCols;
}

int DOctaveVault::isOpen(){
	return logFile.isOpen();
}
/**
 * Writing of Octave files
 */
void DOctaveVault::addValueToOctaveFile( QString name, QString data){
	if (m_nrOfRows > 0)
		qDebug() << "DOctaveVault: writing string to alreday started logging file... will probably break octave-syntax";
		
	logStream << "#  name: "<< name << endl;
	logStream << "#  type: sq_string" << endl;
	logStream << "#  elements: 1" << endl;
	logStream << "#  length: "<< data.length() << endl;
	logStream << data << "\n";
}

void DOctaveVault::writeHeader() {

	QString system =  "<" + QString(getenv("USER")) + "@" + QHostInfo::localHostName() + ">";
	QString date = QDateTime::currentDateTime().toString(Qt::ISODate);

	logStream << "## DFKI Logging-Vault for Octave:" << endl;
	logStream << "##" << endl;
	logStream << "## This log was created by " << system << " at " << date << endl;
	logStream << "##" << endl;
	logStream << "## File uses Octave-notation, loading and ploting in octave is easy:" << endl;
	logStream << "##" << endl;
	logStream << "## octave:1> load('" << logFile.fileName() << "');" << endl;
	logStream << "## octave:2> starttime" << endl;
	logStream << "## octave:3> plot(sequenceData(:,1),sequenceData(:,2),';data;');" << endl;
	logStream << "## octave:4> xlabel('t in ms');ylabel('data');" << endl;

	logStream << endl; // octave-parser seems to need empty line to catch first string

	addValueToOctaveFile( "filename", logFile.fileName());
	addValueToOctaveFile( "recTime", date);
	addValueToOctaveFile( "username", system);
	addValueToOctaveFile( "jointId", "N/A");
	addValueToOctaveFile( "filterId", "N/A");

	// begin Matrix of log-data:
	// TODO: emulate union to include names of logValues
	logStream << "#  name: sequenceData" << endl;
	logStream << "#  type: matrix" << endl;
	// todo: ordentlichen marker, der von der breite von integer dynamisch erstellt wird
	logStream << "#  rows: ";logStream.flush();m_replaceRowNrPosition = logStream.pos();logStream << "----------" << endl;
	logStream << "#  columns: ";logStream.flush();m_replaceColNrPosition = logStream.pos();logStream << "----------" << endl;
}

void DOctaveVault::finishMatrixToOctaveFile(){
	//save current postion before jumping to replacement, so we may return
	logStream.flush();
	qint64 currPos = logStream.pos();

	// set some layout stuff to we will overwrite the existing placeholder. be carefull to overwrite is cimpletely!
	// todo: ordentlichen marker, der von der breite von integer dynamisch erstellt wird
	logStream.setFieldWidth(10);
	logStream.setFieldAlignment(QTextStream::AlignLeft);

	if (m_nrOfRows <= 0)
		qDebug() << "DOctaveVault: Warning: Seems to be that you try to finish without one row of data saved. intended?";

	// off we go... two times -- row and cols! Be nice and do some checks against dumb usage...
	logStream.seek(m_replaceRowNrPosition);
	logStream << m_nrOfRows;
	logStream.seek(m_replaceColNrPosition);
	logStream << m_nrOfCols;
	// reset these settings here...
	logStream.reset();

	//save and return to last position
	logStream.flush();
	logStream.seek(currPos);
}

