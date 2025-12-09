# Bot Detection Analysis: hyatt.com

## Overview

This analysis examines the bot detection techniques used by hyatt.com's JavaScript bot detection system (KPSDK), as observed through a **custom modified Chromium browser** with enhanced observability. The custom build includes instrumentation that logs all bot detection API calls, allowing us to see exactly what the detection scripts are checking.

**Analysis Method**: A custom Chromium build was compiled with modifications to intercept and log:
- Navigator API property accesses
- Screen property accesses  
- Canvas/WebGL context creation and parameter queries
- All bot detection-related JavaScript API calls

The detection script is loaded from `ips.js` and performs extensive browser fingerprinting.

## Evidence Source

- **Custom Chromium Build**: Modified browser with BOT_DETECTION logging enabled
- **JavaScript File**: `script-1765312790553.js` (heavily obfuscated, ~486KB)
- **Network Log**: `test-harness/output/report-1765312794948.json` (contains complete BOT_DETECTION log stream)
- **Screenshots**: `test-harness/output/screenshot-1765312794768.png` (captured during analysis)
- **Detection Script URL**: `https://www.hyatt.com/149e9513-01fa-4fb0-aad4-566afd725d1b/2d206a39-8ed7-437e-a3be-862e0f06eea3/ips.js`

## Raw Observation Data

### Screenshots

A screenshot was captured during the analysis showing the hyatt.com page load. The screenshot is available at:
- `test-harness/output/screenshot-1765312794768.png`

The screenshot shows the page in a headless browser environment, which is relevant to understanding the bot detection context.

### Complete Log Excerpts from Modified Browser

The following are actual log entries captured by the custom Chromium build's BOT_DETECTION instrumentation:

```
[BOT_DETECTION] Bot detection observer initialized
[BOT_DETECTION] Bot detection observer initialized
[BOT_DETECTION] Bot detection observer initialized
[BOT_DETECTION] Bot detection observer initialized
[BOT_DETECTION] [NAVIGATOR] userAgent accessed Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) Ap...
[BOT_DETECTION] [NAVIGATOR] webdriver accessed false
[BOT_DETECTION] [NAVIGATOR] plugins accessed 5 plugins
[BOT_DETECTION] [NAVIGATOR] languages accessed en-US
[BOT_DETECTION] [NAVIGATOR] platform accessed Linux x86_64
[BOT_DETECTION] [NAVIGATOR] hardwareConcurrency accessed 256
[BOT_DETECTION] [NAVIGATOR] deviceMemory accessed 8
[BOT_DETECTION] [NAVIGATOR] maxTouchPoints accessed 0
[BOT_DETECTION] [SCREEN] width accessed 1920
[BOT_DETECTION] [SCREEN] height accessed 1080
[BOT_DETECTION] [SCREEN] colorDepth accessed 24
[BOT_DETECTION] [CANVAS] getContext called type=webgl
[BOT_DETECTION] [WEBGL] getExtension called name=WEBGL_debug_renderer_info
[BOT_DETECTION] [WEBGL] getParameter called param=37445
[BOT_DETECTION] [CANVAS] getContext called type=webgl
[BOT_DETECTION] [WEBGL] getExtension called name=WEBGL_debug_renderer_info
[BOT_DETECTION] [WEBGL] getParameter called param=37446
```

### Network Request Data

The following network requests were captured during the observation:

**Bot Detection Script Load:**
```json
{
  "timestamp": "2025-12-09T20:39:48.601Z",
  "method": "GET",
  "url": "https://www.hyatt.com/149e9513-01fa-4fb0-aad4-566afd725d1b/2d206a39-8ed7-437e-a3be-862e0f06eea3/ips.js?...",
  "resourceType": "script",
  "isBotDetectionRelated": true
}
```

**Error Reporting Endpoints (Multiple):**
```json
{
  "timestamp": "2025-12-09T20:39:50.471Z",
  "method": "POST",
  "url": "https://reporting.cdndex.io/error",
  "resourceType": "xhr",
  "isBotDetectionRelated": true
}
```

**Final Telemetry Submission:**
```json
{
  "timestamp": "2025-12-09T20:39:51.238Z",
  "method": "POST",
  "url": "https://www.hyatt.com/149e9513-01fa-4fb0-aad4-566afd725d1b/2d206a39-8ed7-437e-a3be-862e0f06eea3/tl",
  "resourceType": "xhr",
  "isBotDetectionRelated": true
}
```

