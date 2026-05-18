# mxw01-thermal-printer

Framework-agnostic library for MXW01 thermal printer with support for browsers, Node.js, Bun, and more.

## Features

- 🎯 **Framework-agnostic** - Pure TypeScript core, works everywhere
- 🌐 **Multi-platform** - Browsers (Web Bluetooth), Node.js, Bun
- 📦 **Zero framework dependencies** - Use with React, Vue, Svelte, or vanilla JS
- 🖨️ **Advanced image processing** - Multiple dithering algorithms (Floyd-Steinberg, Bayer, Atkinson, etc.)
- 🔌 **Extensible adapters** - Web Bluetooth, Noble (Node.js), custom adapters
- 🔄 **Event-driven** - Subscribe to printer events
- 📘 **Full TypeScript** - Complete type safety
- 📚 **Examples included** - React, Vue, Node/Bun implementations provided

## Installation

```bash
npm install mxw01-thermal-printer
```

### Optional Dependencies

**For Node.js/Bun (Bluetooth):**
```bash
npm install @stoprocent/noble
```

**For Node.js/Bun (Canvas):**
```bash
npm install canvas
```

**For Node.js/Bun (Fabric):**
```bash
npm install fabric
```

## Quick Start

### Browser (Vanilla JavaScript)

```typescript
import { ThermalPrinterClient, WebBluetoothAdapter } from 'mxw01-thermal-printer';

// Create client
const adapter = new WebBluetoothAdapter();
const printer = new ThermalPrinterClient(adapter);

// Connect
await printer.connect();

// Print from canvas
const canvas = document.getElementById('myCanvas');
const ctx = canvas.getContext('2d');
const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);

await printer.print(imageData, {
  dither: 'steinberg',
  brightness: 128,
  intensity: 93
});

// Disconnect
await printer.disconnect();
```

### Node.js / Bun

```typescript
import { ThermalPrinterClient, NodeBluetoothAdapter } from 'mxw01-thermal-printer';
import { createCanvas } from 'canvas';

// Create client
const adapter = new NodeBluetoothAdapter();
const printer = new ThermalPrinterClient(adapter);

// Connect
await printer.connect();

// Create image
const canvas = createCanvas(384, 200);
const ctx = canvas.getContext('2d');
ctx.fillStyle = 'white';
ctx.fillRect(0, 0, 384, 200);
ctx.fillStyle = 'black';
ctx.font = '30px Arial';
ctx.fillText('Hello from Node.js!', 20, 100);

const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);

// Print
await printer.print(imageData);

// Disconnect
await printer.disconnect();
```

## Framework Integration

The library provides a framework-agnostic core that can be easily integrated with any framework. **Example implementations are included** in the `examples/` directory:

### React Hook Example

See [`examples/react-hook.tsx`](examples/react-hook.tsx) for a complete React hook implementation.

```tsx
import { useThermalPrinter } from './examples/react-hook';
import { useRef, useEffect } from 'react';

function App() {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const { isConnected, connectPrinter, printCanvas } = useThermalPrinter();
  
  // Draw on canvas when component mounts
  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    
    const ctx = canvas.getContext('2d');
    if (!ctx) return;
    
    // White background
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, 384, 200);
    
    // Black text
    ctx.fillStyle = 'black';
    ctx.font = '30px Arial';
    ctx.fillText('Hello from React!', 20, 100);
    
    // Draw a rectangle
    ctx.strokeStyle = 'black';
    ctx.lineWidth = 2;
    ctx.strokeRect(10, 10, 364, 180);
  }, []);
  
  const handlePrint = async () => {
    if (canvasRef.current) {
      await printCanvas(canvasRef.current);
    }
  };
  
  return (
    <div>
      <canvas 
        ref={canvasRef} 
        width={384} 
        height={200}
        style={{ border: '1px solid #ccc' }}
      />
      <div>
        <button onClick={connectPrinter} disabled={isConnected}>
          Connect
        </button>
        <button onClick={handlePrint} disabled={!isConnected}>
          Print
        </button>
      </div>
    </div>
  );
}
```

### Vue 3 Composable Example

See [`examples/vue-composable.ts`](examples/vue-composable.ts) for a complete Vue composable implementation.

