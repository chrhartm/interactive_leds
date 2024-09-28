audio = False
if audio:
    import pyaudio
    import aubio

import numpy as np
from collections import deque
from pyrf24 import RF24, RF24_PA_HIGH, RF24_PA_LOW #max too max
import time
import subprocess
import os
# Needed for pygame when running from cron
os.environ['SDL_VIDEODRIVER'] = 'dummy'
import pygame


# Global parameters
n_programs = 10
program_start = 0
program_musical_start = 6
program = program_start
button_delta = 2
joystick_timeout = 30 # in seconds
joystick_lastactive = time.time()
# allowed sample rates 44100 32000 22050 16000
# samplerate, win_s = 44100, 4096
samplerate, win_s = 44100, 4096 * 4
buffer_size = hop_s = win_s/4
n_energies = 20 
tolerance = 0.8 # pitch
pitch_samples = 3
energy_samples = 1 
energies_decay = 1-1./(10*samplerate/buffer_size)

# SNES controller variables
snes_controller = None
button_states = [0] * 8  # SNES controller has 8 buttons
program_states = [0, 220, 40, 150, 0]

# Radio variables
radio = RF24(25, 0);

def init_snes_controller():
    global snes_controller, joystick_lastactive
    pygame.init()
    pygame.joystick.init()
    
    if pygame.joystick.get_count() > 0:
        snes_controller = pygame.joystick.Joystick(0)
        snes_controller.init()
        joystick_lastactive = time.time()
        print("SNES controller initialized:", snes_controller.get_name())
    else:
        print("No SNES controller found")

def init_radio():
    # Radio setup
    pipes = 0xF0F0F0F0E1
    radio.begin()
    radio.setRetries(0,0)
    radio.disableCRC()
    radio.enableDynamicPayloads()
    radio.setAutoAck(False)
    radio.setChannel(90)
    radio.setPALevel(RF24_PA_LOW, 1)
    radio.openWritingPipe(pipes)
    radio.stopListening()

def init_audio():
     # Audio setup
    global p, audio, stream
    try:
        p = pyaudio.PyAudio()
        pyaudio_format = pyaudio.paFloat32
        n_channels = 1
        stream = p.open(format=pyaudio_format,
                        channels=n_channels,
                        rate=samplerate,
                        input=True,
                        frames_per_buffer=buffer_size)
    except:
       audio = False
       return
    

    # setup pitch
    pitch_o = aubio.pitch("default", win_s, hop_s, samplerate)
    pitch_o.set_unit("midi")
    pitch_o.set_tolerance(tolerance)
    pitchque = deque([], pitch_samples)

    # setup filter
    f = aubio.filterbank(40, win_s) # documentation says 40 for mel coeffs
    f.set_mel_coeffs_slaney(samplerate)
    pv = aubio.pvoc(win_s, hop_s)
    energies_raw = np.zeros((energy_samples, n_energies))
    energies_max = np.zeros(n_energies)
    energies = np.zeros(n_energies)

def get_signal():
    global pitch, energies_raw, energies, energies_max
    try:
        audiobuffer = stream.read(buffer_size)
        signal = np.fromstring(audiobuffer, dtype=np.float32)

        pitch_raw = int(pitch_o(signal)[0])%255
        pitchque.append(pitch_raw)
        pitch = int(((np.median(pitchque)-30)*5)%255)
        
        energies_raw = np.roll(energies_raw, 1, axis=0)
        fftgrain = pv(signal)
        energies_40 = f(fftgrain)
        # calc has to match to n_energies!
        energies_raw[0] = [(energies_40[i*2]+energies_40[i*2+1]/2) 
                            for i in range(n_energies)]
        energies = np.mean(energies_raw, axis=0)
        energies_max = np.maximum(energies, energies_max) # pointwise
        energies = (energies/energies_max * 255).astype(int)
        energies[energies<0]=0
        if sum(energies_max)/len(energies_max)>0.1:
            energies_max *= energies_decay
                        
    except Exception:
        # print(Exception)
        # Buffer can overflow
        pass
    
    return
    
def send_arduino(message):
    message[6] = ((message[0]+message[1]+message[2]+message[3]+
                  message[4]+message[5])%255)
    radio.write(bytearray(message))
    
def process_input():
    global program, button_states, program_states, joystick_lastactive
    for event in pygame.event.get():
        joystick_lastactive = time.time()
        if event.type == pygame.JOYBUTTONDOWN or event.type == pygame.JOYBUTTONUP:
            button_value = event.type == pygame.JOYBUTTONDOWN
            if event.button < 8:  # SNES controller has 8 buttons
                button_states[event.button] = button_value
        
        if event.type == pygame.JOYAXISMOTION:
            # D-pad on SNES controller is typically mapped to axes
            if event.axis == 0:  # Left-Right
                button_states[4] = event.value > 0.5  # Right
                button_states[5] = event.value < -0.5  # Left
            elif event.axis == 1:  # Up-Down
                button_states[6] = event.value < -0.5  # Up
                button_states[7] = event.value > 0.5  # Down

    # Map SNES buttons to program states
    if button_states[0]:  # A button
        program = (program + 1) % n_programs
    if button_states[1]:  # B button
        program = 0
    if button_states[2]:  # X button
        program_states[0] = 1
    else:
        program_states[0] = 0
    
    if button_states[4] and program_states[1]<255-button_delta:
        program_states[1] += button_delta
    if button_states[5] and program_states[1]>=button_delta:
        program_states[1] -= button_delta
    if button_states[6] and program_states[2]<255-button_delta:
        program_states[2] += button_delta
    if button_states[7] and program_states[2]>=button_delta:
        program_states[2] -= button_delta

def main_loop():
    global joystick_lastactive, joystick_timeout
    
    while True:
        if(audio):
            get_signal()
        else:
            time.sleep(0.05)
        process_input()
        message = [program, program_states[0], program_states[1],
                    program_states[2], program_states[3], 
                    program_states[4], 0]
        if program >= program_musical_start:
            for e in energies:
                message.append(e)
            message.append(pitch)
        
        send_arduino(message)
        
        if (time.time() - joystick_lastactive > joystick_timeout):
            print("lost SNES controller")
            break
            

if (__name__ == '__main__'):
    print("Initializing...")
    init_radio()
    print("Radio initialized")
    # init_audio()
    if (not audio):
        n_programs = program_musical_start
    while True:
        try:
            init_snes_controller()
            if snes_controller is None:
                time.sleep(1)
                pygame.joystick.quit()
            else:
                main_loop()
        except KeyboardInterrupt:
            print("*** Ctrl+C pressed, exiting")
            break
    if (audio):
        stream.stop_stream()
        stream.close()
        p.terminate()