### Collected Fingerprint Data

The following fingerprint data was collected by the detection script:

```json
{
  "userAgent": "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
  "webdriver": false,
  "plugins": [
    "PDF Viewer",
    "Chrome PDF Viewer",
    "Chromium PDF Viewer",
    "Microsoft Edge PDF Viewer",
    "WebKit built-in PDF"
  ],
  "languages": ["en-US"],
  "platform": "Linux x86_64",
  "hardwareConcurrency": 256,
  "deviceMemory": 8,
  "maxTouchPoints": 0,
  "screenWidth": 1920,
  "screenHeight": 1080,
  "colorDepth": 24,
  "webglVendor": "Google Inc. (Google)",
  "webglRenderer": "ANGLE (Google, Vulkan 1.3.0 (SwiftShader Device (Subzero) (0x0000C0DE)), SwiftShader driver)"
}
```

## Bot Detection Techniques Identified

### 1. Navigator API Fingerprinting

The bot detection script extensively probes the `navigator` object to collect browser characteristics:

#### Evidence from JSON Report:
```json
"[BOT_DETECTION] [NAVIGATOR] userAgent accessed Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) Ap..."
"[BOT_DETECTION] [NAVIGATOR] webdriver accessed false"
"[BOT_DETECTION] [NAVIGATOR] plugins accessed 5 plugins"
"[BOT_DETECTION] [NAVIGATOR] languages accessed en-US"
"[BOT_DETECTION] [NAVIGATOR] platform accessed Linux x86_64"
"[BOT_DETECTION] [NAVIGATOR] hardwareConcurrency accessed 256"
"[BOT_DETECTION] [NAVIGATOR] deviceMemory accessed 8"
"[BOT_DETECTION] [NAVIGATOR] maxTouchPoints accessed 0"
```

#### Techniques:
- **User Agent Analysis**: Checks the user agent string for inconsistencies
- **WebDriver Detection**: Explicitly checks `navigator.webdriver` property (common bot indicator)
- **Plugin Enumeration**: Counts and identifies installed plugins (5 plugins detected)
- **Language Detection**: Checks `navigator.languages` for locale information
- **Platform Detection**: Verifies `navigator.platform` (detected mismatch: claims Mac OS X but platform is Linux x86_64)
- **Hardware Fingerprinting**: 
  - `hardwareConcurrency`: Detected 256 cores (unusually high, likely virtualized environment)
  - `deviceMemory`: Detected 8GB (reasonable value)
- **Touch Capability**: Checks `maxTouchPoints` (0 = desktop, not touch-enabled)

**Anomaly Detected**: The user agent claims "Macintosh; Intel Mac OS X 10_15_7" but the platform is "Linux x86_64", which is a clear inconsistency that would flag this as a bot.

### 2. Screen/Display Fingerprinting

The script collects screen properties to build a display profile:

#### Evidence from JSON Report:
```json
"[BOT_DETECTION] [SCREEN] width accessed 1920"
"[BOT_DETECTION] [SCREEN] height accessed 1080"
"[BOT_DETECTION] [SCREEN] colorDepth accessed 24"
```

#### Techniques:
- **Screen Resolution**: 1920x1080 (common desktop resolution)
- **Color Depth**: 24-bit color depth
- These values are combined with other properties to create a unique fingerprint

### 3. WebGL/Canvas Fingerprinting

The most sophisticated fingerprinting technique involves WebGL rendering:

#### Evidence from JSON Report:
```json
"[BOT_DETECTION] [CANVAS] getContext called type=webgl"
"[BOT_DETECTION] [WEBGL] getExtension called name=WEBGL_debug_renderer_info"
"[BOT_DETECTION] [WEBGL] getParameter called param=37445"
"[BOT_DETECTION] [CANVAS] getContext called type=webgl"
"[BOT_DETECTION] [WEBGL] getExtension called name=WEBGL_debug_renderer_info"
"[BOT_DETECTION] [WEBGL] getParameter called param=37446"
```

#### Techniques:
1. **WebGL Context Creation**: Creates WebGL rendering contexts (called twice)
2. **Debug Extension Access**: Uses `WEBGL_debug_renderer_info` extension to extract GPU information
3. **GPU Parameter Extraction**: 
   - Parameter `37445` (0x9245) = `UNMASKED_VENDOR_WEBGL`
   - Parameter `37446` (0x9246) = `UNMASKED_RENDERER_WEBGL`

