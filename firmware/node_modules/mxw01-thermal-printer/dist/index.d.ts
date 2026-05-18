/**
 * Abstract interface for Bluetooth adapters
 * Implementations can use Web Bluetooth API, Noble, or other BLE libraries
 */
export declare interface BluetoothAdapter {
    /**
     * Request a Bluetooth device with printer services
     */
    requestDevice(): Promise<BluetoothDevice_2>;
    /**
     * Connect to a Bluetooth device and get service characteristics
     */
    connect(device: BluetoothDevice_2): Promise<BluetoothConnection & BluetoothServiceInfo>;
    /**
     * Check if Bluetooth is available in the current environment
     */
    isAvailable(): boolean;
}

export declare interface BluetoothCharacteristic {
    writeValueWithoutResponse(data: BufferSource): Promise<void>;
    startNotifications(): Promise<void>;
    stopNotifications(): Promise<void>;
    addEventListener(event: string, callback: (event: any) => void): void;
    removeEventListener(event: string, callback: (event: any) => void): void;
}

export declare interface BluetoothConnection {
    device: BluetoothDevice_2;
    disconnect(): Promise<void>;
}

declare interface BluetoothDevice_2 {
    id: string;
    name?: string;
}
export { BluetoothDevice_2 as BluetoothDevice }

export declare interface BluetoothServiceInfo {
    controlCharacteristic: BluetoothCharacteristic;
    dataCharacteristic: BluetoothCharacteristic;
    notifyCharacteristic: BluetoothCharacteristic;
}

/**
 * MXW01 Printer command identifiers
 */
export declare const Command: {
    readonly GetStatus: 161;
    readonly SetIntensity: 162;
    readonly PrintRequest: 169;
    readonly FlushData: 173;
    readonly PrintComplete: 170;
};

/**
 * Options for image processing
 */
export declare type DitherMethod = "threshold" | "steinberg" | "bayer" | "atkinson" | "pattern";

/**
 * Dithering method type
 */
declare type DitherMethod_2 = "threshold" | "steinberg" | "bayer" | "atkinson" | "pattern";

/**
 * Encode a row of boolean pixels to binary format
 */
export declare function encode1bppRow(rowBool: boolean[]): Uint8Array;

export declare interface ImageProcessorOptions {
    dither: DitherMethod;
    rotate: 0 | 90 | 180 | 270;
    flip: "none" | "h" | "v" | "both";
    brightness: number;
}

/**
 * Image processor options
 */
declare interface ImageProcessorOptions_2 {
    dither: DitherMethod_2;
    rotate: 0 | 90 | 180 | 270;
    flip: "none" | "h" | "v" | "both";
    brightness: number;
}

export declare const MIN_DATA_BYTES: number;

/**
 * MXW01 Thermal Printer Controller
 * Simplified class that delegates to protocol and state management modules
 */
export declare class MXW01Printer {
    private controlWrite;
    private dataWrite;
    private stateManager;
    constructor(controlWrite: WriteFunction, dataWrite: WriteFunction);
    /**
     * Get current printer state
     */
    get state(): PrinterState_2;
    /**
     * Process incoming notification from printer
     */
    notify(message: Uint8Array): void;
    /**
     * Set print intensity (darkness)
     */
    setIntensity(intensity?: number): Promise<void>;
    /**
     * Request current printer status
     */
    requestStatus(): Promise<Uint8Array>;
    /**
     * Send print request with number of lines
     */
    printRequest(lines: number, mode?: number): Promise<Uint8Array>;
    /**
     * Flush data to printer
     */
    flushData(): Promise<void>;
    /**
     * Send data chunks to printer
     */
    sendDataChunks(data: Uint8Array, chunkSize?: number): Promise<void>;
    /**
     * Wait for print completion
     */
    waitForPrintComplete(timeoutMs?: number): Promise<void>;
}

/**
 * Node.js Bluetooth adapter using Noble
 * Provides native Bluetooth access for Node.js and Bun environments
 *
 * @example
 * ```typescript
 * import { ThermalPrinterClient } from 'react-mxw01-printer';
 * import { NodeBluetoothAdapter } from 'react-mxw01-printer/adapters/node';
 *
 * const adapter = new NodeBluetoothAdapter();
 * const printer = new ThermalPrinterClient(adapter);
 * ```
 *
 * @requires @stoprocent/noble
 */
