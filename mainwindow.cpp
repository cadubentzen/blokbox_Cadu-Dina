#include "mainwindow.h"
#include "ui_mainwindow.h"

// constructor: warm up all stuff
MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
//,audioInfo(QAudioDeviceInfo::defaultInputDevice())
{
  // draws the ui
  ui->setupUi(this);

  // test for saving settings
  QCoreApplication::setOrganizationName("Agostinho");

  /** some settings attempt
   */
  QSettings settings; /*!<aloha */
  settings.setValue("alo","maria");

  // defines sample size equals to spectrum size
  sample.resize(SPECSIZE);

  // threads are as separate processes running within the same
  // program. for fft calculation, it is better to move it
  // to another thread to make the calcs faster.
  // moreover, it will not slow down the ui
//  fftThread = new QThread(this);

  calculator = new FFTCalc();
 // calculator->moveToThread(fftThread);

  // launches the new media player
  player = new QMediaPlayer();

  // starts a new playlist
  playlist = new QMediaPlaylist();

  // starts the playlist model
  playlistModel = new PlaylistModel(this);

  // tell playlistmodel where is the playlist
  playlistModel->setPlaylist(playlist);

  // attach the listView to the playlistModel
  ui->listViewPlaylist->setModel(playlistModel);

  // set current index to the first element
  ui->listViewPlaylist->setCurrentIndex(playlistModel->index(playlist->currentIndex(), 0));

  loadPlaylist();

  // attachs the playlist to the player
  player->setPlaylist(playlist);

  // playlist plays in loop mode. It restarts after last song has finished playing.
  playlist->setPlaybackMode(QMediaPlaylist::Loop);

  // this allow the user to select the media it wants to play
  connect(ui->listViewPlaylist, SIGNAL(doubleClicked(QModelIndex)),
          this, SLOT(goToItem(QModelIndex)));

  // if some metadata changed for media, display it somewhere
  // it seems not work on windows
  // but works for linux :)
  connect(player,SIGNAL(metaDataChanged()),
          this, SLOT(metaDataChanged()));

  // the media status changed (new stream has arrived)
  connect(player, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
          this, SLOT(mediaStatusChanged(QMediaPlayer::MediaStatus)));

  // the user selected a new position on music to play
  // perharps using some scrollbar
  connect(this,SIGNAL(positionChanged(qint64)),
          player,SLOT(setPosition(qint64)));

  connect(player,SIGNAL(volumeChanged(int)),
                        ui->control,SLOT(onVolumeChanged(int)));

  connect(player,SIGNAL(stateChanged(QMediaPlayer::State)),
          SLOT(mediaStateChanged(QMediaPlayer::State)));
  // that is the audio probe object that "listen to"
  // the music. It will help with fft stuff
  probe = new QAudioProbe();

  // fft is delivered using a QVector<double> but
  // signal/slot scheme does not recognizes this type by default
  // therefore, we have to register it
  qRegisterMetaType< QVector<double> >("QVector<double>");

  // here goes the control unit event handlers
  connect(ui->control, SIGNAL(playPause()), this, SLOT(playPause()));
  connect(ui->control, SIGNAL(prev()), this, SLOT(prev()));
  connect(ui->control, SIGNAL(next()), this, SLOT(next()));
  connect(this, SIGNAL(playPauseChanged(bool)),
          ui->control,SLOT(onPlayerStateChanged(bool)));

  // when the music position changes on player, it has to be
  // informed to the control unit to redraw it ui
  connect(player, SIGNAL(positionChanged(qint64)),
          ui->control,SLOT(onElapsedChanged(qint64)));

  // fft goes here...
  // if a new audio buffer is ok, we have to make some
  // calcs (fft) to display the spectrum
  connect(probe, SIGNAL(audioBufferProbed(QAudioBuffer)),
          this, SLOT(processBuffer(QAudioBuffer)));

  // when fft is available, we deliver it to
  // the visualization widget
  connect(this,  SIGNAL(spectrumChanged(QVector<double>&)),
          ui->visualizer,SLOT(loadSamples(QVector<double>&)));

  // communicate the left and right audio levels...
  // ...mean levels
  connect(this,  SIGNAL(levels(double,double)),
          ui->visualizer,SLOT(loadLevels(double,double)));

  // when fft is available, we deliver it to
  // the visualization widget
  //connect(this,  SIGNAL(spectrumChanged(QVector<double>&)),
  //       ui->glVisualizer,SLOT(loadSamples(QVector<double>&)));

  // communicate the left and right audio levels...
  // ...mean levels
  //connect(this,  SIGNAL(levels(double,double)),
  //       ui->glVisualizer,SLOT(loadLevels(double,double)));

  // if the user selected a new position on stream to play
  // we have to tell it to the player
  connect(ui->control, SIGNAL(elapsedSelected(qint64)),
          player, SLOT(setPosition(qint64)));

  // changing audio volume
  connect(ui->control, SIGNAL(volumeSelected(int)),
          player, SLOT(setVolume(int)));

  // calculator is the thead that calcs the ffts we need to display
  // every time a new spectrum is available, the calculator
  // emits a calculatedSpectrum signal
  connect(calculator, SIGNAL(calculatedSpectrum(QVector<double>)),
          this, SLOT(spectrumAvailable(QVector<double>)));

  connect(ui->library,SIGNAL(addMediaToPlayList(QString)),
          SLOT(onAddMediaToPlayList(QString)));

  // tells the probe what to probe
  probe->setSource(player);

  // load directories to library
  connect(ui->actionLoadDirectory,SIGNAL(triggered()),this,SLOT(onAddFolderToLibrary()));

  // load a single file to library
  connect(ui->actionLoadFile,SIGNAL(triggered()),this,SLOT(loadMedia()));

  // it connects the signals emiteds via the visualizer to the buttons (ui->control) and the lightCycle(ui->widgetInfo)
  connect(ui->visualizer,SIGNAL(trocaCor(QColor)),ui->control,SLOT(onColorChanged(QColor)));
  connect(ui->visualizer,SIGNAL(trocaCor(QColor)),ui->widgetInfo,SLOT(changedColor(QColor)));

  //this->setStyleSheet(QString("QMainWindow {background-color: black}"));
}

