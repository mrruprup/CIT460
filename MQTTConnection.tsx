//Where most of the information came from 
//https://www.hivemq.com/blog/ultimate-guide-on-how-to-use-mqtt-with-node-js/
//https://www.npmjs.com/package/mqtt

import React, { useEffect, useState, useRef } from 'react';
import mqtt from 'mqtt'; 
import { StyleSheet, Text, View, Button, TextInput } from 'react-native';
import Constants from 'expo-constants';


export default function MQTTConnection() {
  const [motorSpeed, setMotorSpeed] = useState('0'); 
  const [inputSpeed, setInputSpeed] = useState('0');
  const clientRef = useRef<any>(null);

  // Safe access to Expo config (prevents crashes)
  const MQTT_CONFIG = Constants.expoConfig?.extra || {};

  const MQTT_URL = MQTT_CONFIG.MQTT_URL;
  const MQTT_USERNAME = MQTT_CONFIG.MQTT_USERNAME;
  const MQTT_PASSWORD = MQTT_CONFIG.MQTT_PASSWORD;
  
  useEffect(() => {
    // Connect to HiveMQ Cloud
    const client = mqtt.connect(MQTT_URL, {
      username: MQTT_USERNAME,
      password: MQTT_PASSWORD,
      reconnectPeriod: 1000,
    });

    clientRef.current = client;

    client.on('connect', () => {
      console.log('Connected to HiveMQ Cloud');
      //subscribe to the topic where the Arduino publishes motor speed
      client.subscribe('factory/conveyor/speed');
    });

    // Listen for messages on the subscribed topic
    client.on('message', (topic, message) => {
      // When a message is received, check if it's from the motor speed topic
      if (topic === 'factory/conveyor/speed') {
        setMotorSpeed(message.toString()); // Update state with new motor speed
        console.log(`Received message on topic ${topic}: ${message.toString()}`); // Log the received message
      }
    });

    client.on('error', (err) => {
      console.log('MQTT Error:', err);
    });

    return () => {
      client.end(); // disconnect when component unmounts
    };
  }, []);

  // Function to send a start command to the Arduino
  const sendCommandStart = () => {
    clientRef.current?.publish('factory/conveyor/command', 'START'); // Publish a command to start the motor
    console.log('Sent command: START');
  };

  // Function to send a start command to the Arduino
  const sendCommandStop = () => {
    clientRef.current?.publish('factory/conveyor/command', 'STOP'); // Publish a command to stop the motor
    console.log('Sent command: STOP');
  };

  // Function to send a start command to the Arduino
  const sendCommandSpeed = () => {
    clientRef.current?.publish(
      'factory/conveyor/command',
      `SPEED:${inputSpeed}` // Publish a command to set the motor speed
    );
    console.log('Sent command: SPEED');
  };

  //Display the motor speed and buttons to control the motor
  return (
    <View style={styles.container}>
      
      <Text style={styles.title}>Motor Speed Monitor</Text>
      <Text style={styles.speed}>Motor Speed: {motorSpeed}</Text>
      <View style={styles.buttonsall}>
        <View style={styles.buttons}>
          <Button title="Start Motor" onPress={sendCommandStart} />
        </View>
        <View style={styles.buttons}>
          <Button title="Stop Motor" onPress={sendCommandStop} />
        </View>
        <View style={styles.buttons}>
          <Button title="Set Motor Speed" onPress={sendCommandSpeed} />
        </View>
      </View>
      <TextInput
        style={styles.input}
        placeholder="Enter motor speed"
        keyboardType="numeric"
        value={inputSpeed}
        onChangeText={setInputSpeed}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  container: { 
    flex: 1, justifyContent: 'center', alignItems: 'center', backgroundColor: '#cde4f1', width: '100%'
  },
  title: { 
    fontSize: 26, fontWeight: 'bold', marginBottom: 20, color: '#000000c8'
  },
  speed: { 
    fontSize: 20, 
    marginBottom: 25
  },
  input: { 
    borderWidth: 2, 
    borderColor: '#0b0a0a', 
    width: '50%', 
    padding: 10, 
    marginBottom: 20, 
    backgroundColor: '#ffffff', 
    borderRadius: 10, 
    alignContent: 'center', 
    textAlign: 'center'
  },
  buttons: {
    color: '#080609',
    backgroundColor: '#181818',
    padding: 10,
    borderRadius: 15,
    marginBottom: 10,
  },
  buttonsall: {
    marginBottom: 20,
  },
});