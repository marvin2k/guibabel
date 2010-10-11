#include "DLogger.h"

#include <QDebug>
#include <QDateTime>

/**
 * Logging class... See header-file for details
 *
 */
DLogger::DLogger(QGroupBox *parent, QString basename) : QGroupBox(parent){
	setupUi(this);

	m_baseName = basename;
	m_marker = 0;
	isRecording = false;

	// The two used "Vaults" to store data in it. Will be new'ed every time logging starts. Otherwise, this NULL is used as a marker
	csvVault = NULL;
	octaveVault = NULL;

	// default-state of logging formats:
	cBx_csvVault->setChecked(false);
	cBx_octaveVault->setChecked(true);
	cBx_wavVault->setChecked(true);

	// self-refreshing Gui:
	guiTimer.start(250);// 250ms == 4Hz, more than enough

	// initialize functionality of widget:
	connect(pB_startStop_recording, SIGNAL(clicked()), this, SLOT(trigger_recording()));
	connect(checkBox_autochooseFilename, SIGNAL(stateChanged(int)), this, SLOT(trigger_autochooseFilename(int)));
	connect(pB_set_marker, SIGNAL(clicked()), this, SLOT(triggerMarker()));
	connect(&guiTimer, SIGNAL(timeout()), this, SLOT(updateGui()));

	// disable some parts of the Gui while no file is open...
	pB_set_marker->setDisabled(true);
	lineEdit_fileName->setDisabled(true);
	checkBox_autochooseFilename->setChecked(true);

	// TODO: this would be fancy... but needs qt4.7!!!
	/*lineEdit_fileName->setPlaceholderText(m_baseName + "_" + "yyyy-MM-dd_hh-mm-ss");*/

	// add our own, encapsulated payload
	m_logNames.append("logTime");
	m_logNames.append("marker");
}

DLogger::~DLogger(){

	if (isRecording) {
		if (csvVault)
			delete csvVault;
		if (octaveVault)
			delete octaveVault;
		if (wavVault)
			delete wavVault;
	}
}

/**
 * Proper setting of valueNames. This property is stored internally
 */
void DLogger::addColumns(const QList<QString> *names){
	for (int i = 0; i < names->size(); i++)
		m_logNames.append( names->at(i) );
}
void DLogger::addColumn(const QString *name){
	m_logNames.append( *name );
}

/**
 * 
 */
void DLogger::addLogValues(const QList<double> *dataList){
	if (isRecording){

		QList<double> dataAll;
		dataAll.append(m_sampleTime.elapsed());
		dataAll.append(m_marker);
		dataAll += *dataList;

		if (csvVault)
			*csvVault << dataAll;
		if (octaveVault)
			*octaveVault << dataAll;
		if (wavVault)
			*wavVault << dataList->first();// ATTENTION: only taking first value!!! Single-Channel-Audio and so on
	}
	m_marker = 0;// if a marker was set, return to zero to allow new setting
}
/**
 * Now the functionality of our "Widget"
 */
