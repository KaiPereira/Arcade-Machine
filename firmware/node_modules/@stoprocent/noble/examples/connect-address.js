const noble = require('../index');
const direct = require('debug')('connection/direct');
const scan = require('debug')('connection/scan');

function sleep (ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function run () {
  noble.on('stateChange', async function (state) {
    if (state === 'poweredOn') {
      try {
        direct('connecting');
        // const uuid = 'f1:36:1c:ab:94:cc'.split(':').join(''); // HCI Address UUID
        const uuid = '2561b846d6f83ee27580bca8ed6ec079'; // MacOS UUID
        const peripheral = await noble.connectAsync(uuid);
        direct(`connected ${peripheral.uuid}`);
        await peripheral.disconnectAsync();
        direct('disconnected');
        console.log('sleeping for 2000ms');
        await sleep(2000);
        scan('connecting by scan');
        await noble.startScanningAsync();
        noble.on('discover', async peripheral => {
          if (peripheral.uuid === uuid) {
            await noble.stopScanningAsync();
            await peripheral.connectAsync();
            scan(`connected ${peripheral.uuid}`);
            await peripheral.disconnectAsync();
            scan('disconnected');
          }
        });
      } catch (error) {
        console.log(error);
      }
    }
  });
}

run();
