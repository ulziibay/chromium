# Reverse Engineering Bot Detection: A Deep Dive into Chromium Modifications and hyatt.com's Detection Mechanisms

## Introduction

Bot detection systems have become increasingly sophisticated, using a combination of browser fingerprinting, behavioral analysis, and environment consistency checks to identify automated traffic. As security researchers, understanding these mechanisms is crucial—not to bypass them, but to analyze their effectiveness and help improve web security.

In this article, I'll walk through how I modified Chromium to add observability into JavaScript bot detection mechanisms, and what we discovered when analyzing hyatt.com's bot detection system. This project demonstrates a powerful technique for reverse engineering obfuscated bot detection scripts by instrumenting the browser itself.

## The Challenge

Modern bot detection scripts are heavily obfuscated, making static analysis extremely difficult. The JavaScript code is minified, uses encoded strings, and employs complex control flow to hide its true purpose. When analyzing a site like hyatt.com, we're faced with a ~486KB obfuscated script that's nearly impossible to understand through traditional reverse engineering.

The solution? Instead of trying to deobfuscate the JavaScript, we can modify the browser to log every API call the script makes. This provides a clear view of what the detection system is actually checking, regardless of how obfuscated the code is.

## Technical Approach: JavaScript Injection

Rather than modifying C++ bindings for each API (which would be time-consuming and error-prone), I chose a JavaScript injection approach. This involves:

1. Creating an observer script that wraps browser APIs
2. Embedding this script in Chromium's C++ code
3. Injecting it into every page before any other scripts execute
4. Logging all bot detection-related API accesses

This approach is elegant because:
- It's easier to implement and maintain
- It runs in the JavaScript context, making it harder to detect
- It can wrap any JavaScript API without modifying C++ bindings
- It provides immediate visibility into what scripts are doing

## Implementation Details

### Commit Overview

The modifications were made in a single commit (`36557831e057b`) that added bot detection observability to Chromium. Here's what was changed:

**Files Modified:**
- `third_party/blink/renderer/core/dom/document.cc` - Added injection point
- `third_party/blink/renderer/core/frame/local_frame.cc` - Added injection method
- `third_party/blink/renderer/core/frame/local_frame.h` - Added method declaration
- `third_party/blink/renderer/core/frame/build.gni` - Updated build configuration
- `third_party/blink/renderer/core/frame/bot_detection_observer_script.h` - New file with observer script

### The Observer Script

The core of the implementation is a JavaScript observer script embedded in a C++ header file. This script wraps critical browser APIs that bot detection systems commonly probe:

#### Navigator API Wrapping

The script intercepts access to Navigator properties that are commonly used for fingerprinting:

```javascript
// Navigator.webdriver - Explicit bot detection check
Object.defineProperty(Navigator.prototype, 'webdriver', {
  get: function() {
    const value = webdriverDesc.get ? webdriverDesc.get.call(this) : webdriverDesc.value;
    log('NAVIGATOR', 'webdriver accessed', value);
    return value;
  },
  configurable: true
});
```

Similar wrappers were added for:
- `userAgent` - Browser identification
- `platform` - Operating system detection
- `plugins` - Plugin enumeration
- `languages` - Locale detection
- `hardwareConcurrency` - CPU core count
- `deviceMemory` - RAM detection
- `maxTouchPoints` - Touch capability detection

#### Screen Property Wrapping

Screen properties are wrapped to detect display fingerprinting:

```javascript
['width', 'height', 'colorDepth', 'pixelDepth'].forEach(prop => {
  const desc = Object.getOwnPropertyDescriptor(Screen.prototype, prop);
  if (desc && desc.get) {
    Object.defineProperty(Screen.prototype, prop, {
      get: function() {
        const value = desc.get.call(this);
        log('SCREEN', `${prop} accessed`, value);
        return value;
      },
      configurable: true
    });
  }
});
```

#### WebGL/Canvas Fingerprinting Detection

WebGL fingerprinting is one of the most sophisticated techniques. The script wraps:

- `HTMLCanvasElement.prototype.getContext()` - Detects WebGL context creation
- `WebGLRenderingContext.prototype.getExtension()` - Detects debug extension usage
- `WebGLRenderingContext.prototype.getParameter()` - Logs GPU parameter queries

These are particularly important because WebGL can reveal GPU information that's unique to hardware configurations.

#### Additional API Wrappers

The observer also monitors:
- `AudioContext` creation - Audio fingerprinting
- `navigator.permissions.query()` - Permission API checks
- `Performance.now()` - Timing attack detection
- `window.chrome` - Chrome-specific property checks

### Injection Mechanism

The script is injected during the document parsing lifecycle:

