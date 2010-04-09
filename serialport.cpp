
#include "serialport.h"

/* prints filename and linenumber in addition to given fprintf-fmt */
#define VERBOSE_PRINTF(...) if (mVerbose_flag > 0) { \
                                printf("%s:%i: ",__FILE__,__LINE__);\
                                printf(__VA_ARGS__);\
                            }
#include <stdio.h>						// printf

#include <stdint.h>//uint8_t


#include <string.h>						// memset

#include <unistd.h>						// serial, file
#include <fcntl.h>						// serial, file

#include <errno.h>
serialport::serialport(){
	mIsOpen = false;
}

serialport::serialport(std::string devicename, int baudrate, int verbose_flag){
	init(devicename, baudrate, verbose_flag);
}

bool serialport::init(std::string devicename, int baudrate, int verbose_flag) {
	mDevicename = devicename;
	mVerbose_flag = verbose_flag;
	speed_t new_baudrate;

	VERBOSE_PRINTF("Trying to open %s with %i Baud, 8N1\n",mDevicename.c_str(),baudrate);

	fd = open(mDevicename.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

    if (fd < 0) {
    	perror("Error opening a serialport -- are you root? device plugged in? node existant?\n");
		mIsOpen = false;
		return mIsOpen;
    }

    tcgetattr(fd, &tty_old);				// save current port settings

    memset((void*)&tty, 0, sizeof(tty));	// Initialize the port settings structure to all zeros
	cfmakeraw(&tty);

	switch (baudrate) {
		case 115200:
			new_baudrate = B115200;
			break;
		case 230400:
			new_baudrate = B230400;
			break;
		default:
			VERBOSE_PRINTF("Unsopported Baudrate, falling back to 230400 Baud\n");
			new_baudrate = B230400;
	}

	if ( cfsetospeed(&tty, new_baudrate) ) {
		perror("Cannot write outputbaudrate into struct\n");
		mIsOpen = false;
		return mIsOpen;
	}
	if ( cfsetispeed(&tty, new_baudrate) ) {
		perror("Cannot write inputbaudrate into struct\n");
		mIsOpen = false;
		return mIsOpen;
	}
	//revise...
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;
    if ( flush() ) {				// flush old data of port, before applying new setting
		perror("Cannot flush serialport\n");
		mIsOpen = false;
		return mIsOpen;
	}
    if ( tcsetattr(fd, TCSANOW, &tty) ) {				// apply new settings
		perror("Cannot apply new setting of serialport\n");
		mIsOpen = false;
		return mIsOpen;
	} else {
		VERBOSE_PRINTF("Successfully opened serialport %s -- congratulations!\n",mDevicename.c_str());

		mIsOpen = true;
	}

	return mIsOpen;
}

serialport::~serialport() {
	uninit();
	VERBOSE_PRINTF("object serialport deleted\n");
}

void serialport::uninit(){
	if ( mIsOpen ) {
		if (tcsetattr(fd, TCSANOW, &tty_old) ) {
			perror("Cannot restore old settings of serialport\n");
		}

		if (!close(fd)) {
			mIsOpen = false;

			VERBOSE_PRINTF("Closed serialport\n");
		} else {
			perror("problems closing serialport\n");
		}
	}

}

int serialport::read_pcm( int* values, int number ) {

	char byte;
	int16_t result = 0;
	static int state = 0;
	static int8_t first_byte = 0;
	static int8_t second_byte = 0;
	int counter = 0;

	//VERBOSE_PRINTF("beginning read_pcm loop, state=%i, firstbyte=%i, secondbyte=%i\n",state,first_byte,second_byte);

	while (counter < number) {
		VERBOSE_PRINTF("read raw byte %i at state %i\n",byte,state);

		//check if first byte, encoded by leading "one" (in MSB)
		if ( (byte & 0x80) && (state == 0)){
			first_byte = byte;
			state = 1;
		// second byte, leading zero
		} else if ( !(byte & 0x80) && (state == 1)) {
			second_byte = byte;
			// Step back, pure magic...
			// mask out first two bits of second_byte (marker bit + sign) and
			// shift result into correct position, together with second byte
			result = ((first_byte&0x7f)<<9) | ((second_byte&0x7f)<<2);
			result = result>>2;

			state = 0;
			values[counter] = result;
			mLastPcmValue = result;
			counter++;
		} else {
			VERBOSE_PRINTF("skipping value\n");
			state = 0;
		}

	}
	return counter;//nuber of pcm-bytes read
}

int serialport::read( char* buffer, size_t nr ) {
    // non-blocking read with select
    // see: http://www.developerweb.net/forum/showthread.php?t=2933
    // (first and second example)

	fd_set readset;

    int result;
    // Initialize time out struct
    struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500 * 1000;//500ms

    FD_ZERO(&readset);
    FD_SET(fd, &readset);

    result = select( fd+1, &readset, NULL, NULL, &tv);

	if (result < 0){
		VERBOSE_PRINTF("error while select()\n");
		return -1;
	} else if (result > 0 && FD_ISSET(fd, &readset)){

		result = ::read( fd, buffer, nr);
        return result;

    }

    VERBOSE_PRINTF("got timeout while trying to read from serialport %s\n",mDevicename.c_str());

	return -2;
}

int serialport::write( char* buffer, size_t nr ) {

	return ::write(fd, buffer, nr);

}

int serialport::flush( void ) {
	return tcflush(fd, TCIFLUSH);
}
