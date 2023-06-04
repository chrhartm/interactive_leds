import pyaudio
import numpy as np
import aubio
from collections import deque
from RF24 import RF24
import pygame
import time
import imp
import subprocess

# Global parameters
n_programs = 10
program_start = 0
program_musical_start = 6
program = program_start
button_delta = 2
joystick_timeout = 30 # in minutes?
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

# Joystick variables
joysticks = []
# 1, 2, 3, 4, L1, R1, L2, R2, Start, Select
button_states = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
program_states = [0, 220, 40, 150, 0]

# Radio variables
radio = RF24(25, 0);

def init_bluetooth():
    global bluetoothprocess
    bluetoothprocess = subprocess.Popen(['bluetoothctl'],
                                        shell=False,
                                        stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE)
    time.sleep(0.02)
    bluetoothprocess.stdin.write('agent on\n')
    time.sleep(0.02)
    bluetoothprocess.stdin.write('connect DB:F6:AB:36:77:85\n')
    time.sleep(1)
    bluetoothprocess.stdin.write('connect FF:6C:F2:BA:E9:AA\n')
    time.sleep(1)
    bluetoothprocess.stdin.write('exit\n')
    print(bluetoothprocess.stdout.read())

def init_joystick():
    global joysticks, joystick_lastactive
    # Joystick setup
    pygame.joystick.quit()
    pygame.quit()
    pygame.init()
    pygame.joystick.init()

    for i in range(0, pygame.joystick.get_count()):
        joysticks.append(pygame.joystick.Joystick(i))
        joysticks[-1].init()
        joystick_lastactive = time.time()
        print ("Detected joystick "),joysticks[-1].get_name(),"'"

def init_radio():
    # Radio setup
    pipes = 0xF0F0F0F0E1
    radio.begin()
    radio.setRetries(0,0)
    radio.disableCRC()
    radio.enableDynamicPayloads()
    radio.setAutoAck(False)
    radio.setChannel(90)
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
    global program, button_states, program_states
    for event in pygame.event.get():
        joystick_lastactive = time.time()
        # print(event)
        if(event.type == pygame.JOYBUTTONDOWN or event.type == pygame.JOYBUTTONUP):
            if event.type == pygame.JOYBUTTONUP:
                value = False
            else:
                value = True
            if event.button in [6,7,8,9,10,11,12]:
                button_states[8] = value
            if event.button == 6:
                program = 0
            elif event.button == 7:
                program = 1
            elif event.button == 8:
                pass
                program = (program+1)%program_musical_start
            elif event.button == 9:
                program = 2
            elif event.button == 10:
                program = 3
            elif event.button == 11:
                program = 4
            elif event.button == 12:
                program = 5
            elif event.button in [4,5,13,14]:
                button_states[9] = value
            elif event.button == 0:
                button_states[1] = value
            elif event.button == 1:
                button_states[3] = value
            elif event.button == 2:
                button_states[2] = value
            elif event.button == 3:
                button_states[0] = value

        if event.type == pygame.JOYAXISMOTION:
            # Mirror right button logic on same layout
            if event.axis==3 and event.value < 0:
                button_states[0] = True
            if event.axis==3 and event.value > 0:
                button_states[1] = True
            if event.axis == 2 and event.value < 0:
                button_states[2] = True
            if event.axis == 2 and event.value > 0:
                button_states[3] = True
            if event.axis == 3 and event.value == 0:
                button_states[1] = button_states[0] = False
            if event.axis == 2 and event.value == 0:
                button_states[2] = button_states[3] = False
            # Joy hat logic
            if event.axis==0 and event.value < 0:
                button_states[5] = True
            if event.axis==0 and event.value > 0:
                button_states[4] = True
            if event.axis == 1 and event.value < 0:
                button_states[6] = True
            if event.axis == 1 and event.value > 0:
                button_states[7] = True
            if event.axis == 0 and event.value == 0:
                button_states[4] = button_states[5] = False
            if event.axis == 1 and event.value == 0:
                button_states[7] = button_states[6] = False          
        if event.type == pygame.JOYHATMOTION:
            if event.value == (-1, 0):
                button_states[5] = True
            if event.value == (1, 0):
                button_states[4] = True
            if event.value == (0, -1):
                button_states[7] = True
            if event.value == (0, 1):
                button_states[6] = True
            if event.value == (0, 0):
                button_states[4] = button_states[5] = False
                button_states[6] = button_states[7] = False
                
    if button_states[0] and program_states[1]<255-button_delta:
        program_states[1] += button_delta
    if button_states[1] and program_states[1]>=button_delta:
        program_states[1] -= button_delta
    if button_states[2] and program_states[2]<255-button_delta:
        program_states[2] += button_delta
    if button_states[3] and program_states[2]>=button_delta:
        program_states[2] -= button_delta
    if button_states[4] and program_states[3]<255-button_delta:
        program_states[3] += button_delta
    if button_states[5] and program_states[3]>=button_delta:
        program_states[3] -= button_delta
    if button_states[6] and program_states[4]<255-button_delta:
        program_states[4] += button_delta
    if button_states[7] and program_states[4]>=button_delta:
        program_states[4] -= button_delta
    if button_states[9]:
        program_states[0] = 1
    else:
        program_states[0] = 0

def main_loop():
    global joysticks, joystick_lastactive, joystick_timeout
    
    while True:
        if(audio):
            get_signal()
        else:
            time.sleep(0.02)
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
            joysticks = []
            print("lost joystick")
            break
            

    

if (__name__ == '__main__'):
    init_bluetooth()
    init_radio()
    init_audio()
    if (not audio):
        n_programs = program_musical_start
    while True:
        try:
            init_joystick()
            if len(joysticks)<1:
                time.sleep(1)
                pygame.joystick.quit()
            else:
                main_loop()
        except KeyboardInterrupt:
            print("*** Ctrl+C pressed, exiting")
            break
    
    bluetoothprocess.terminate()
    stream.stop_stream()
    stream.close()
    p.terminate()