**In `document.cc`:**
```cpp
void Document::FinishedParsing() {
  // ... existing code ...
  
  if (LocalFrame* frame = GetFrame()) {
    // Inject bot detection observer script
    frame->InjectBotDetectionObserver();
    
    // ... rest of method ...
  }
}
```

**In `local_frame.cc`:**
```cpp
void LocalFrame::InjectBotDetectionObserver() {
  if (!GetDocument() || !GetDocument()->GetScriptController().CanExecuteScripts(
                           kNotAboutToExecuteScript)) {
    return;
  }
  
  String script = GetBotDetectionObserverScript();
  GetDocument()->GetScriptController().ExecuteScriptInMainWorld(script);
}
```

This ensures the observer is injected early in the page lifecycle, before most bot detection scripts execute.

### Logging Mechanism

The observer uses a dual logging approach:

1. **Console Logging**: All API accesses are logged to the browser console with a `[BOT_DETECTION]` prefix for easy visibility during development and debugging.

2. **Programmatic Access**: Logs are also stored in `window.__botDetectionLogs` as an array of objects, allowing test harnesses and automation tools to programmatically extract the data:

```javascript
window.__botDetectionLogs.push({
  timestamp: Date.now(),
  category: 'NAVIGATOR',
  message: 'webdriver accessed',
  value: 'false'
});
```

## Results: Analyzing hyatt.com's Bot Detection

With the modified Chromium build, I analyzed hyatt.com's bot detection system. The site uses a JavaScript bot detection SDK (KPSDK) loaded from `ips.js`. Here's what we discovered:

### Detection Script Loading

The bot detection script is loaded from:
```
https://www.hyatt.com/149e9513-01fa-4fb0-aad4-566afd725d1b/2d206a39-8ed7-437e-a3be-862e0f06eea3/ips.js
```

This is a heavily obfuscated ~486KB JavaScript file that would be nearly impossible to analyze through static analysis alone.

### Fingerprinting Techniques Discovered

#### 1. Navigator API Fingerprinting

The detection script extensively probes Navigator properties:

```
[BOT_DETECTION] [NAVIGATOR] userAgent accessed Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)...
[BOT_DETECTION] [NAVIGATOR] webdriver accessed false
[BOT_DETECTION] [NAVIGATOR] plugins accessed 5 plugins
[BOT_DETECTION] [NAVIGATOR] languages accessed en-US
[BOT_DETECTION] [NAVIGATOR] platform accessed Linux x86_64
[BOT_DETECTION] [NAVIGATOR] hardwareConcurrency accessed 256
[BOT_DETECTION] [NAVIGATOR] deviceMemory accessed 8
[BOT_DETECTION] [NAVIGATOR] maxTouchPoints accessed 0
```

**Key Findings:**
- **WebDriver Check**: Explicitly checks `navigator.webdriver` (a common bot indicator)
- **Platform Mismatch Detection**: The user agent claims "Macintosh; Intel Mac OS X 10_15_7" but `navigator.platform` returns "Linux x86_64" - this is a clear inconsistency that would flag the browser as automated
- **Unusual Hardware**: `hardwareConcurrency: 256` is extremely high, typical of virtualized environments
- **Plugin Enumeration**: Counts installed plugins (5 detected in this case)

#### 2. Screen/Display Fingerprinting

Screen properties are collected to build a display profile:

```
[BOT_DETECTION] [SCREEN] width accessed 1920
[BOT_DETECTION] [SCREEN] height accessed 1080
[BOT_DETECTION] [SCREEN] colorDepth accessed 24
```

These values are combined with other properties to create a unique fingerprint.

#### 3. WebGL/Canvas Fingerprinting

The most sophisticated technique involves WebGL rendering:

```
[BOT_DETECTION] [CANVAS] getContext called type=webgl
[BOT_DETECTION] [WEBGL] getExtension called name=WEBGL_debug_renderer_info
[BOT_DETECTION] [WEBGL] getParameter called param=37445
[BOT_DETECTION] [CANVAS] getContext called type=webgl
[BOT_DETECTION] [WEBGL] getExtension called name=WEBGL_debug_renderer_info
[BOT_DETECTION] [WEBGL] getParameter called param=37446
```

**Analysis:**
- Parameter `37445` (0x9245) = `UNMASKED_VENDOR_WEBGL`
- Parameter `37446` (0x9246) = `UNMASKED_RENDERER_WEBGL`

**Critical Discovery**: The WebGL renderer revealed:
```
webglVendor: "Google Inc. (Google)"
webglRenderer: "ANGLE (Google, Vulkan 1.3.0 (SwiftShader Device (Subzero) (0x0000C0DE)), SwiftShader driver)"
```

**SwiftShader** is a software-based WebGL implementation commonly used in:
- Headless browser environments
- Virtualized/containerized environments
- Automated testing frameworks

