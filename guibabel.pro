######################################################################
# Automatically generated by qmake (2.01a) Mi. Mrz 31 13:51:28 2010
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

# Input
HEADERS += gui.h serialport.h CurvePlot.h sequenceRecorder.h
FORMS += gui.ui
SOURCES += gui.cpp main.cpp serialport.cpp CurvePlot.cpp sequenceRecorder.cpp

INCLUDEPATH += /usr/include/qwt-qt4 
DEPENDPATH += /usr/include/qwt-qt4 
LIBS += -lqwt-qt4 -lsndfile
