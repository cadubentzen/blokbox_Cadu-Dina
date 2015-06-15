#include "spectrograph.h"
#include <iostream>
#include <QtCore/qmath.h>
#include <QBrush>
#include <QPen>
#include <fstream>
#include <QGradient>
#include <QDebug>
#include <QResizeEvent>
#include <QTimerEvent>
#include <QPainterPath>
#include <QEasingCurve>

Spectrograph::Spectrograph(QWidget *parent) :
  AbstractSpectrograph(parent){
  // at each 15ms, the timer clicks and timerEvent is called
  startTimer(15);
  // however 256 samples may be delivered to this widget, only
  // 64 are to be displayed becaus of speed limitations ;)
  NUM_BANDS = 64;

  //
  spectrum.resize(NUM_BANDS);
  decay.resize(NUM_BANDS);
  gravity.resize(NUM_BANDS);
  delay[0].resize(NUM_BANDS);
  delay[1].resize(NUM_BANDS);
  for(int i=0; i<NUM_BANDS; i++){
      decay[i]=0;
      gravity[i]=1;
      spectrum[i]=1;
      delay[0][i]=1;
      delay[1][i]=1;
  }
  leftLevel = rightLevel = 1;
  gradient = QLinearGradient(rect().topLeft(), rect().bottomRight());
  gradientBrush = QBrush(gradient);
  gradientBrush.setStyle(Qt::SolidPattern);
  backgroundBrush.setColor(Qt::white);
  backgroundBrush.setStyle(Qt::SolidPattern);
//  transparentBrush.setColor(Qt::transparent);
  pen.setColor(qRgb(0,0,0));
  decayBrush.setColor(QColor(255,0,0,255));
  decayBrush.setStyle(Qt::SolidPattern);
//  barWidth = MIN_BARWIDTH;
  barWidth = 1;
  barSpacing = 1;
  rectColor = QColor(255,255,255);
  corBarra = rand()%360;
  oldValue = 0;
  angulo = M_PI/6;
}

void Spectrograph::resizeEvent(QResizeEvent *e){
  e->accept();
  /*gradient = QLinearGradient(QPoint(rect().width()/2,0),QPoint(rect().width()/2,rect().height()/2)) ;
  gradient.setColorAt(1, Qt::black);
  gradient.setColorAt(0, Qt::white);
  gradient.setSpread(QGradient::ReflectSpread);
  gradientBrush = QBrush(gradient);*/
  barWidth = (float)width()/NUM_BANDS;
  //qDebug() << "barwidth = " << barWidth;
  widgetHeight = height();
  repaint();
}

void Spectrograph::loadLevels(double left, double right){
  if(leftLevel < 5*width()/2*left)
    leftLevel = 5*width()/2*left;
  if(rightLevel < 5*width()/2*right)
    rightLevel = 5*width()/2*right;
}

void Spectrograph::mousePressEvent(QMouseEvent *e)
{
  e->accept();
  posInicial = e->x();
}

void Spectrograph::mouseMoveEvent(QMouseEvent *e)
{
  e->accept();
  posFinal = e->x();
  angulo = posFinal - posInicial;
}

