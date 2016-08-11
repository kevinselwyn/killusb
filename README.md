#killusb

Anti-forensic tool that triggers kill functionality when a USB device is added or removed

##Installation

```
make && sudo make install
```

`killusb` depends on `libusb`. Install with:

```
sudo apt-get install libusb-dev
```

Some macros can be set at build time:

| Macro    | Default              | Notes                      |
|----------|----------------------|----------------------------|
| -DEVICES | 256                  | Max USB devices to check   |
| -DELAY   | 1                    | Polling delay (in seconds) |
| -DLOG    | /var/log/killusb.log | Log file path              |

Build with custom macros with:

```
gcc -o killusb -DEVICES=256 -DELAY=1 -DLOG=\"/var/log/killusb.log\" killusb.c -lusb
```

##Usage

```
 _    _ _ _           _
| |  (_) | |         | |
| | ___| | |_   _ ___| |__
| |/ / | | | | | / __| '_ \
|   <| | | | |_| \__ \ |_) |
|_|\_\_|_|_|\__,_|___/_.__/
             Version 1.0.1

Usage: ./killusb [options]

Options:
 -h, --help                  Show help
 -v, --verbose               Log events to screen
 -f, --force                 Trigger execution on Ctrl-C
 -d, --delay <seconds>       Polling delay in seconds
 -s, --script <path>         Script to run on USB change
 -w, --whitelist <name,...>  Comma separated USB device names
 -l, --log <path>            Log file path
```

##Explanation

Run without any arguments, `killusb` will shutdown the computer. If passed a script with `-s`, it will execute that script instead.

This program follows more of a [Unix Philosophy](http://www.catb.org/esr/writings/taoup/html/ch01s06.html) in that it doesn't try to do much by default when a USB drive is added or removed.

If the user wants to clear all RAM or erase files, this functionality must be put into a Bash script to be run by `killusb`

##Acknowledgements

Obvious ripoff of [hephaest0s](https://github.com/hephaest0s)'s [usbkill](https://github.com/hephaest0s/usbkill), with the obvious notable change of not being dependent on Python, but also being less portable. (Good luck building on OSX)