```vue
<script setup lang="ts">
import { useThermalPrinter } from './examples/vue-composable';
import { ref, onMounted } from 'vue';

const canvasRef = ref<HTMLCanvasElement | null>(null);
const { isConnected, connectPrinter, printCanvas } = useThermalPrinter();

// Draw on canvas when component mounts
onMounted(() => {
  const canvas = canvasRef.value;
  if (!canvas) return;
  
  const ctx = canvas.getContext('2d');
  if (!ctx) return;
  
  // White background
  ctx.fillStyle = 'white';
  ctx.fillRect(0, 0, 384, 200);
  
  // Black text
  ctx.fillStyle = 'black';
  ctx.font = '30px Arial';
  ctx.fillText('Hello from Vue!', 20, 100);
  
  // Draw a rectangle
  ctx.strokeStyle = 'black';
  ctx.lineWidth = 2;
  ctx.strokeRect(10, 10, 364, 180);
});

const handlePrint = async () => {
  if (canvasRef.value) {
    await printCanvas(canvasRef.value);
  }
};
</script>

<template>
  <div>
    <canvas 
      ref="canvasRef" 
      width="384" 
      height="200"
      style="border: 1px solid #ccc;"
    />
    <div>
      <button @click="connectPrinter" :disabled="isConnected">
        Connect
      </button>
      <button @click="handlePrint" :disabled="!isConnected">
        Print
      </button>
    </div>
  </div>
</template>
```

## Core API

### ThermalPrinterClient

The main client class for interacting with the printer.

```typescript
import { ThermalPrinterClient, WebBluetoothAdapter } from 'mxw01-thermal-printer';

const adapter = new WebBluetoothAdapter();
const printer = new ThermalPrinterClient(adapter);
```

#### Methods

- `connect(): Promise<void>` - Connect to the printer
- `disconnect(): Promise<void>` - Disconnect from the printer
- `print(imageData: ImageData, options?: PrintOptions): Promise<void>` - Print an image
- `getStatus(): Promise<PrinterState | null>` - Get printer status
- `setDitherMethod(method: DitherMethod): void` - Set dithering algorithm
- `setPrintIntensity(intensity: number): void` - Set print intensity (0-255)
- `on(eventType, listener): () => void` - Subscribe to events
- `dispose(): void` - Clean up resources

#### Properties

- `isConnected: boolean` - Connection status
- `isPrinting: boolean` - Printing status
- `printerState: PrinterState | null` - Current printer state
- `statusMessage: string` - Current status message
- `ditherMethod: DitherMethod` - Current dithering method
- `printIntensity: number` - Current print intensity

#### Events

```typescript
printer.on('connected', (event) => {
  console.log('Connected to:', event.device.name);
});

printer.on('disconnected', () => {
  console.log('Disconnected');
});

printer.on('stateChange', (event) => {
  console.log('Printer state:', event.state);
});

printer.on('error', (event) => {
  console.error('Error:', event.error);
});
```

## Configuration

### Print Options

```typescript
interface PrintOptions {
  dither?: DitherMethod;           // Dithering algorithm
  rotate?: 0 | 90 | 180 | 270;    // Rotation angle
  flip?: 'none' | 'h' | 'v' | 'both'; // Flip direction
  brightness?: number;              // Image brightness (0-255, default: 128)
  intensity?: number;               // Print intensity (0-255, default: 93)
}
```

### Dithering Methods

| Method       | Best For              | Description                    |
| ------------ | --------------------- | ------------------------------ |
| `threshold`  | Text, simple graphics | Basic black/white conversion   |
| `steinberg`  | Photos, general use   | Floyd-Steinberg (recommended)  |
| `bayer`      | Patterns, textures    | Ordered dithering              |
| `atkinson`   | Comics, illustrations | Atkinson dithering             |
| `pattern`    | Special effects       | Pattern-based dithering        |

### Understanding Parameters

#### `brightness` - Image Pre-processing

- **Range**: 0-255 (default: 128)
- **Effect**: Adjusts image lightness before printing
  - Lower values (0-127): Darker image (more black pixels)
  - 128: Normal (recommended)
  - Higher values (129-255): Lighter image (fewer black pixels)

#### `intensity` - Print Head Heat

- **Range**: 0-255 (default: 93)
- **Effect**: Controls thermal print head temperature
  - 50-80: Light printing
  - 80-100: Normal printing (recommended)
  - 100-150: Dark printing
  - 150-255: Very dark (risk of paper damage)

### Recommended Settings

| Use Case    | brightness | intensity | Description        |
| ----------- | ---------- | --------- | ------------------ |
| Normal text | 128        | 93        | Balanced, readable |
| Photos      | 140        | 100       | Good contrast      |
| Barcodes/QR | 128        | 110       | High contrast      |
| Light draft | 150        | 70        | Saves heat         |
| Dark/bold   | 110        | 120       | Maximum darkness   |

## Examples

