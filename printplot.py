import matplotlib.pyplot as plt
import numpy as np


filename = input("Input name of the text file containing timestamp,temp pairs:")
f = open(filename)

temps = []
plt.title("Temperature over time")
plt.xlabel("Time in seconds")
plt.ylabel("Temperature in Fahrenheit")
for line in f:
    data = line.split(",")

    temps.append(int(data[1]))

ypoints = np.array(temps)
xpoints = np.array([i for i,x in enumerate(temps)])

plt.plot(xpoints,ypoints)
plt.show()


