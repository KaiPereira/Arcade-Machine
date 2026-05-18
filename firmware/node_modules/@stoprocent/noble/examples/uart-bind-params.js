const noble = require('../with-custom-binding');

// Needs to export env: BLUETOOTH_HCI_SOCKET_FORCE_UART=1

const nobleUartA = noble({ bindParams: { uart: { port: '/dev/tty.usbmodem1...' } } });
const nobleUartB = noble({ bindParams: { uart: { port: '/dev/tty.usbmodem2...' } } });

nobleUartA.on('discover', peripheral => {
  console.log('UART A', peripheral.address);
});

nobleUartB.on('discover', peripheral => {
  console.log('UART B', peripheral.address);
});

nobleUartA.on('stateChange', state => {
  if (state === 'poweredOn') {
    nobleUartA.setAddress('00:11:22:33:44:01');
    nobleUartA.startScanning();
  }
});

nobleUartB.on('stateChange', state => {
  if (state === 'poweredOn') {
    nobleUartB.setAddress('00:11:22:33:44:02');
    nobleUartB.startScanning();
  }
});