void MainWindow::goToItem(const QModelIndex &index){
  if (index.isValid()) {
    playlist->setCurrentIndex(index.row());
    player->play();
  }
}

// prepares the playlist to display the media to be played
// it should be stored into a file with settings.
// I will let this to a far future
void MainWindow::loadPlaylist(void){
}

void MainWindow::onAddFolderToLibrary(){
  QFileDialog d;
  QString folder;
  folder = d.getExistingDirectory(this,tr("Select Folder"),
                                  QDir::homePath(),
                                  QFileDialog::ShowDirsOnly |
                                  QFileDialog::DontResolveSymlinks);
  ui->library->addToSearchPath(folder);
}

void MainWindow::onAddMediaToPlayList(QString media){
  playlist->addMedia(QUrl::fromLocalFile(media));
}

// process audio buffer for fft calculations
void MainWindow::processBuffer(QAudioBuffer buffer){
  qreal peakValue;
  int duration;

  if(buffer.frameCount() < 512)
    return;

  // return left and right audio mean levels
  levelLeft = levelRight = 0;
  // It only knows how to process stereo audio frames
  // mono frames = :P
  if(buffer.format().channelCount() != 2)
    return;

  sample.resize(buffer.frameCount());
  // audio is signed int
  if(buffer.format().sampleType() == QAudioFormat::SignedInt){
    QAudioBuffer::S16S *data = buffer.data<QAudioBuffer::S16S>();
    // peak value changes according to sample size.
    if (buffer.format().sampleSize() == 32)
      peakValue=INT_MAX;
    else if (buffer.format().sampleSize() == 16)
      peakValue=SHRT_MAX;
    else
      peakValue=CHAR_MAX;

    // scale everything to [0,1]
    for(int i=0; i<buffer.frameCount(); i++){
      // for visualization purposes, we only need one of the
      // left/right channels
      sample[i] = data[i].left/peakValue;
      levelLeft+= abs(data[i].left)/peakValue;
      levelRight+= abs(data[i].right)/peakValue;
    }
  }

  // audio is unsigned int
  else if(buffer.format().sampleType() == QAudioFormat::UnSignedInt){
    QAudioBuffer::S16U *data = buffer.data<QAudioBuffer::S16U>();
    if (buffer.format().sampleSize() == 32)
      peakValue=UINT_MAX;
    else if (buffer.format().sampleSize() == 16)
      peakValue=USHRT_MAX;
    else
      peakValue=UCHAR_MAX;
    for(int i=0; i<buffer.frameCount(); i++){
      sample[i] = data[i].left/peakValue;
      levelLeft+= abs(data[i].left)/peakValue;
      levelRight+= abs(data[i].right)/peakValue;
    }
  }

  // audio is float type
  else if(buffer.format().sampleType() == QAudioFormat::Float){
    QAudioBuffer::S32F *data = buffer.data<QAudioBuffer::S32F>();
    peakValue = 1.00003;
    for(int i=0; i<buffer.frameCount(); i++){
      sample[i] = data[i].left/peakValue;
      // test if sample[i] is infinity (it works)
      // some tests produced infinity values :p
      if(sample[i] != sample[i]){
        sample[i] = 0;
      }
      else{
        levelLeft+= abs(data[i].left)/peakValue;
        levelRight+= abs(data[i].right)/peakValue;
      }
    }
  }
  // if the probe is listening to the audio
  // do fft calculations
  // when it is done, calculator will tell us
  if(probe->isActive()){
    duration = buffer.format().durationForBytes(buffer.frameCount())/1000;
    //qDebug() << "duracao =" << duration;
    calculator->calc(sample, duration);
  }
  // tells anyone interested about left and right mean levels
  emit levels(levelLeft/buffer.frameCount(),levelRight/buffer.frameCount());
}

