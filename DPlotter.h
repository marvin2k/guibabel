#ifndef DPLOTTER_H
#define DPLOTTER_H

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include <QColor>
#include <QTimer>

#include <ui_DPlotter.h>

#include "DVault.h"

/**
 * Class to handle Gui functionality of DPlotter. Like adding/removing curves
 *
 * This Class provides plotting functionality in one window. It stores all data for every curve inside 
 * a custom class, DVault, it stores and processes the data and shoudl be controlled via signals from 
 * here (TODO: implement his right). Some Statistic is also generated.
 *
 * Periodically, the Gui-Elemets are updated, the plot area too, the timing is done by a private timer.
 * In real-world the Gui might be updated using a public signal which is provided.
 *
 * TODO:
 * - check mutex's for neccessarity or efficiency
 * - implement "take snapshot"-function -- stream the data of all plot-values (plus status information from plotting) to an
 *   DLogger (with all_default and no Gui elements). This is not trivial to make it right, since all logvalues
 *   have their own timeindex, which may differ over time. But logging need aligned logValues...
 * - add additional log-cols for snapshotting, like "isActive"
 * - clean-up everything, this code in ugly...
 * - here are some timing-problem hidden, I think. Sometimes during testing, a Q_ASSERTS are hits...
 *
 */

class DPlotter : public QGroupBox, private Ui::GroupBox_DPlotter {
	Q_OBJECT

	public:
		DPlotter(QGroupBox *parent = 0);
		virtual ~DPlotter();

		struct curve_t{
			DVault *dataVault;
			QwtPlotCurve *dataCurve;
		};

		void addPlotValues(const QHash<QString, double>* dataHash ){
			QHashIterator<QString, double> i(*dataHash);
			while (i.hasNext()) {
				i.next();
				addPlotValue(i.key(), i.value());
			}
		};
		void addCurve( const QString, const QColor, bool vis = false );
		void addCurves( QHash<QString, QColor>* );

		void setCurveVisible( const QString, const bool );

	public slots:
		void addPlotValue(const QString name, double val);
		void updateGui();
		void clearCurves();
		void change_keepTime(const double keepTime_s);
		void setMarker();
		void removeCurve( const QString name );

	signals:
		void prepare_updateGui(const QString, const bool xAxis);
		void doAddCurve(const QString, const QColor);
		void doRemoveCurve(const QString);
		void doCurveVisibility(const QString, QwtPlot*, const bool);

	protected:
	private:

		QButtonGroup *buttonGroup_scaleYSelect;
		QButtonGroup *buttonGroup_scaleXSelect;
		
		void setComboBoxColor(QString name, QColor color);

		// used for actualizing the gui
		QTimer guiTimer;

		QHash<QString,curve_t*> allCurves;
		
	private slots:
		void selectCurve(QString name);
		void selectColor();
		void handle_curveVisibility();
		void handle_plotYScale(int id);
		void handle_plotXScale(int id);
		void handle_manualPlotScale(double value);
		void handle_curveOffset(double offset);
		void handle_curveScale(double scale);
};

#endif // DPLOTTER_H
