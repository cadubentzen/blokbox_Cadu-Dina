#ifndef CONTROLS_H
#define CONTROLS_H
#include "abstractcontrol.h"

#include <QTime>
#include <QIcon>

namespace Ui {
class Controls;
}
/**
 * @brief The Controls class provides support to user control
 * @details This class allows to programm the interface for
 * dealing with play/pause/prev/next/volume/elapsed time commands
 */
class Controls : public AbstractControl{
    Q_OBJECT
public:
  /**
     * @brief Class constructor
     * @param parent is the pointer to parent widget (it is supposed to be the place
     * where this widget has to be drawn)
     */
    explicit Controls(QWidget *parent = 0);

  /**
    * @brief Class destructor, to clean up some mess
    */
    ~Controls();

private:
  /**
    * @brief ui access the form that draws the controls user interface
    */
    Ui::Controls *ui;

    /**
     * @brief duration stores the total amount of time of current media
     * that is playing
     */
    qint64 duration;
    /**
     * @brief play and pause icons for alternating when user media plays or pause
     */
    QIcon playIcon, pauseIcon;
public slots:
    /**
     * @brief onPlayPauseClicked is activated when the user presses the play/pause button
     */
    void onPlayPauseClicked(void);
    /**
     * @brief onPrevClicked is activated when the user presses prev button
     */
    void onPrevClicked(void);
    /**
     * @brief onNextClicked is activated when the user presses next button
     */
    void onNextClicked(void);
    /**
     * @brief onVolumeChanged is activated when the user turns the dial button
     * @param value is passed to this function by some valueChanged signal
     */
    void onVolumeChanged(int value);
    /**
     * @brief onElapsedChanged is signal activated periodically by mainwindow widget
     * @details every a new part of the song is played, the stream position changes.
     * This position is comunicated to controls widget using some specific signal.
     *
     * We need two overloaded function, since the scrollbar widget only deals with integer
     * values and the signal provided by media player offers a 64 bits integer :p
     * @param value receives the elapsed time
     */
    void onElapsedChanged(qint64 value);
    /**
     * @brief onDurationChanged slot is called when a new media arives from the playlist
     * @details When a new media starts to play, mainwindow tells this widget the total amount
     * of time associated to this media.
     * @param value stores the amount of time for the current media
     */
    void onDurationChanged(qint64 value);
    /**
     * @brief onPlayerStateChanged deals with the state of play/pause button
     * @details The developer may want to state different icons for each state
     * @param state stores play/pause state
     * - true: Player is on playing state
     * - false: Player is on paused state
     */
    void onPlayerStateChanged(bool state);

    void onColorChanged(QColor cor);
protected slots:
    /**
     * @brief onSliderReleased is called when the user releases the duration slider
     * @details When this slider is released, the widget tells mainwindow that user
     * selected a new part of the song to play.
     */
    void onSliderReleased();
signals:
    /**
     * @brief playPause is emitted when the user presses the play/pause button
     */
    void playPause();
    /**
     * @brief next signal is emitted when the user presses next button
     */
    void next();
    /**
     * @brief prev signal is emitted when the user presses prev button
     */
    void prev();
    /**
     * @brief volumeSelected signal is emitted when the user selects a new volume
     */
    void volumeSelected(int);
    /**
     * @brief elapsedSelected tells qt which part of the song the user selected to play
     */
    void elapsedSelected(qint64);
};

#endif // CONTROLS_H
