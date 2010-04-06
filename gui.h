#ifndef GUI_H
#define GUI_H

#include "ui_gui.h"

#include "serialport.h"
#include "CurvePlot.h"
#include <QTimer>
#include <sys/time.h>
#include "sequenceRecorder.h"

class gui : public QMainWindow, public Ui::MainWindow{
	Q_OBJECT

	public:
		gui (QMainWindow *parent = 0);
		~gui();


	private:
		serialport* mySerialport;
		CurvePlot* pcmplot;
		bool isDrawing;
		bool isRecording;
		QTimer* myTimer_pcmplot_refresh;
		sequenceRecorder* record1;

		struct timeval t_begin;
		struct timeval t_sequence;

		int mVerboseLevel;

	private slots:
		void refresh_serialports();
		void trigger_serialport();
		void stateChanged_checkbox_basename(int newstate);
		void refresh_pcmplot();
		void trigger_sequence_recorder_start();
		void trigger_new_scale_command(int val);
		void trigger_button_scale_command();
};
#endif //GUI_H