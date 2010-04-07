/*
 * CurvePlot.h
 *
 *  Created on: 24.01.2010
 *      Author: bartsch
 */

#ifndef CURVEPLOT_H_
#define CURVEPLOT_H_

#include<QObject>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_legend.h>
#include <qwt_legend_item.h>
#include <QMap>
#include <QList>
#include <QTimer>
#include "string.h"


struct curve {
	QwtPlotCurve *plotCurve;
	double *x;
	double *y;
};

class CurvePlot : public QObject
{
	Q_OBJECT

public:

	CurvePlot(QObject* parent = 0, QwtPlot *plot = 0, int period = 1000);
	virtual ~CurvePlot();

	void setPlot(QwtPlot *plot);
	void addCurve(QString name, int yAxis, const QColor & color);
	void removeCurve(QString name);
	void setCurveVisible(QString name, bool visible);

	void setTimePeriod(int period);

	double addPointToCurve(QString name, int time, double value);

private:
	QwtPlot *plot;
	QwtLegend legend;

	int timePeriod;
	int minX;
	int maxX;

	QMap<QString, curve*> map;

	QTimer updateTimer;

public slots:
	void setCurveVisible(QwtPlotItem *plotItem, bool visible);

};

#endif /* CURVEPLOT_H_ */
