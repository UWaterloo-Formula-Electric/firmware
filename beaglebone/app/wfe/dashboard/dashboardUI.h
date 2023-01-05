#ifndef DASHBOARDUI_H
#define DASHBOARDUI_H

#include <vector>
#include <memory>
#include <QtWidgets>
#include <QApplication>
#include "displays/textdisplay.cpp"
#include "displays/errordisplay.cpp"
#include "displays/dial.cpp"
#include "queuedata.cpp"

class WfeDashboardUI : public QWidget {
    private:
        // If the corresponding properties are greater than these values,
        // their respective dials display in red
        const int OVERSPEED = 200;
        const int OVERTEMP = 40;
        const int OVERVOLTAGE = 30;

        const int WIDTH = 800;
        const int HEIGHT = 480;

        // Radii of big and small dials
        const int RSMALL = 200;
        const int RBIG = 320;

        //std::shared_ptr<QTimer> timer;

        // Display instances
        std::shared_ptr<TextDisplay> modeDisplay;
        std::shared_ptr<TextDisplay> batteryDisplay;
        std::shared_ptr<TextDisplay> speedDisplay;
        std::shared_ptr<TextDisplay> tempDisplay;
        std::shared_ptr<TextDisplay> voltageDisplay;
        std::shared_ptr<ErrorDisplay> errorDisplay;

        // Dial instances
        std::shared_ptr<Dial> speedDial;
        std::shared_ptr<Dial> tempDial;
        std::shared_ptr<Dial> voltageDial;

        // Data (updating in separate thread)
        std::shared_ptr<QueueData> data;

        void updateErrorDisplay();
        std::string toXDecimalString(double doubleVal, int precisionVal);


    public:
        WfeDashboardUI(QApplication& app, std::shared_ptr<QueueData> data);
        ~WfeDashboardUI();
        void paintEvent(QPaintEvent *);
        void update();


};

#endif