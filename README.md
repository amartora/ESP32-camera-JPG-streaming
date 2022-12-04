# ESP32-camera-JPG-streaming
This is a better version of my previous project: [camerastream](https://github.com/amartora/camerastream)<br />

The streaming now uses jpg instead of lines of raw pixel data, so it is much more efficient, and frames can be sent in less packets.<br />

I am streaming video from a LILYGO TTGO T-Camera and receiving it with a LILYGO TTGO T-Watch 2020 V1.

## Demonstration:<br />

https://user-images.githubusercontent.com/46986576/205510079-e7f1335e-acc3-4870-96d0-0fc91c2e74e4.mp4

LILYGO TTGO T-Watch 2020 V1           |  LILYGO TTGO T-Camera
:-------------------------:|:-------------------------:
![t-watch2020](https://user-images.githubusercontent.com/46986576/205510335-086e1562-b65c-4367-bf82-db75e9d0e189.jpg)|![ttgo-camera](https://user-images.githubusercontent.com/46986576/205510337-6986e965-206d-49f3-ac5a-81f51c03815a.jpg)

The result is obviously still choppy, but is better than the last version. The current bottleneck is the speed at which I am able to update the display, which could be because I update every pixel on the display with each jpg recieved, rather than only the sections of the image that have been changed.
