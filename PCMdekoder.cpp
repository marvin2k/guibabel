#include "PCMdekoder.h"

#include <QDebug>
#include <QString>

#define VERBOSE_PRINTF(...) if (m_verboselevel > 0) { \
                                printf("%s:%i: ",__FILE__,__LINE__);\
                                printf(__VA_ARGS__);\
                            }

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

void PCMdekoder::run(){
	VERBOSE_PRINTF("Dekoder-Thread called on %s with thread ID %i\n",m_portname.toAscii().data(),currentThreadId());
	//exec();
	struct timeval t_start, t_seq, t_seq2;
	float t_now;
	gettimeofday(&t_start, NULL);

	while ( !stop_running ) {
		source->read_pcm(&m_lastValue,1);
		if (is_recording)  {
			gettimeofday(&t_seq,NULL);
			t_seq2.tv_sec = t_seq.tv_sec - t_start.tv_sec;
			t_seq2.tv_usec = t_seq.tv_usec - t_start.tv_usec;
			t_now = t_seq2.tv_sec*1000 + t_seq2.tv_usec/1000.0;
			drain->pushPCMword(m_lastValue, t_seq2);
			printf("recorded %f ms of %i ms\n",t_now, m_recordingTime);
			if (t_now > m_recordingTime) {
				VERBOSE_PRINTF("After %f microseconds, guibabel finished recording\n",t_now);

				drain->close();
				delete drain;
				is_recording = false;
			}
		}
	}
	printf("finished running\n");
}

void PCMdekoder::send_command(char * data, int len){
	source->write(data, len);
}
