#include "DPlotter.h"

#include <QString>
#include <QColorDialog>
#include <QPixmap>
#include <QIcon>
#include <QDebug>

#include <qwt_scale_div.h>

DPlotter::DPlotter(QGroupBox *parent) : QGroupBox(parent) {
	setupUi(this);

	guiTimer.start(50);

	qwtPlot->setCanvasBackground(Qt::white);
	qwtPlot->setAxisTitle(QwtPlot::xBottom, QwtText("t in Sekunden",QwtText::PlainText) );
	qwtPlot->setAxisScale(QwtPlot::xBottom, -doubleSpinBox_keepSeconds->value()*1000,0,0);

	buttonGroup_scaleYSelect = new QButtonGroup();
	buttonGroup_scaleYSelect->addButton(radioButton_autoPlotYScale);
	buttonGroup_scaleYSelect->setId(radioButton_autoPlotYScale,1);
	buttonGroup_scaleYSelect->addButton(radioButton_manualPlotYScale);
	buttonGroup_scaleYSelect->setId(radioButton_manualPlotYScale,2);
	radioButton_manualPlotYScale->setChecked(true);
	handle_plotYScale(buttonGroup_scaleYSelect->checkedId());

	buttonGroup_scaleXSelect = new QButtonGroup();
	buttonGroup_scaleXSelect->addButton(radioButton_realPlotXScale);
	buttonGroup_scaleXSelect->setId(radioButton_realPlotXScale,1);
	buttonGroup_scaleXSelect->addButton(radioButton_dynPlotXScale);
	buttonGroup_scaleXSelect->setId(radioButton_dynPlotXScale,2);
	radioButton_dynPlotXScale->setChecked(true);
	handle_plotXScale(buttonGroup_scaleXSelect->checkedId());

	connect(&guiTimer, SIGNAL(timeout()), this, SLOT(updateGui()));

	connect(doubleSpinBox_keepSeconds, SIGNAL(valueChanged(double)), this, SLOT(change_keepTime(double)));
	connect(checkBox_showCurve, SIGNAL(clicked()), this, SLOT(handle_curveVisibility()));
	connect(comboBox_curveNames, SIGNAL(currentIndexChanged(QString)), this, SLOT(selectCurve(QString)));
	connect(toolButton_selectColor, SIGNAL(clicked()), this, SLOT(selectColor()));
	connect(buttonGroup_scaleYSelect, SIGNAL(buttonClicked(int)), this, SLOT(handle_plotYScale(int)));
	connect(buttonGroup_scaleXSelect, SIGNAL(buttonClicked(int)), this, SLOT(handle_plotXScale(int)));
	connect(doubleSpinBox_minPlotScale, SIGNAL(valueChanged(double)), this, SLOT(handle_manualPlotScale(double)));
	connect(doubleSpinBox_maxPlotScale, SIGNAL(valueChanged(double)), this, SLOT(handle_manualPlotScale(double)));
	connect(doubleSpinBox_curveOffset, SIGNAL(valueChanged(double)), this, SLOT(handle_curveOffset(double)));
	connect(doubleSpinBox_curveScale, SIGNAL(valueChanged(double)), this, SLOT(handle_curveScale(double)));
	connect(pushButton_clearPlot, SIGNAL(clicked()), this, SLOT(clearCurves()));
	connect(pushButton_mark, SIGNAL(clicked()), this, SLOT(setMarker()));
}

DPlotter::~DPlotter() {
	delete buttonGroup_scaleYSelect;
	delete buttonGroup_scaleXSelect;

	QHashIterator<QString, DPlotter::curve_t*> i(allCurves);
	while (i.hasNext()) {
		i.next();
		removeCurve(i.key());
	}
}


/**
 * Adding and removing curves:
 */
