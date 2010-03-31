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
	// setup quit-hook
	connect(actionQuit,SIGNAL(triggered()), qApp, SLOT(quit()));

	// setup display of available com-ports and connectionssignals
	connect(button_refresh_serialports, SIGNAL(clicked()), this, SLOT(refresh_serialports()));
	refresh_serialports();
	connect(button_connect_disconnect_serialport, SIGNAL(clicked()), this, SLOT(trigger_serialport()));
	connect(action_connect_disconnect_serialport, SIGNAL(triggered()), this, SLOT(trigger_serialport()));

	// setup some cosmetic values
	QString myTitle;
	QTextStream(&myTitle) << "guibabel, compiled since "<<__DATE__<<":"<<__TIME__;
	setWindowTitle(myTitle);

	// prepare basename input
	checkBox_basename->setCheckState( Qt::Checked );
	connect(checkBox_basename, SIGNAL(stateChanged(int)), this, SLOT(stateChanged_checkbox_basename(int)));
	lineEdit_datafile_basename->setText("babel_%Y-%m-%d_%H-%M-%S");

	// prepare plotting-section
	qwtPlot_pcm->setAxisScale( QwtPlot::yLeft, -8192, 8191, 0);
//	we won't add the curve here, this is done when we click the connect-button
//	pcmplot = new CurvePlot(this,qwtPlot_pcm, 2000);
//	pcmplot->addCurve("PCM S14", QwtPlot::yLeft, Qt::red);
	isDrawing = false;
	myTimer_pcmplot_refresh = new QTimer(this);
	connect(myTimer_pcmplot_refresh, SIGNAL(timeout()), this, SLOT(refresh_pcmplot()));
	myTimer_pcmplot_refresh->setInterval(1);

	// prepare serialport
	mySerialport = new serialport();
	
	// prepare sequenceRecorder
	connect(pushButton_start_sequence_recorder, SIGNAL(clicked()), this, SLOT(trigger_sequence_recorder_start()));
	connect(pushButton_send_scale_command, SIGNAL(clicked()), this, SLOT(trigger_send_scale_command()));
}
 
gui::~gui(){
	if (isDrawing)
		delete pcmplot;
	delete myTimer_pcmplot_refresh;
	delete mySerialport;
}

void gui::trigger_send_scale_command() {

		int8_t cmd = (int8_t)spinBox_scale_command->value();
		VERBOSE_PRINTF("writing new scale command \"%i\"\n",cmd);
		mySerialport->write((char*)&cmd, 1);
}

void gui::trigger_sequence_recorder_start() {
	struct timeval t_start, t_seq, t_seq2;
	float t_now = 0;
	int t_end = spinBox_sequence_recorder_runtime->value();
	int val;


	if (lineEdit_datafile_basename->text() == "") {
		printf("please provide a logfilebasename\n");
		return;
	}

	if (lineEdit_datafile_basename->isEnabled()) {
		std::string basename = lineEdit_datafile_basename->text().toAscii().data();
		record1 = new sequenceRecorder(basename);
	} else {
		record1 = new sequenceRecorder();
	}
	record1->setVerbosity(mVerboseLevel);

	if (!record1->open()){
		printf("fehler beim öffner\n");
		delete record1;
		return;
	}

	VERBOSE_PRINTF("recording started, length of %ims and basename of %s\n",t_end,record1->getBasename().c_str());

	isRecording = true;

	gettimeofday(&t_start, NULL);
	while ( t_now < t_end ) {
		mySerialport->read_pcm(&val,1);
		gettimeofday(&t_seq,NULL);
		t_seq2.tv_sec = t_seq.tv_sec - t_start.tv_sec;
		t_seq2.tv_usec = t_seq.tv_usec - t_start.tv_usec;
		t_now = t_seq2.tv_sec*1000 + t_seq2.tv_usec/1000.0;
		record1->pushPCMword(val,t_seq2);

	}
	VERBOSE_PRINTF("After %f microseconds, guibabel finished recording\n",t_now);

	record1->close();
	delete record1;
	isRecording = false;
}

void gui::stateChanged_checkbox_basename( int newstate ) {

	if ( newstate == Qt::Checked) {
		lineEdit_datafile_basename->setText("babel_%Y-%m-%d_%H-%M-%S");
		lineEdit_datafile_basename->setDisabled(true);

	} else {
		lineEdit_datafile_basename->clear();
		lineEdit_datafile_basename->setEnabled(true);
	}

}

void gui::trigger_serialport(){

	if (button_connect_disconnect_serialport->text() == "connect serialport") {

		std::string portname = comboBox_avail_serialports->currentText().toAscii().data();
		if (mySerialport->init(portname, 230400, mVerboseLevel)) {
			// success!
			button_connect_disconnect_serialport->setText("disconnect serialport");
			action_connect_disconnect_serialport->setText("disconnect serialport");
			comboBox_avail_serialports->setDisabled(true);
			button_refresh_serialports->setDisabled(true);
			checkBox_basename->setDisabled(true);
			myTimer_pcmplot_refresh->start();

			pcmplot = new CurvePlot(this,qwtPlot_pcm, 2000);
			isDrawing = true;
			pcmplot->addCurve("PCM S14", QwtPlot::yLeft, Qt::red);
			gettimeofday(&t_begin, NULL);

			pushButton_start_sequence_recorder->setEnabled(true);
			pushButton_send_scale_command->setEnabled(true);
		}

	} else {

		mySerialport->uninit();
		button_connect_disconnect_serialport->setText("connect serialport");
		action_connect_disconnect_serialport->setText("connect serialport");
		comboBox_avail_serialports->setEnabled(true);
		button_refresh_serialports->setEnabled(true);
		checkBox_basename->setEnabled(true);
		myTimer_pcmplot_refresh->stop();

		pcmplot->removeCurve("PCM S14");
		delete pcmplot;//attention, exiting while plot is drawing will result is memory error
		isDrawing = false;

		pushButton_start_sequence_recorder->setDisabled(true);
		pushButton_send_scale_command->setDisabled(true);
	}
}

void gui::refresh_pcmplot(){
	int value;
	int time_ms;
	mySerialport->flush();
	mySerialport->read_pcm(&value, 1);
	gettimeofday(&t_sequence, NULL);

	time_ms = (t_sequence.tv_sec - t_begin.tv_sec)*1000 + (t_sequence.tv_usec - t_begin.tv_usec)/1000;

	pcmplot->addPointToCurve("PCM S14", time_ms, value);
}

void gui::refresh_serialports(){
	// try to open every ttyUSB-device we can find and check if it's a serial device
	// if we ware successfull, add resulting portname to the list of a corresponding dropbox
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
	delete portname;
}
