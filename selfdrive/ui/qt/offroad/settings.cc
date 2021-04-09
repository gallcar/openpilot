#include <string>
#include <iostream>
#include <sstream>
#include <cassert>

#ifndef QCOM
#include "networking.hpp"
#endif
#include "settings.hpp"
#include "widgets/input.hpp"
#include "widgets/toggle.hpp"
#include "widgets/offroad_alerts.hpp"
#include "widgets/scrollview.hpp"
#include "widgets/controls.hpp"
#include "widgets/ssh_keys.hpp"
#include "common/params.h"
#include "common/util.h"
#include "selfdrive/hardware/hw.h"
#include "home.hpp"


QWidget * toggles_panel() {
  QVBoxLayout *toggles_list = new QVBoxLayout();

  toggles_list->addWidget(new ParamControl("OpenpilotEnabledToggle",
                                            "오픈파일럿 사용",
                                            "어댑티브 크루즈 컨트롤 및 차선 유지 지원을 위해 오픈파일럿 시스템을 사용하십시오. 이 기능을 사용하려면 항상 주의를 기울여야 합니다. 이 설정을 변경하는 것은 자동차의 전원이 꺼졌을 때 적용됩니다.",
                                            "../assets/offroad/icon_openpilot.png"
                                              ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("IsLdwEnabled",
                                            "차선이탈 경보 사용",
                                            "50km/h이상의 속도로 주행하는 동안 방향 지시등이 활성화되지 않은 상태에서 차량이 감지된 차선 위를 넘어갈 경우 원래 차선으로 다시 방향을 전환하도록 경고를 보냅니다.",
                                            "../assets/offroad/icon_warning.png"
                                              ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("IsRHD",
                                            "우핸들 운전방식 사용",
                                            "오픈파일럿이 좌측 교통 규칙을 준수하도록 허용하고 우측 운전석에서 운전자 모니터링을 수행하십시오.",
                                            "../assets/offroad/icon_openpilot_mirrored.png"
                                            ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("IsMetric",
                                            "미터법 사용",
                                            "mi/h 대신 km/h 단위로 속도를 표시합니다.",
                                            "../assets/offroad/icon_metric.png"
                                            ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("CommunityFeaturesToggle",
                                            "커뮤니티 기능 사용",
                                            "comma.ai에서 유지 또는 지원하지 않고 표준 안전 모델에 부합하는 것으로 확인되지 않은 오픈 소스 커뮤니티의 기능을 사용하십시오. 이러한 기능에는 커뮤니티 지원 자동차와 커뮤니티 지원 하드웨어가 포함됩니다. 이러한 기능을 사용할 때는 각별히 주의해야 합니다.",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  toggles_list->addWidget(horizontal_line());
  ParamControl *record_toggle = new ParamControl("RecordFront",
                                            "운전자 영상 녹화 및 업로드",
                                            "운전자 모니터링 카메라에서 데이터를 업로드하고 운전자 모니터링 알고리즘을 개선하십시오.",
                                            "../assets/offroad/icon_network.png");
  toggles_list->addWidget(record_toggle);
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("EndToEndToggle",
                                           "차선이 없을 때 사용 버전 알파",
                                           "차선이 없는 곳에서 사람과 같이 운전을 하는 것을 목표함니다.",
                                           "../assets/offroad/icon_road.png"));

#ifdef QCOM2
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("EnableWideCamera",
                                           "와이드 카메라 화면을 사용",
                                           "와이드 카메라 화면을 사용합니다. 설정 후 재부팅이 필요합니다.",
                                           "../assets/offroad/icon_openpilot.png"));
#endif

  bool record_lock = Params().getBool("RecordFrontLock");
  record_toggle->setEnabled(!record_lock);

  QWidget *widget = new QWidget;
  widget->setLayout(toggles_list);
  return widget;
}

DevicePanel::DevicePanel(QWidget* parent) : QWidget(parent) {
  QVBoxLayout *device_layout = new QVBoxLayout;

  Params params = Params();

  QString dongle = QString::fromStdString(params.get("DongleId", false));
  device_layout->addWidget(new LabelControl("동글 아이디", dongle));
  device_layout->addWidget(horizontal_line());

  QString serial = QString::fromStdString(params.get("HardwareSerial", false));
  device_layout->addWidget(new LabelControl("시리얼", serial));

  QHBoxLayout *reset_layout = new QHBoxLayout();
  reset_layout->setSpacing(30);

  // reset calibration button
  QPushButton *reset_calib_btn = new QPushButton("캘리브레이션 리셋");
  reset_layout->addWidget(reset_calib_btn);
  QObject::connect(reset_calib_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("캘리브레이션을 다시 하시겠습니까?")) {
      Params().remove("CalibrationParams");
      Params().remove("LiveParameters");
      QTimer::singleShot(1000, []() {
        Hardware::reboot();
      });
    }
  });

  device_layout->addWidget(horizontal_line());
  device_layout->addLayout(reset_layout);

  // offroad-only buttons
  QList<ButtonControl*> offroad_btns;

  offroad_btns.append(new ButtonControl("운전자 영상", "미리보기",
                                   "운전자 영상이 제대로 작동하는지 확인을 합니다. 차량을 끄고 사용하도록 하십시오.",
                                   [=]() { 
                                      Params().putBool("운전자 영상 보기", true);
                                      GLWindow::ui_state.scene.driver_view = true; }
                                    ));

  QString resetCalibDesc = "오픈파일럿은 좌우로 4° 위아래로 5° 를 보정합니다. 그 이상의 경우 보정이 필요합니다.";
  ButtonControl *resetCalibBtn = new ButtonControl("캘리브레이션 리셋", "리셋", resetCalibDesc, [=]() {
    if (ConfirmationDialog::confirm("캘리브레이션을 리셋하시겠습니까?")) {
      Params().remove("CalibrationParams");
    }
  });
  connect(resetCalibBtn, &ButtonControl::showDescription, [=]() {
    QString desc = resetCalibDesc;
    std::string calib_bytes = Params().get("CalibrationParams");
    if (!calib_bytes.empty()) {
      try {
        AlignedBuffer aligned_buf;
        capnp::FlatArrayMessageReader cmsg(aligned_buf.align(calib_bytes.data(), calib_bytes.size()));
        auto calib = cmsg.getRoot<cereal::Event>().getLiveCalibration();
        if (calib.getCalStatus() != 0) {
          double pitch = calib.getRpyCalib()[1] * (180 / M_PI);
          double yaw = calib.getRpyCalib()[2] * (180 / M_PI);
          desc += QString(" Your device is pointed %1° %2 and %3° %4.")
                                .arg(QString::number(std::abs(pitch), 'g', 1), pitch > 0 ? "up" : "down",
                                     QString::number(std::abs(yaw), 'g', 1), yaw > 0 ? "right" : "left");
        }
      } catch (kj::Exception) {
        qInfo() << "invalid CalibrationParams";
      }
    }
    resetCalibBtn->setDescription(desc);
  });
  offroad_btns.append(resetCalibBtn);

  offroad_btns.append(new ButtonControl("트레이닝 가이드", "가이드 보기",
                                        "오픈파일럿의 제한적 상황과 규정을 확인", [=]() {
    if (ConfirmationDialog::confirm("트레이닝 가이드를 확인하시겠습니까?")) {
      Params().remove("CompletedTrainingVersion");
      emit reviewTrainingGuide();
    }
  }));

  QString brand = params.getBool("Passive") ? "대시캠" : "";
  offroad_btns.append(new ButtonControl("오픈파일럿 제거 " + brand, "오픈파일럿 제거", "", [=]() {
    if (ConfirmationDialog::confirm("오픈파일럿을 제거하시겠습니까?")) {
      Params().putBool("DoUninstall", true);
    }
  }));

  for(auto &btn : offroad_btns){
    device_layout->addWidget(horizontal_line());
    QObject::connect(parent, SIGNAL(offroadTransition(bool)), btn, SLOT(setEnabled(bool)));
    device_layout->addWidget(btn);
  }

  // power buttons
  QHBoxLayout *power_layout = new QHBoxLayout();
  power_layout->setSpacing(30);

  QPushButton *reboot_btn = new QPushButton("재부팅");
  power_layout->addWidget(reboot_btn);
  QObject::connect(reboot_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("장치를 재부팅 하시겠습니까?")) {
      Hardware::reboot();
    }
  });

  QPushButton *poweroff_btn = new QPushButton("전원 끄기");
  poweroff_btn->setStyleSheet("background-color: #E22C2C;");
  power_layout->addWidget(poweroff_btn);
  QObject::connect(poweroff_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("전원을 끄시겠습니까?")) {
      Hardware::poweroff();
    }
  });

  device_layout->addLayout(power_layout);

  setLayout(device_layout);
  setStyleSheet(R"(
    QPushButton {
      padding: 0;
      height: 120px;
      border-radius: 15px;
      background-color: #393939;
    }
  )");
}

