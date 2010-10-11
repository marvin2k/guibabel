#include "DVault.h"

#include <QtCore>
#include <QDebug>


DVault::DVault()
{
	// what if different DVaults have different satrt()-times? Then, with the same m_keepTime and the same data, different results are posted...?
	// this is an relative value, which is hidden to the outside. The m_keepTime is substracted from this, only the difference is used to distinguish too old data
	// hence, different relative times in different QObjects still deliever the same result.
	this->QTime::start();

	setKeepTime(15000);//positive and in ms
	setOffset(0);
	setScale(1);

	m_dynAxis = true;

	m_statistics.head = 0;
	m_statistics.f_sample = 0;
	
	m_dynamicMemoryStruct.dataY = new double[MAX_ELEMENT_COUNT];
	m_dynamicMemoryStruct.dataT = new double[MAX_ELEMENT_COUNT];
}

DVault::~DVault()
{
	delete[] m_dynamicMemoryStruct.dataY;
	delete[] m_dynamicMemoryStruct.dataT;
}

// preprocessing of stored data, to be ready for plotting
DVault::dataPtr_t* DVault::getDataPtr()
{

/**
	Problem: By saving the age of a logValue as an relative number to Object age we have an
	expensive operation when deciding where the boundary lies. Every Item beginning from
	the start (or end?) has to be looked up... This comes from the "feature" to discard the data asynchronously

	We could use qUpperBound() instead, which performs a binary search

	Nevertheless, it is better to make this decision here, than every time new data is fed into the module -- like it was done before

	So we use two QList<double> which are synchronize loosely internally...
	
	Here is an example, how the memroy is laid out: Some old values were already discarded, the QList's begin at 400.
	The Value 5 (first one) was taken 400ms initializing of this Class, the actual timeVal in this Class is 1100,
	the m_keeptime is set to 400.
	
	m_logValues:       400 500 600 700 800 900 1000
	m_timeogValues:      5	-1	 4	21	 6	 5	 -4
	elapsedTime = 1100
	m_keepTime = 400
	
	This leads to the conclusion that we only have to output the values which are newer than 400ms before "now", 
	thus taken after, or having a bigger timeValue than, (1100-400)=700ms
	
*/
	// find out, how much data should be put out: 
	double elapsedTime = this->QTime::elapsed();
	int i;
	for (i = 0; i < m_timeValues.size(); i++)
	{
		if (elapsedTime-m_timeValues.at(i) <= m_keepTime*1.1)// the oldest timeVal we accept
			break;//the timevalues are growing monotonic...
	}
	int beg = i;
	int len = m_timeValues.size()-beg;
	//qDebug() << "DVault: acceptable timeval"<<m_timeValues.at(beg)-elapsedTime<<"at"<<beg<<"of"<<m_timeValues.size()<<". Putting"<<len<<"values";
	return getDataPtr( len );
}
	
DVault::dataPtr_t* DVault::getDataPtr(int len)
{
	double elapsedTime = this->QTime::elapsed();
	int beg = m_timeValues.size()-len;

	Q_ASSERT(beg+len == m_timeValues.size());
	Q_ASSERT(m_logValues.size() == m_timeValues.size());

	/**
		begin from the end of the array, and write downwards... at first data:
	 */
	for (int i = len-1; i >= 0; i-- )
	{
		m_dynamicMemoryStruct.dataY[i] = (m_scale*m_logValues.at(i+beg))+m_offset;
	}
	/**
		Now the time columns, special feature: dynamic scaling, it timeMarks are inaccurate due to buffering at high samplerate:
	 */
	if (!m_dynAxis) {
		for (int i = len-1; i >= 0; i-- )
		{
			m_dynamicMemoryStruct.dataT[i] = m_timeValues.at(i+beg) - elapsedTime;
		}
		
	} else {
		// preparing for dynamix-x-axis. not very clean solution to do it here, the same calculationi s done for getStats()
		double sum = 0;
		for (int i = 0;i < m_timeValues.size()-1; i++)
			sum += (m_timeValues.at(i+1) - m_timeValues.at(i));
		double t_sample = sum/(m_timeValues.size()-1);

		if (len>0)
			m_dynamicMemoryStruct.dataT[len-1] = -t_sample;
		for (int i = len-2; i >= 0; i-- )
		{
			m_dynamicMemoryStruct.dataT[i] = m_dynamicMemoryStruct.dataT[i+1] - t_sample;
		}
	}

	m_dynamicMemoryStruct.len = len;

	return &m_dynamicMemoryStruct;
}

// deletin everything
void DVault::clear()
{
	Q_ASSERT( m_timeValues.size() == m_logValues.size() );

	if (!m_timeValues.isEmpty()){
		//dataMutex.lock();
			m_timeValues.clear();
			m_logValues.clear();
		//dataMutex.unlock();
	}

	Q_ASSERT( m_timeValues.size() == 0);
	Q_ASSERT( m_logValues.size() == 0);
}

// Some statistics. Results are stored in here, only a pointer is supplied
DVault::stats_t* DVault::getStats(){

	double sum = 0;
	double f_sample = 0;
	double beg = m_timeValues.size()-m_dynamicMemoryStruct.len;
	double len = m_dynamicMemoryStruct.len;
	double up = m_timeValues.size();

	//dataMutex.lock();
		for (int i = beg;i < up-1; i++)
			sum += (m_timeValues.at(i+1) - m_timeValues.at(i));

		f_sample = 1.0/(sum/(1000.0*(len-1)));/*shiver*/
		if (f_sample < 0)//this happens, if len=0, so for empty log-vals
			f_sample = 0;
	//dataMutex.unlock();

	m_statistics.f_sample = f_sample;
	m_statistics.points = m_dynamicMemoryStruct.len;

	double mean = 0;
	double rms = 0;
	double min = 1/0.0; 
	double max = -1/0.0;

	//dataMutex.lock();
		for (int i = beg;i < up; i++){
			mean += m_logValues.at(i);
			rms += m_logValues.at(i)*m_logValues.at(i);
			min = qMin(m_logValues.at(i),min);
			max = qMax(m_logValues.at(i),max);
		}
		mean = mean/len;
		rms = qSqrt(rms/len);
	//dataMutex.unlock();

	m_statistics.mean = mean;
	m_statistics.min = min;
	m_statistics.max = max;
	m_statistics.rms = rms;
	m_statistics.CF = qMax(abs(min),abs(max))/rms;

	// calculation of std has to wait until we know the mean... I don't think this "connection"
	// needs to be secured by a mutex... the size won't change so often so radical. In singlethreaded not at all
	double std = 0;

	//dataMutex.lock();
		for (int i = beg;i < up; i++){
			std += (m_logValues.at(i) - mean)*(m_logValues.at(i) - mean);
		}
		std = sqrt(std/len);
	//dataMutex.unlock();
	
	if (!m_logValues.isEmpty())
		m_statistics.head = m_logValues.last();

	return &m_statistics;
}

// read in new data
void DVault::slurp(double val)
{
	//dataMutex.lock();
		m_logValues.append(val);
		m_timeValues.append(this->elapsed());

		if (m_timeValues.size() >= MAX_ELEMENT_COUNT){
			m_logValues.removeFirst();
			m_timeValues.removeFirst();
		}
		
		Q_ASSERT(m_timeValues.size() == m_logValues.size());
		Q_ASSERT(m_timeValues.size() < MAX_ELEMENT_COUNT);
	//dataMutex.unlock();
}
