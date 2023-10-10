#include "dashboardUI.h"
#include <iostream>

WfeDashboardUI::WfeDashboardUI(QApplication& app, std::shared_ptr<QueueData> data) {
    this->data = data;

    // Set window title
    this->setWindowTitle("WFE Dashboard");

    // Set window size
    this->setGeometry(10, 10, WIDTH, HEIGHT);

    // Set window position to center of screen
    QRect qtRectangle = this->frameGeometry();
    QPoint centerPoint = app.primaryScreen()->availableGeometry().center();
    qtRectangle.moveCenter(centerPoint);
    this->move(qtRectangle.topLeft());

    // Set background to black
    QPalette pal = this->palette();
    pal.setColor(QPalette::Background, Qt::black);
    this->setAutoFillBackground(true);
    this->setPalette(pal);

    // Initialize display instances
    modeDisplay = std::make_shared<TextDisplay>(this, "Mode: Norm", Qt::white, Qt::AlignLeft);
    batteryDisplay = std::make_shared<TextDisplay>(this, "Battery: 100%", Qt::green, Qt::AlignRight);
    speedDisplay = std::make_shared<TextDisplay>(this, "Speed: 0 kph", QColor(44, 197, 239), Qt::AlignHCenter);
    tempDisplay = std::make_shared<TextDisplay>(this, "Temp: N/A", QColor(255, 204, 0), Qt::AlignLeft, 22, 45);
    voltageDisplay = std::make_shared<TextDisplay>(this, "Voltage: 0 V", QColor(181, 124, 255), Qt::AlignRight, 22, 45);
    errorDisplay = std::make_shared<ErrorDisplay>(this, Qt::AlignBottom);

    // Initialize dial instances
    speedDial = std::make_shared<Dial>(this, QColor(44, 197, 239), Qt::red, WIDTH/2 - RBIG/2, 100, RBIG,
                                       20, 0, 280, OVERSPEED, 210, -240, "kph", 0);

    tempDial = std::make_shared<Dial>(this, QColor(255, 204, 0), Qt::red, 90, 200, RSMALL, 15, -20,
                                      80, OVERTEMP, 210, -140, "°C", -20);

    voltageDial = std::make_shared<Dial>(this, QColor(181, 124, 255), Qt::red, WIDTH-90-RSMALL, 200, RSMALL,
                                         15, -20, 60, OVERVOLTAGE, -30, 140, "V", 20);
    //speedDial->setValue(100);
    voltageDial->setValue(0);
    tempDial->setValue(0);

    // Update values every 10 milliseconds
    QTimer *timer = new QTimer(this);
    timer->setInterval(10);
    connect(timer, &QTimer::timeout, this, &WfeDashboardUI::update);
    timer->start();
    //update();
    this->show();
}

WfeDashboardUI::~WfeDashboardUI(){}

void WfeDashboardUI::updateErrorDisplay() {
    for(std::pair<int, std::string> i : data->dtcMessagePayload) {
        errorDisplay->addErrorMessage(i.second, i.first);
    }
    data->dtcMessagePayload.clear();
}


std::string WfeDashboardUI::toXDecimalString(double doubleVal, int precisionVal) {
    return std::to_string(doubleVal).substr(0, std::to_string(doubleVal).find(".") + precisionVal + 1);
}

void WfeDashboardUI::update() {

    updateErrorDisplay();

    batteryDisplay->setText("Battery: " + toXDecimalString(data->battery, 2) + "%");
    if(data->battery > 66) {
        batteryDisplay->setColour(Qt::green);
    } else if(data->battery > 33) {
        batteryDisplay->setColour(Qt::yellow);
    } else {
        batteryDisplay->setColour(Qt::red);
    }

    speedDisplay->setText("Speed: " + toXDecimalString(data->speed, 2) + " kph");
    tempDisplay->setText("Temp: "+ toXDecimalString(data->temperature, 2) +"°C");
    voltageDisplay->setText("Voltage: "+ toXDecimalString(data->voltage, 2) +" V");
    speedDial->setValue(data->speed);
    tempDial->setValue(data->temperature);
    voltageDial->setValue(data->voltage);
    this->repaint();
}

void WfeDashboardUI::paintEvent(QPaintEvent *) {
    QPainter qp(this);

    modeDisplay->draw(qp);
    batteryDisplay->draw(qp);
    speedDisplay->draw(qp);
    tempDisplay->draw(qp);
    voltageDisplay->draw(qp);
    errorDisplay->draw(qp);
    speedDial->draw(qp);
    tempDial->draw(qp);
    voltageDial->draw(qp);
}

