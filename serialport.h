#ifndef SERIALPORT_H_
#define SERIALPORT_H_

#include <string>
#include <termios.h>					// serial
#include <stdint.h>

class serialport {
  public:
    serialport();
    serialport(std::string devicename, int baudrate, int verbose_flag);

	bool init(std::string devicename, int baudrate, int verbose_flag);
	void uninit();

    ~serialport();

	int read( char *buffer, size_t len);
	int readPCMword( int* value );

	int write( char* buffer, size_t len);

	int flush( void );

    bool mIsOpen;// "open" means that our new tty-setting are written to device and fd is ready for reading


	int m_valid;
	int m_invalid;

  private:

	// will be used our "raw" port settings
	struct termios tty;
	// will be used to save and restore old port settings
	struct termios tty_old;

	int mVerbose_flag;

	std::string mDevicename;

	// filedescriptor
    int fd;
};

#endif //SERIALPORT_H_
