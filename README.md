# E.D.I.T.H. Glasses

My approach at developing [E.D.I.T.H. Glasses](https://spiderman.fandom.com/wiki/E.D.I.T.H_Glasses). Final Project for MTE301, Programming for Mechatronics Engineering.

## Features
E.D.I.T.H. currently has rudimentary voice recognition, as well as PhotoMath integration.

### Voice Recognition
Currently, detects a specific spike in amplitude, which signifies the phrase "EDITH" was spoken. 
The initial plan was to train a machine learning model using TensorFlow, then use TensorFlowLite on the ESP
to predict if the word was said. This however could not be done as the number of samples required to train the model
accurately would generate a file too large for the ESP32-S3-WROOM-1.

The command recognition was done through Google Cloud's Speech to Text API.
The microphone data is sent to a proxy server that processes the voice.

### PhotoMath
This was achieved by reverse engineering the PhotoMath API, and implementing my own wrapper to interact with it.
Initially I tried to request directly through the ESP, however the ESP does not support TLSv1.3.
The PhotoMath server only accepts TLSv1.3, therefore this was not a viable option.

The solution was to send the camera data to the proxy server, then parse the response. The response from
PhotoMath was a JSON, with the solution and equation being a binary tree. Each child contains a type (operation or symbol) then a value or children.

## How to use
You will require the materials in [Parts Used](#parts-used). 
1. Use your IDE of choice to open the PlatformIO project. CLion has a wonderful plugin where you just click the run button and it uploads.
2. Wire the components to match the pins in the code (Wiring diagram soon).
3. Clone the [proxy server](https://github.com/noahpistilli/E.D.I.T.H.-Glasses-Proxy-Server) and run it. You will need Python 3.12 at minimum.
4. Update `SERVER_URL` to be the IP address of the proxy server
5. Upload to the ESP, and say "EDITH", followed by "solve equation". Point ESP camera to equation and watch it solve.


## Parts Used
1. [Freenove ESP32-S3 CAM](https://www.amazon.ca/Freenove-ESP32-S3-WROOM-Compatible-Wireless-Detailed/dp/B0BMQ8F7FN/ref=asc_df_B0BMQ8F7FN?mcid=49dccb9c42b73dedbb7f54509044bd20&tag=googleshopc0c-20&linkCode=df0&hvadid=706738331967&hvpos=&hvnetw=g&hvrand=16075296240018575231&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=9000945&hvtargid=pla-1965584988630&hvocijid=16075296240018575231-B0BMQ8F7FN-&hvexpln=0&gad_source=1&th=1)
2. [MAX9814 Microphone](https://www.amazon.ca/CANADUINO®-MAX9814-microphone-amplifier-module/dp/B07PSBZ4NT/ref=asc_df_B07PSBZ4NT?mcid=4621daf01e2b35a489338555db731fd8&tag=googleshopc0c-20&linkCode=df0&hvadid=706724917158&hvpos=&hvnetw=g&hvrand=16855303096966388985&hvpone=&hvptwo=&hvqmt=&hvdev=c&hvdvcmdl=&hvlocint=&hvlocphy=9000945&hvtargid=pla-2379292904865&psc=1&hvocijid=16855303096966388985-B07PSBZ4NT-&hvexpln=0&gad_source=1)
3. [0.96 OLED Screen](https://www.amazon.com/UCTRONICS-Display-Module-Arduino-SSD1306/dp/B072Q2X2LL/ref=sr_1_8?dchild=1&keywords=arduino%2B0.98%2Binches%2Boled&qid=1605787749&sr=8-8&th=1)