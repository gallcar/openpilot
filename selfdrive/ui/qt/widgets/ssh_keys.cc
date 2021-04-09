#include <QNetworkReply>
#include <QHBoxLayout>
#include "widgets/input.hpp"
#include "widgets/ssh_keys.hpp"
#include "common/params.h"


SshControl::SshControl() : AbstractControl("SSH Keys", "경고: 이 버튼을 승인할 시 당신의 깃허브 설정에서 공개키로 ssh 접속이 허가됩니다. 본인의 계정 이외의 다른 계정을 입력하지 마십시오. 콤마에서는 절대 개인의 깃허브 아이디를 요청하지 않습니다.", "") {

  // setup widget
  hlayout->addStretch(1);

  username_label.setAlignment(Qt::AlignVCenter);
  username_label.setStyleSheet("color: #aaaaaa");
  hlayout->addWidget(&username_label);

  btn.setStyleSheet(R"(
    padding: 0;
    border-radius: 50px;
    font-size: 35px;
    font-weight: 500;
    color: #E4E4E4;
    background-color: #393939;
  )");
  btn.setFixedSize(250, 100);
  hlayout->addWidget(&btn);

  QObject::connect(&btn, &QPushButton::released, [=]() {
    if (btn.text() == "추가") {
      username = InputDialog::getText("Enter your GitHub username");
      if (username.length() > 0) {
        btn.setText("LOADING");
        btn.setEnabled(false);
        getUserKeys(username);
      }
    } else {
      Params().remove("GithubUsername");
      Params().remove("GithubSshKeys");
      refresh();
    }
  });

  // setup networking
  manager = new QNetworkAccessManager(this);
  networkTimer = new QTimer(this);
  networkTimer->setSingleShot(true);
  networkTimer->setInterval(5000);
  connect(networkTimer, SIGNAL(timeout()), this, SLOT(timeout()));

  refresh();
}

void SshControl::refresh() {
  QString param = QString::fromStdString(Params().get("GithubSshKeys"));
  if (param.length()) {
    username_label.setText(QString::fromStdString(Params().get("GithubUsername")));
    btn.setText("제거");
  } else {
    username_label.setText("");
    btn.setText("추가");
  }
  btn.setEnabled(true);
}

void SshControl::getUserKeys(QString username){
  QString url = "https://github.com/" + username + ".keys";

  QNetworkRequest request;
  request.setUrl(QUrl(url));
#ifdef QCOM
  QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
  ssl.setCaCertificates(QSslCertificate::fromPath("/usr/etc/tls/cert.pem",
                        QSsl::Pem, QRegExp::Wildcard));
  request.setSslConfiguration(ssl);
#endif

  reply = manager->get(request);
  connect(reply, SIGNAL(finished()), this, SLOT(parseResponse()));
  networkTimer->start();
}

void SshControl::timeout(){
  reply->abort();
}

void SshControl::parseResponse(){
  QString err = "";
  if (reply->error() != QNetworkReply::OperationCanceledError) {
    networkTimer->stop();
    QString response = reply->readAll();
    if (reply->error() == QNetworkReply::NoError && response.length()) {
      Params().put("GithubUsername", username.toStdString());
      Params().put("GithubSshKeys", response.toStdString());
    } else if(reply->error() == QNetworkReply::NoError){
      err = "Username '" + username + "' has no keys on GitHub";
    } else {
      err = "Username '" + username + "' doesn't exist on GitHub";
    }
  } else {
    err = "Request timed out";
  }

  if (err.length()) {
    ConfirmationDialog::alert(err);
  }

  refresh();
  reply->deleteLater();
  reply = nullptr;
}
