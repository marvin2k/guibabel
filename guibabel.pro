######################################################################
# Automatically generated by qmake (2.01a) Mi. Mrz 31 13:51:28 2010
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += . /home/zenzes/olelo.wiki/DFKI/DLib/lib
INCLUDEPATH += . /home/zenzes/olelo.wiki/DFKI/DLib/lib
CONFIG += warn_on debug 

QT += network

# Input
HEADERS += gui.h serialport.h PCMdekoder.h
FORMS += gui.ui
SOURCES += gui.cpp main.cpp serialport.cpp PCMdekoder.cpp

INCLUDEPATH += /usr/include/qwt-qt4 
DEPENDPATH += /usr/include/qwt-qt4 
LIBS += -lqwt-qt4 -lsndfile -lDLib -L/home/zenzes/olelo.wiki/DFKI/DLib/lib -lfftw3 -lm

#extra clean-target to get rid of resulting plotfiles
#noch nciht so richtig ausgereift
mytarget.target = logfilecleanclean
mytarget.commands = rm -f *.{wav,mat}
mytarget.depends = clean
QMAKE_EXTRA_TARGETS += mytarget