void DPlotter::addCurves( QHash<QString, QColor> *curves){
	QHashIterator<QString, QColor> i(*curves);
	while (i.hasNext()) {
		i.next();
		addCurve(i.key(), i.value());
	}
}
void DPlotter::addCurve( const QString name, const QColor color, bool vis){

	// add curve to Hash:
	if (!allCurves.contains(name)){
		qDebug() << "DPlotter: Adding curve"<<name;

		DPlotter::curve_t* newCurve = new DPlotter::curve_t;

		newCurve->dataCurve = new QwtPlotCurve(name);
		newCurve->dataCurve->setStyle(QwtPlotCurve::Lines);
		//newCurve->dataCurve->setRenderHint(QwtPlotItem::RenderAntialiased);// nice looking! but slow...
		newCurve->dataCurve->setPen(QPen(color));

		newCurve->dataVault = new DVault;

		allCurves.insert(name,newCurve);

		// update gui-elements
		comboBox_curveNames->insertItem(comboBox_curveNames->maxCount(),name);// insert at end of the dropbox
		setComboBoxColor(name,color);
		
		if (vis)
			setCurveVisible(name,vis);
	}
}

void DPlotter::removeCurve( const QString name ){

	// remove curve from QHash:
	if (allCurves.contains(name)){
		qDebug() << "DPlotter: removing curve"<<name;

		allCurves.value(name)->dataCurve->detach();
		delete allCurves.value(name)->dataCurve;
		delete allCurves.value(name)->dataVault;
		delete allCurves.value(name);

		allCurves.remove(name);

		// update gui-elements
		comboBox_curveNames->removeItem(comboBox_curveNames->findText(name));
	}
}

void DPlotter::addPlotValue(const QString name, double val){
	allCurves.value(name)->dataVault->slurp(val);
}

// will also delete the marker curves...
void DPlotter::clearCurves(){
	QHashIterator<QString, DPlotter::curve_t*> i(allCurves);
	while (i.hasNext()) {
		i.next();
		i.value()->dataVault->clear();
		if (i.key() == "marker")
			removeCurve("marker");
	}
}

void DPlotter::updateGui(){

	DVault::dataPtr_t* ptr;// intermediate structure to save pointers and length information
	QHashIterator<QString, DPlotter::curve_t*> i(allCurves);
	while (i.hasNext()) {
		i.next();
		// first, get the informations assembled in the Vault, see description of this class
		ptr = i.value()->dataVault->getDataPtr();
		// then feed this data into the Curve
		i.value()->dataCurve->setRawData(ptr->dataT, ptr->dataY, ptr->len);
	}
	// at the end, update the plot
	qwtPlot->replot();

	DVault::stats_t *stats = allCurves.value(comboBox_curveNames->currentText())->dataVault->getStats();
	label_fsample->setText(QString::number((int)stats->f_sample)+"Hz");
	label_mean->setText(QString::number(stats->mean,'f',2));
	label_max->setText(QString::number(stats->max,'f',2));
	label_min->setText(QString::number(stats->min,'f',2));
	label_points->setText(QString::number(stats->points));
	label_head->setText(QString::number(stats->head,'f',2));
	label_CF->setText(QString::number(stats->CF,'f',2));
	label_rms->setText(QString::number(stats->rms,'f',2));

	// dirty hack...
	if (ptr->len >= 1024){
		ptr = allCurves.value(comboBox_curveNames->currentText())->dataVault->getDataPtr(1024);
		stats = allCurves.value(comboBox_curveNames->currentText())->dataVault->getStats();
	} else {
		ptr = allCurves.value(comboBox_curveNames->currentText())->dataVault->getDataPtr(ptr->len);
		stats = allCurves.value(comboBox_curveNames->currentText())->dataVault->getStats();
	}
}

void DPlotter::setMarker()
{
	// add new temporary curve and slurp two points in it. it will be deleted automatically, of its run empty
	QString name = QString("marker");
	addCurve( name, Qt::red, true);
	allCurves.value(name)->dataCurve->setStyle(QwtPlotCurve::Sticks);
	addPlotValue(name, (qwtPlot->axisScaleDiv(QwtPlot::yLeft))->upperBound());
	addPlotValue(name, (qwtPlot->axisScaleDiv(QwtPlot::yLeft))->lowerBound());

	allCurves.value("marker")->dataVault->setDynamixXAxis(false);
}

/**
 * Gui-Functionality:
 */
