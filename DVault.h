#ifndef DVAULT_H
#define DVAULT_H

#include <QTime>
//#include <QMutex>
#include <QDebug>

/**
 *
 * Central data-storage of logValues which are discarding over time
 *
 * An QObject, which has two QList<double> of an abritrary length. It stores one supplied
 * logValue together with the timeValue of the recording moment, in ms since programm execution.
 * New Data-Items are appended to their QList, aswell as the timeValues. Old data is discarded at the end, FiFo-like
 * 
 * The data is not directly accessible, only the newest values are supplied together with their age relative to the newest
 * one as an guarded memory region. This region is garanteed to remain until the next fresh snapshot is asked for. The data is
 * presented by to pointers and one Integer "len" residing in an struct, whose pointer is provided. No locking, new'ing or deleting
 * has to be done for this region, this QObject takes care.
 * 
 * The keepTime, the time-range of the presented data relative to the newest one, can be set by calling a slot
 * setKeepTime(double keepTime). By keeping the number of stored item hidden, we can maybe provide more functionality.
 * Looking back dynamically in the received data, or DFT's with larger nfft than displayed data. One can name this
 * feature a bug, since it's unefficient to crawl the data asynchronsly for resizing, which we crawl it anyway when
 * we provide the plotting data. But maybe the extra effort is worth it...
 * 
 * Data is fed in by calling the slot slurp(double). This is intended to be used by emitting this signal as data "falls on" 
 * or generated in your normal programm execution. Doing so allows asynchronous data storage for QwtPlotting
 * 
 * It has nice features, too:
 * - m_scale and m_offset (m_shift doesn't make sense?) can dynamically alter the data, settable via signal/slot connections
 * - has a slot cleanUp() -- which is triggered externally to drop too old data periodically from the QList.
 * - auto-cleanUp if internal data get twice as long as the keepTime
 * - QwtPlot can plot asynchronous events/measuremnts/datablocks of different length together in the same plot-window. This
 *   allows to show horizontal marker in realtime-plot-window
 * - We can supply some statistical data on demand (in the same way as we provide our dynamic data -- via shared memory)
 * - we can clear the stored data completely using a slot clear()
 *
 * For inspiration, see http://stackoverflow.com/questions/3848427/qt-best-way-to-implement-oscilloscope-like-realtime-plotting
 *
 * TODO:
 * - Think about a way of implementing an DFT...
 * - here are some timing-problem hidden, I think. Sometimes during testing, a Q_ASSERTS are hits...
 * - use qUpperBound/qLowerBound for searching in QList<double> timeValues
 * - think about implementing datastorage using a stl::deque instead of QList?
 * - statistic-values inside QMap, for more flexibility...
 * - implement setMaxSize(int); und int getMaxSize();
 *
 */

/* 40000@9000Hz=4.5s, 30000*sizeof(double)=320kB) */
#define MAX_ELEMENT_COUNT 40000




class DVault : public QTime
{

	public:
		DVault();
		virtual ~DVault();

		struct stats_t{
			double f_sample;
			double points;
			double mean;
			double CF;
			double rms;
			double max;
			double min;
			double head;
		};

		struct dataPtr_t{
			int len;
			double* dataT;
			double* dataY;
		};

		dataPtr_t* getDataPtr();
		dataPtr_t* getDataPtr(int len);
		stats_t* getStats();

		double getOffset(){return m_offset;};
		double getScale(){return m_scale;};

		void setKeepTime(double keepTime){m_keepTime = keepTime;};// keepTime is positive, and in ms!!!
		void setOffset(double offset){m_offset = offset;};
		void setScale(double scale){m_scale = scale;};
		void setDynamixXAxis(bool dynAxis = true){m_dynAxis = dynAxis;};

		void slurp(double);	// for inputting of new data

		void clear();		// removing all elements

	protected:

	private:
	
		// this mutex is intented for future use, in a mutithreaded environment
//		QMutex dataMutex;

		dataPtr_t m_dynamicMemoryStruct;
		stats_t m_statistics;

		QList<double> m_logValues;
		QList<double> m_timeValues;

		double m_keepTime;

		double m_scale;
		double m_offset;
		
		bool m_dynAxis;

};

#endif // DVAULT_H
