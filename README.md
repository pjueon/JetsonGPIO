# JetsonGPIO (C++)
`JetsonGPIO` is an unofficial C++ version of the [NVIDIA's `Jetson.GPIO` Python library](https://github.com/NVIDIA/jetson-gpio).

NVIDIA Jetson series development boards contain a 40 pin GPIO header, similar to the 40 pin header in the Raspberry Pi. These GPIOs can be controlled for digital input and output using this library. `JetsonGPIO` provides C++ APIs that are almost same as the `Jetson.GPIO` Python library.
  
  
# Package Components
In addition to this document, the `JetsonGPIO` library package contains the following:
1. The `src` and `include` subdirectories contain the C++ codes that implement all library functionality. The `JetsonGPIO.h` file in the `include` subdirectory is the only header file that should be included into an application and provides the needed APIs. 
2. The `samples` subdirectory contains sample applications to help in getting familiar with the library API and getting started on an application. 
3. The `tests` subdirectory contains test codes to test the library APIs and private utilities that are used to implement the library. Some test codes require specific setups. Check the required setups from the codes before you run them. 
4. The `docs` subdirectory contains documents about `JetsonGPIO`. 

# Documents
- [Installation Guide](docs/installation_guide.md)
- [How to link `JetsonGPIO` to your project](docs/how_to_link_to_your_project.md)
- [Complete library API](docs/library_api.md)
- [Using `JetsonGPIO` from a docker container](docs/using_from_docker_container.md)
