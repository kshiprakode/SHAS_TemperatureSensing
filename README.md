# Smart-Home-Automation---Temperature-Sensing

The project aimed at simulation of a Smart Home Automation System based on IoT (Internet of Things). The system was implemented as an environment of sensors, smart devices and gateways using the basic concepts of distributed operating systems like socket programming, clock synchronization, fault-tolerance and multicast communication.

We had assumed three classes of IoT objects:
Gateway, which is a tiny server that interacts with sensors and smart devices.
Sensor, connected to the gateway and can be of different types like
Temperature Sensor
Smart Device, connected to and controlled the gateway and can be of different types like-

Architecture Assumptions - 
Sensors and Smart devices connect to the Gateway using sockets. Gateway is multi-threaded such that it can accept requests from multiple sensors. Each item (Sensor, Smart Device or Gateway) is identified by an IP Address and a Port number. When the Gateway process switches on, it listens on the port for “register” requests. Whenever a new item registers with the gateway, it will assign an integer value in sequential order to the item. Gateway takes the inputs from the sensors and decides if it should “switch” the devices “on” or “off” depending on the task. Gateway displays the current state whenever some value from the sensors or the smart device changes.