export declare class NodeBluetoothAdapter implements BluetoothAdapter {
    private noble;
    private peripheral;
    private characteristics;
    constructor();
    /**
     * Check if Bluetooth is available (Noble is loaded)
     * The powered on state is checked during requestDevice()
     */
    isAvailable(): boolean;
    /**
     * Scan for and request a Bluetooth printer device
     * Automatically finds devices with MXW01 printer service UUID
     */
    requestDevice(): Promise<BluetoothDevice_2>;
    /**
     * Connect to a Bluetooth device and get printer service characteristics
     */
    connect(device: BluetoothDevice_2): Promise<BluetoothConnection & BluetoothServiceInfo>;
}

/**
 * Prepare image data buffer with padding
 */
export declare function prepareImageDataBuffer(imageRowsBool: boolean[][]): Uint8Array;

export declare const PRINTER_WIDTH = 384;

export declare const PRINTER_WIDTH_BYTES: number;

/**
 * Event types emitted by ThermalPrinterClient
 */
export declare type PrinterEvent = {
    type: "connected";
    device: BluetoothDevice_2;
} | {
    type: "disconnected";
} | {
    type: "stateChange";
    state: PrinterState;
} | {
    type: "printProgress";
    progress: number;
} | {
    type: "error";
    error: Error;
};

export declare type PrinterEventListener<T extends PrinterEventType = PrinterEventType> = (event: Extract<PrinterEvent, {
    type: T;
}>) => void;

export declare type PrinterEventType = PrinterEvent["type"];

/**
 * Image data interface - compatible with both Canvas and Node.js
 * Renamed to avoid conflict with DOM ImageData
 */
export declare interface PrinterImageData {
    data: Uint8ClampedArray;
    width: number;
    height: number;
}

export declare interface PrinterState {
    printing: boolean;
    paper_jam: boolean;
    out_of_paper: boolean;
    cover_open: boolean;
    battery_low: boolean;
    overheat: boolean;
}

/**
 * Printer state interface
 */
declare interface PrinterState_2 {
    printing: boolean;
    paper_jam: boolean;
    out_of_paper: boolean;
    cover_open: boolean;
    battery_low: boolean;
    overheat: boolean;
}

/**
 * Print options
 */
export declare interface PrintOptions extends Partial<ImageProcessorOptions> {
    intensity?: number;
}

/**
 * Process an image for thermal printer
 * @param imageData Source image data
 * @param options Processing options
 * @returns Processed image data and binary rows for printing
 */
export declare function processImageForPrinter(imageData: ImageData, options: ImageProcessorOptions_2): {
    processedData: Uint32Array;
    width: number;
    height: number;
    binaryRows: boolean[][];
};

/**
 * Platform-agnostic thermal printer client
 * Works with any BluetoothAdapter implementation (Web Bluetooth, Noble, etc.)
 */
export declare class ThermalPrinterClient {
    private adapter;
    private printer;
    private connection;
    private device;
    private eventEmitter;
    private state;
    constructor(adapter: BluetoothAdapter);
    get isConnected(): boolean;
    get isPrinting(): boolean;
    get printerState(): PrinterState | null;
    get statusMessage(): string;
    get ditherMethod(): ImageProcessorOptions["dither"];
    get printIntensity(): number;
    setDitherMethod(method: ImageProcessorOptions["dither"]): void;
    setPrintIntensity(intensity: number): void;
    /**
     * Subscribe to events
     */
    on<T extends PrinterEventType>(eventType: T, listener: PrinterEventListener<T>): () => void;
    /**
     * Update status message and emit state change if needed
     */
    private updateStatus;
    /**
     * Connect to printer via Bluetooth
     */
    connect(): Promise<void>;
    /**
     * Setup notification listener
     */
    private setupNotifications;
    /**
     * Get current printer status
     */
    getStatus(): Promise<PrinterState | null>;
    /**
     * Print from image data
     */
    print(imageData: PrinterImageData, options?: PrintOptions): Promise<void>;
    /**
     * Disconnect from printer
     */
    disconnect(): Promise<void>;
    /**
     * Dispose of the client and clean up resources
     */
    dispose(): void;
}

/**
 * Web Bluetooth adapter for browser environments
 * Uses the Web Bluetooth API to connect to Bluetooth devices
 */
export declare class WebBluetoothAdapter implements BluetoothAdapter {
    private device;
    private server;
    /**
     * Check if Web Bluetooth is available
     */
    isAvailable(): boolean;
    /**
     * Request a Bluetooth device with printer services
     */
    requestDevice(): Promise<BluetoothDevice_2>;
    /**
     * Connect to a Bluetooth device and get service characteristics
     */
    connect(device: BluetoothDevice_2): Promise<BluetoothConnection & BluetoothServiceInfo>;
}

export declare type WriteFunction = (data: BufferSource) => Promise<void>;

export { }
