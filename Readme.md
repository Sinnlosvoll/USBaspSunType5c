# USBaspSunType5c

A single board computer adapter based on the USBasp in-circuit-programmer that adapts the 1994 built Sun Type 5c keyboard to a USB interface using standard USB HID drivers.

## Getting Started

So you have your own Sun Type 5c keyboard sitting around idle in your closet because you dont have the workstation to go with it anymore? I have great news for you. You can use it literally anywhere with any device that has a usb port available. You will need to do some modifications though. This is a guide to almost invisibly adapting the Type 5c to usb.

### What you need
 - a Sun Type 5c
 - a tiny USB 3.0 hub
 - two USBasp
 - linux
 - a flat-headed screw-driver
 - a soldering iron
 - a piece of (solid core) wire or a paper-clip
 - a 6 pin female 2.54mm pitch pinheader

Also useful would be an usb 3.0 extension cord, if you don't want to use it on a raspberry pi zero.

### Preparing the actual adapter

So in order to get the adapter working we will need to reflash one of the two USBasps to act as the translator between the computer and the keyboard.

For that we will need the `avr-gcc, arvdude, avr-libc, make` packages which can be installed (on arch) with
```
sudo pacman -Sy avr-gcc arvdude avr-libc make
```
After installing the required software clone the repo:
```
git clone https://github.com/Sinnlosvoll/USBaspSunType5c && cd USBaspSunType5c
```

Now connect one of the USBasps to a usb port and attach the cable to the other one.
[IMG]

Type but **don't execute** the following command
```
sudo make flash
```
Connect the paper-clip to both pads marked _J2_ on the USBasp that is not connected to the usb port.

_Hit enter._

The console should now show some avr-gcc messages and then show the progress of avrdude flashing the second USBasp like this:
```
[...]
avrdude: verifying ...
avrdude: 3156 bytes of flash verified

avrdude: safemode: hfuse reads as C9
avrdude: safemode: Fuses OK (E:FF, H:C9, L:EF)

avrdude done.  Thank you.
```
Once that is done you can release the paper click and unplug everything.

The USBasp that had the paper clip has now the correct firmware to be installed inside the keyboard.

Now we need to create the cable that the adapter uses to talk to the keyboard.

### Soldering and cutting

In order for the adapter to talk to the keyboard an adapter cable needs to be created.
Take one of the two cables that came with the USBasps and cut the cable at about 16cm.
Attach the female pinheader to the cable ends as in this image shown:
[adapter cable](doc/images/adaptercable.png)
Once that is done I recommend to add some hot glue to the pinheaders back, in order to make it easier to insert and protect it from accidental shorts.

#### Fully hidden adapter, no chassis modification

If you don't care about a usb 3 hub in your Type 5c and would like it to look brand-new (and aged), then you need to create a small usb 2 extension, that connects the USBasp to the usb 3 hub, both of which then will be stored inside the keyboard.
You can either make the cable yourself if you have a usb plug and socket or remove the plug from the USBasp and reattach it on wires, making sure to twist the two data wires for signal integrity.
After that, open the keyboard.

Now connect the extended plug to one port of the hub and snugly put both boards into the hollow right cavern of the keyboard chassis.

#### With usb hub, chassis modification

If you would rather have a usb 3 hub inside your Type 5c and don't care about it that much (already scratched by other people) then the procedure is a bit different. You still need to extend the usb plug away from the USBasp board, but this this it needs to go outside and back into the hub. Cable length should be around 15cm. 

For the hub to be useable the ports need to be cut out of the back of the keyboard:
 - first remove the case of the usb hub.
 - mark the location of the ports on the back of the keyboard using a marker.
 	+ note the orientation of the usb ports, an inserted usb stick should have the holes on top
 - open the keyboard
 - drill a hole inside each of the marked out ports
 - use your favorite method of removal of the remaining material to allow the usb hub to fit each usb port into the now square holes
 - attach the usb-hub inside the keyboard using super-glue and material from the case of the hub
Now it should look like this:
[USB HUB cutout](doc/images/usbhubCutOut)
Finally attach the extended usb plug to the hub on the outside. 
[usbaspwithhubexternalcable](doc/images/usbaspinplace.jpg)

It might not be the most beautiful method, but the pitch on the usb 3 connector directly is hard to solder and removing the port itself poses risks of breaking the pcb/traces.

### Attaching the adapter

Now take the soldered adapter cable and plug each end into the fitting plug on the adapter and the keyboard respectively.
Place the usb-hub's cable through the center hole of the keyboard, where the original cable came in.


### Using it

after having completed all the steps above you can now plug in the usb-extension to the cable hanging out the bottom of the keyboard and then starat having fun. Or type. Whatever you want to do.

## But why?

One night going to my local hack-shack I just saw this 'little' Keyboard lying in front of the door, amongst a pile of other older computing equipment. I couldn't bear seeing a perfectly useable keyboard going to be crushed or thrown on a landfill so I took it home. The rest is logical.

## Contributing

If you see something that you would like to change or fix, I would be more than welcome to accept a pull request.

## Authors

* **Christopher Hofmann** - *Initial work* - [Sinnlosvoll](https://github.com/Sinnlosvoll)

## License

This project is licensed under the GPL3 License - see the [LICENSE](LICENSE) file for details

## Acknowledgments

* [Thomas Fischl](http://www.fischl.de/usbasp/) for his awesome USBasp programmers
* [Code-and-life](http://codeandlife.com/2012/02/22/v-usb-with-attiny45-attiny85-without-a-crystal/) for his tutorial motivating me to do this
* [MightyPork](https://gist.github.com/MightyPork/6da26e382a7ad91b5496ee55fdc73db2) For the keycode file
* [OBDev](https://www.obdev.at/products/vusb/index.html) For their amazing work getting the software only usb emulation onto avrs