#### Actual Values Detected:
```json
"webglVendor": "Google Inc. (Google)",
"webglRenderer": "ANGLE (Google, Vulkan 1.3.0 (SwiftShader Device (Subzero) (0x0000C0DE)), SwiftShader driver)"
```

**Key Finding**: The WebGL renderer reveals "SwiftShader" which is a software-based WebGL implementation. This is a strong indicator of:
- Headless browser environment
- Virtualized/containerized environment
- Automated testing environment

The SwiftShader driver is commonly used in headless browsers and automated testing frameworks, making it a reliable bot indicator.

### 4. Error Reporting and Telemetry

The bot detection system sends telemetry data to external endpoints:

#### Evidence from Network Log:
```json
{
  "timestamp": "2025-12-09T20:39:50.471Z",
  "method": "POST",
  "url": "https://reporting.cdndex.io/error",
  "resourceType": "xhr",
  "isBotDetectionRelated": true
}
```

Multiple POST requests to `https://reporting.cdndex.io/error` suggest:
- Error tracking and anomaly detection
- Behavioral analysis data collection
- Real-time bot detection scoring

#### Final Telemetry Endpoint:
```json
{
  "timestamp": "2025-12-09T20:39:51.238Z",
  "method": "POST",
  "url": "https://www.hyatt.com/149e9513-01fa-4fb0-aad4-566afd725d1b/2d206a39-8ed7-437e-a3be-862e0f06eea3/tl",
  "resourceType": "xhr",
  "isBotDetectionRelated": true
}
```

The `/tl` endpoint (likely "telemetry" or "tracking log") receives the final fingerprinting data.

### 5. Timing and Behavioral Analysis

#### Evidence:
- Multiple "Bot detection observer initialized" messages at different timestamps
- Script execution timing tracked via `KPSDK.scriptStart=KPSDK.now()`
- Error logs suggest the script monitors for timing anomalies

### 6. Environment Inconsistencies Detected

The fingerprinting reveals several inconsistencies that would flag this as a bot:

1. **Platform Mismatch**: 
   - User Agent: "Macintosh; Intel Mac OS X 10_15_7"
   - Platform: "Linux x86_64"
   - **This is a clear red flag**

2. **Unusual Hardware Configuration**:
   - `hardwareConcurrency: 256` (extremely high, typical of virtualized environments)

3. **WebGL Renderer**:
   - SwiftShader software renderer (common in headless/automated environments)

4. **WebGL Warnings**:
   ```json
   "Automatic fallback to software WebGL has been deprecated"
   ```
   This indicates software-based WebGL rendering, another bot indicator.

## Detection Flow

Based on the evidence, the bot detection flow appears to be:

1. **Initialization**: Script loads from `ips.js` with encoded parameters
2. **Fingerprint Collection**: 
   - Navigator properties
   - Screen properties
   - WebGL/Canvas fingerprinting
3. **Anomaly Detection**: Compares collected values against expected patterns
4. **Error Reporting**: Sends anomalies to `reporting.cdndex.io/error`
5. **Final Telemetry**: Sends complete fingerprint to `/tl` endpoint
6. **Decision**: Server-side analysis determines bot probability

## Obfuscation Techniques

The JavaScript is heavily obfuscated:
- Variable names are minified to single characters
- String encoding/decoding functions
- Proxy objects for property access interception
- Complex control flow with generators/async functions

This obfuscation makes static analysis difficult, but the custom Chromium build's BOT_DETECTION logs reveal the actual API calls being made.

## Conclusion

Through observation using the custom modified Chromium browser, we can confirm that hyatt.com uses a sophisticated multi-layered bot detection system that:

1. **Fingerprints the browser** through Navigator, Screen, and WebGL APIs
2. **Detects inconsistencies** between claimed and actual environment
3. **Identifies automation tools** through WebDriver checks and software rendering
4. **Collects telemetry** for server-side analysis
5. **Uses behavioral timing** to detect non-human patterns

The detection is particularly effective at identifying:
- Headless browsers (SwiftShader renderer)
- Automated testing frameworks (WebDriver property)
- Virtualized environments (unusual hardware specs)
- Environment mismatches (platform vs user agent inconsistencies)

## Recommendations for Analysis

To better understand the detection system:
1. Deobfuscate the JavaScript to see the exact fingerprinting logic
2. Monitor the `/tl` endpoint payload to see what data is sent
3. Test with different browser configurations to see which properties trigger alerts
4. Analyze the error reporting patterns to understand scoring thresholds
