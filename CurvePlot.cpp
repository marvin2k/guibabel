/*
 * CurvePlot.cpp
 *
 *  Created on: 24.01.2010
 *      Author: bartsch
 */

#include "CurvePlot.h"
#include "qwt_symbol.h"
#include <stdio.h>

CurvePlot::CurvePlot(QObject* parent, QwtPlot *plot, int period) {
	this->plot = plot;
	this->timePeriod = period;
	minX = 0;
	maxX = timePeriod-1;
	plot->setAxisScale(QwtPlot::xBottom, minX, maxX, timePeriod/5);
    legend.setItemMode(QwtLegend::CheckableItem);
	//plot->insertLegend(&legend, QwtPlot::BottomLegend);
	connect(&updateTimer, SIGNAL(timeout()), plot, SLOT(replot()));
	updateTimer.start(25);
}

CurvePlot::~CurvePlot() {
	// TODO Auto-generated destructor stub
}


void CurvePlot::setPlot(QwtPlot *plot) {
	this->plot = plot;
}

void CurvePlot::addCurve(QString name, int yAxis, const QColor & color) {
	struct curve *curve = new struct curve();
	curve->x = new double[timePeriod];
	curve->y = new double[timePeriod];
	for (int i = 0; i < timePeriod; i++) {
		curve->x[i] = minX+i;
		curve->y[i] = 0;
	}
	curve->plotCurve = new QwtPlotCurve(name);
	curve->plotCurve->setYAxis(yAxis);
	curve->plotCurve->setPen(QPen(color));
	curve->plotCurve->setRawData(curve->x, curve->y, timePeriod);
	curve->plotCurve->attach(plot);
	connect(plot, SIGNAL(legendChecked(QwtPlotItem*, bool)), this, SLOT(setCurveVisible(QwtPlotItem*, bool)));
	map.insert(name, curve);
}

void CurvePlot::removeCurve(QString name) {
	struct curve *curve = map[name];
	if (curve != NULL) {
		curve->plotCurve->detach();
		map.remove(name);
		delete(curve->plotCurve);
		delete(curve->x);
		delete(curve->y);
		delete(curve);
	}
}

void CurvePlot::setCurveVisible(QString name, bool visible) {
	struct curve *curve = map[name];
	if (curve != NULL) {
		((QwtLegendItem*)curve->plotCurve->legendItem())->setChecked(!visible);
		curve->plotCurve->updateLegend(&legend);
		//curve->plotCurve->setVisible(visible);
		qDebug("Setting Curve Visible");
	};
}

void CurvePlot::setCurveVisible(QwtPlotItem *plotItem, bool visible)  {
	plotItem->setVisible(!visible);
	qDebug("Updating PlotItem");
}

void CurvePlot::setTimePeriod(int period) {
	if (period == timePeriod) {
		return;
	}
	if (period < timePeriod) {
		//reduce size of all arrays
		timePeriod = period;
		QList<curve*> allCurves = map.values();
		QList<curve*>::iterator i;
		for (i = allCurves.begin(); i != allCurves.end(); i++) {
	       // *i->x =
	    }
	}
	else {
		//extend size aof all arrays

	}
}

double CurvePlot::addPointToCurve(QString name, int time, double value) {

	struct curve *curve = map[name];
	double mean = 0;

	if (curve != NULL) {
		if (time-minX > timePeriod) {
			maxX = time;
			minX = maxX - timePeriod;
		}
		if (curve->x[timePeriod-1] < maxX) {
			int lastX =  (int)curve->x[timePeriod-1];
			double lastY = curve->y[timePeriod-1];
			int offset = maxX - lastX;
			if(offset < timePeriod) {
				memmove(curve->x, curve->x+offset, (timePeriod-offset-1)*sizeof(double));
				memmove(curve->y, curve->y+offset, (timePeriod-offset-1)*sizeof(double));
				int start = timePeriod-offset-1;
				for(int i = start; i < timePeriod; i++) {
					curve->x[i] = lastX++;
					curve->y[i] = lastY + ((value-lastY)*((double)(i-start)/(double)(timePeriod-start)));//lastY;
//mean += curve->y[i];
				}
				curve->y[timePeriod-1] = value;
				curve->x[timePeriod-1] = time;
			}
			else {
				for(int i = 0; i < timePeriod; i++) {
					curve->x[i] = time-(timePeriod-i);
					curve->y[i] = lastY + ((value-lastY)*((double)(i-0)/(double)(timePeriod-0)));//lastY;
				}
				curve->y[timePeriod-1] = value;
			}
		}
		else {
			for (int i = 0; i < timePeriod; i++) {
				if (curve->x[i] >= time) {
					curve->y[i] = value;
				}
			}
		}
		plot->setAxisScale(QwtPlot::xBottom, minX, maxX, timePeriod/5);
		for (int i = 0;i<timePeriod;i++){
mean += curve->y[i];
		}
	}
	else {
		printf("\n Curve Unknown! \n");
		return 0;
	}
	return mean/timePeriod;
}
