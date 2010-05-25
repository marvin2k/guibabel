#ifndef SEQUENCERECORDER_H
#define SEQUENCERECORDER_H

#include <sndfile.h>					//ubuntu 9.10: "apt-get intall libsndfile-dev"
#include <string>
#include <time.h>						  // nice timestrings for log files
#include <sys/time.h>					// hires usecs for timestamps
#include <fstream>
#include <vector>
#include <QString>

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
		void setjointId( QString id );
		void setfilterId( QString id );
		void setVerbosity(int flag);
		void setpwmspeedtorque( int pwm, int speed, int torque );

		int mRecordedElements;

	private:
		QString m_jointId;
		QString m_filterId;
		std::vector<int> motorData;

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
		int appendVectorToOctaveFile( std::fstream *fd, std::string type, std::vector<int> *data);
		int appendScalarToOctaveFile( std::fstream *fd, std::string name, int *data);


};

#endif // SEQUENCERECORDER_H
