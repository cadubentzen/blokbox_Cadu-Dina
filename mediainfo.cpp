#include "mediainfo.h"
#include "ui_mediainfo.h"
#include <QDebug>
MediaInfo::MediaInfo(QWidget *parent) :
  AbstractMediaInfo(parent),
  ui(new Ui::MediaInfo){
  ui->setupUi(this);

  connect(this,SIGNAL(trocaCor(QColor)),ui->widgetLightCycle,SLOT(changedColor(QColor)));
  startTimer(5000);
}

MediaInfo::~MediaInfo(){
  delete ui;
}

void MediaInfo::timerEvent(QTimerEvent *e){
  Q_UNUSED(e);
  // conta a mensagem que sera exibida
  static int mes=0;
  if(mes == 6){
    mes = 0;
  }
  switch (mes) {
  case 0:
    ui->widgetLightCycle->setText(artist);
    break;
  case 1:
    ui->widgetLightCycle->setText(title);
    break;
  case 2:
    ui->widgetLightCycle->setText(album);
    break;
  case 3:
    ui->widgetLightCycle->setText(codec);
    break;
  case 4:
    ui->widgetLightCycle->setText(bitrate);
    break;
  case 5:
    ui->widgetLightCycle->setText(track);
    break;
  default:
    break;
  }
  mes++;
}

void MediaInfo::setAtribute(QString property, QString value){
  if(property == "AlbumArtist"){
    artist = value;
    ui->labelAlbumArtist->setText(value);
    ui->widgetLightCycle->setText(value);
  }
  else if(property == "Title"){
    title = value;
    ui->labelTitle->setText(value);
  }
  else if(property == "AlbumTitle"){
    album = value;
    ui->labelAlbumTitle->setText(value);
  }
  else if(property == "TrackNumber")
    ui->labelTrackNumber->setText(value);
  else if(property == "AudioBitRate")
    ui->labelAudioBitRate->setText(value);
  else if(property == "AudioCodec")
    ui->labelAudioCodec->setText(value);
}

void MediaInfo::changedColor(QColor cor){
  emit trocaCor(cor);
}

