#include "gui.h"

#include <sys/ioctl.h>
#include <linux/serial.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <QString>
#include <QTextStream>

/* prints filename and linenumber in addition to given fprintf-fmt */
#define VERBOSE_PRINTF(...) if (mVerboseLevel > 0) { \
                                printf("%s:%i: ",__FILE__,__LINE__);\
                                printf(__VA_ARGS__);\
                            }

gui::gui(QMainWindow *parent) : QMainWindow(parent){
	setupUi(this);
	
	mVerboseLevel = 1;
	VERBOSE_PRINTF("starting construction of GUI\n");
	
	QString PCM = QString("PCM");
	
	myLog = new DLogger(widget_logger, QString("guibabel"));
	myLog->addColumn(&PCM);
	
	myPlot = new DPlotter(widget_plotter);
	myPlot->addCurve(PCM,Qt::blue,true);
	myPlot->change_keepTime(-1.5);
	
	// setup display of available com-ports and connectionssignals
	connect(button_refresh_serialports, SIGNAL(clicked()), this, SLOT(refresh_serialports()));
	refresh_serialports();
	
	connect(button_connect_disconnect_serialport, SIGNAL(clicked()), this, SLOT(trigger_serialport()));
	
	connect(myLog, SIGNAL(loggingStarted()), this, SLOT(started_recording()));
	connect(myLog, SIGNAL(loggingStopped()), this, SLOT(stopped_recording()));

	connect(&guiTimer, SIGNAL(timeout()), this, SLOT(updateGui()));

	// setup some cosmetic values
	QString myTitle;
	QTextStream(&myTitle) << "guibabel, compiled since "<<__DATE__<<":"<<__TIME__;
	setWindowTitle(myTitle);

	// prepare properties section:
	cbx_filterID->addItem("fir_massive");
	cbx_filterID->addItem("fir_three_staged");
	cbx_filterID->addItem("laufsumme_filter2");
	cbx_filterID->addItem("cic_compensated");
	cbx_filterID->addItem("cic_pure");
	cbx_filterID->addItem("other");

	cbx_jointID->addItem("seven");
	cbx_jointID->addItem("YanYue");
	cbx_jointID->addItem("other");

	guiTimer.setInterval(1000);

	// just some dummy value...
	PCMmeasurement.append(0);

	VERBOSE_PRINTF("finished construction of GUI\n");
}

gui::~gui(){
	if (myDekoder->isRunning()) {
		myDekoder->uninit();
		delete myDekoder;
	}
	delete myLog;
	delete myPlot;
}

// setting of baudrate in gui
void gui::setBaudrate(int newBaudrate) {
	mBaudrate = newBaudrate;
}

void gui::setBasename(QString name) {
	myLog->setLogfileBaseName(name);
}

void gui::setVerbosity(int newVerbosity){
	mVerboseLevel = newVerbosity;
}

void gui::newData(const QString name, const double data){
	myPlot->addPlotValue(name, data);
	PCMmeasurement.replace(0,data);
	myLog->AddLogValues(&PCMmeasurement);
}

void gui::started_connection(){
	button_connect_disconnect_serialport->setText("disconnect serialport");
	comboBox_avail_serialports->setDisabled(true);
	button_refresh_serialports->setDisabled(true);
	cbx_jointID->setDisabled(true);
	cbx_filterID->setDisabled(true);
}

void gui::stopped_connection(){
	button_connect_disconnect_serialport->setText("connect serialport");
	comboBox_avail_serialports->setEnabled(true);
	button_refresh_serialports->setEnabled(true);
	cbx_jointID->setEnabled(true);
	cbx_filterID->setEnabled(true);
}	

void gui::started_recording(){
	le_pwm->setDisabled(true);
	le_speed->setDisabled(true);
	le_torque->setDisabled(true);
}
void gui::stopped_recording(){
	le_pwm->setEnabled(true);
	le_speed->setEnabled(true);
	le_torque->setEnabled(true);

	lab_valid->setText("---");
	lab_invalid->setText("---");
}

void gui::trigger_serialport(){

	if (button_connect_disconnect_serialport->text() == "connect serialport") {
		VERBOSE_PRINTF("connecting serialport, starting workerthread and displaying graph\n");

		myDekoder = new PCMdekoder();

		myDekoder->Set_portname(comboBox_avail_serialports->currentText());
		myDekoder->Set_verbosity(mVerboseLevel);
		myDekoder->Set_baudrate(mBaudrate);

		if (myDekoder->init()){

			connect(myDekoder, SIGNAL(newData(const QString, const double)), this, SLOT(newData(const QString, const double)));
			connect(myDekoder, SIGNAL(started()), this, SLOT(started_connection()));
			connect(myDekoder, SIGNAL(started()), &guiTimer, SLOT(start()));
			connect(myDekoder, SIGNAL(finished()), this, SLOT(stopped_connection()));
			connect(myDekoder, SIGNAL(finished()), &guiTimer, SLOT(stop()));

			// success! aaaaand GO!
			myDekoder->start();

		} else {
			qDebug() << "gui: Something wrong while starting decoder";
			delete myDekoder;
		}

	} else {

		myDekoder->uninit();

		disconnect(myDekoder, SIGNAL(newData(const QString, const double)), this, SLOT(newData(const QString, const double)));
		disconnect(myDekoder, SIGNAL(started()), this, SLOT(started_connection()));
		disconnect(myDekoder, SIGNAL(finished()), this, SLOT(stopped_connection()));
		
		delete myDekoder;

		updateGui();
	}
}

void gui::updateGui(){

	struct PCMdekoder::DekoderStatus_t *status = myDekoder->Get_Status();

	lab_valid->setNum(status->validPCMwords);
	lab_invalid->setNum(status->invalidPCMwords);
}

void gui::refresh_serialports(){
	// try to open every ttyUSB-device we can find and check if it's a serial device
	// if we ware successfull, add resulting portname to the list of a corresponding dropbox
	VERBOSE_PRINTF("refreshing list ov available serialports\n");
	int i, fd;
	char* portname = new char[80];
	struct serial_struct serinfo;

	comboBox_avail_serialports->clear();

	for (i=0;i<20;i++){
		sprintf(portname,"/dev/ttyUSB%i",i);

		fd = ::open(portname, O_RDWR | O_NONBLOCK );
		if (fd >= 0) {
			if (ioctl (fd, TIOCSSERIAL, &serinfo)) {
			// serial device is a serial device, indeed
			// see http://stackoverflow.com/questions/2530096/linux-how-to-find-all-serial-devices-ttys-ttyusb-without-opening-them
				comboBox_avail_serialports->addItem(portname);
			}
			::close(fd);
		}
	}
	delete[] portname;
}
