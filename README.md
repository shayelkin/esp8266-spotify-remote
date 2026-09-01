# ThingPulse esp8266-spotify-remote

## Purpose of this project

This project lets you control a Spotify player (phone, browser, etc) from an ESP8266. Title and artist name, along with playback
progress, are fetched from Spotify's Web API over WiFi and shown on a SH1106 128x64 OLED display. The currently played song can be
paused, played and skipped to the next or previous song in the playlist using three physical buttons.

A full OAuth 2.0 web flow is used to acquire the necessary access and refresh tokens to permit the user to control the player. The device
serves its OAuth callback directly over HTTPS with a self-signed certificate, so no proxy is required.

## About this fork

This original project was designed for [ThingPulse ESP8266 Color
Kit](https://thingpulse.com/product/esp8266-wifi-color-display-kit-2-4/), which I didn't have. But
I had a spare [DSTIKE Deauther MINI](https://dstike.com/products/dstike-wifi-deauther-mini)
device. I ported this project to that device, updated it to use
[PlatformIO](https://platformio.org/) as a build environment, and migrated it to serve the OAuth
webflow over HTTPS, [which is required by Spotify since April 2025](https://developer.spotify.com/documentation/web-api/concepts/redirect_uri).

## Features

 - Now playing screen: title, artist, album name, playback progress
 - Control playback with buttons: Play/Pause, Next, Prev
 - WS2812B status LED (connecting, authenticating, playing, paused, error)
 - Authentication and authorization (OAuth 2.0 flow) on device

## Recommended Hardware

This fork targets the **DStike Deauther Mini OLED** board (ESP-07, SH1106 1.3" I2C OLED):

 - OLED (SH1106, I2C): SDA=GPIO5, SCL=GPIO4
 - WS2812B status LED: GPIO15
 - Buttons: select=GPIO14, up=GPIO12, down=GPIO13

Pin assignments live in [`include/pins.h`](include/pins.h).


## Contributions

Please see our [Guidelines](CONTRIBUTING.md) if you want to contribute to this project. Contributions are more than welcome!

## Setup Instructions

### Precondition

This project builds with [PlatformIO](https://platformio.org/install/cli). Install the `pio` CLI before continuing.

### Prepare the Project

1. Download this project either as ZIP file or check it out with Git
1. Copy `include/secrets.h.example` to `include/secrets.h`
1. Set your *WiFi credentials* in `include/secrets.h`
1. Complete the steps below to get the values for the *Spotify settings* required in `include/secrets.h`

### Get Access to the Spotify API

1. Go to
[https://developer.spotify.com/dashboard/login](https://developer.spotify.com/dashboard/login) and
login to or sign up for the Spotify Developer Dashboard.

2. Click on "Create app".

3. Fill out the form. Give your new app a name you can attribute to this project.

4. In the "Redirect URIs" field, add `https://<espotifierNodeName>.local/callback/` as the Redirect URI. Set that same URL, URL-encoded, as `redirectUri` in `include/secrets.h`, and set
   `espotifierNodeName` to the hostname the device should listen on.

5. Read and agree to the terms, and submit the form.

6. Copy the "Client ID" and "Client secret", and update them in `include/secrets.h`.

**NOTE** If you're running more than one of these devices in the same WiFi network you should
  choose a unique `espotifierNodeName`. You can share the same client ID and secret, as you can add
  multiple redirect URIs to the same app.

### Compile and run the application

After all this configuration it's about time to run the application!

1. First check the configuration in `include/secrets.h` one more time to ensure the Spotify values match those set on the Spotify Developer Dashboard. Better safe than sorry, they say.

2. Attach your DStike Deauther Mini OLED board to your computer and build/flash it:

   ```sh
   pio run -t upload --upload-port /dev/tty.YOUR_SERIAL_PORT
   ```

3. When you run this the first time you'll have to go through additional steps. The display will ask you to open a browser at a specific HTTPS location. Accept the self-signed certificate warning, then continue to Spotify's authorization dialog. Spotify redirects directly back to the device when authorization completes.

4. Now open your Spotify player and start a song. If everything worked out you'll see the song information on the OLED screen!