Complete working examples are provided in the `examples/` directory:

- **[`nodejs-canvas-example.ts`](examples/nodejs-canvas-example.ts)** - Basic Node.js/Bun implementation
- **[`nodejs-fabric-example.ts`](examples/nodejs-fabric-example.ts)** - Node.js/Bun with Fabric.js for advanced graphics
- **[`react-hook.tsx`](examples/react-hook.tsx)** - React hook implementation
- **[`vue-composable.ts`](examples/vue-composable.ts)** - Vue 3 composable implementation

These examples show how to integrate the core library with different frameworks. You can copy and adapt them to your project.



## Platform Support

| Platform     | Support | Adapter                  | Notes                              |
| ------------ | ------- | ------------------------ | ---------------------------------- |
| Browser      | ✅      | WebBluetoothAdapter      | Requires Web Bluetooth API         |
| Node.js      | ✅      | NodeBluetoothAdapter     | Requires @stoprocent/noble         |
| Bun          | ✅      | NodeBluetoothAdapter     | Same as Node.js                    |

### Browser Compatibility

Web Bluetooth API is supported in:

- ✅ Chrome/Edge 56+
- ✅ Opera 43+
- ✅ Chrome for Android

Not supported in:

- ❌ Firefox
- ❌ Safari (as of 2024)

## Architecture

```
┌─────────────────────────────────────┐
│      Framework Examples              │  ← React, Vue (examples/)
├─────────────────────────────────────┤
│    Platform-Agnostic Core            │  ← ThermalPrinterClient (src/core/)
├─────────────────────────────────────┤
│        Bluetooth Adapters            │  ← Web, Node.js adapters (src/adapters/)
├─────────────────────────────────────┤
│        Service Layer                 │  ← Protocol, image processing (src/services/)
└─────────────────────────────────────┘
```

The library is designed with clear separation of concerns:

- **Core** - Framework-agnostic client with event system
- **Adapters** - Platform-specific Bluetooth implementations
- **Services** - Printer protocol, image processing, dithering
- **Examples** - Reference implementations for different frameworks

## Advanced Usage

### Creating Custom Adapters

You can create custom Bluetooth adapters for other platforms:

```typescript
import type { BluetoothAdapter } from 'mxw01-thermal-printer';

class MyCustomAdapter implements BluetoothAdapter {
  async isAvailable(): Promise<boolean> {
    // Check if Bluetooth is available
  }

  async requestDevice(): Promise<BluetoothDevice> {
    // Request device from user
  }

  async connect(device: BluetoothDevice): Promise<BluetoothConnection> {
    // Connect and return connection with characteristics
  }
}

// Use your custom adapter
const printer = new ThermalPrinterClient(new MyCustomAdapter());
```

### Direct Protocol Access

For advanced use cases, you can use the low-level protocol directly:

```typescript
import { MXW01Printer, prepareImageDataBuffer, encode1bppRow } from 'mxw01-thermal-printer';

// Create printer instance
const printer = new MXW01Printer(controlWrite, dataWrite);

// Set intensity
await printer.setIntensity(93);

// Request status
await printer.requestStatus();

// Print image
const imageBuffer = prepareImageDataBuffer(binaryRows);
await printer.printRequest(binaryRows.length, 0);
await printer.sendDataChunks(imageBuffer);
await printer.flushData();
await printer.waitForPrintComplete();
```

## Troubleshooting

### Connection Issues

- Ensure Bluetooth is enabled on your device
- Make sure the printer is charged and turned on
- Try disconnecting and reconnecting
- Check that no other application is connected to the printer

### Print Quality Issues

- Adjust `brightness` and `intensity` settings
- Try different dithering algorithms
- Check that the thermal paper is properly loaded
- Clean the thermal print head if necessary

### Node.js Issues

- Ensure `@stoprocent/noble` is properly installed
- On Linux, you may need to grant Bluetooth permissions
- On Windows, ensure Bluetooth drivers are up to date
- Check that no other Bluetooth service is using the adapter

## TypeScript Support

The library is written in TypeScript and provides complete type definitions:

```typescript
import type {
  ThermalPrinterClient,
  PrinterState,
  PrintOptions,
  DitherMethod,
  BluetoothAdapter,
  PrinterEvent
} from 'mxw01-thermal-printer';
```

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## License

MIT

## Credits

Based on the MXW01 thermal printer protocol. The identification of the protocol and its operation would not have been possible without [dropalltables/catprinter](https://github.com/dropalltables/catprinter).

Big thank you to the original researchers and contributors.

## Author

Made with ❤️ by Clément Van Peuter
