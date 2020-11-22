import pyaudio
import numpy as np
import aubio
from collections import deque
from RF24 import RF24
import pygame   

# Global variables
n_programs = 10
program_start = 0
program_musical_start = 6
program = program_start
button_delta = 2
# allowed sample rates 44100 32000 22050 16000
# samplerate, win_s = 44100, 4096
samplerate, win_s = 16000, 1024 * 4
buffer_size = hop_s = win_s/4
n_energies = 20 
tolerance = 0.8 # pitch
pitch_samples = 3
energy_samples = 1 
energies_decay = 1-1./(10*samplerate/buffer_size)

# Joystick setup
pygame.init()
pygame.joystick.init()
joysticks = []
for i in range(0, pygame.joystick.get_count()):
    joysticks.append(pygame.joystick.Joystick(i))
    joysticks[-1].init()
    print ("Detected joystick "),joysticks[-1].get_name(),"'"
# 1, 2, 3, 4, L1, R1, L2, R2, Start, Select
button_states = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
program_states = [0, 220, 40, 150, 0]

# Radio setup
radio = RF24(25, 0);
pipes = 0xF0F0F0F0E1

radio.begin()
radio.setRetries(0,0)
radio.disableCRC()
radio.enableDynamicPayloads()
radio.setAutoAck(False)
radio.setChannel(90)
radio.openWritingPipe(pipes)
radio.stopListening()

# Audio setup
p = pyaudio.PyAudio()
pyaudio_format = pyaudio.paFloat32
n_channels = 1
stream = p.open(format=pyaudio_format,
                channels=n_channels,
                rate=samplerate,
                input=True,
                frames_per_buffer=buffer_size)

# setup pitch
pitch_o = aubio.pitch("default", win_s, hop_s, samplerate)
pitch_o.set_unit("midi")
pitch_o.set_tolerance(tolerance)
pitch = 0
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
        try:
            button_states[event.button] = (event.type 
                                            == pygame.JOYBUTTONDOWN)
            if button_states[8]:
                program += 1
                if program>=(n_programs+program_start):
                    program = program_start            
            program_states[0] = int(button_states[9])
        except:
            print('Problem reading joystick')
            
    if button_states[0] and program_states[1]<255-button_delta:
        program_states[1] += button_delta
    if button_states[2] and program_states[1]>=button_delta:
        program_states[1] -= button_delta
    if button_states[1] and program_states[2]<255-button_delta:
        program_states[2] += button_delta
    if button_states[3] and program_states[2]>=button_delta:
        program_states[2] -= button_delta
    if button_states[4] and program_states[3]<255-button_delta:
        program_states[3] += button_delta
    if button_states[6] and program_states[3]>=button_delta:
        program_states[3] -= button_delta
    if button_states[5] and program_states[4]<255-button_delta:
        program_states[4] += button_delta
    if button_states[7] and program_states[4]>=button_delta:
        program_states[4] -= button_delta
    
while True:
    try:
        get_signal()
        process_input()
        message = [program, program_states[0], program_states[1],
                    program_states[2], program_states[3], 
                    program_states[4], 0]
        if program >= program_musical_start:
            for e in energies:
                message.append(e)
            message.append(pitch)
                     
        send_arduino(message)
        
    except KeyboardInterrupt:
        print("*** Ctrl+C pressed, exiting")
        break

stream.stop_stream()
stream.close()
p.terminate()
