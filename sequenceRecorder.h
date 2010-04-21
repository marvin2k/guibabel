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
		void setjointID( std::string id );
		void setfilterID( std::string id );
		void setVerbosity(int flag);

		int mRecordedElements;

	private:
		std::string m_jointID;
		std::string m_filterID;

		struct timeval time_start;
		struct timeval time_now;

		int isOpen;

		std::string mBasename;
		int mVerboseFlag;

		std::fstream fd_matlab;					//file to write out logged values
		std::string matlabfilename;
		SNDFILE* fd_wavfile;					//File to save wav-Data
		std::string wavfilename;

		int write_octave_header(std::fstream *fd);
		int appendStringToOctaveFile( std::fstream *fd, std::string type, std::string text );


};

#endif // SEQUENCERECORDER_H
