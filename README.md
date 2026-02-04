# CIT460
This will be the repository for the Advanced Topics in Programming course. The project will be to create an app so that PLC values can be edited and read via an app. The app will also return data about the system. 

Conveyor Control Project
CIT460 – Advanced Topics in Programming
Marissa Rupert

PROJECT DESCRIPTION

This project focuses on creating a mini conveyor system that is controlled using an Arduino. The Arduino will communicate with a React Native application through the MQTT protocol. The application will allow the user to change data that affects how the conveyor operates, such as motor speed, and view data about how the system is running.
This project connects Automation and Engineering: Mechatronics with Software Development and Information Management. It was chosen because it presents a challenge and requires learning new technologies, including C++ for Arduino programming, deeper use of React Native, and hands-on setup of MQTT communication. The goal is to create a working system that allows conveyor data to be altered through a mobile application while remaining user-friendly and secure.

TECHNOLOGIES

Arduino ESP32 is used to control the conveyor system. It reads inputs and sends outputs to components such as motors and sensors. The Arduino is programmed using C++ through the Arduino IDE and was chosen because it integrates well with MQTT and meets the project’s hardware needs.
MQTT is used as the communication protocol between the Arduino and the React Native application. It allows both systems to publish and subscribe to data in real time, making it suitable for changing and monitoring conveyor values.
React Native is used to build the mobile application. It provides the user interface that allows users to send inputs to the Arduino and view system outputs. Learning React Native more in depth also supports software development career goals.

DEPLOYMENT

Deployment is handled using Docker and HiveMQ Cloud. Docker is used to provide an isolated environment for running MQTT-related services. HiveMQ Cloud is used as the MQTT broker to allow communication between the Arduino and the React Native application.
Cloud-based MQTT was chosen so the Arduino and the application do not need to be on the same local network. This allows the conveyor to be located anywhere while still being controlled remotely, which better reflects how MQTT is used in industrial environments.

PROJECT PLAN AND TIMELINE
Before the first sprint, conveyor parts will be gathered, the conveyor will be built, and Arduino code will be written to control the system. This ensures a working hardware product early in the project.
Before the second sprint, the MQTT protocol will be set up using Docker, and communication between the publisher and subscriber will be verified. Basic communication between the Arduino and the React Native application should be established.
Before the third sprint, the React Native application will be fully developed. Efficient communication will be implemented, security such as proper credentials will be added, and both the Arduino and application will be able to publish and subscribe to the MQTT broker.
