#include "PCMdekoder.h"


#define VERBOSE_PRINTF(...) if (m_verboselevel > 0) { \
                                printf("%s:%i: ",__FILE__,__LINE__);\
                                printf(__VA_ARGS__);\
                            }

PCMdekoder::PCMdekoder() {
	//ctor
	m_sample_down_counter = 50;

	Set_portname("/dev/ttyUSB0");

	stop_running = false;
	is_recording = false;

}

PCMdekoder::~PCMdekoder() {
	//dtor
}

bool PCMdekoder::init(){

	m_status.invalidPCMwords = 0;
	m_status.validPCMwords = 0;
	m_status.recordedPCMwords = 0;

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
	while (this->isRunning()){
		VERBOSE_PRINTF("Stopping serial thread, waiting for him to exit...\n");
		stop_running = true;
		sleep(1);
	}
	source->uninit();
	delete source;
}

bool PCMdekoder::start_recording(){
// sequence recorder is prepared in gui.cpp

	if (is_recording){
		VERBOSE_PRINTF("can record, already running\n");
		return false;
	}

	m_status.recordedPCMwords = 0;
	is_recording = true;

	if (m_sample_down_counter>=0)
		VERBOSE_PRINTF("started recording of %i samples\n",m_sample_down_counter);
	if (m_sample_down_counter<0)
		VERBOSE_PRINTF("started indefinetly recording\n");

	return true;
}

void PCMdekoder::run(){
	VERBOSE_PRINTF("Dekoder-Thread called on %s with thread ID %i\n",m_portname.toAscii().data(),(int)currentThreadId());
	//exec();//normal in qthreads to enter qt-command queue

	while ( !stop_running ) {
		source->readPCMword(&m_lastValue);
		m_status.invalidPCMwords = source->m_invalid;
		m_status.validPCMwords = source->m_valid;

		if (is_recording){
			// only decrement counter, if it's greater zero
			if (m_sample_down_counter > 0)
				m_sample_down_counter--;
			m_status.recordedPCMwords++;
			if ( drain->pushPCMword(m_lastValue) < 0 ) {
				VERBOSE_PRINTF("Error: can't write, will stop recording\n");
				is_recording = false;
				emit recordingFinished();
			}
			// only stop if equal to zero. doing this and the decrementing-if-greater above, we will record indefinetly if m_sample_downcaounter is negative
			if (m_sample_down_counter == 0) {
				VERBOSE_PRINTF("guibabel finished recording\n");

				is_recording = false;
				emit recordingFinished();
			}
		}
	}

	if (drain->isOpen) {
				drain->close();
				delete drain;
	}

	VERBOSE_PRINTF("serial worker thread was exited, and finished running\n");
}

void PCMdekoder::send_command(char * data, int len){
	VERBOSE_PRINTF("sending scale command %i\n",(int8_t)*data);
	if ( source->write(data, len) != len)
		perror("problem writing to seriel port\n");
}