This is a strong indicator of automation and would likely trigger bot detection.

#### 4. Error Reporting and Telemetry

The bot detection system sends telemetry to multiple endpoints:

1. **Error Reporting**: Multiple POST requests to `https://reporting.cdndex.io/error`
   - Suggests real-time anomaly detection
   - Behavioral analysis data collection
   - Bot detection scoring

2. **Final Telemetry**: POST to `https://www.hyatt.com/.../tl` (likely "telemetry" or "tracking log")
   - Receives complete fingerprinting data
   - Server-side analysis determines bot probability

### Environment Inconsistencies Detected

The fingerprinting revealed several red flags that would identify this as a bot:

1. **Platform Mismatch**: 
   - User Agent: "Macintosh; Intel Mac OS X 10_15_7"
   - Platform: "Linux x86_64"
   - **This is a clear inconsistency**

2. **Unusual Hardware Configuration**:
   - `hardwareConcurrency: 256` (extremely high, typical of virtualized environments)

3. **WebGL Renderer**:
   - SwiftShader software renderer (common in headless/automated environments)
   - WebGL warnings about "Automatic fallback to software WebGL"

4. **Headless Environment Indicators**:
   - Software-based rendering
   - Virtualized hardware characteristics

### Detection Flow

Based on the observed behavior, the bot detection flow appears to be:

1. **Initialization**: Script loads from `ips.js` with encoded parameters
2. **Fingerprint Collection**: 
   - Navigator properties
   - Screen properties
   - WebGL/Canvas fingerprinting
3. **Anomaly Detection**: Compares collected values against expected patterns
4. **Error Reporting**: Sends anomalies to `reporting.cdndex.io/error`
5. **Final Telemetry**: Sends complete fingerprint to `/tl` endpoint
6. **Server-Side Decision**: Server analyzes the data and determines bot probability

## Technical Insights

### Why This Approach Works

1. **Bypasses Obfuscation**: No matter how obfuscated the JavaScript is, it must call browser APIs. By intercepting at the API level, we see exactly what's being checked.

2. **Early Injection**: Injecting the observer during `FinishedParsing()` ensures it runs before most bot detection scripts, capturing all API calls.

3. **Non-Intrusive**: The wrappers preserve original functionality while adding logging, so the browser behaves normally.

4. **Comprehensive Coverage**: By wrapping prototype properties, we catch all access patterns, including indirect access through proxies or getters.

### Limitations and Considerations

1. **Timing**: The observer must be injected early enough to catch all API calls. Some scripts might execute before `FinishedParsing()`.

2. **Detection Risk**: While the observer itself is hard to detect, sophisticated bot detection systems might notice inconsistencies in timing or behavior.

3. **Coverage**: We can't catch everything—some detection techniques might use APIs we haven't wrapped yet.

## Conclusion

This project demonstrates a powerful technique for reverse engineering bot detection systems. By modifying Chromium to log API accesses, we were able to:

1. **Understand obfuscated code**: Without deobfuscating the JavaScript, we saw exactly what it was checking
2. **Identify detection techniques**: Discovered Navigator fingerprinting, WebGL fingerprinting, and environment consistency checks
3. **Find inconsistencies**: Detected platform mismatches and software rendering that would flag automation
4. **Map the detection flow**: Understood how data flows from fingerprinting to telemetry submission

The analysis of hyatt.com revealed a sophisticated multi-layered bot detection system that:
- Fingerprints browsers through Navigator, Screen, and WebGL APIs
- Detects inconsistencies between claimed and actual environment
- Identifies automation tools through WebDriver checks and software rendering
- Collects telemetry for server-side analysis

### Key Takeaways

1. **Browser instrumentation** is a powerful tool for understanding obfuscated JavaScript
2. **WebGL fingerprinting** is particularly effective at detecting headless/automated environments
3. **Environment consistency** checks are crucial—mismatches between user agent and platform are red flags
4. **Software rendering** (like SwiftShader) is a strong indicator of automation

### Future Work

Potential enhancements to the observer:
- Add more API wrappers (Battery API, MediaDevices, etc.)
- Capture timing information for behavioral analysis
- Log network requests related to bot detection
- Add support for WebGL2 and other newer APIs

This approach can be extended to analyze any bot detection system, providing valuable insights into how modern web security mechanisms work.

---

## References

- **Chromium Source**: [chromium.googlesource.com](https://chromium.googlesource.com)
- **Blink Renderer**: Located in `third_party/blink/` - handles Web APIs
- **Commit**: `36557831e057b` - "Implement JavaScript injection for bot detection observability"
- **Analysis Data**: Available in `test-harness/output/report-*.json`

---