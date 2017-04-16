User Interface
--------------

PATH:
~/project/ThirdEye

// You have to set the environment before launch the server
Set environment:
~$: cd ~/project
~$: . venv/bin/activate

Launch WebServer:
~$: cd ~/project/ThirdEye
~$: export FLASK_APP=hello.py
~$: flask run --host=192.168.0.100

// "This 192.168.0.100 is the RPi IP on the TPLink router"

Web Browser:
http://192.168.0.100:5000/

// How the web app works
1. Three blue triangles represent three beacons.
2. Set positions of three beacons by moving the traingles and click "set anchors".
3. Hit "Start" button to start the program.
4. The red dot represents the moving tag.
5. The position of the red dot is randomly generated.

To Access RPi:
Connect RPi and router using Ethernet cable
Turn on both RPi and router
Using any device connect to the router's WIFI:
(The WIFI username and password are on the back of the router)

ssh pi@192.168.0.100
password: pi


DecaWave Code
-------------

1 tag 4 beacon code: dw1000_api_rev2p03/examples/myRanging/

Use Makefile to compile

RPi specific code: dw1000_api_rev2p03/myPlatform