DeveloperPanel::DeveloperPanel(QWidget* parent) : QFrame(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  setLayout(main_layout);
  setStyleSheet(R"(QLabel {font-size: 50px;})");
}

void DeveloperPanel::showEvent(QShowEvent *event) {
  Params params = Params();
  std::string brand = params.getBool("Passive") ? "dashcam" : "openpilot";
  QList<QPair<QString, std::string>> dev_params = {
    {"Version", brand + " v" + params.get("Version", false).substr(0, 14)},
    {"Git Branch", params.get("GitBranch", false)},
    {"Git Commit", params.get("GitCommit", false).substr(0, 10)},
    {"Panda Firmware", params.get("PandaFirmwareHex", false)},
    {"OS Version", Hardware::get_os_version()},
  };

  for (int i = 0; i < dev_params.size(); i++) {
    const auto &[name, value] = dev_params[i];
    QString val = QString::fromStdString(value).trimmed();
    if (labels.size() > i) {
      labels[i]->setText(val);
    } else {
      labels.push_back(new LabelControl(name, val));
      layout()->addWidget(labels[i]);
      if (i < (dev_params.size() - 1)) {
        layout()->addWidget(horizontal_line());
      }
    }
  }
}

QWidget * network_panel(QWidget * parent) {
#ifdef QCOM
  QVBoxLayout *layout = new QVBoxLayout;
  layout->setSpacing(30);

  // wifi + tethering buttons
  layout->addWidget(new ButtonControl("네트워크 설정열기", "열기", "",
                                      [=]() { HardwareEon::launch_wifi(); }));
  layout->addWidget(horizontal_line());

  layout->addWidget(new ButtonControl("테더링 설정열기", "열기", "",
                                      [=]() { HardwareEon::launch_tethering(); }));
  layout->addWidget(horizontal_line());

  // SSH key management
  layout->addWidget(new SshToggle());
  layout->addWidget(horizontal_line());
  layout->addWidget(new SshControl());

  layout->addStretch(1);

  QWidget *w = new QWidget;
  w->setLayout(layout);
#else
  Networking *w = new Networking(parent);
#endif
  return w;
}

