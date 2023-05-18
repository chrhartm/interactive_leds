# Setup

Notes on setup

```
sudo crontab -e
reboot python /home/pi/Desktop/interactive_leds/Emitter_RaspberryPi/main.py &


sudo nano /etc/xdg/autostart/myapp.desktop
Exec=lxterminal --command”/bin/bash -c ‘python ~/Desktop/interactive_leds/Emitter_RaspberryPi/main.py; /bin/bash’”

arecord -l
aplay -l
```

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

```
pip install aubio
sudo apt-get install portaudio19-dev python-pyaudio
```