void DLogger::trigger_recording(){

	if (pB_startStop_recording->text() == "start recording") {

		m_sampleTime.restart();

		if (cBx_csvVault->isChecked())
			csvVault = new DCsvVault(getFileName(), m_logNames);
		if (cBx_octaveVault->isChecked())
			octaveVault = new DOctaveVault(getFileName(), m_logNames);
		if (cBx_wavVault->isChecked())
			wavVault = new DWavVault(getFileName());// ATTENTION: dropping the possivle filenames, just choosing the first col

		if ((csvVault && csvVault->isOpen()) || (octaveVault && octaveVault->isOpen()) || (wavVault && wavVault->isOpen())){

			// modify UI if successfull, else nothing is changed
			lineEdit_fileName->setDisabled(true);
			pB_set_marker->setEnabled(true);
			pB_startStop_recording->setText("stop recording");
			checkBox_autochooseFilename->setDisabled(true);
			
			cBx_csvVault->setDisabled(true);
			cBx_octaveVault->setDisabled(true);
			cBx_wavVault->setDisabled(true);

			isRecording = true;

			emit loggingStarted();

		} else {
			if (csvVault) {
				delete csvVault;
				csvVault = NULL;
			}
			if (octaveVault){
				delete octaveVault;
				octaveVault = NULL;
			}
			if (wavVault){
				delete wavVault;
				wavVault = NULL;
			}
			qDebug() << "DLogger: Problem starting logging";
		}

	} else {
		if (csvVault) {
			delete csvVault;
			csvVault = NULL;
		}
		if (octaveVault) {
			delete octaveVault;
			octaveVault = NULL;
		}
		if (wavVault) {
			delete wavVault;
			wavVault = NULL;
		}

		lineEdit_fileName->setEnabled(!checkBox_autochooseFilename->isChecked());
		pB_set_marker->setDisabled(true);
		pB_startStop_recording->setText("start recording");
		checkBox_autochooseFilename->setEnabled(true);
		
		cBx_csvVault->setEnabled(true);
		cBx_octaveVault->setEnabled(true);
		cBx_wavVault->setEnabled(true);

		isRecording = false;

		emit loggingStopped();
	}
}
void DLogger::trigger_autochooseFilename(int newstate){
	if ( newstate == Qt::Checked) {
		lineEdit_fileName->setDisabled(true);
		//lineEdit_fileName->setText("");//seems not to be necessary?
	} else {
		lineEdit_fileName->setEnabled(true);
	}
}

/**
 * We allow triggering from outside:
 * Payload is set to zero by default -- so normaly we just increment the counter which will be
 * resetted after writing the old value to a file
 *
 * we will save and reset this value with the next line to write.
 * if button is pressed more than ones since then, a higher value will be printed
 * This is a feature...
 */
void DLogger::triggerMarker(double payload){
	if (!isRecording)
		qDebug() << "DLogger: marker accessed while _not_ recording. Intended? Will increase marker anyways...";

	if (payload)
		m_marker = payload;
	else
		m_marker++;
}

bool DLogger::startLogging(QString basename){
	setLogfileBaseName(basename);
	return startLogging();
}

bool DLogger::startLogging(){
	if (isRecording) {
		qDebug() << "can't start logging, already in progress";
		return false;
	}
	emit trigger_recording();
	return true;
}

bool DLogger::stopLogging(){
	if (!isRecording) {
		qDebug() << "can't stop logging, nothing in progress";
		return false;
	}
	emit trigger_recording();
	return true;
}
/**
 * If no filename was given, we will make one based on the given prefix, ending and the current date
 * - if calling this function, while not recording (ie from outside this class) to get filename, a
 *   false-name is generated, which is never used at all!!!
 */
QString DLogger::getFileName(){
	if (checkBox_autochooseFilename->isChecked()){
		lineEdit_fileName->setText(m_baseName+"_"+QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
	}
	return lineEdit_fileName->text();
}
void DLogger::setLogfileBaseName(QString filename){
	checkBox_autochooseFilename->setChecked(false);
	lineEdit_fileName->setText(filename);
}
// some statistics
void DLogger::updateGui(){
	if (isRecording) {
		if (csvVault)
			label_size_csvVault->setText(QString::number(csvVault->size()/1024.0,'f',2));
		if (octaveVault)
			label_size_octaveVault->setText(QString::number(octaveVault->size()/1024.0,'f',2));
		if (wavVault)
			label_size_wavVault->setText(QString::number(wavVault->size()/1024.0,'f',2));

		// ATTENTION. ignoring dimension of wav-recordering
		if (csvVault)
			label_recorded_dimension->setText("("+QString::number(csvVault->rows())+":"+QString::number(csvVault->cols())+")");
		else
			label_recorded_dimension->setText("("+QString::number(octaveVault->rows())+":"+QString::number(octaveVault->cols())+")");

	} else {
		label_size_csvVault->setText("----");
		label_size_octaveVault->setText("----");
		label_size_wavVault->setText("----");
		label_recorded_dimension->setText("----");
	}
}
