
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

```python
# example messages from console:
#           icao         alt  GS Track  lat      lon   var squawk
# 'MSG,8,,,407F38,,,,,,,     ,   ,   ,        ,        ,   ,    , , , , \n'
# 'MSG,5,,,440669,,,,,,,36000,   ,   ,        ,        ,   ,    ,0,0,0,0\n'
# 'MSG,8,,,440669,,,,,,,     ,   ,   ,        ,        ,   ,    , , , , \n'
# 'MSG,4,,,440669,,,,,,,     ,474,288,        ,        ,320,    ,0,0,0,0\n'
# 'MSG,5,,,407F38,,,,,,,38975,   ,   ,        ,        ,   ,    ,0,0,0,0\n'
# 'MSG,4,,,407F38,,,,,,,     ,454,136,        ,        ,64 ,    ,0,0,0,0\n'
# 'MSG,5,,,407F38,,,,,,,38975,   ,   ,        ,        ,   ,    ,0,0,0,0\n'
# 'MSG,3,,,407F38,,,,,,,38975,   ,   ,48.59436,11.01562,   ,    ,0,0,0,0\n'
# 'MSG,3,,,440669,,,,,,,36000,   ,   ,48.98427,11.76137,   ,    ,0,0,0,0\n'
# 'MSG,8,,,39E688,,,,,,,     ,   ,   ,        ,        ,   ,    , , , , \n'
# 'MSG,3,,,782158,,,,,,,35000,   ,   ,        ,        ,   ,    ,0,0,0,0\n'
# 'MSG,4,,,782158,,,,,,,     ,535,186,        ,        ,-64,    ,0,0,0,0\n'
# 'MSG,6,,,406E10,,,,,,,     ,   ,   ,        ,        ,   ,3106,0,0,0,0\n'
# 'MSG,5,,,440669,,,,,,,36000,   ,   ,        ,        ,   ,    , , , , \n'
# 'MSG,3,,,39E688,,,,,,,20800,   ,   ,48.85210,11.19500,   ,    ,0,0,0,0\n'
# 'MSG,5,,,782158,,,,,,,35000,   ,   ,        ,        ,   ,    , , , , \n'
# 'MSG,5,,,782158,,,,,,,35000,   ,   ,        ,        ,   ,    ,0,0,0,0\n'
# 'MSG,5,,,782158,,,,,,,35000,   ,   ,        ,        ,   ,    ,0,0,0,0\n'
```

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