#ifndef DVAULTOCTAVE_H
#define DVAULTOCTAVE_H

#include <QList>
#include <QString>
#include <QFile>
#include <QTextStream>

/**
 * Cool Feature. Write Octave-Files in correct syntax to provide some extra informations after logging.
 */
class DOctaveVault{
	
	public:
		DOctaveVault(QString filename, QList<QString> &logValues);
		virtual ~DOctaveVault();

		friend DOctaveVault& operator<<(DOctaveVault& cont,const QList<double> values){
			cont.AddLogValue(values);
			return cont;
		}

		int isOpen();

		int size();
		int rows();
		int cols();

	protected:

	private:

		void AddLogValue(const QList<double> logdata);

		QString filename;
		QFile logFile;
		QTextStream logStream;

		int m_nrOfCols;
		int m_nrOfRows;
		
		// extra-cool: allow editing of lines after they have been written. normally, files are just a stream of data with no understanding of a "line"
		qint64 m_replaceRowNrPosition;
		qint64 m_replaceColNrPosition;
		
		void writeHeader();
		void finishMatrixToOctaveFile();

		void addValueToOctaveFile( QString name, QString data);
};

#endif // DVAULTOCTAVE_H