// what to do when fft spectrum is available
void MainWindow::spectrumAvailable(QVector<double> spectrum){
  // just tell the spectrum
  // the visualization widget will catch the signal...
  emit spectrumChanged(spectrum);
}

// destructor... clear all mess
MainWindow::~MainWindow(){
  //stops the player
  player->stop();

  // wait for the calculator to stop
  //calculator.wait();
  // delete the fftTrhead
  delete calculator;
//  fftThread->exit();
//  fftThread->wait();
//  delete fftThread;

  // finish the ui
  delete ui;
}

void MainWindow::loadMedia(){
  // mime database to detect file type
  QMimeDatabase db;

  // the mime type (to test if it is an audio file
  QMimeType type;

  // file list to be inserted into playlist
  QStringList filelist;

  // audio file to be opened
  QFileDialog d;
  filelist = d.getOpenFileNames(this,tr("Open File"),
                                "/home",
                                tr("Audio (*.wav *.mp3 *.ogg *.flac)"));

  // retrieve mime type
  for(QList<QString>::const_iterator it=filelist.begin(); it!= filelist.end(); it++){
    type = db.mimeTypeForFile(*it);
    // test if the file is an audio file
    // if yes, send it to the playlist
    if(type.name().startsWith("audio")){
      playlist->addMedia(QUrl::fromLocalFile(*it));
    }
  }
}

// play the previous song
void MainWindow::prev(){
    playlist->previous();
}

// play next song
void MainWindow::next(){
    playlist->next();
}


void MainWindow::setMediaAt(qint32 percent){
  if(percent < 0){
    percent = 0;
  }
  if(percent > 100){
    percent = 100;
  }
  emit positionChanged(percent*player->duration()/100);
}

void MainWindow::mediaStateChanged(QMediaPlayer::State state){
  if(state == QMediaPlayer::PlayingState){
    emit playPauseChanged(true);
  }
  else{
    emit playPauseChanged(false);
  }
}

// new song arriving
void MainWindow::mediaStatusChanged(QMediaPlayer::MediaStatus status){
  Q_UNUSED(status);
  ui->control->onDurationChanged(player->duration());
}

// this is for windows compilations
// display the song info

// and now the linux one
// display the song info
// void QMediaObject::metaDataChanged(const QString & key, const QVariant & value) [signal]

void MainWindow::metaDataChanged(){
  if(player->isMetaDataAvailable()){
    ui->widgetInfo->setAtribute("AlbumArtist",player->metaData("AlbumArtist").toString());
    ui->widgetInfo->setAtribute("Title",player->metaData("Title").toString());
    ui->widgetInfo->setAtribute("AlbumTitle",player->metaData("AlbumTitle").toString());
    ui->widgetInfo->setAtribute("TrackNumber",player->metaData("TrackNumber").toString());
    ui->widgetInfo->setAtribute("AudioBitRate",player->metaData("AudioBitRate").toString());
    ui->widgetInfo->setAtribute("AudioCodec",player->metaData("AudioCodec").toString());
  }
  // pehraps lots of metadata may be available, only the above are passed ahead
  //  ui->widgetInfo->setAtribute(key,variant.toString());*/
}

// sets up the volume
void MainWindow::setVolume(int volume){
  player->setVolume(volume);
}

// deal with play/pause button
// no explanation needed here
void MainWindow::playPause(){
  if(player->state() == (QMediaPlayer::PausedState|QMediaPlayer::StoppedState))
    player->play();
  else
    player->pause();
}
