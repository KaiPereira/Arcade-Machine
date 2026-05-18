import noble from '@stoprocent/noble';
globalThis.__noble = noble;

import { ThermalPrinterClient, NodeBluetoothAdapter } from 'mxw01-thermal-printer';
import { createCanvas } from 'canvas';

const adapter = new NodeBluetoothAdapter();
const printer = new ThermalPrinterClient(adapter);

noble.on('stateChange', state => console.log('BT state:', state));
noble.on('discover', device => console.log('Found:', device.advertisement.localName, device.id));

await printer.connect();

const canvas = createCanvas(384, 200);
const ctx = canvas.getContext('2d');
ctx.fillStyle = 'white';
ctx.fillRect(0, 0, 384, 200);
ctx.fillStyle = 'black';
ctx.font = '30px Arial';
ctx.fillText('Hello from Node.js!', 20, 100);
const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);

await printer.print(imageData);
await printer.disconnect();