void Spectrograph::paintEvent(QPaintEvent *e){
  QPainter p(this);
  QPen pen;
  // this lines makes the color change proportionally to the bass of the music
  corBarra = (corBarra>=359)?0:corBarra+(float)spectrum[0]/(10*widgetHeight);
  rectColor.setHsv((int)corBarra,255,255);
  // it emits the new color to the other widgets
  emit trocaCor(rectColor);
  // this lines draw the gradient of the spectrum
  gradient = QLinearGradient(QPoint(rect().width()/2,0),QPoint(rect().width()/2,rect().height()/2));
  gradient.setColorAt(0, QColor(0,0,0));
  gradient.setColorAt(1, rectColor);
  gradient.setSpread(QGradient::RepeatSpread);
  gradientBrush = QBrush(gradient);
  float p1x, p1yU, p1yD, p2x; //p1yUH, p1yDH;
  e->accept();
  QEasingCurve curve;
  p.setBrush(gradientBrush);
  p.drawRect(rect());
  p.setRenderHint(QPainter::Antialiasing);
  pen.setStyle(Qt::SolidLine);
  pen.setColor(Qt::black);
  pen.setWidth(1);
  p.setPen(pen);
  //QLinearGradient gradBarra;
  //QBrush brushBarra;
  QLinearGradient cinzaWater (QPointF(0,height()/2),QPointF(width(),height()));
  cinzaWater.setColorAt(1,QColor(168,168,168,100));
  cinzaWater.setColorAt(0,QColor(208,208,208,100));
  p.setBrush(QBrush(cinzaWater));
  p.drawRect(QRectF(QPointF(0,height()/2),QPointF(width(),height())));

  // now it's time to draw the bars
  QPointF pontos[4];
  for(int i=0; i<NUM_BANDS;i++){
    p1x = (i)*barWidth;
    p2x = p1x+barWidth;
    p1yU = (widgetHeight-spectrum[i]*0.7)/2;
    p1yD = (widgetHeight+spectrum[i]*0.7)/2;
    rectColor.setHsv((int)corBarra,255,255);
    gradBarra = QLinearGradient (QPointF(p1x,p1yU),QPointF(p2x,widgetHeight/2));
    gradBarra.setColorAt(0,rectColor);
    gradBarra.setColorAt(1,Qt::black);
    brushBarra = QBrush(gradBarra);
    pen.setStyle(Qt::SolidLine);
    pen.setColor(Qt::black);
    pen.setWidth(1);
    p.setPen(pen);
    p.setBrush(brushBarra);
    // this is the vertical upper bar
    p.drawRect(QRectF(QPointF(p1x,p1yU),QPointF(p2x,widgetHeight/2)));

    gradBarra = QLinearGradient (QPointF(p1x,p1yD),QPointF(p2x,widgetHeight/2));
    rectColor.setHsv((int)corBarra,200,150);
    gradBarra.setColorAt(0,rectColor);
    gradBarra.setColorAt(1,Qt::black);
    brushBarra = QBrush(gradBarra);
    pen.setStyle(Qt::NoPen);
    pen.setColor(Qt::black);
    pen.setWidth(1);
    p.setPen(pen);
    p.setBrush(brushBarra);
    pontos[0] = QPointF(p2x,widgetHeight/2);
    pontos[1] = QPointF(p1x,widgetHeight/2);
    pontos[2] = QPointF(p1x-(p1yD-widgetHeight/2)*tan(M_PI/6),p1yD);
    //pontos[2] = QPointF(p1x-barWidth,p1yD);
    //pontos[3] = QPointF(p1x,p1yD);
    pontos[3] = QPointF(p1x-(p1yD-widgetHeight/2)*tan(M_PI/6)+barWidth,p1yD);
    // this is the below and inclined bar
    p.drawConvexPolygon(pontos,4);
    }
  oldValue = spectrum[0];
}

void Spectrograph::timerEvent(QTimerEvent *e){
  // accepts the event (boring messages are not displayed)
  e->accept();
  // processes all spectrum bands
  for(int i=0; i<NUM_BANDS; i++){
    // make the spectrum bar smaller
    spectrum[i]-=delay[0][i];
    // but not with negative size
    if(spectrum[i] <0 ){
      spectrum[i]=0;
    }
    // waits for a while while gravity forces start to act
    // it simulates a small stop when the the bar reachs its
    // maximum
    //
    // however, this only work for the decay bars that stay
    // at the top of each spectrum bar
    //
    // gravity stop does not affect the spectrum bars
    if(gravity[i] < 0.8){
      // make decay bar lower
      decay[i]-=delay[1][i];
      // but not too lower
      if(decay[i] < 0){
        decay[i] = 0;
      }
      // increments decay bar reduction
      delay[1][i]++;
    }
    delay[0][i]++;
    // reduces gravity for that decay bar (not the spectrum bar)
    gravity[i] = gravity[i]*0.98;
  }
  // reduces left and right audio mean level
  // please note that this reduction is linear
  if(leftLevel > 0)
    leftLevel--;
  if(rightLevel > 0)
    rightLevel--;
  // repaint the screen
  repaint();
}

void Spectrograph::loadSamples(QVector<double> &_spectrum){
  int incr, value;
  incr = _spectrum.size()/NUM_BANDS;
  newSpectrum=_spectrum;
  for(int i=0; i<NUM_BANDS;i++){
    value = ceil(newSpectrum[i*incr]*height());
    if(value > spectrum[i]){
      spectrum[i] = value;
      delay[0][i] = 0;
      delay[1][i] = 0;
    }
    if(value > decay[i]){
      decay[i]    = value;
      gravity[i]  = 1;
      delay[1][i] = 0;
    }
  }
  repaint();
}
