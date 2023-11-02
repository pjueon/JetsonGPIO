# Installation Guide
### 1. Clone the repository.
```
git clone https://github.com/pjueon/JetsonGPIO
```

### 2. Make build directory and change directory to it. 

```
cd JetsonGPIO
mkdir build && cd build
```

### 3. Configure the cmake
```
cmake .. [OPTIONS]
```

|Option|Default value|Description|
|------|-------------|-----------|
|`-D CMAKE_INSTALL_PREFIX=`|`/usr/local`|Installation path|
|`-D BUILD_EXAMPLES=`|ON|Build example codes in `samples`|
|`-D JETSON_GPIO_POST_INSTALL=`|ON|Run the post-install script after installation to set user permissions. If you set this `OFF`, you must run your application as root to use Jetson GPIO.|

### 4. Build and Install the library
```
sudo cmake --build . --target install
```

# Compiling the Samples
You can add cmake option `-D BUILD_EXAMPLES=ON` to build example codes in `samples`.
Assuming you are in `build` directory:
```
cmake .. -DBUILD_EXAMPLES=ON
cmake --build . --target examples 
```
You can find the compiled results in `build/samples`.
