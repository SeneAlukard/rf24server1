#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QTimer>

#include "gbs.hpp"

class GbsWindow : public QWidget {
    Q_OBJECT
public:
    GbsWindow(GroundBaseStation &station, QWidget *parent = nullptr)
        : QWidget(parent), station_(station) {
        droneList_ = new QListWidget(this);
        lastCommandLabel_ = new QLabel("Last Command: none", this);
        logArea_ = new QTextEdit(this);
        logArea_->setReadOnly(true);
        commandEdit_ = new QLineEdit(this);
        QPushButton *sendBtn = new QPushButton("Send", this);

        QHBoxLayout *cmdLayout = new QHBoxLayout;
        cmdLayout->addWidget(commandEdit_);
        cmdLayout->addWidget(sendBtn);

        QVBoxLayout *layout = new QVBoxLayout;
        layout->addWidget(new QLabel("Connected Drones:"));
        layout->addWidget(droneList_);
        layout->addWidget(lastCommandLabel_);
        layout->addLayout(cmdLayout);
        layout->addWidget(new QLabel("Messages:"));
        layout->addWidget(logArea_);
        setLayout(layout);

        connect(sendBtn, &QPushButton::clicked, this, &GbsWindow::sendCommand);

        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &GbsWindow::updateDrones);
        timer->start(1000);
    }

private slots:
    void updateDrones() {
        station_.handleIncoming();
        droneList_->clear();
        for (const auto &d : station_.getDronesSnapshot()) {
            droneList_->addItem(QString("%1 (id %2)")
                                    .arg(QString::fromStdString(d.name))
                                    .arg(d.id));
        }
    }

    void sendCommand() {
        QString cmd = commandEdit_->text();
        if (cmd.isEmpty())
            return;
        QListWidgetItem *current = droneList_->currentItem();
        DroneIdType id = 0;
        if (current) {
            // extract id from text 'name (id X)'
            QString text = current->text();
            int idx = text.lastIndexOf("id ");
            if (idx != -1)
                id = static_cast<DroneIdType>(text.mid(idx + 3).toInt());
        }
        std::string cmdStr = cmd.toStdString();
        if (id == 0)
            station_.broadcastCommand(cmdStr);
        else
            station_.sendCommandToDrone(id, cmdStr);
        lastCommandLabel_->setText(QString("Last Command: %1 -> %2")
                                       .arg(id)
                                       .arg(cmd));
        logArea_->append(lastCommandLabel_->text());
        commandEdit_->clear();
    }

private:
    GroundBaseStation &station_;
    QListWidget *droneList_;
    QLabel *lastCommandLabel_;
    QTextEdit *logArea_;
    QLineEdit *commandEdit_;
};


int main(int argc, char **argv) {
    QApplication app(argc, argv);

    RadioInterface txRadio(27, 10);
    RadioInterface rxRadio(22, 0);

    if (!txRadio.begin() || !rxRadio.begin()) {
        qCritical("Radio init failed");
        return 1;
    }

    txRadio.configure(1, RadioDataRate::MEDIUM_RATE);
    rxRadio.configure(1, RadioDataRate::MEDIUM_RATE);
    txRadio.setAddress(0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL);
    rxRadio.setAddress(0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL);

    GroundBaseStation station(rxRadio, txRadio);

    GbsWindow window(station);
    window.resize(500, 400);
    window.show();

    return app.exec();
}

#include "gui_main.moc"
