#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_BOT_DETECTION_OBSERVER_SCRIPT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_BOT_DETECTION_OBSERVER_SCRIPT_H_

#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

inline String GetBotDetectionObserverScript() {
  return String(R"(
(function() {
  'use strict';
  const PREFIX = '[BOT_DETECTION]';
  const log = (category, message, value) => {
    const msg = `${PREFIX} [${category}] ${message}`;
    console.log(msg, value !== undefined ? value : '');
    if (!window.__botDetectionLogs) window.__botDetectionLogs = [];
    window.__botDetectionLogs.push({
      timestamp: Date.now(),
      category,
      message,
      value: value !== undefined ? String(value) : undefined
    });
  };

  // Navigator.webdriver
  try {
    const navProto = Navigator.prototype;
    const webdriverDesc = Object.getOwnPropertyDescriptor(navProto, 'webdriver');
    if (webdriverDesc) {
      Object.defineProperty(navProto, 'webdriver', {
        get: function() {
          const value = webdriverDesc.get ? webdriverDesc.get.call(this) : webdriverDesc.value;
          log('NAVIGATOR', 'webdriver accessed', value);
          return value;
        },
        configurable: true
      });
    }
  } catch (e) {}

  // Navigator.plugins
  try {
    const pluginsDesc = Object.getOwnPropertyDescriptor(Navigator.prototype, 'plugins');
    if (pluginsDesc && pluginsDesc.get) {
      Object.defineProperty(Navigator.prototype, 'plugins', {
        get: function() {
          const plugins = pluginsDesc.get.call(this);
          log('NAVIGATOR', 'plugins accessed', plugins.length + ' plugins');
          return plugins;
        },
        configurable: true
      });
    }
  } catch (e) {}

  // Navigator.languages
  try {
    const languagesDesc = Object.getOwnPropertyDescriptor(Navigator.prototype, 'languages');
    if (languagesDesc && languagesDesc.get) {
      Object.defineProperty(Navigator.prototype, 'languages', {
        get: function() {
          const langs = languagesDesc.get.call(this);
          log('NAVIGATOR', 'languages accessed', langs.join(', '));
          return langs;
        },
        configurable: true
      });
    }
  } catch (e) {}

  // Navigator.platform
  try {
    const platformDesc = Object.getOwnPropertyDescriptor(Navigator.prototype, 'platform');
    if (platformDesc && platformDesc.get) {
      Object.defineProperty(Navigator.prototype, 'platform', {
        get: function() {
          const platform = platformDesc.get.call(this);
          log('NAVIGATOR', 'platform accessed', platform);
          return platform;
        },
        configurable: true
      });
    }
  } catch (e) {}

  // Navigator.userAgent
  try {
    const uaDesc = Object.getOwnPropertyDescriptor(Navigator.prototype, 'userAgent');
    if (uaDesc && uaDesc.get) {
      Object.defineProperty(Navigator.prototype, 'userAgent', {
        get: function() {
          const ua = uaDesc.get.call(this);
          log('NAVIGATOR', 'userAgent accessed', ua.substring(0, 50) + '...');
          return ua;
        },
        configurable: true
      });
    }
  } catch (e) {}

  // Navigator.hardwareConcurrency
  try {
    const hcDesc = Object.getOwnPropertyDescriptor(Navigator.prototype, 'hardwareConcurrency');
    if (hcDesc && hcDesc.get) {
      Object.defineProperty(Navigator.prototype, 'hardwareConcurrency', {
        get: function() {
          const hc = hcDesc.get.call(this);
          log('NAVIGATOR', 'hardwareConcurrency accessed', hc);
          return hc;
        },
        configurable: true
      });
    }
  } catch (e) {}

  // Navigator.deviceMemory
  try {
    const dmDesc = Object.getOwnPropertyDescriptor(Navigator.prototype, 'deviceMemory');
    if (dmDesc && dmDesc.get) {
      Object.defineProperty(Navigator.prototype, 'deviceMemory', {
        get: function() {
          const dm = dmDesc.get.call(this);
          log('NAVIGATOR', 'deviceMemory accessed', dm);
          return dm;
        },
        configurable: true
      });
    }
  } catch (e) {}

  // Navigator.maxTouchPoints
  try {
    const mtpDesc = Object.getOwnPropertyDescriptor(Navigator.prototype, 'maxTouchPoints');
    if (mtpDesc && mtpDesc.get) {
      Object.defineProperty(Navigator.prototype, 'maxTouchPoints', {
        get: function() {
          const mtp = mtpDesc.get.call(this);
          log('NAVIGATOR', 'maxTouchPoints accessed', mtp);
          return mtp;
        },
        configurable: true
      });
    }
  } catch (e) {}

  // Screen properties
  try {
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
  } catch (e) {}

  // window.chrome
  try {
    Object.defineProperty(window, 'chrome', {
      get: function() {
        log('WINDOW', 'chrome accessed');
        return window.__chrome || undefined;
      },
      set: function(value) {
        window.__chrome = value;
      },
      configurable: true
    });
  } catch (e) {}

  // Canvas.toDataURL
  try {
    const canvasProto = HTMLCanvasElement.prototype;
    const originalToDataURL = canvasProto.toDataURL;
    if (originalToDataURL) {
      canvasProto.toDataURL = function(...args) {
        log('CANVAS', 'toDataURL called', `type=${args[0] || 'image/png'}`);
        return originalToDataURL.apply(this, args);
      };
    }
  } catch (e) {}

  // Canvas.getContext
  try {
    const canvasProto = HTMLCanvasElement.prototype;
    const originalGetContext = canvasProto.getContext;
    if (originalGetContext) {
      canvasProto.getContext = function(...args) {
        log('CANVAS', 'getContext called', `type=${args[0]}`);
        return originalGetContext.apply(this, args);
      };
    }
  } catch (e) {}

  // WebGL getParameter
  try {
    const webglProto = WebGLRenderingContext.prototype;
    const originalGetParameter = webglProto.getParameter;
    if (originalGetParameter) {
      webglProto.getParameter = function(parameter) {
        log('WEBGL', 'getParameter called', `param=${parameter}`);
        return originalGetParameter.call(this, parameter);
      };
    }
  } catch (e) {}

  // WebGL getExtension
  try {
    const webglProto = WebGLRenderingContext.prototype;
    const originalGetExtension = webglProto.getExtension;
    if (originalGetExtension) {
      webglProto.getExtension = function(name) {
        log('WEBGL', 'getExtension called', `name=${name}`);
        return originalGetExtension.call(this, name);
      };
    }
  } catch (e) {}

  // AudioContext
  try {
    const AudioContextProto = window.AudioContext || window.webkitAudioContext;
    if (AudioContextProto) {
      const OriginalAudioContext = AudioContextProto;
      window.AudioContext = function(...args) {
        log('AUDIO', 'AudioContext created');
        return new OriginalAudioContext(...args);
      };
      window.AudioContext.prototype = OriginalAudioContext.prototype;
    }
  } catch (e) {}

  // Permissions.query
  try {
    if (navigator.permissions && navigator.permissions.query) {
      const originalQuery = navigator.permissions.query;
      navigator.permissions.query = function(descriptor) {
        log('PERMISSIONS', 'query called', descriptor.name);
        return originalQuery.call(this, descriptor);
      };
    }
  } catch (e) {}

  // Performance.now
  try {
    const perfProto = Performance.prototype;
    const originalNow = perfProto.now;
    if (originalNow) {
      perfProto.now = function() {
        log('TIMING', 'Performance.now called');
        return originalNow.call(this);
      };
    }
  } catch (e) {}

  console.log(PREFIX, 'Bot detection observer initialized');
})();
  )");
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_BOT_DETECTION_OBSERVER_SCRIPT_H_
