import React from 'react';
import { StyleSheet, View } from 'react-native';
import MQTTConnection from './components/MQTTConnection';

export default function App() {
  return (
    <View style={styles.container}>
      <MQTTConnection />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
});