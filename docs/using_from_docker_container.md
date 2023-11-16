
# Using *JetsonGPIO* from a docker container
The following describes how to use *JetsonGPIO* from a docker container. 

## Preparing the docker image
### Pulling from Docker hub
A pre-built image that contains *JetsonGPIO* is available on docker hub([pjueon/jetson-gpio](https://hub.docker.com/r/pjueon/jetson-gpio/)).

```shell
docker pull pjueon/jetson-gpio
```

You can use it as a base image for your application.

### Building from the source
You can also build the image from the source.
`docker/Dockerfile` is the docker file that was used for the pre-built image on docker hub. 
You can modify it for your application.

The following command will build a docker image named `testimg` from `docker/Dockerfile`: 

```shell
sudo docker image build -f docker/Dockerfile -t testimg .
```

## Running the container
### Basic options 
You should map `/sys/devices`, `/sys/class/gpio` into the container to access to the GPIO pins.  
So you need to add following options to `docker container run` command:

```shell
-v /sys/devices/:/sys/devices/ \
-v /sys/class/gpio:/sys/class/gpio
```

and if you want to use GPU from the container you also need to add following options:  

```shell
--runtime=nvidia --gpus all
```

### Running the container in privilleged mode
The library determines the jetson model by checking `/proc/device-tree/compatible` and `/proc/device-tree/chosen` by default.
These paths only can be mapped into the container in privilleged mode.

The following example will run `/bin/bash` from the container in privilleged mode. 
```shell
sudo docker container run -it --rm \
--runtime=nvidia --gpus all \
--privileged \
-v /proc/device-tree/compatible:/proc/device-tree/compatible \
-v /proc/device-tree/chosen:/proc/device-tree/chosen \
-v /sys/devices/:/sys/devices/ \
-v /sys/class/gpio:/sys/class/gpio \
pjueon/jetson-gpio /bin/bash
```

### Running the container in non-privilleged mode
If you don't want to run the container in privilleged mode, you can directly provide your jetson model name to the library through the environment variable `JETSON_MODEL_NAME`:

```shell
# ex> -e JETSON_MODEL_NAME=JETSON_NANO
-e JETSON_MODEL_NAME=[PUT_YOUR_JETSON_MODEL_NAME_HERE]
```
> [!WARNING]
> If the name of the input Jetson model does not match the actual model, the behavior is undefined.


You can get the proper value for this environment variable by running `samples/jetson_model` in privilleged mode:
```shell
sudo docker container run --rm \
--privileged \
-v /proc/device-tree/compatible:/proc/device-tree/compatible \
-v /proc/device-tree/chosen:/proc/device-tree/chosen \
-v /sys/devices/:/sys/devices/ \
-v /sys/class/gpio:/sys/class/gpio \
pjueon/jetson-gpio /gpio-cpp/samples/jetson_model
```

The following example will run `/bin/bash` from the container in non-privilleged mode. 

```shell
sudo docker container run -it --rm \
--runtime=nvidia --gpus all \
-v /sys/devices/:/sys/devices/ \
-v /sys/class/gpio:/sys/class/gpio \
-e JETSON_MODEL_NAME=[PUT_YOUR_JETSON_MODEL_NAME_HERE] \  
pjueon/jetson-gpio /bin/bash
```
