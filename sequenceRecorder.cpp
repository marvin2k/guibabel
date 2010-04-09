#include "sequenceRecorder.h"

/* prints filename and linenumber in addition to given fprintf-fmt */
#define VERBOSE_PRINTF(...) if (mVerboseFlag > 0) { \
                                printf("%s:%i: ",__FILE__,__LINE__);\
                                printf(__VA_ARGS__);\
                            }

#include <cstdlib>

sequenceRecorder::sequenceRecorder() {
	mVerboseFlag = 0;
	isOpen = 0;
}
sequenceRecorder::sequenceRecorder(std::string basename) {
	mVerboseFlag = 0;
	isOpen = 0;
	setBasename(basename);
}

int sequenceRecorder::setBasename( ) {
	//prepare default basename, will eventually be overwritten by cmd-line option if present
	time_t date_now;
	struct tm timedata;

    date_now = time(NULL);
    timedata = *(localtime(&date_now));

	// basic prefix for all sequence-files
	char buffer[80];
    strftime(buffer,80,"babel_%Y-%m-%d_%H-%M-%S",&timedata);
	std::string basename( buffer );

	return setBasename(basename);
}
int sequenceRecorder::setBasename( std::string basename ){
	if (isOpen) {
		printf("error, alreday open...\n");
		return EXIT_FAILURE;
	}
	mBasename = basename;

	return EXIT_SUCCESS;
}

std::string sequenceRecorder::getBasename() {
	return mBasename;
}

void sequenceRecorder::setVerbosity( int flag ){
	mVerboseFlag = flag;
}

int sequenceRecorder::open( ){
	//-------------------
	// Prepare Datafiles:
	//-------------------
	if (isOpen) {
		printf("error, is schon open\n");
		return EXIT_FAILURE;
	}

	if (mBasename.length() == 0){
		setBasename();
	}

    VERBOSE_PRINTF("Opening sequence files with basename \"%s\"\n",mBasename.c_str());

	std::string name;

	name = "data/"+mBasename+".m";
	VERBOSE_PRINTF("Matlabfile: %s\n", name.c_str());
	fd_matlab.open(name.c_str());

	if (fd_matlab.bad()){
		perror("Error opening matlabfile\n");
		return EXIT_FAILURE;
	}

	len = sprintf(buf,"%% MATLAB/Octave compatible logging-format. Recorded by babel\n");
	fd_matlab.write(buf,len);

	//open wav file
	struct SF_INFO wav_info;

	wav_info.samplerate = 9000;
	wav_info.channels = 1;
	wav_info.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);

	name = "wavs/"+mBasename+".wav";
	VERBOSE_PRINTF("Wavfile: %s\n", name.c_str());
	fd_wavfile = sf_open(name.c_str(), SFM_WRITE, &wav_info );

	if (fd_wavfile == NULL) {
		fd_matlab.close();
		perror("Error opening wavfile\n");
		return EXIT_FAILURE;
	}

	isOpen = 1;

	return isOpen;
}

int sequenceRecorder::pushPCMword( int16_t word, struct timeval t_seq ) {
	// do actual writing
	if (!isOpen) {
		printf("fehler beim schreiben in sequencerekorder: keine datei\n");
		return EXIT_FAILURE;
	}

	//wavfile
	sf_writef_short( fd_wavfile, (const short *)&word, 1);
	//some kind of matlab-readable format
	int t_usec = t_seq.tv_sec*1000000 + t_seq.tv_usec;
	len = sprintf(buf,"%i, %i;\n",t_usec, word);
	fd_matlab.write(buf,len);

	return EXIT_SUCCESS;
}

int sequenceRecorder::close() {
	if (!isOpen){
		VERBOSE_PRINTF("Vergebliches schließen der SequenceRecorder Filedescriptoren\n");
		return EXIT_SUCCESS;
	}
	// close wavfile
	sf_close( fd_wavfile );

	// close matlabfile
	fd_matlab.close();

	return EXIT_SUCCESS;
}

sequenceRecorder::~sequenceRecorder() {
	close();
	VERBOSE_PRINTF("Closed sequenceRecorder\n");
	//dtor
}
