#include <QApplication>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QTimer>
#include <QWidget>

#include "radio.hpp"
#include "sensor_utils.hpp"

#include <chrono>
#include <unordered_map>

#define TX_CE_PIN 27
#define TX_CSN_PIN 0
#define RX_CE_PIN 22
#define RX_CSN_PIN 10
static constexpr uint64_t BASE_TX = 0xF0F0F0F0E1ULL;
static constexpr uint64_t BASE_RX = 0xF0F0F0F0D2ULL;
static constexpr auto OFFLINE_TIMEOUT = std::chrono::seconds(2);

struct SimplePacket {
  uint8_t drone_id;
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
  bool leader;
};

struct GyroData {
  int16_t ax{};
  int16_t ay{};
  int16_t az{};
  int16_t gx{};
  int16_t gy{};
  int16_t gz{};
  bool leader{};
  bool strong_signal{};
  std::chrono::steady_clock::time_point last_update{};
  bool online = false;
};

class GyroServerWindow : public QWidget {
  Q_OBJECT
public:
  explicit GyroServerWindow(QWidget *parent = nullptr)
      : QWidget(parent), txRadio_(TX_CE_PIN, TX_CSN_PIN),
        rxRadio_(RX_CE_PIN, RX_CSN_PIN) {
    droneList_ = new QListWidget(this);
    logArea_ = new QPlainTextEdit(this);
    logArea_->setReadOnly(true);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(droneList_);
    layout->addWidget(logArea_, 1);
    setLayout(layout);

    if (!txRadio_.begin() || !rxRadio_.begin()) {
      logArea_->appendPlainText("Radio init failed");
    }
    txRadio_.configure(1, RadioDataRate::MEDIUM_RATE);
    rxRadio_.configure(1, RadioDataRate::MEDIUM_RATE);
    txRadio_.setAddress(BASE_TX, BASE_RX);
    rxRadio_.setAddress(BASE_TX, BASE_RX);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GyroServerWindow::pollRadio);
    timer->start(250);
  }

private slots:
  void pollRadio() {
    SimplePacket pkt{};
    if (rxRadio_.receive(&pkt, sizeof(pkt))) {
      auto &droneData = drones_[pkt.drone_id];
      bool firstTime = droneData.last_update.time_since_epoch().count() == 0;
      droneData.ax = pkt.accel_x;
      droneData.ay = pkt.accel_y;
      droneData.az = pkt.accel_z;
      droneData.gx = pkt.gyro_x;
      droneData.gy = pkt.gyro_y;
      droneData.gz = pkt.gyro_z;
      droneData.leader = pkt.leader;
      droneData.strong_signal = rxRadio_.testRPD();
      droneData.last_update = std::chrono::steady_clock::now();
      if (firstTime || !droneData.online) {
        logArea_->appendPlainText(QString("Drone %1 online").arg(pkt.drone_id));
      }
      droneData.online = true;
    }

    auto now = std::chrono::steady_clock::now();
    for (auto &[id, d] : drones_) {
      bool is_online = now - d.last_update <= OFFLINE_TIMEOUT;
      if (d.online != is_online) {
        d.online = is_online;
        logArea_->appendPlainText(QString("Drone %1 %2")
                                      .arg(id)
                                      .arg(is_online ? "online" : "offline"));
      }
    }

    droneList_->clear();
    for (const auto &[id, d] : drones_) {
      QString item = QString("ID %1 - acc:(%2,%3,%4) gyro:(%5,%6,%7) %8")
                         .arg(id)
                         .arg(d.ax)
                         .arg(d.ay)
                         .arg(d.az)
                         .arg(d.gx)
                         .arg(d.gy)
                         .arg(d.gz)
                         .arg(d.online ? "online" : "offline");
      droneList_->addItem(item);
    }
  }

private:
  QListWidget *droneList_{};
  QPlainTextEdit *logArea_{};
  RadioInterface txRadio_;
  RadioInterface rxRadio_;
  std::unordered_map<uint8_t, GyroData> drones_;
};

#include "gyro_gui.moc"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  GyroServerWindow window;
  window.resize(800, 400);
  window.show();
  return app.exec();
}
