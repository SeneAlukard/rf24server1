#include <QApplication>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
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
    droneTable_ = new QTableWidget(this);
    droneTable_->setColumnCount(8);
    QStringList headers = {"Drone", "Accel_X", "Accel_Y", "Accel_Z",
                           "Gyro_X", "Gyro_Y", "Gyro_Z", "Online"};
    droneTable_->setHorizontalHeaderLabels(headers);
    droneTable_->verticalHeader()->setVisible(false);
    droneTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    droneTable_->setSelectionMode(QAbstractItemView::NoSelection);
    // slightly reduce font size in the table
    QFont tableFont = droneTable_->font();
    tableFont.setPointSize(tableFont.pointSize() - 2);
    droneTable_->setFont(tableFont);
    logArea_ = new QPlainTextEdit(this);
    logArea_->setReadOnly(true);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(droneTable_);
    layout->addWidget(logArea_);
    // make the drone table slightly larger than the log area
    layout->setStretch(0, 3);
    layout->setStretch(1, 2);
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

      if (pkt.leader && currentLeader_ != pkt.drone_id) {
        currentLeader_ = pkt.drone_id;
        logArea_->appendPlainText(QString("New leader %1").arg(pkt.drone_id));
      } else if (!pkt.leader && currentLeader_ == pkt.drone_id) {
        currentLeader_ = -1;
      }
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

    if (currentLeader_ != -1) {
      auto it = drones_.find(static_cast<uint8_t>(currentLeader_));
      if (it == drones_.end() || !it->second.online) {
        currentLeader_ = -1;
      }
    }

    droneTable_->setRowCount(static_cast<int>(drones_.size()));
    int row = 0;
    for (const auto &[id, d] : drones_) {
      droneTable_->setItem(row, 0,
                           new QTableWidgetItem(QString("Drone %1").arg(id)));
      droneTable_->setItem(row, 1,
                           new QTableWidgetItem(QString::number(d.ax)));
      droneTable_->setItem(row, 2,
                           new QTableWidgetItem(QString::number(d.ay)));
      droneTable_->setItem(row, 3,
                           new QTableWidgetItem(QString::number(d.az)));
      droneTable_->setItem(row, 4,
                           new QTableWidgetItem(QString::number(d.gx)));
      droneTable_->setItem(row, 5,
                           new QTableWidgetItem(QString::number(d.gy)));
      droneTable_->setItem(row, 6,
                           new QTableWidgetItem(QString::number(d.gz)));
      QTableWidgetItem *statusItem = new QTableWidgetItem;
      statusItem->setBackground(d.online ? Qt::green : Qt::red);
      droneTable_->setItem(row, 7, statusItem);
      ++row;
    }
  }

private:
  QTableWidget *droneTable_{};
  QPlainTextEdit *logArea_{};
  RadioInterface txRadio_;
  RadioInterface rxRadio_;
  std::unordered_map<uint8_t, GyroData> drones_;
  int currentLeader_ = -1;
};

#include "gyro_gui.moc"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  GyroServerWindow window;
  window.resize(900, 400);
  window.show();
  return app.exec();
}
