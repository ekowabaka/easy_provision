# ESP Wifi Portal
A library? for adding a portal to help with provisioning wifi networks in projects that use the ESP8266 chip in combination with the RP2040. This library is still a work in progress but the goal is to run project specific networking middleware on the ESP with the RP2040 communicating with the ESP over UART.

## How does it work?
When the library is initialized, it checks for persisted wifi credentials. If any credentials are found, a connection attempt proceeds, and when successful the rest of the application works as usual. In the case where a connection attempt fails, or in the case where there wasn't any persisted credentials to begin with, a provisioning workflow is executed.

This workflow involves setting up the ESP in SoftAP mode and running a webserver on the ESP. The user will then use the app served to pick a wifi network 