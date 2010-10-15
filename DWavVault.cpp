#include "DWavVault.h"

#include <QDebug>

#include <sys/stat.h>

DWavVault::DWavVault(QString baseName)
{
	struct SF_INFO wav_info;

	wav_info.samplerate = 9000;
	wav_info.channels = 1;
	wav_info.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);

	m_filename = baseName+".wav";
	wav = NULL;
	wav = sf_open(m_filename.toAscii().data(), SFM_WRITE, &wav_info );

	if (wav == NULL) {
		qFatal("Error opening wavfile\n");
	} else {
		qDebug() << "DWavVault: Successfully opened logfile" << m_filename;
	}

	m_nrOfRows = 0;

}

DWavVault::~DWavVault()
{
	sf_close(wav);

	qDebug() << "DWavVault: Successfully closed logFile" << m_filename;
}

void DWavVault::AddLogValue(const double value){

	//wavfile
	int16_t valueI = (int16_t)value;
	sf_writef_short( wav, (const short *)&valueI, 1);
	
	m_nrOfRows++;
}

int DWavVault::size(){
	sf_write_sync(wav);

    struct stat results;
    
    if (stat(m_filename.toAscii().data(), &results) == 0)
    {
        return results.st_size;
    } else
    {
    	qFatal("DWavVault: an error occured");
    	return -1;
    }

}

int DWavVault::rows(){
	return m_nrOfRows;
}

int DWavVault::cols(){
	return 1;
}

int DWavVault::isOpen(){
	if (wav == NULL)
		return true;
	else
		return false;
}