QWidget * community_panel() {
  QVBoxLayout *toggles_list = new QVBoxLayout();
  //toggles_list->setMargin(50);

  toggles_list->addWidget(new ParamControl("LongControlEnabled",
                                            "롱컨트롤 사용",
                                            "N 롱컨트롤 기능 사용. 오픈파일럿이 속도를 조절합니다. 주의 하시길 바랍니다.",
                                            "../assets/offroad/icon_road.png"
                                              ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("MadModeEnabled",
                                            "매드모드 사용",
                                            "HKG 매드모드 사용. 가감속의 사용 하지 않아도 핸들 조향을 사용합니다.",
                                            "../assets/offroad/icon_openpilot.png"
                                              ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("AutoLaneChangeEnabled",
                                            "자동 차선변경 사용",
                                            "자동 차선 변경. 사용에 주의 하십시오",
                                            "../assets/offroad/icon_road.png"
                                              ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("SccSmootherEnabled",
                                            "가감속 스무서 사용",
                                            "N Smoother. 순정 ASCC 기능을 이용하여 가속과 감속을 부드럽게 할 수 있도록 도와 줍니다. 사용법을 정확히 인지하고 사용하십시오.",
                                            "../assets/offroad/icon_road.png"
                                            ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("SccSmootherSlowOnCurves",
                                            "커브 감속 사용",
                                            "SCC 설정 시 곡률에 따른 속도 감속 기능을 사용",
                                            "../assets/offroad/icon_road.png"
                                            ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("SccSmootherSyncGasPressed",
                                            "크루즈 속도의 동기화",
                                            "크루즈 속도를 설정 후 엑셀로 인해 설정 속도보다 가속 속도가 높아지면 그 속도에 크루즈 설정 속도가 동기화 됩니다.",
                                            "../assets/offroad/icon_road.png"
                                            ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("SccSmootherSwitchGapOnly",
                                            "스무스 설정 버튼 변경",
                                            "크루즈 갭 설정 버튼으로 순정 ASCC와 가감속 스무서 설정을 변경합니다.",
                                            "../assets/offroad/icon_road.png"
                                            ));
                                            toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("ShowDebugUI",
                                            "디버그 내용 보기",
                                            "가감속 등 디버그 내용을 화면에 띄웁니다.",
                                            "../assets/offroad/icon_shell.png"
                                            ));

  QWidget *widget = new QWidget;
  widget->setLayout(toggles_list);
  return widget;
}


SettingsWindow::SettingsWindow(QWidget *parent) : QFrame(parent) {
  // setup two main layouts
  QVBoxLayout *sidebar_layout = new QVBoxLayout();
  sidebar_layout->setMargin(0);
  panel_widget = new QStackedWidget();
  panel_widget->setStyleSheet(R"(
    border-radius: 30px;
    background-color: #292929;
  )");

  // close button
  QPushButton *close_btn = new QPushButton("X");
  close_btn->setStyleSheet(R"(
    font-size: 60px;
    font-weight: bold;
    border 1px grey solid;
    border-radius: 100px;
    background-color: #292929;
  )");
  close_btn->setFixedSize(200, 200);
  sidebar_layout->addSpacing(45);
  sidebar_layout->addWidget(close_btn, 0, Qt::AlignCenter);
  QObject::connect(close_btn, SIGNAL(released()), this, SIGNAL(closeSettings()));

  // setup panels
  DevicePanel *device = new DevicePanel(this);
  QObject::connect(device, SIGNAL(reviewTrainingGuide()), this, SIGNAL(reviewTrainingGuide()));

  QPair<QString, QWidget *> panels[] = {
    {"장치", device},
    {"네트워크", network_panel(this)},
    {"토글메뉴", toggles_panel()},
    {"개발자", new DeveloperPanel()},
    {"커뮤니티", community_panel()},
  };

  sidebar_layout->addSpacing(45);
  nav_btns = new QButtonGroup();
  for (auto &[name, panel] : panels) {
    QPushButton *btn = new QPushButton(name);
    btn->setCheckable(true);
    btn->setStyleSheet(R"(
      QPushButton {
        color: grey;
        border: none;
        background: none;
        font-size: 65px;
        font-weight: 500;
        padding-top: 18px;
        padding-bottom: 18px;
      }
      QPushButton:checked {
        color: white;
      }
    )");

    nav_btns->addButton(btn);
    sidebar_layout->addWidget(btn, 0, Qt::AlignRight);

    panel->setContentsMargins(50, 25, 50, 25);

    ScrollView *panel_frame = new ScrollView(panel, this);
    panel_widget->addWidget(panel_frame);

    QObject::connect(btn, &QPushButton::released, [=, w = panel_frame]() {
      panel_widget->setCurrentWidget(w);
    });
  }
  qobject_cast<QPushButton *>(nav_btns->buttons()[0])->setChecked(true);
  sidebar_layout->setContentsMargins(50, 50, 100, 50);

  // main settings layout, sidebar + main panel
  QHBoxLayout *settings_layout = new QHBoxLayout();

  sidebar_widget = new QWidget;
  sidebar_widget->setLayout(sidebar_layout);
  sidebar_widget->setFixedWidth(500);
  settings_layout->addWidget(sidebar_widget);
  settings_layout->addWidget(panel_widget);

  setLayout(settings_layout);
  setStyleSheet(R"(
    * {
      color: white;
      font-size: 48px;
    }
    SettingsWindow {
      background-color: black;
    }
  )");
}

void SettingsWindow::hideEvent(QHideEvent *event){
#ifdef QCOM
  HardwareEon::close_activities();
#endif
}

