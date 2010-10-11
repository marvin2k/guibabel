#include "DWavVault.h"

#include <QDebug>

DWavVault::DWavVault(QString baseName)
{
	struct SF_INFO wav_info;

	wav_info.samplerate = 9000;
	wav_info.channels = 1;
	wav_info.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);

	filename = baseName+".wav";
	logFile = NULL;
	logFile = sf_open(filename.toAscii().data(), SFM_WRITE, &wav_info );

	if (logFile == NULL) {
		qFatal("Error opening wavfile\n");
	}

//	if (logFile->isOpen()){
//		qDebug() << "DWavVault: Successfully opened logFile" << filename;
//	} else {
//		qWarning() << "DWavVault: Could'n open logFile";
//	}

	m_nrOfRows = 0;

}

DWavVault::~DWavVault()
{
	sf_close(logFile);

	qDebug() << "DWavVault: Successfully closed logFile" << filename;
}

void DWavVault::AddLogValue(const double value){

	//wavfile
	int16_t valueI = (int16_t)value;
	sf_writef_short( logFile, (const short *)&valueI, 1);
	
	m_nrOfRows++;
}

int DWavVault::size(){
	sf_write_sync(logFile);
	qDebug() << "DWavVault: size() not implemented...";
	return -1;
}

int DWavVault::rows(){
	return m_nrOfRows;
}

int DWavVault::cols(){
	return 1;
}

int DWavVault::isOpen(){
	qDebug() << "DWavVault: isOpen() not implemented...";
	return true;
}

