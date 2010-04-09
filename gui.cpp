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
	mDefaultBasename = "babel_%Y-%m-%d_%H-%M-%S";
	connect(checkBox_basename, SIGNAL(stateChanged(int)), this, SLOT(stateChanged_checkbox_basename(int)));
	setBasename(mDefaultBasename);

	// prepare plotting-section
	trigger_update_bitwidth(14);// set bitwidth, there will
	qwtPlot_pcm->setCanvasBackground(Qt::white);
//	we won't add the curve here, this is done when we click the connect-button
	isDrawing = false;
	myTimer_pcmplot_refresh = new QTimer(this);
	connect(myTimer_pcmplot_refresh, SIGNAL(timeout()), this, SLOT(refresh_pcmplot()));
	myTimer_pcmplot_refresh->setInterval(1);
	connect(spinBox_bitwidth, SIGNAL(valueChanged(int)), this, SLOT(trigger_update_bitwidth(int)));


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

	while (myDekoder->isRunning()) {
		VERBOSE_PRINTF("waiting for data-thread to exit\n");
		myDekoder->stop_running = true;
		usleep(1);
	}
	delete myDekoder;

	VERBOSE_PRINTF("finished deletion of GUI\n");
}

// handling of enabling/disablin the propriate boxes, according to the basename
void gui::setBasename(QString newBasename){
	if (newBasename == mDefaultBasename){
		lineEdit_datafile_basename->setDisabled(true);
		checkBox_basename->setCheckState(Qt::Checked);
	} else {
		lineEdit_datafile_basename->setEnabled(true);
		checkBox_basename->setCheckState(Qt::Unchecked);
	}
	lineEdit_datafile_basename->setText(newBasename);
}

// setting of baudrate in gui
void gui::setBaudrate(int newBaudrate) {
	mBaudrate = newBaudrate;
}

// set all gui element to a new scale command
void gui::setScaleCommand(int newScale){
	if (newScale < horizontalSlider_scalecommand->minimum()) {
		VERBOSE_PRINTF("new scale command not supported, too low\n");
		return;
	}
	if (newScale > horizontalSlider_scalecommand->maximum()) {
		VERBOSE_PRINTF("new scale command not supported, too high\n");
		return;
	}
	lineEdit_scalecommand->setText(QString::number(newScale));
	horizontalSlider_scalecommand->setSliderPosition(newScale);
}

void gui::setVerbosity(int newVerbosity){
	mVerboseLevel = newVerbosity;
}

void gui::setRecordingtime(int rectime){
	if (rectime <= 0) {
		VERBOSE_PRINTF("negative or zero rec-time doesn't make sense\n");
		return;
	} else {
		spinBox_sequence_recorder_runtime->setValue(rectime);
	}
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
	myDekoder->source->write((char*)&cmd, 1);
}

void gui::trigger_update_bitwidth(int bw) {

	qwtPlot_pcm->setAxisScale( QwtPlot::yLeft, -pow(2,bw-1), pow(2,bw-1)-1, 0);
}

void gui::trigger_sequence_recorder_start() {
	VERBOSE_PRINTF("starting sequence recorder\n");

	int t_end = spinBox_sequence_recorder_runtime->value();


	if (lineEdit_datafile_basename->text() == "") {
		printf("please provide a logfilebasename, can't record\n");
		return;
	}

	if (lineEdit_datafile_basename->isEnabled()) {
		std::string basename = lineEdit_datafile_basename->text().toAscii().data();
		myDekoder->drain = new sequenceRecorder(basename);
	} else {
		myDekoder->drain = new sequenceRecorder();
	}
	VERBOSE_PRINTF("Basename for recording is %s\n",myDekoder->drain->getBasename().c_str());

	myDekoder->drain->setVerbosity(mVerboseLevel);

	if (!myDekoder->drain->open()){
		printf("Error opening recorder, can't record\n");
		delete myDekoder->drain;
		return;
	}

	VERBOSE_PRINTF("Recording started, length is %ims, Basename %s\n",t_end,myDekoder->drain->getBasename().c_str());

	myDekoder->m_recordingTime = spinBox_sequence_recorder_runtime->value();
	myDekoder->is_recording = true;

}

void gui::stateChanged_checkbox_basename( int newstate ) {
	VERBOSE_PRINTF("checkbox for disabling/enabling basename was pressed\n");
	if ( newstate == Qt::Checked) {
		lineEdit_datafile_basename->setText(mDefaultBasename);
		lineEdit_datafile_basename->setDisabled(true);

	} else {
		lineEdit_datafile_basename->clear();
		lineEdit_datafile_basename->setEnabled(true);
	}

}

void gui::trigger_serialport(){

	if (button_connect_disconnect_serialport->text() == "connect serialport") {
		VERBOSE_PRINTF("connecting serialport, starting workerthread and displaying grph\n");

		myDekoder = new PCMdekoder();

		myDekoder->Set_portname(comboBox_avail_serialports->currentText());
		myDekoder->Set_verbosity(mVerboseLevel);
		myDekoder->Set_baudrate(mBaudrate);
		myDekoder->Set_recordingTime(-1);//run indefinetly
		if (myDekoder->init()){
			// success! aaaaand GO!
			myDekoder->start();

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

		myDekoder->uninit();
		delete myDekoder;

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

	value = myDekoder->Get_lastValue();

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
