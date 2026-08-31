# ThingPulse esp8266-spotify-remote

[![ThingPulse logo](https://thingpulse.com/assets/ThingPulse-w300.svg)](https://thingpulse.com)

## Purpose of this project

This project lets you control a Spotify player (phone, browser, etc) from an ESP8266. Title and artist name, along with playback
progress, are fetched from Spotify's Web API over WiFi and shown on a SH1106 128x64 OLED display. The currently played song can be
paused, played and skipped to the next or previous song in the playlist using three physical buttons.

A full OAuth 2.0 web flow is used to acquire the necessary access and refresh tokens to permit the user to control the player. In order to
run this project on your device you will have to setup an application on Spotify's developer dashboard. Spotify requires the OAuth
redirect URI to be HTTPS (or `http://127.0.0.1`), so this project's on-device callback server is reached via a proxy you run
yourself that forwards Spotify's redirect on to the device over your LAN — see the Spotify dashboard setup step below.

## Features

 - Now playing screen: title, artist, playback progress
 - Control playback with buttons: Play/Pause, Next, Prev
 - WS2812B status LED (connecting, authenticating, playing, paused, error)
 - Authentication and Authorization (OAuth 2.0 flow) On device.

## Recommended Hardware

This fork targets the **DStike Deauther Mini OLED** board (ESP-07, SH1106 1.3" I2C OLED):

 - OLED (SH1106, I2C): SDA=GPIO5, SCL=GPIO4
 - WS2812B status LED: GPIO15
 - Buttons: select=GPIO14, up=GPIO12, down=GPIO13

Pin assignments live in [`include/pins.h`](include/pins.h).


## Contributions

Please see our [Guidelines](CONTRIBUTING.md) if you want to contribute to this project. Contributions are more than welcome!

## Service level promise

<table><tr><td><img src="https://thingpulse.com/assets/ThingPulse-open-source-community.png" width="150">
</td><td>This is a ThingPulse <em>community</em> project. See our <a href="https://thingpulse.com/about/open-source-commitment/">open-source commitment declaration</a> for what this means.</td></tr></table>

## Setup Instructions

### Precondition

This project builds with [PlatformIO](https://platformio.org/install/cli). Install the `pio` CLI before continuing.

### Prepare the Project

1. Download this project either as ZIP file or check it out with Git
1. Copy `include/secrets.h.example` to `include/secrets.h`
1. Set your *WiFi credentials* in `include/secrets.h`
1. Complete the steps below to get the values for the *Spotify settings* required in `include/secrets.h`

### Get Access to the Spotify API

1. Go to [https://developer.spotify.com/dashboard/login](https://developer.spotify.com/dashboard/login) and login to or sign up for the Spotify Developer Dashboard

2. Click on "My New App"
<img src="./images/SpotifyDashboard.png" width="400">

3. Fill out the form. Give your new app a name you can attribute to this project. It's safe to select "I don't know" for the type of application.
<img src="./images/SpotifyAppSignUp1.png" width="400">

4. At the end of the 3 steps click "Submit"
<img src="./images/SpotifyppSignUp3.png" width="400">

5. Set the unique Client ID and Client Secret as values for the respective variables in `include/secrets.h`
<img src="./images/SpotifyCredentials.png" width="400">

6. Click on "Edit Settings". Spotify requires the Redirect URI to be HTTPS (or `http://127.0.0.1`), so add the HTTPS URL of a proxy
   you control that forwards the redirect on to `http://<espotifierNodeName>.local/callback/?code=...` on your LAN. Set that same
   HTTPS URL as `redirectUri` in `include/secrets.h` (URL-encoded), and set `espotifierNodeName` in `include/secrets.h` to whatever
   hostname the device should listen on.

   **NOTE** If you're running more than one of these devices in the same WiFi network you should choose a unique `espotifierNodeName`.
<img src="./images/SpotifyAppSettings.png" width="400">

7. Don't forget to save your settings.
<img src="./images/SpotifyAppSettingsSave.png" width="400">

### Compile and run the application

After all this configuration it's about time to run the application!

1. First check the configuration in `include/secrets.h` one more time to ensure the Spotify values match those set on the Spotify Developer Dashboard. Better safe than sorry, they say.

2. Attach your DStike Deauther Mini OLED board to your computer and build/flash it:

   ```sh
   pio run -t upload --upload-port /dev/tty.YOUR_SERIAL_PORT
   ```

3. When you run this the first time you'll have to go through additional steps. The display will ask you to open a browser at a specific location. This will redirect you to Spotify's authorization dialog, and afterwards through your proxy back to the device.
<img src="./images/SpotifyConnectScreen.png" width="400">

4. Now open your Spotify player and start a song. If everything worked out you'll see the song information on the OLED screen!
