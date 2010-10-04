#include "PCMdekoder.h"


#define VERBOSE_PRINTF(...) if (m_verboselevel > 0) { \
                                printf("%s:%i: ",__FILE__,__LINE__);\
                                printf(__VA_ARGS__);\
                            }

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

	if (source->init(m_portname.toAscii().data(), m_baudrate, m_verboselevel)) {
		return true;
	} else {
		source->uninit();
		delete source;
		return false;
	}
}

void PCMdekoder::uninit(){
	stop_running = true;
	while (this->isRunning()){
		VERBOSE_PRINTF("Stopping serial thread, waiting for him to exit...\n");
		stop_running = true;
		usleep(100);
	}
	source->uninit();
	delete source;
}

void PCMdekoder::run(){
	QString PCM = QString("PCM");
	VERBOSE_PRINTF("Dekoder-Thread called on %s with thread ID %i\n",m_portname.toAscii().data(),(int)currentThreadId());
	//exec();//normal in qthreads to enter qt-command queue

	while ( !stop_running ) {
		source->readPCMword(&m_lastValue);
		m_status.invalidPCMwords = source->m_invalid;
		m_status.validPCMwords = source->m_valid;

		emit newData(PCM,m_lastValue);
	}
}

