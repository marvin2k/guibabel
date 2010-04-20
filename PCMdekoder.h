#ifndef PCMDEKODER_H
#define PCMDEKODER_H

#include <QThread>
#include <QString>

#include "serialport.h"
#include "sequenceRecorder.h"

class PCMdekoder : public QThread
{
	public:
		PCMdekoder();
		virtual ~PCMdekoder();

		void run();
		bool init();
		void uninit();

		bool start_recording();

		void send_command( char* data, int len );

		int Get_recordingTime() { return m_recordingTime; }
		int Get_lastValue() { return m_lastValue; }
		QString Get_portname() { return m_portname; }
		int Get_verbosity() {return m_verboselevel; }
		int Get_baudrate() {return m_baudrate; }

		void Set_recordingTime(int val) { m_recordingTime = val; }
		void Set_lastValue(int val) { m_lastValue = val; }
		void Set_portname(QString name) { m_portname = name; }
		void Set_verbosity(int val) { m_verboselevel = val; }
		void Set_baudrate(int val) { m_baudrate = val; }
		void Set_sample_down_counter(int val) { m_sample_down_counter = val; }

		bool stop_running;
		bool is_recording;

		sequenceRecorder* drain;

		struct DekoderStatus_t {
			int validPCMwords;
			int invalidPCMwords;
			int recordedPCMwords;
		};

		DekoderStatus_t Get_Status() {return m_status; }

	protected:

	private:

		DekoderStatus_t m_status;

		int m_recordingTime;
		int m_lastValue;
		QString m_portname;
		int m_verboselevel;
		int m_baudrate;

		int m_sample_down_counter;

		int m_invalid;

		serialport* source;

};

#endif // PCMDEKODER_H
