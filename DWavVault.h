#ifndef DWavVault_H
#define DWavVault_H

#include <QDebug>
#include <QString>

#include <sndfile.h>					//ubuntu 9.10: "apt-get intall libsndfile-dev"
/**
 * Super-Simple, just an encapsulated wav-file. Nothing special, but everything encapsulated
 */
class DWavVault
{
	public:
		DWavVault(QString filename);
		virtual ~DWavVault();

		friend DWavVault& operator<<(DWavVault& cont,const double value){
			cont.AddLogValue(value);
			return cont;
		}

		int isOpen();

		int size();
		int rows();
		int cols();

	protected:

	private:

		void AddLogValue(const double value);

		QString filename;
		SNDFILE* logFile;

		int m_nrOfRows;
};

#endif // DWavVault_H
