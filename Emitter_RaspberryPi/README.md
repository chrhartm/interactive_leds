# Setup

Notes on setup

```
sudo crontab -e
@reboot sleep 30 && python /home/<USER>/Desktop/interactive_leds/Emitter_RaspberryPi/main.py >> /home/<USER>/Desktop/interactive_leds/Emitter_RaspberryPi/cron.log 2>&1```


# Notes for bluetooth

To pair a new controller
* sudo bluetoothctl
* agent on
* scan on
* pair XX:XX:XX...

# Notes for audio

* aubio https://aubio.org/ was used for sonic runway
* https://jonamiki.com/2019/07/04/sound-on-raspberry-pi-separate-speaker-and-microphone/
* https://aubio.readthedocs.io/en/latest/python.html
* `arecord -l`
* `aplay -l`
```

```
pip install aubio
sudo apt-get install portaudio19-dev python-pyaudio
```