void DPlotter::selectCurve(QString name){
	if (!name.isEmpty()){// the last item, before we exit, when all others are deleted, is empty...
		checkBox_showCurve->setChecked(allCurves.value(name)->dataCurve->plot());//plot returns true is attached, false if not
		doubleSpinBox_curveOffset->setValue(allCurves.value(name)->dataVault->getOffset());
		doubleSpinBox_curveScale->setValue(allCurves.value(name)->dataVault->getScale());
	}
}

void DPlotter::setComboBoxColor(QString name, QColor color){
	// create the nice color-block in the dropbown menu
	QIcon myIcon;
	QPixmap myPix(10,10);

	myPix.fill(color);
	myIcon.addPixmap(myPix);

	int id = comboBox_curveNames->findText(name);
	comboBox_curveNames->setItemIcon(id, myIcon);
}

void DPlotter::selectColor(){
	QString name = comboBox_curveNames->currentText();
	// create an extra window to ask the user, therefore we need to know the current color of the actual curve
	QColor color = QColorDialog::getColor( allCurves.value(name)->dataCurve->pen().color() );
	setComboBoxColor(name,color);
	// set curve to color:
	allCurves.value(name)->dataCurve->setPen(QPen(color));
}

void DPlotter::handle_curveVisibility(){
	QString name = comboBox_curveNames->currentText();

	if (checkBox_showCurve->isChecked()) {
		allCurves.value(name)->dataCurve->attach(qwtPlot);
	} else {
		allCurves.value(name)->dataCurve->detach();
	}
}
//wrapperfunction to be able to trigger the above function from outside
void DPlotter::setCurveVisible(const QString name, const bool vis){
	if (vis)
		allCurves.value(name)->dataCurve->attach(qwtPlot);
	else
		allCurves.value(name)->dataCurve->detach();
	
	if (name == comboBox_curveNames->currentText())//???
		checkBox_showCurve->setChecked(true);
}

void DPlotter::change_keepTime(const double keepTime_s){
	doubleSpinBox_keepSeconds->setValue(keepTime_s);
	qwtPlot->setAxisScale(QwtPlot::xBottom, -keepTime_s*1000,0,0);

	// Set new keeptime in all curves:
	QHashIterator<QString, DPlotter::curve_t*> i(allCurves);
	while (i.hasNext()) {
		i.next();
		i.value()->dataVault->setKeepTime(keepTime_s*1000.0);
	}
	
}
void DPlotter::handle_curveOffset(double offset){
	QString name = comboBox_curveNames->currentText();
	allCurves.value(name)->dataVault->setOffset(offset);
}

void DPlotter::handle_curveScale(double scale){
	QString name = comboBox_curveNames->currentText();
	allCurves.value(name)->dataVault->setScale(scale);
}

void DPlotter::handle_manualPlotScale(double value){
	value++;//ignore the given value, read both needed freshly from the widget
	if (buttonGroup_scaleYSelect->checkedId() == 2)
		qwtPlot->setAxisScale(QwtPlot::yLeft, doubleSpinBox_minPlotScale->value(), doubleSpinBox_maxPlotScale->value(),0);
}

void DPlotter::handle_plotYScale(int id){
	if (id == 1) {//eg "auto"
		qwtPlot->setAxisAutoScale(QwtPlot::yLeft);
	} else {
		qwtPlot->setAxisScale(QwtPlot::yLeft, doubleSpinBox_minPlotScale->value(), doubleSpinBox_maxPlotScale->value(),0);
	}
}

void DPlotter::handle_plotXScale(int id){
	if (comboBox_curveNames->currentText() != QString("")) {//this is the case during contruction of widget, where this handler is triggered
		if (id == 1) {//eg "real"
			qDebug() << "DPlotter: disabling dynamic x-axis for curve"<<comboBox_curveNames->currentText();
			allCurves.value(comboBox_curveNames->currentText())->dataVault->setDynamixXAxis(false);
		} else if (id == 2) {
			qDebug() << "DPlotter: enabling dynamic x-axis for curve"<<comboBox_curveNames->currentText();
			allCurves.value(comboBox_curveNames->currentText())->dataVault->setDynamixXAxis(true);
		} else {
			qDebug() << "DPlotter: can't find id"<<id<<"in buttonGroup plotXscale";
		}
	}
}
