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
	trigger_update_bitwidth(14);// set bitwidth, there will
	qwtPlot_pcm->setCanvasBackground(Qt::white);
//	we won't add the curve here, this is done when we click the connect-button
	isDrawing = false;
	myTimer_pcmplot_refresh = new QTimer(this);
	connect(myTimer_pcmplot_refresh, SIGNAL(timeout()), this, SLOT(refresh_pcmplot()));
	myTimer_pcmplot_refresh->setInterval(1);
	connect(spinBox_bitwidth, SIGNAL(valueChanged(int)), this, SLOT(trigger_update_bitwidth(int)));

	// prepare serialport
	mySerialport = new serialport();

	// prepare sequenceRecorder
	connect(pushButton_start_sequence_recorder, SIGNAL(clicked()), this, SLOT(trigger_sequence_recorder_start()));
	connect(horizontalSlider_scalecommand, SIGNAL(sliderMoved(int)), this, SLOT(trigger_new_scale_command(int)));
	connect(pushButton_send_scale_command, SIGNAL(clicked()), this, SLOT(trigger_button_scale_command()));

	VERBOSE_PRINTF("finished construction of GUI\n");
}

gui::~gui(){
	VERBOSE_PRINTF("starting deletion of GUI\n");

	if (isDrawing)
		delete pcmplot;
	delete myTimer_pcmplot_refresh;
	delete mySerialport;

	VERBOSE_PRINTF("finished deletion of GUI\n");
}

void gui::trigger_button_scale_command(){
	VERBOSE_PRINTF("button for scalecommand was pressed\n");
	int val = lineEdit_scalecommand->text().toInt();
	horizontalSlider_scalecommand->setSliderPosition(val);
	trigger_new_scale_command(val);
}

void gui::trigger_new_scale_command(int val){
	VERBOSE_PRINTF("slider for scalecommand was moved, new value will be written\n");
	lineEdit_scalecommand->setText(QString::number(val));

	int8_t cmd = (int8_t)val;
	VERBOSE_PRINTF("writing new scale command \"%i\"\n",cmd);
	mySerialport->write((char*)&cmd, 1);
}

void gui::trigger_update_bitwidth(int bw) {

	qwtPlot_pcm->setAxisScale( QwtPlot::yLeft, -pow(2,bw-1), pow(2,bw-1)-1, 0);
}

void gui::trigger_sequence_recorder_start() {
	VERBOSE_PRINTF("starting sequence recorder\n");

	struct timeval t_start, t_seq, t_seq2;
	float t_now = 0;
	int t_end = spinBox_sequence_recorder_runtime->value();
	int val;


	if (lineEdit_datafile_basename->text() == "") {
		printf("please provide a logfilebasename, can't record\n");
		return;
	}

	if (lineEdit_datafile_basename->isEnabled()) {
		std::string basename = lineEdit_datafile_basename->text().toAscii().data();
		record1 = new sequenceRecorder(basename);
	} else {
		record1 = new sequenceRecorder();
	}
	VERBOSE_PRINTF("Basename for recording is %s\n",record1->getBasename().c_str());

	record1->setVerbosity(mVerboseLevel);

	if (!record1->open()){
		printf("Error opening recorder, can't record\n");
		delete record1;
		return;
	}

	VERBOSE_PRINTF("Recording started, length is %ims, Basename %s\n",t_end,record1->getBasename().c_str());

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
	VERBOSE_PRINTF("checkbox for disabling/enabling basename was pressed\n");
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
			pcmplot->addCurve("PCM S14", QwtPlot::yLeft, Qt::blue);
			gettimeofday(&t_begin, NULL);

			pushButton_start_sequence_recorder->setEnabled(true);
			pushButton_send_scale_command->setEnabled(true);
			spinBox_bitwidth->setEnabled(true);
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
		spinBox_bitwidth->setDisabled(true);
	}
}

void gui::refresh_pcmplot(){
	int value;
	int time_ms;
	mySerialport->flush();
	if (mySerialport->read_pcm(&value, 1) == 0) {
		printf("can't read data\n");
		myTimer_pcmplot_refresh->stop();
	}

	gettimeofday(&t_sequence, NULL);

	time_ms = (t_sequence.tv_sec - t_begin.tv_sec)*1000 + (t_sequence.tv_usec - t_begin.tv_usec)/1000;

	double mean = pcmplot->addPointToCurve("PCM S14", time_ms, value);

	char myString[80];
	sprintf(myString,"actual dataplot, mean: %-4.2f",mean);
	QString myTitle(myString);
	label_pcmplot->setText(myTitle);
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
	delete portname;
}
