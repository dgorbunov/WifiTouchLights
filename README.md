# Networked RGB Touch Lamps

![Finished Lamp](https://github.com/dgorbunov/WifiTouchLights/blob/master/Photos/IMG_3224.jpg)
![Finished Lamp](https://github.com/dgorbunov/WifiTouchLights/blob/master/Photos/IMG_3223.jpg)

I made a set of networked rgb capactive lights this summer, which are similar to [Filimin](https://filimin.com/) lights for my family and they loved them. I was inspired to do this project when I came across this [instructable](https://www.instructables.com/id/Networked-RGB-Wi-Fi-Decorative-Touch-Lights/) written by the Filimin founder. The assembly of the lamp loosely follows this guide, but there are some key differences in my version.

While you could just buy a Filimin, making the light yourself is half the fun and is a really meaningful present. The goals of this project were:

 - Low total cost
 - Low latency + reliable server connection
 - Easily changeable WiFi networks
 - Reliable touch sensing
# Parts List
 - 3.5" square wooden plaque
 - 12" x 12" x 1/8" Acrylic Sheet (this is just enough for 1 lamp)
 -  Bare Conductive Paint (I recommend just one 50ml bottle)
 - **WeMos D1 Mini** (as opposed to pricey Particle boards + Pi server)
 - 16 Neopixel Ring (buy a clone)
 - Black Spray Paint
 - Acrylic Cement #16 (this is extremely runny, be careful)
 - 470k Resistor
 - High Quality USB Power Supply
 - Gaff tape
 - Tacky Glue

# Assembly
I found that putting the paint in a ziploc bag and cutting the corner worked much better than the pens that Bare Conductive sells. Instead of soldering the USB cable to the board as was done in the original guide, I just glued the WeMos in place with the microUSB port flush with the back of the light. A 470k resistor is used because this is the maximum due to the 3.3V logic of the WeMos
![Base Top](https://github.com/dgorbunov/WifiTouchLights/blob/master/Photos/IMG_3199.jpg)
![Drying Paint](https://github.com/dgorbunov/WifiTouchLights/blob/master/Photos/IMG_3210.jpg)
![Drying Paint](https://github.com/dgorbunov/WifiTouchLights/blob/master/Photos/IMG_3212.jpg)

# Code
I used a WeMos D1 Mini because they are Wifi capable Arduinos that are under $2 a board, compared to the $35 Particle Boards and Raspberry Pi that was originally used. One big difference is that instead of polling a server like was done in the original Filimin Prototype, I used MQTT for its reliability, low latency, and low packet count. This means that wherever the lights are in the world, they will *update instantly* with no human noticable latency, compared to the~5 seconds it took the original lights to update.

Additionally, the WifiManager library is used so that if the light ever disconnects from it's WiFi network, an access point will appear where a user can connect and a list will appear with networks that can be connected to.

I wrote all of this code myself, with help from the PubSubClient, WifiManager, Neopixel, and CapacitveSensing example code.



