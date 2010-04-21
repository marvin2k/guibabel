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
	m_jointID = "default";
	m_filterID = "default";
}
sequenceRecorder::sequenceRecorder(std::string basename) {
	mVerboseFlag = 0;
	isOpen = 0;
	m_jointID = "default";
	m_filterID = "default";
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
    strftime(buffer,80,"babel_%Y_%m_%d_%H_%M_%S",&timedata);
	std::string basename( buffer );

	return setBasename(basename);
}

void sequenceRecorder::setjointID( std::string id ){
	m_jointID = id;
}
void sequenceRecorder::setfilterID( std::string id ){
	m_filterID = id;
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

void sequenceRecorder::setpwmspeedtorque( int pwm, int speed, int torque ) {

	if (motorData.size() > 0)
		VERBOSE_PRINTF("clearing motorData vector\n");

	motorData.push_back(pwm);
	motorData.push_back(speed);
	motorData.push_back(torque);

}


int sequenceRecorder::write_octave_header(std::fstream *fd){
	char *timestring = new char[80];
	char *hostname = new char[80];

	time_t date_now;
	struct tm timedata;

	date_now = time(NULL);
	timedata = *(localtime(&date_now));

	strftime(timestring,80,"%a %b %d %H:%M:%S %Y %Z",&timedata);
	if (gethostname(hostname,80)) {
		printf("some problem accessing the hostname\n");
		sprintf(hostname,"defaulthostname");
	}

	*fd << "# created by guibabel, "<<timestring<<" <"<<getenv("USER")<<"@"<<hostname<<">"<<std::endl;

	appendStringToOctaveFile(fd, "recTime", std::string(timestring) );

	*fd << "# name: sequenzData"<<std::endl;

	*fd << "# type: matrix"<<std::endl;

	*fd << "# REPLACEME1:"<< std::endl;

	*fd << "# columns: 2"<<std::endl;

	delete timestring;
	delete hostname;

	return 1;
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

	matlabfilename = "data/"+mBasename+".mat";
	VERBOSE_PRINTF("Matlabfile: %s\n", matlabfilename.c_str());
	fd_matlab.open(matlabfilename.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::trunc );

	if (fd_matlab.bad()){
		perror("Error opening matlabfile\n");
		return EXIT_FAILURE;
	}

	write_octave_header(&fd_matlab);

	//open wav file
	struct SF_INFO wav_info;

	wav_info.samplerate = 9000;
	wav_info.channels = 1;
	wav_info.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);

	wavfilename = "wavs/"+mBasename+".wav";
	VERBOSE_PRINTF("Wavfile: %s\n", wavfilename.c_str());
	fd_wavfile = sf_open(wavfilename.c_str(), SFM_WRITE, &wav_info );

	if (fd_wavfile == NULL) {
		fd_matlab.close();
		perror("Error opening wavfile\n");
		return EXIT_FAILURE;
	}

	gettimeofday(&time_start, NULL);

	isOpen = true;
	mRecordedElements = 0;

	return isOpen;
}

int sequenceRecorder::pushPCMword( int16_t word ) {
	if (!isOpen) {
		printf("fehler beim schreiben in sequencerekorder: keine dateien geöffnet\n");
		return EXIT_FAILURE;
	}

	gettimeofday(&time_now, NULL);

	//wavfile
	sf_writef_short( fd_wavfile, (const short *)&word, 1);

	//some kind of MATLAB/octave-readable format
	int t_usec = (time_now.tv_sec-time_start.tv_sec)*1000000 + (time_now.tv_usec-time_start.tv_usec);
	fd_matlab << " " << t_usec <<" "<<word<<std::endl;

	mRecordedElements++;

	return EXIT_SUCCESS;
}

int sequenceRecorder::appendVectorToOctaveFile( std::fstream *fd, std::string type, std::vector<int> *data){
	*fd << "# name: "<< type <<std::endl;

	*fd << "# type: matrix"<<std::endl;

	*fd << "# rows: 1"<< std::endl;

	*fd << "# columns: "<< data->size()<<std::endl;

	for (unsigned int i=0;i<data->size();i++){
		*fd << " " << data->at(i);
	}

	*fd << std::endl;

	return 1;
}

int sequenceRecorder::appendStringToOctaveFile( std::fstream *fd, std::string type, std::string text){
	*fd << "# name: "<< type <<std::endl;

	*fd << "# type: sq_string"<<std::endl;

	*fd << "# elements: 1"<< std::endl;

	*fd << "# length: "<< text.length()<<std::endl;

	*fd << text << std::endl;

	return 1;
}

int sequenceRecorder::close() {
	if (!isOpen){
		VERBOSE_PRINTF("Vergebliches schließen der SequenceRecorder Filedescriptoren, vermutlich schon geschlossen\n");
		return EXIT_SUCCESS;
	}
	// close wavfile
	sf_close( fd_wavfile );

	// append jointID and filterID to logfile
	appendStringToOctaveFile(&fd_matlab, "filterID", m_filterID );
	appendStringToOctaveFile(&fd_matlab, "jointID", m_jointID );
	appendVectorToOctaveFile(&fd_matlab, "motorData", &motorData);

	// close matlabfile, write number of rows to header
	std::fstream tempfile;
	tempfile.open(".tempfile.mat", std::ios_base::in | std::ios_base::out | std::ios_base::trunc);
	std::string tempstring;
	fd_matlab.flush();
	fd_matlab.seekp(std::ios_base::beg);

	while (!fd_matlab.eof()){
		getline(fd_matlab, tempstring);
		if (tempstring == "# REPLACEME1:") {
			char buf[80];
			sprintf(buf,"# rows: %i",mRecordedElements);
			tempstring = buf;
			VERBOSE_PRINTF("found header-rows-line in octavefile, replacing it %i\n",mRecordedElements);
		}
		tempfile << tempstring << std::endl;
	}

	tempfile.close();
	fd_matlab.close();

    remove(matlabfilename.c_str());
    // rename old to new
    rename(".tempfile.mat",matlabfilename.c_str());

    // all done!
	isOpen = false;
	VERBOSE_PRINTF("successfully closed files\n");

	return EXIT_SUCCESS;
}

sequenceRecorder::~sequenceRecorder() {
	if (isOpen)
		close();
	VERBOSE_PRINTF("Closed sequenceRecorder\n");
	//dtor
}
