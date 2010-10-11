#include "Dfft.h"

#include <QDebug>
#include <QTime>

#include <math.h>

/**

 We need:


 */

Dfft::Dfft(QGroupBox *parent) : QGroupBox(parent)
{
	setupUi(this);

	m_windowData = new double[MAX_FFT_SIZE];
	m_freqData = new double[(MAX_FFT_SIZE+1)/2];
	m_fftData = new double[(MAX_FFT_SIZE+1)/2];

	m_f_sample = 100;
	m_nfft = 64;

	fftPlot->setAxisScale(QwtPlot::yLeft, -80, 0, 0);
	fftPlot->setCanvasBackground(Qt::white);
	fftPlot->setAxisTitle(QwtPlot::xBottom, QwtText("f in Hz",QwtText::PlainText) );
	fftPlot->setAxisTitle(QwtPlot::yLeft, QwtText("amp in dB",QwtText::PlainText) );

	fftCurve.setStyle(QwtPlotCurve::Lines);
	fftCurve.attach(fftPlot);

	// set the guiTimer indirectly
	spinBox_f_fft->setValue(20);
	connect(spinBox_f_fft, SIGNAL(valueChanged(int)), this, SLOT(changeFfft(int)));
	emit changeFfft(20);
//	guiTimer.start(50);
	connect(&guiTimer, SIGNAL(timeout()), this, SLOT(updateGui()));

	myHelper = new DcalcThread;
	myHelper->start();
	
	connect(this, SIGNAL(fftStart(double*, int)), myHelper, SLOT(fftStart(double*, int)));
	connect(myHelper, SIGNAL(fftReady(double*, int)),this,SLOT(fftReady(double*, int)));	

	comboBox_window->insertItem(comboBox_window->maxCount(),"hann");
	comboBox_window->insertItem(comboBox_window->maxCount(),"hamming");
	comboBox_window->insertItem(comboBox_window->maxCount(),"blackman-harris");
	comboBox_window->insertItem(comboBox_window->maxCount(),"gaussian");
	comboBox_window->insertItem(comboBox_window->maxCount(),"rectangular");
	
	connect(comboBox_window, SIGNAL(currentIndexChanged(QString)), this, SLOT(updateWindowCoefficients(QString)));

	comboBox_nfft->insertItem(comboBox_nfft->maxCount(),"20000");
	comboBox_nfft->insertItem(comboBox_nfft->maxCount(),"16384");
	comboBox_nfft->insertItem(comboBox_nfft->maxCount(),"8192");
	comboBox_nfft->insertItem(comboBox_nfft->maxCount(),"4096");
	comboBox_nfft->insertItem(comboBox_nfft->maxCount(),"2048");
	comboBox_nfft->insertItem(comboBox_nfft->maxCount(),"1024");
	comboBox_nfft->insertItem(comboBox_nfft->maxCount(),"512");

	// we set the nfft as a string in a combox, this need translation to a number
	connect(comboBox_nfft, SIGNAL(currentIndexChanged(QString)), this, SLOT(handle_nfft(QString)));
	connect(this, SIGNAL(nfftChanged(int)), this, SLOT(setDimension(int)));
	connect(this, SIGNAL(nfftChanged(int)), myHelper, SLOT(setDimension(int)));
}

Dfft::~Dfft()
{
	myHelper->quit();
	while (myHelper->isRunning()){usleep(100);}
	delete myHelper;
	
	delete[] m_windowData;
	delete[] m_freqData;
	delete[] m_fftData;
}

// here, QString contains hopefully the number "nfft", the size of the fft
void Dfft::handle_nfft(QString value)
{
	int nfft = value.toInt();
	emit nfftChanged(nfft);
}

// using QList again for storing of raw data. Due to windowing, the data has to be moved anyways into the fft-array, so it doesn't matter how it's stored
void Dfft::addLogValue(double val)
{
	m_logValues.append(val);
	if (m_logValues.size() > MAX_FFT_SIZE)
		m_logValues.removeFirst();
}

// calling setSampleTime to update the x-axis data
void Dfft::setDimension(int nfft)
{
	if (comboBox_nfft->findText(QString::number(nfft))==-1) {
		comboBox_nfft->insertItem(comboBox_nfft->maxCount(),QString::number(nfft));
	} else {
		comboBox_nfft->setCurrentIndex(comboBox_nfft->findText(QString::number(nfft)));
	}

	Q_ASSERT(m_nfft<MAX_FFT_SIZE);
	
	if (m_nfft != nfft){
		m_nfft = nfft;
		updateWindowCoefficients(comboBox_window->currentText());

		// update the freqData-array.
		setSampleTime(m_f_sample);
		
		while (m_logValues.size() < m_nfft)
			addLogValue(0);
	}
}

void Dfft::setSampleTime(double f_sample)
{
	qDebug() << "Dfft: setting sampleTime to"<<f_sample;
	m_f_sample = f_sample;
	fftPlot->setAxisScale(QwtPlot::xBottom, 0, m_f_sample/2,0);

	for (int i = 0; i < (m_nfft+1)/2; i++)
	{
		m_freqData[i] = i*m_f_sample/m_nfft;
	}
}

