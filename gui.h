#ifndef GUI_H
#define GUI_H

#include "ui_gui.h"

#include "serialport.h"
#include "CurvePlot.h"
#include "PCMdekoder.h"
#include <QTimer>
#include <sys/time.h>
#include "sequenceRecorder.h"

class gui : public QMainWindow, public Ui::MainWindow{
	Q_OBJECT


	public:
		gui (QMainWindow *parent = 0);
		~gui();

		void setBaudrate(int newBaudrate);
		void setBasename(QString newBasename);
		void setVerbosity(int newVerbosity);
		void setRecordlength(int newLength );

	private:
		CurvePlot* pcmplot;
		PCMdekoder* myDekoder;
		bool isDrawing;
		bool isRecording;
		QTimer* myTimer_pcmplot_refresh;
		sequenceRecorder* record1;

		struct timeval t_begin;
		struct timeval t_sequence;

		int mVerboseLevel;

		QString mDefaultBasename;
		QString mCurrentBasename;
		int mBaudrate;
		int mRecordingtime;

	private slots:
		void refresh_serialports();
		void trigger_serialport();
		void stateChanged_checkbox_basename(int newstate);
		void refresh_pcmplot();
		void trigger_sequence_recorder();
		void trigger_update_bitwidth(int bw);
		void sequence_recording_finished();
		void rB_samples_handler();
		void rB_dauer_handler();
};
#endif //GUI_H
