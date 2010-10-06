#ifndef PCMDEKODER_H
#define PCMDEKODER_H

#include <QtCore>
#include <QThread>
#include <QString>

#include <QDebug>
#include <QString>

#include "serialport.h"

class PCMdekoder : public QThread
{
	Q_OBJECT

	public:
		PCMdekoder();
		~PCMdekoder();

		void run();
		bool init();
		void uninit();

		int Get_lastValue() { return m_lastValue; }
		QString Get_portname() { return m_portname; }
		int Get_verbosity() {return m_verboselevel; }
		int Get_baudrate() {return m_baudrate; }

		void Set_portname(QString name) { m_portname = name; }
		void Set_verbosity(int val) { m_verboselevel = val; }
		void Set_baudrate(int val) { m_baudrate = val; }

		struct DekoderStatus_t {
			int validPCMwords;
			int invalidPCMwords;
		};

		DekoderStatus_t* Get_Status() {return &m_status; }

	signals:
		void newData(const QString, const double);

	protected:

	private:

		serialport* source;
		QString m_portname;

		bool stop_running;

		DekoderStatus_t m_status;

		int m_lastValue;
		int m_verboselevel;
		int m_baudrate;

};

#endif // PCMDEKODER_H