// triggering the helper thread. updating takes place in fftReady
void Dfft::updateGui()
{
	myHelper->sharedIn.lock();
		int len = m_logValues.size();
		for (int i = 0; i < m_nfft; i++)
		{
			// the data lays upside down in m_logValues
			// TODO: move window-functionality to subthread
			m_fftData[i] = m_logValues.at(len-i-1)*m_windowData[i];
		}
	myHelper->sharedIn.unlock();
		
	emit fftStart(m_fftData, m_nfft);
}

void Dfft::fftReady(double* fft, int size){
	Q_ASSERT( size = (m_nfft+1)/2);

	myHelper->sharedOut.lock();
		fftCurve.setRawData(m_freqData, fft, size);
		fftPlot->replot();
	
		// some statistics:
		double f_peak = 0;
		double a_peak = -100000;// serious...
		for (int i = 4; i < size; i++)// starting at 4 to skip the always-high-DC-values
		{
			if (a_peak < fft[i])
			{
				a_peak = fft[i];
				f_peak = m_freqData[i];
			}
		}
	myHelper->sharedOut.unlock();
	

	label_f_peak->setText(QString::number((int)f_peak)+"Hz");
	double fft_length = (1.0/m_f_sample)*size;
	label_n_over->setText(QString::number((fft_length - guiTimer.interval()/1000.0)*100*fft_length,'f',2)+"%");
}

void Dfft::updateWindowCoefficients(QString name)
{
	qDebug() << "Dfft: updating window to"<<name;
	if (name == "rectangular") {
		for (int i = 0; i < m_nfft; i++)
			m_windowData[i] = 1.0;
	} else if (name == "hann") {
		for (int i = 0; i < m_nfft; i++)
			m_windowData[i] = 0.5*(1-cos((2*M_PI*i)/(m_nfft-1)));
	} else if (name == "hamming") {
		for (int i = 0; i < m_nfft; i++)
			m_windowData[i] = 0.54 - 0.46*cos((2*M_PI*i)/(m_nfft-1));
	} else if (name == "blackman-harris") {
		for (int i = 0; i < m_nfft; i++)
			m_windowData[i] = 0.35875 - 0.48829*cos((2*M_PI*i)/(m_nfft-1)) + 0.14128*cos((4*M_PI*i)/(m_nfft-1)) - 0.01168*cos((6*M_PI*i)/(m_nfft-1));
	} else if (name == "gaussian") {
		for (int i = 0; i < m_nfft; i++)
			m_windowData[i] = exp(-0.5*(((i-(m_nfft-1)/2)/(0.4*(m_nfft-1)/2))*((i-(m_nfft-1)/2)/(0.4*(m_nfft-1)/2))));
	} else {
		qDebug() << "Dfft: the window"<<name<<" is not supported";
	}
}

void Dfft::changeFfft(int freq)
{
	guiTimer.start(1.0/freq*1000.0);
}

/**

 Little helper class to have the calcualation handy
 
 There is nothing to see, go away!

 */
DcalcThread::DcalcThread(QObject *parent) : QThread(parent)
{
	m_nfft = 1024;

	m_resultFFT = new double[(MAX_FFT_SIZE+1)/2];

	in = (double*) fftw_malloc(sizeof(double) * m_nfft);
	out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (m_nfft+1)/2);

	p = fftw_plan_dft_r2c_1d(m_nfft, in, out, FFTW_ESTIMATE);	
}

DcalcThread::~DcalcThread()
{
	fftw_destroy_plan(p);

	fftw_free(in);
	fftw_free(out);

	delete[] m_resultFFT;
}

void DcalcThread::run()
{
	exec();
}

void DcalcThread::setDimension(int nfft)
{
	if (m_nfft != nfft) {
		m_nfft = nfft;
		qDebug() << "DcalcThread: setting size to"<<nfft<<"-- this will overwrite transfer-memory";
//		QTime stopWatch;
//		stopWatch.start();
		fftw_free(in);
		fftw_free(out);

		in = (double*) fftw_malloc(sizeof(double) * m_nfft);
		out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * m_nfft);//not (nfft+1)/2???

		// FIXME: this may not ber the best coice, there exits an r2r-function out there... but this works, so...
		p = fftw_plan_dft_r2c_1d(m_nfft, in, out, FFTW_ESTIMATE);
//		qDebug() << "DcalcThread: finished setting size. took"<<stopWatch.elapsed()<<"ms";
	}
}

void DcalcThread::fftStart(double *mem, int nfft)
{
	if (nfft != m_nfft)
		setDimension(nfft);
	
//	QTime stopWatch;
//	stopWatch.start();
//	qDebug() << "DcalcThread: starting fft";

	sharedIn.lock();
		// TODO: implement window function here. currently, this is done in Dfft
		memmove((void*)in, (void*)mem, nfft*sizeof(double));
	sharedIn.unlock();
	
	fftw_execute(p);
	
	double max = 0;
	sharedOut.lock();		
		for (int i = 0; i < m_nfft/2+1; i++)
			max = qMax(max,cabs(out[i]));// inefficient... 

		for (int i = 0; i < m_nfft/2+1; i++)
		{
			m_resultFFT[i] = -20*log10(max/cabs(out[i]));
		}
	sharedOut.unlock();		

//	qDebug() << "DcalcThread: done fft. took"<<stopWatch.elapsed()<<"ms";
	
	emit fftReady(m_resultFFT, m_nfft/2+1 );
}