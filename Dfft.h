#ifndef DFFT_H
#define DFFT_H

#include <QObject>
#include <QWidget>
#include <QVector>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QTimer>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include <complex.h> 
#include <fftw3.h>
#include "ui_Dfft.h"

/**

 Little helper class to have the calcualation handy

 */
class DcalcThread : public QThread
{
	Q_OBJECT

	public:
		DcalcThread(QObject *parent = 0);
		virtual ~DcalcThread();

		void run();

		// used to synchronize the datatransfer. FIXME: there seems to exist solutions for an "in the place"-mode in fftw -- this reduces the mutexes to one... and saves some memory
		QMutex sharedIn;
		QMutex sharedOut;

	public slots:

		void setDimension(int nfft);
		void fftStart(double *data, int nfft);

	signals:
		void fftReady(double *freq, int size);

	protected:

	private:
		double *in;
		fftw_complex *out;
		fftw_plan p;

		int m_nfft;

		double* m_resultFFT;

	private slots:

};

/**

 Realtime fft on some data.
 
 Works rather crufty, not very efficient... more or less one big working hack ;-)
 
 TODO:
 - In place transformation? real2real-mode in fftw?

 */
#define MAX_FFT_SIZE 40000
class Dfft : public QGroupBox, private Ui::GroupBox_Dfft 
{
	Q_OBJECT

	public:
		Dfft(QGroupBox *parent = 0);
		virtual ~Dfft();

		struct stats_t{
			double n_overlap;
			double f_peak;
		};


	public slots:
		//void addFFTname(QString name, QColor col);// only one FFT allowed, in the moment at least
		void addLogValue(double val);
		void setDimension(int nfft);
		void setSampleTime(double f_sample);
		void updateGui();
		void fftReady(double* data, int size);
		void changeFfft(int);
		void handle_nfft(QString);
		
	signals:
		void fftStart(double *fftData, int m_nfft);
		void nfftChanged(int);
		
	protected:

	private:
		DcalcThread* myHelper;
		QwtPlotCurve fftCurve;

		QTimer guiTimer;

		stats_t m_statistics;

		double m_f_sample;
		int m_nfft;
		
		double* m_windowData;
		double* m_freqData;
		double* m_fftData;

		QList<double> m_logValues;

	private slots:
		void updateWindowCoefficients(QString name);

};

#endif // DFFT_H
