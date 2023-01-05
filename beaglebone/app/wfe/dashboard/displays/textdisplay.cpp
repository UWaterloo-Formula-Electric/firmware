#include <QtWidgets>
#include <QApplication>

class TextDisplay {
private:
    QWidget* parent;
    std::string text;
    QColor colour;
    Qt::AlignmentFlag align;
    QFont font;
    int x;
    int y;
    int width;
    int height;

public:
    TextDisplay(
            QWidget* parent,
            std::string text,
            QColor colour,
            Qt::AlignmentFlag align,
            int fontSize = 22,
            int yOffset = 0
    ) {
        this->parent = parent;
        this->text = text;
        this->colour = colour;
        this->align = align;
        //this->font = QFont('Arial', fontSize);
        this->font = QFont(QString("Arial"), fontSize);

        // Creates padding around window edge
        x = parent->width() * 0.05 / 2;
        y = parent->height() * 0.05 / 2 + yOffset;
        width = parent->width() * 0.95;
        height = parent->height() * 0.95;
    }

    void setText(std::string text) {
        this->text = text;
    }

    void setColour(QColor colour) {
        this->colour = colour;
    }

    void draw(QPainter& qp) {
        qp.setPen(colour);
        qp.setFont(font);
        QFontMetrics metrics = QFontMetrics(font);
        QString elidedText  = metrics.elidedText(QString::fromStdString(text), Qt::ElideRight, width);
        qp.drawText(x, y, width, height, align, elidedText);
    }

    ~TextDisplay() {}
};
