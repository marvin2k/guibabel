######################################################################
# Automatically generated by qmake (2.01a) Mi. Mrz 31 13:51:28 2010
######################################################################

TEMPLATE = app
TARGET = guibabel
DEPENDPATH += 
INCLUDEPATH +=
CONFIG += warn_on debug 

QT += network

# Input
HEADERS += gui.h serialport.h PCMdekoder.h DCsvVault.h  Dfft.h  DLogger.h  DOctaveVault.h  DPlotter.h  DVault.h DWavVault.h 
FORMS += gui.ui Dfft.ui  DLogger.ui  DPlotter.ui
SOURCES += gui.cpp main.cpp serialport.cpp PCMdekoder.cpp DCsvVault.cpp  Dfft.cpp  DLogger.cpp  DOctaveVault.cpp  DPlotter.cpp  DVault.cpp DWavVault.cpp

INCLUDEPATH += /usr/include/qwt-qt4 
DEPENDPATH += /usr/include/qwt-qt4 
LIBS += -lqwt-qt4 -lsndfile -lfftw3 -lm

#extra clean-target to get rid of resulting plotfiles
#noch nciht so richtig ausgereift
mytarget.target = logfilecleanclean
mytarget.commands = rm -f *.{wav,mat}
mytarget.depends = clean
QMAKE_EXTRA_TARGETS += mytarget
