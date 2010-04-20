#ifndef SEQUENCERECORDER_H
#define SEQUENCERECORDER_H

#include <sndfile.h>					//ubuntu 9.10: "apt-get intall libsndfile-dev"
#include <string>
#include <time.h>						  // nice timestrings for log files
#include <sys/time.h>					// hires usecs for timestamps
#include <fstream>

class sequenceRecorder
{
	public:
		/** Default constructor */
		sequenceRecorder();
		sequenceRecorder(std::string basename);
		/** Default destructor */
		~sequenceRecorder();

		int open();
		int close();
		int pushPCMword( int16_t word );

		int setBasename();
		std::string getBasename();
		int setBasename(std::string basename);
		void setVerbosity(int flag);

	private:

		struct timeval time_start;
		struct timeval time_now;

		int isOpen;

		int mRecordedElements;

		std::string mBasename;
		int mVerboseFlag;

		std::fstream fd_matlab;					//file to write out logged values
		std::string matlabfilename;
		std::string wavfilename;
		SNDFILE* fd_wavfile;					//File to save wav-Data

		int write_octave_header(std::fstream *fd);


};

#endif // SEQUENCERECORDER_H
