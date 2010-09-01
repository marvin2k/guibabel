#include "gui.h"
#include <QApplication>
#include <sys/ioctl.h>
#include <linux/serial.h>
#include <fcntl.h>

#include <signal.h>

#include <QDebug>

#define VERBOSE_PRINTF(...) if (verbose_flag > 0) { \
                                printf("%s:%i: ",__FILE__,__LINE__);\
                                printf(__VA_ARGS__);\
                            }
int verbose_flag = 1;

void termination_handler (int signum);

PCMdekoder* myDekoder = NULL;

int main( int argc, char* argv[])
{
	signal (SIGINT, termination_handler);

	QApplication a(argc, argv);

	QString devicename("/dev/ttyUSB0");
	QString basename;
	QString jointId("seven");
	QString filterId("fir_massive");
	int pwm = 0;
	int record_sample_number = 16384;
	int baudrate = 230400;
	bool start_gui  = true;
//-------------------
// parsing of cmdline-options
//-------------------
    int c;
    while ((c = getopt (argc, argv, "nvt:f:p:b:m:j:r:")) != -1) {
    /* getopt... see http://www.gnu.org/s/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt */
        switch (c) {
            case 'm':
				pwm = atoi(optarg);
                VERBOSE_PRINTF("-m pwm-value set to %i\n",pwm);
                break;
            case 'r':
				filterId = optarg;
                VERBOSE_PRINTF("-r filterId set to %s\n",filterId.toAscii().data());
                break;
            case 'j':
				jointId = optarg;
                VERBOSE_PRINTF("-j: jointId set to %s\n",jointId.toAscii().data());
                break;
			case 'v':
                verbose_flag = 1;
                printf("-v: Will be verbose\n");
                break;
			case 'n':
				start_gui = false;
				VERBOSE_PRINTF("starting/showing gui disabled, cmd-line-only version\n");
				break;
            case 'p':
                VERBOSE_PRINTF("-p: Devicename given for serialport: %s !!!NOT SUPPORTED WITH GUI!!! trying to open...\n",optarg);
				struct serial_struct serinfo;
	            int fd;
	            fd = ::open(optarg, O_RDWR | O_NONBLOCK );
				if (fd >= 0) {
					if (ioctl (fd, TIOCSSERIAL, &serinfo)) {
					// serial device is a serial device, indeed
					// see http://stackoverflow.com/questions/2530096/linux-how-to-find-all-serial-devices-ttys-ttyusb-without-opening-them
					devicename = QString::fromAscii(optarg);
					VERBOSE_PRINTF("\t\tsucessfully opened serial port %s, given device is valid...\n",devicename.toAscii().data());
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
            case 'f':
				basename = optarg;
                VERBOSE_PRINTF("-f: Basename for Outputfiles: %s\n",basename.toAscii().data());
                break;
            case 't':
                record_sample_number = atoi(optarg);
                VERBOSE_PRINTF("-t: Usergiven number of samples to record detected, will record %i samples\n",record_sample_number);
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
                printf("\t-t \"recordinglength in samples\"\n");
                printf("\t-m \"set pwm-value of running motor\"\n");
                printf("\t-j \"set jointId of used motor\"\n");
                printf("\t-r \"set filterId of used motor\"\n");
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
		w.setRecordlength(record_sample_number);
		w.show();
		return a.exec();
	} else {
		myDekoder = new PCMdekoder();
		myDekoder->Set_baudrate(baudrate);
		myDekoder->Set_portname(devicename);
		myDekoder->Set_verbosity(verbose_flag);
		myDekoder->init();
		myDekoder->start();//now, serial port is beeing read

		myDekoder->drain = new sequenceRecorder();
		myDekoder->drain->setVerbosity(verbose_flag);
		myDekoder->drain->setpwmspeedtorque(pwm, -1, -1);
		myDekoder->drain->setfilterId( filterId );
		myDekoder->drain->setjointId( jointId );
		if (!basename.isEmpty())
			myDekoder->drain->setBasename(basename.toAscii().data());

		if (!myDekoder->drain->open()){
			printf("Error opening recorder, can't record\n");
			delete myDekoder->drain;
			return EXIT_FAILURE;
		}
		myDekoder->Set_sample_down_counter(record_sample_number);
		myDekoder->start_recording();
		while (myDekoder->is_recording) {
			usleep(100);
		}
		myDekoder->uninit();
		delete myDekoder;
		return EXIT_SUCCESS;
	}
}

void termination_handler (int signum) {

	VERBOSE_PRINTF("received signal %i, exiting\n",signum);

	if (myDekoder) {
		myDekoder->uninit();
		delete myDekoder;
	}

	_exit(0);
}
