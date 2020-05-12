import numpy as np
import serial
import time

waitTime = 0.1
num = 0
Song_Length = 42

song_py = [[0 for i in range(42)] for j in range(3)]
noteLength_py = [[0 for i in range(42)] for j in range(3)]

song_py[0] = [
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261,
  392, 392, 349, 349, 330, 330, 294,
  392, 392, 349, 349, 330, 330, 294,
  261, 261, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 261]

noteLength_py[0] = [
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2,
  1, 1, 1, 1, 1, 1, 2]

song_py[1] = [
  261, 261, 261, 261, 261, 261, 261,
  293, 293, 293, 293, 293, 293, 293,
  329, 329, 329, 329, 329, 329, 329,
  349, 349, 349, 349, 349, 349, 349,
  392, 392, 392, 392, 392, 392, 392,
  440, 440, 440, 440, 440, 440, 440]
  
noteLength_py[1] = [
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1] 
  
song_py[2] = [
    440, 440, 440, 440, 440, 440, 440,
    392, 392, 392, 392, 392, 392, 392,
    349, 349, 349, 349, 349, 349, 349,
    329, 329, 329, 329, 329, 329, 329,
    293, 293, 293, 293, 293, 293, 293,
    261, 261, 261, 261, 261, 261, 261]
    
noteLength_py[2] = [
  2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2]

# send the waveform table to K66F
serdev = '/dev/ttyACM1'
s = serial.Serial(serdev)

# output formatter
formatter = lambda x: "%d" % x


while (1):
  line = s.readline() # Read an echo string from K66F terminated with '\n'
  # print line
  num = num + int(line)
  if (num < 0):   
      num = 0
  elif (num > 2):
      num = 2

  for i in range(Song_Length):
    s.write(bytes(formatter(song_py[num][i]), 'UTF-8'))
    time.sleep(waitTime)

  for i in range(Song_Length):
    s.write(bytes(formatter(noteLength_py[num][i]), 'UTF-8'))
    time.sleep(waitTime)

# num = 0
# for i in range(Song_Length):
#   s.write(bytes(formatter(song_py[num][i]), 'UTF-8'))
#   time.sleep(waitTime)

# for i in range(Song_Length):
#   s.write(bytes(formatter(noteLength_py[num][i]), 'UTF-8'))
#   time.sleep(waitTime)

s.close()