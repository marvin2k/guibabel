#ifndef DCSVVAULT_H
#define DCSVVAULT_H

#include <QDebug>
#include <QList>
#include <QString>
#include <QFile>
#include <QTextStream>

/**
 * Super-Simple, just a Csv-List. Nothing special, but everything encapsulated
 */
class DCsvVault
{
	public:
		DCsvVault(QString filename, QList<QString> &logValues);
		virtual ~DCsvVault();

		friend DCsvVault& operator<<(DCsvVault& cont,const QList<double> values){
			cont.AddLogValue(values);
			return cont;
		}

		int isOpen();

		int size();
		int rows();
		int cols();

	protected:

	private:

		void AddLogValue(const QList<double> values);

		QString filename;
		QFile logFile;
		QTextStream logStream;

		int m_nrOfCols;
		int m_nrOfRows;
};

#endif // DCSVVAULT_H
