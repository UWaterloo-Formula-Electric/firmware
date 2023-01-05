#include <QtWidgets>
#include <QApplication>
#include <vector>
#include <string>
#include <any>
#include <iostream>

// Used to display DTC messages.
class ErrorDisplay {
private:
    QWidget* parent;
    Qt::AlignmentFlag align;
    QFont font;
    std::vector<std::string> errorMessages;
    std::vector<int> severities;
    const std::map<int, std::string> severityHeaderSettings = {
        {1, "F"},
        {2, "C"},
        {3, "E"},
        {4, "W"}
    };
    const std::map<int, QColor> severityColourSettings = {
        {1, Qt::red},
        {2, QColor(255, 103, 0)},
        {3, QColor(255, 193, 7)},
        {4, Qt::yellow}
    };
    int x;
    int y;
    int width;
    int height;


public:
    ErrorDisplay(
            QWidget* parent,
            Qt::AlignmentFlag align
    ) {
        this->parent = parent;
        this->align = align;
        this->font = QFont(QString("Arial"), 20);

        // Creates padding around window edge
        x = parent->width() * 0.05 / 2;
        y = parent->height() * 0.05 / 2;
        width = parent->width() * 0.95;
        height = parent->height() * 0.95;
    }

    void addErrorMessage(std::string errMsg, int severity) {
        if(errorMessages.size() == 3) {
            errorMessages.erase(errorMessages.begin());
            severities.erase(severities.begin());
        }
        std::string header;
        try {
          header = severityHeaderSettings.at(severity);
        }
        catch (const std::out_of_range& e ) {
          // std::cerr << e.what() << std::endl;
          header = "U";
        }
        errorMessages.push_back(header+": "+errMsg);
        severities.push_back(severity);
    }

    void draw(QPainter& qp) {
        qp.setFont(font);
        int numErrorMsgs = errorMessages.size();

        // If there are no error messages
        if(numErrorMsgs == 0) {
            qp.setPen(Qt::white);
            qp.drawText(x, y, width, height, align, "No DTC Messages Received");
        }
        for(int i = 0; i < numErrorMsgs; i++) {
            std::string errorMsg = errorMessages.at(i);
            int severity = severities.at(i);
            QColor colour = severityColourSettings.at(severity);
            qp.setPen(colour);
            QFontMetrics metrics = QFontMetrics(font);
            QString elidedText  = metrics.elidedText(QString::fromStdString(errorMsg), Qt::ElideRight, width);
            int yLoc = y - (numErrorMsgs - 1 - i) * metrics.height();
            qp.drawText(x, yLoc, width, height, align, elidedText);

        }
    }

    ~ErrorDisplay() {}
};
