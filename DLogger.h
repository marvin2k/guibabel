#ifndef DLOGGER_H
#define DLOGGER_H

#include <QtCore>
#include <QGroupBox>
#include <QTextStream>
#include <QString>
#include <QTimer>
#include <QTime>

#include <ui_DLogger.h>

#include "DCsvVault.h"
#include "DOctaveVault.h"
#include "DWavVault.h"

/**
 * Logging functionality for various DFKI Guis... Or other Guis... Or whatever else this may be usefull for...
 *
 * Can be included in a Wigdet which is residing in your Gui. It should have at least 491x171 pixel and
 * may be called "widget_DLogger".
 *
 * For basic instruction in usage see test-Class "TestLogger". In a nutshell:
 * - give wigdet in constructor, to initilize Gui-Elements and so
 * - for autogenerated filename, you can provide a prefix in the constuctor
 * - create a QList<QString> of the name of every column you want to log and put it into the logger
 * - start logging by calling/emitting triggerRecording()
 * - put every value you may want to log into an QList<double> of the same ĺength as there are logValues, and move
 *   it with addLogValues line by line into you logging class...
 
 * do this _always_, don't care about when to log (or trigger it whith the signal/slots), this is handled inside
 * this very subclass! Don't care about timestamps and marker-values (or trigger markers yourself). Don't care
 * about updating this part of the Gui... This is done automatically. Or trigger updateGui() from outside, 
 * but then _please_ disable the timer in this class
 *
 * There are some Signals, Slots and Methods to make usage easier, see Header-Definition
 *
 * Currently, *.csv- and Octave's *.mat-Files are supported.
 * Only Csv-Files are generated by default, but this can be easily modified in the constructor of this very class.
 *
 * It is currently maybe supported to change the number/names of the logged columns while no logging is
 * in progress... but be carefull, this is not tested and will result in "damaged" log-files
 *
 * TODO:
 * - catch SIG_INT to write footer of Octave-Files correctly, see http://doc.qt.nokia.com/4.6/unix-signals.html
 * - Implement unions in Octave-File to have the Names of the Data-Columns at hand (or something similar, like extra vector with strings)
 * - implement us-time-resoulution für _UNIX
 * - select nr of rows to record (recordLength), via slot/Gui
 *
 */
class DLogger : public QGroupBox, private Ui::GroupBox_DLogger {
	Q_OBJECT

	public:
		DLogger(QGroupBox *parent = 0, QString basename ="log");
		virtual ~DLogger();

		void addLogValues(const QList<double>*);

		void addColumn( const QString* );// Adding the columns of a logfile, either one by one or all together
		void addColumns( const QList<QString>* );

		QString getFileName();//if autochoose activated, will return filename (generated by a datastring), which will never be used...
		void setLogfileBaseName(QString baseName);//setting filename explicitly means no autogeneration. its disabled here!

		bool isRecording;

	public slots:
		void triggerMarker(double payload = 0);
		bool startLogging(QString basename);
		bool startLogging();
		bool stopLogging();
		void updateGui();

	signals:

		void loggingStarted(void);
		void loggingStopped(void);

	protected:

	private:

		// FIXME: remove if triggerd externally
		QTimer guiTimer;

		// The two supported fileformats
		DCsvVault *csvVault;
		DOctaveVault *octaveVault;
		DWavVault *wavVault;

		QList<QString> m_logNames;
		QString m_baseName;

		QTime m_sampleTime;
		int m_marker;

	private slots:
		void trigger_recording();
		void trigger_autochooseFilename(int);
};

#endif // DLOGGER_H