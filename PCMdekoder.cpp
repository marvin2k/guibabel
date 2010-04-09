#include "PCMdekoder.h"

#include <QDebug>
#include <QString>

#define VERBOSE_PRINTF(...) if (m_verboselevel > 0) { \
                                printf("%s:%i: ",__FILE__,__LINE__);\
                                printf(__VA_ARGS__);\
                            }

#define PCM_RATE 9043

PCMdekoder::PCMdekoder() {
	//ctor
	Set_portname("/dev/ttyUSB0");

	stop_running = false;
	is_recording = false;

}

PCMdekoder::~PCMdekoder() {
	//dtor
}

bool PCMdekoder::init(){

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
		usleep(100);
	}
	source->uninit();
	delete source;
}

void PCMdekoder::start_recording(int rec_time){
// sequence recorder is prepared in gui.cpp
	m_sample_down_counter = (int)((PCM_RATE*rec_time)/1000);
	is_recording = true;
	VERBOSE_PRINTF("startet recording of %i samples\n",m_sample_down_counter);

}

void PCMdekoder::run(){
	VERBOSE_PRINTF("Dekoder-Thread called on %s with thread ID %i\n",m_portname.toAscii().data(),currentThreadId());
	//exec();
	struct timeval t_start, t_seq, t_seq2;
	float t_now;
	gettimeofday(&t_start, NULL);

	while ( !stop_running ) {
		source->read_pcm(&m_lastValue,1);
		if (is_recording){
			if (m_sample_down_counter > 0)  {
				m_sample_down_counter--;
				drain->pushPCMword(m_lastValue, t_seq2);
			} else {
				VERBOSE_PRINTF("guibabel finished recording\n",t_now);

				drain->close();
				delete drain;
				is_recording = false;
			}
		} else {
			// wait a little bit to unstress cpu
			usleep(10);
		}
	}
	VERBOSE_PRINTF("serial worker thread finished running\n");
}

void PCMdekoder::send_command(char * data, int len){
	source->write(data, len);
}
