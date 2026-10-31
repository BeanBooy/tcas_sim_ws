
## How to set up dump1090

1. Make sure all dependencies are installed:
```console
$ sudo apt-get update
$ sudo apt-get install git make gcc librtlsdr-dev pkg-config libusb-1.0-0-dev
````
2. Clone the dump1090 repository:
```console
$ cd ~
$ git clone https://github.com
$ cd dump1090
```

3. Compile
```console
$ make
````

4. Start dump1090
```console
$ ./dump1090 --interactive --net
```
The parameter `--net` opens the web ports.
Test the connection in a different terminal with command: `nc localhost 30003`

## How to get colored output from ros2

Tells ros that my terminal can display colored text:
```console
$ echo "export RCUTILS_COLORIZED_OUTPUT=1" >> ~/.bashrc
```

## Usefull links:

1. Decode SBS1 (BaseStation) format: http://woodair.net/sbs/article/barebones42_socket_data.htm

```console
$ clear && colcon build && source install/setup.bash && ros2 launch tcas_sim live_data.launch.py 
```