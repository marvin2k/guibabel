#ifndef GUI_H
#define GUI_H

#include "ui_gui.h"

#include "serialport.h"
#include "PCMdekoder.h"
#include <QTimer>
#include "DLogger.h"
#include "DPlotter.h"
#include "Dfft.h"

class gui : public QMainWindow, public Ui::MainWindow{
	Q_OBJECT

	public:
		gui (QMainWindow *parent = 0);
		~gui();


	public slots:
		void setBaudrate(int newBaudrate);
		void setBasename(QString newBasename);
		void setVerbosity(int newVerbosity);
		void newData(const QString name, const double data);
	
	signals:

	private:
		PCMdekoder *myDekoder;
		DLogger *myLog;
		DPlotter *myPlot;
		Dfft *myFFT;

		QTimer guiTimer;

		int mVerboseLevel;

		int mBaudrate;

		QList<double> PCMmeasurement;

	private slots:
		void refresh_serialports();
		void trigger_serialport();
		void started_connection();
		void stopped_connection();
		void started_recording();
		void stopped_recording();
		void updateGui();
};
#endif //GUI_H
