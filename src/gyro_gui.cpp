#include <QAbstractItemView>
#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QTableWidget>
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
static constexpr uint64_t RX_PIPELINES[3] = {
    0xF0F0F0F0D2ULL,
    0xF0F0F0F0D3ULL,
    0xF0F0F0F0D4ULL,
};
static constexpr auto OFFLINE_TIMEOUT = std::chrono::seconds(10);

struct SimplePacket {
  uint8_t drone_id;
  int16_t accel_x;
  int16_t accel_y;
  int16_t accel_z;
  int16_t gyro_x;
  int16_t gyro_y;
  int16_t gyro_z;
  int8_t rssi;
  bool leader;
};

struct GyroData {
  int16_t ax{};
  int16_t ay{};
  int16_t az{};
  int16_t gx{};
  int16_t gy{};
  int16_t gz{};
  int8_t rssi{};
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
    droneTable_->setColumnCount(10);
    QStringList headers = {
        "Drone",        "Accel_X (m/s^2)", "Accel_Y (m/s^2)", "Accel_Z (m/s^2)",
        "Gyro_X (dps)", "Gyro_Y (dps)",    "Gyro_Z (dps)",    "RSSI (dBm)",
        "Online",       "Leader"};
    droneTable_->setHorizontalHeaderLabels(headers);
    droneTable_->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
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
    txRadio_.setAddress(BASE_TX, RX_PIPELINES[0]);
    rxRadio_.setAddress(BASE_TX, RX_PIPELINES[0]);
    rxRadio_.openListeningPipe(2, RX_PIPELINES[1]);
    rxRadio_.openListeningPipe(3, RX_PIPELINES[2]);

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
      droneData.rssi = pkt.rssi;
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

    if (haveLeader_) {
      auto it = drones_.find(currentLeader_);
      if (it == drones_.end() || !it->second.online) {
        haveLeader_ = false;
      }
    }

    if (!haveLeader_) {
      bool found = false;
      uint8_t best_id = 0;
      auto best_time = std::chrono::steady_clock::time_point::min();
      for (const auto &kv : drones_) {
        if (!kv.second.online)
          continue;
        if (!found || kv.second.last_update > best_time) {
          found = true;
          best_id = kv.first;
          best_time = kv.second.last_update;
        }
      }
      if (found) {
        currentLeader_ = best_id;
        haveLeader_ = true;

        char msg[32]{};
        std::snprintf(msg, sizeof(msg), "%u Leader = True", currentLeader_);
        txRadio_.send(msg, sizeof(msg));
        uint8_t arc = txRadio_.getARC();

        logArea_->appendPlainText(
            QString("New leader: %1 ARC: %2").arg(currentLeader_).arg(arc));
      } else {
        logArea_->appendPlainText("No leader");
      }
    }

    droneTable_->setRowCount(static_cast<int>(drones_.size()));
    int row = 0;
    for (const auto &[id, d] : drones_) {
      droneTable_->setItem(row, 0,
                           new QTableWidgetItem(QString("Drone %1").arg(id)));

      float ax = accelRawToMs2(d.ax);
      float ay = accelRawToMs2(d.ay);
      float az = accelRawToMs2(d.az);
      float gx = gyroRawToDps(d.gx);
      float gy = gyroRawToDps(d.gy);
      float gz = gyroRawToDps(d.gz);

      droneTable_->setItem(row, 1,
                           new QTableWidgetItem(QString::number(ax, 'f', 2)));
      droneTable_->setItem(row, 2,
                           new QTableWidgetItem(QString::number(ay, 'f', 2)));
      droneTable_->setItem(row, 3,
                           new QTableWidgetItem(QString::number(az, 'f', 2)));
      droneTable_->setItem(row, 4,
                           new QTableWidgetItem(QString::number(gx, 'f', 2)));
      droneTable_->setItem(row, 5,
                           new QTableWidgetItem(QString::number(gy, 'f', 2)));
      droneTable_->setItem(row, 6,
                           new QTableWidgetItem(QString::number(gz, 'f', 2)));
      droneTable_->setItem(row, 7,
                           new QTableWidgetItem(QString::number(d.rssi)));
      QTableWidgetItem *statusItem = new QTableWidgetItem(
          QString("[%1]").arg(QChar(0x25CF))); // bullet inside []
      statusItem->setForeground(d.online ? Qt::green : Qt::red);
      statusItem->setTextAlignment(Qt::AlignCenter);
      droneTable_->setItem(row, 8, statusItem);

      QTableWidgetItem *leaderItem =
          new QTableWidgetItem((d.online && d.leader) ? "Yes" : "No");
      leaderItem->setTextAlignment(Qt::AlignCenter);
      droneTable_->setItem(row, 9, leaderItem);

      ++row;
    }
  }

private:
  QTableWidget *droneTable_{};
  QPlainTextEdit *logArea_{};
  RadioInterface txRadio_;
  RadioInterface rxRadio_;
  std::unordered_map<uint8_t, GyroData> drones_;
  uint8_t currentLeader_ = 0;
  bool haveLeader_ = false;
};

#include "gyro_gui.moc"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  GyroServerWindow window;
  window.resize(1600, 400);
  window.show();
  return app.exec();
}
