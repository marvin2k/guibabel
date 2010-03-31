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
		int pushPCMword( int16_t word, struct timeval t_seq );

		int setBasename();
		std::string getBasename();
		int setBasename(std::string basename);
		void setVerbosity(int flag);

	private:

		int isOpen;

		std::string mBasename;
		int mVerboseFlag;

		std::ofstream fd_matlab;					//file to write out logged values
		SNDFILE* fd_wavfile;					//File to save wav-Data

		// some temporary space to store data
		char buf[80];
		int len;

};

#endif // SEQUENCERECORDER_H
