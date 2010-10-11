#include "PCMdekoder.h"

PCMdekoder::PCMdekoder() {

	Set_portname("/dev/ttyUSB0");

	stop_running = false;
}

PCMdekoder::~PCMdekoder() {
}

bool PCMdekoder::init(){

	m_status.invalidPCMwords = 0;
	m_status.validPCMwords = 0;

	// prepare serialport
	source = new serialport();

	if (source->init(m_portname.toAscii().data(), m_baudrate, 1)) {
		return true;
	} else {
		source->uninit();
		delete source;
		return false;
	}
}

void PCMdekoder::uninit(){
	stop_running = true;
	usleep(10000);
	while (this->isRunning()){
		qDebug() << "PCMdekoder: Stopping serial thread...";
		usleep(10000);
	}
	source->uninit();
	delete source;
}

void PCMdekoder::run(){
	QString PCM = QString("PCM");
	qDebug() << "PCMdekoder: Dekoder-Thread called on"<<m_portname<<"with thread ID"<<currentThreadId();
	//exec();//normal in qthreads to enter qt-command queue

	while ( !stop_running ) {
		source->readPCMword(&m_lastValue);
		m_status.invalidPCMwords = source->m_invalid;
		m_status.validPCMwords = source->m_valid;

		emit newData(PCM,m_lastValue);
	}
}

