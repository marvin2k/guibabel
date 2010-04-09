#include "gui.h"
#include <QApplication>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <fcntl.h>

#include <QDebug>

#define VERBOSE_PRINTF(...) if (verbose_flag > 0) { \
                                printf("%s:%i: ",__FILE__,__LINE__);\
                                printf(__VA_ARGS__);\
                            }
int verbose_flag = 0;

int main( int argc, char* argv[])
{
	QApplication a(argc, argv);

	QString devicename("/dev/ttyUSB0");
	QString basename;
	int newScale = -1;
	int t_recordlen = 50;
	int baudrate = 230400;
	bool start_gui  = true;
//-------------------
// parsing of cmdline-options
//-------------------
    int c;
    while ((c = getopt (argc, argv, "nvt:f:s:p:b:")) != -1) {
    /* getopt... see http://www.gnu.org/s/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt */
        switch (c) {
            case 'v':
                verbose_flag = 1;
                printf("-v: Will be verbose\n");
                break;
			case 'n':
				start_gui = false;
				VERBOSE_PRINTF("starting/showing gui disabled, cmd-line-only version\n");
				break;
            case 'p':
                VERBOSE_PRINTF("-p: Devicename serialport: %s !!!NOT SUPPORTED WITHGUI!!! trying to open...\n",devicename.toAscii().data());
				struct serial_struct serinfo;
	            int fd;
	            fd = ::open(optarg, O_RDWR | O_NONBLOCK );
				if (fd >= 0) {
					if (ioctl (fd, TIOCSSERIAL, &serinfo)) {
					// serial device is a serial device, indeed
					// see http://stackoverflow.com/questions/2530096/linux-how-to-find-all-serial-devices-ttys-ttyusb-without-opening-them
					devicename.fromAscii(optarg);
					VERBOSE_PRINTF("\t\tsucessfully opened serial port!\n");
					} else {
						printf("\t\tcould not access serial port, exiting\n");
						::close(fd);
						exit(EXIT_FAILURE);
					}
					::close(fd);
				} else {
					printf("\t\tcould not open serial port, exiting\n");
					exit(EXIT_FAILURE);
				}
                break;
        	case 's':
				newScale = atoi(optarg);
                VERBOSE_PRINTF("-s: Send scale-command: %i\n",newScale);
				if (newScale < 0 || newScale > 56) {
					printf("Unsopported scale range, but won't exit -- unspoorted behaviour may occour\n");
				}
                break;
            case 'f':
				basename = optarg;
                VERBOSE_PRINTF("-f: Basename for Outputfiles: %s\n",basename.toAscii().data());
                break;
            case 't':
                t_recordlen = atoi(optarg);
                VERBOSE_PRINTF("-t: Usergiven recordingtime detected, will record for %i microseconds\n",t_recordlen);
				break;
            case 'b':
                baudrate = atoi(optarg);
                VERBOSE_PRINTF("-b: Baudrate of serialport set to %i Baud\n",baudrate);
                break;
            case '?':
                printf("usage  of babel:\n");
                printf("\t-v \"boolean switch to be more verbose\"\n");
                printf("\t-n \"boolean switch to disable gui\"\n");
                printf("\t-f \"basename for logging/writing\"\n");
                printf("\t-p \"serial port\"\n");
                printf("\t-b \"baudrate\"\n");
                printf("\t-t \"recordingtime in microseconds\"\n");
                printf("\t-s \"send a special scaleCommand in the range of [0..24] via RS232 to filtersubsystem and exit\n");
                exit(EXIT_FAILURE);
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        printf("Non-supported cmdline arguments detected...\n");
        exit(EXIT_FAILURE);
    }

//-------------------
// hello world
//-------------------
	printf("\n\tWelcome to guibabel\n\n");
	VERBOSE_PRINTF("compiled on %s, at %s, running with threadID %i\n",__DATE__,__TIME__,(int)QThread::currentThreadId());

	if (start_gui) {
		gui w;
		w.setBaudrate(baudrate);
		w.setVerbosity(verbose_flag);
		if (!basename.isEmpty())
			w.setBasename(basename);
		if (newScale != -1)
			w.setScaleCommand(newScale);
		w.setRecordingtime(t_recordlen);
		w.show();
		return a.exec();
	} else {
		PCMdekoder* myDekoder;
		myDekoder = new PCMdekoder();
		myDekoder->Set_baudrate(baudrate);
		myDekoder->Set_portname(devicename);
		myDekoder->Set_verbosity(verbose_flag);
		myDekoder->init();
		myDekoder->start();//now, serial port is beein read

		myDekoder->drain = new sequenceRecorder();
		myDekoder->drain->setVerbosity(verbose_flag);

		if (!myDekoder->drain->open()){
			printf("Error opening recorder, can't record\n");
			delete myDekoder->drain;
			return EXIT_FAILURE;
		}
		myDekoder->start_recording(t_recordlen);
		while (myDekoder->is_recording) {
			usleep(100);
		}
		myDekoder->uninit();
		delete myDekoder;
		return EXIT_SUCCESS;
	}
}
